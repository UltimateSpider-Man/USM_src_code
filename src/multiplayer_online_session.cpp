#include "multiplayer_online_session.h"
#if !defined(OPENUSM_XBPACK_MODE)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/random.h>
#include <unistd.h>
#endif
#include <limits>
#include <utility>

namespace usm::online {
namespace {
#ifdef _WIN32
using Socket=SOCKET;using SockLen=int;
constexpr Socket invalid_socket=INVALID_SOCKET;
int net_error(){return WSAGetLastError();}
bool would_block(int e){return e==WSAEWOULDBLOCK||e==WSAEINPROGRESS;}
void socket_close(Socket s){if(s!=invalid_socket)closesocket(s);}
bool nonblocking(Socket s){u_long yes=1;return ioctlsocket(s,FIONBIO,&yes)==0;}
bool random64(std::uint64_t& out){return BCryptGenRandom(nullptr,reinterpret_cast<PUCHAR>(&out),sizeof(out),BCRYPT_USE_SYSTEM_PREFERRED_RNG)==0 && out!=0;}
#else
using Socket=int;using SockLen=socklen_t;
constexpr Socket invalid_socket=-1;
int net_error(){return errno;}
bool would_block(int e){return e==EAGAIN||e==EWOULDBLOCK||e==EINPROGRESS||e==EINTR;}
void socket_close(Socket s){if(s!=invalid_socket)::close(s);}
bool nonblocking(Socket s){int f=fcntl(s,F_GETFL,0);return f>=0&&fcntl(s,F_SETFL,f|O_NONBLOCK)==0;}
bool random64(std::uint64_t& out){std::size_t n=0;while(n<sizeof(out)){auto r=getrandom(reinterpret_cast<char*>(&out)+n,sizeof(out)-n,0);if(r<0&&errno==EINTR)continue;if(r<=0)return false;n+=std::size_t(r);}return out!=0;}
#endif
int send_flags(){
#ifdef MSG_NOSIGNAL
    return MSG_NOSIGNAL;
#else
    return 0;
#endif
}
bool address(const std::string& ip,std::uint16_t port,sockaddr_in& out,bool any=false) {
    out={};out.sin_family=AF_INET;out.sin_port=htons(port);
    if(inet_pton(AF_INET,ip.c_str(),&out.sin_addr)!=1)return false;
    const auto n=ntohl(out.sin_addr.s_addr);
    return (any||n!=0) && n!=0xffffffffu && (n>>28)!=0xe; // no broadcast/multicast joins
}
bool same_endpoint(const sockaddr_in& a,const sockaddr_in& b){return a.sin_addr.s_addr==b.sin_addr.s_addr&&a.sin_port==b.sin_port;}
void tcp_options(Socket s){int one=1;setsockopt(s,IPPROTO_TCP,TCP_NODELAY,reinterpret_cast<const char*>(&one),sizeof(one));}
bool connection_complete(Socket s,int& error) {
    if(s==invalid_socket)return false;
#ifndef _WIN32
    if(s>=FD_SETSIZE){error=EINVAL;return true;}
#endif
    fd_set write,error_set;FD_ZERO(&write);FD_ZERO(&error_set);FD_SET(s,&write);FD_SET(s,&error_set);timeval tv{};
    int r=select(int(s)+1,nullptr,&write,&error_set,&tv);
    if(r<=0)return false;
    SockLen len=sizeof(error);error=0;
    if(getsockopt(s,SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&error),&len)!=0)error=net_error();
    return true;
}
struct Link {
    Socket socket=invalid_socket;sockaddr_in tcp{},udp{};
    std::uint32_t id=0;std::uint64_t token=0;
    bool connecting=false,udp_known=false,closing=false;
    double opened=0,heard=0,next_ping=0,last_udp=0,closing_since=0;
    std::uint64_t ping_cookie=0;double ping_time=0;
    Bytes rx,tx;std::size_t sent=0;
    void clear(){socket_close(socket);*this=Link{};}
};
}
struct Session::Impl {
    State state=State::offline;Options options{};Counters count{};std::string failure;
    Socket listener=invalid_socket,udp=invalid_socket;
    bool winsock=false,server=false,clock_valid=false,got_snapshot=false;
    std::array<Link,max_players-1> links{};
    std::array<Participant,max_players> people{};
    sockaddr_in destination{};
    std::uint32_t self_id=0,next_id=2,local_sequence=0,snapshot_sequence=0,last_snapshot=0;
    std::uint64_t session=0,token=0;
    unsigned local_seat=0;
    std::uint16_t actual_port=0;
    double start=0,now=0,next_udp=0,clock_offset=0;
    std::uint64_t millis()const{return static_cast<std::uint64_t>(std::max(0.0,now-start)*1000.0);}
    bool queue(Link& l,const Writer& w) {
        if(w.bytes.size()>max_packet || l.tx.size()-l.sent+w.bytes.size()+2>16384)return false;
        if(l.sent){l.tx.erase(l.tx.begin(),l.tx.begin()+std::ptrdiff_t(l.sent));l.sent=0;}
        const auto b=frame(w);l.tx.insert(l.tx.end(),b.begin(),b.end());return true;
    }
    bool flush(Link& l) {
        if(l.sent==l.tx.size()){l.tx.clear();l.sent=0;return true;}
        int n=::send(l.socket,reinterpret_cast<const char*>(l.tx.data()+l.sent),int(l.tx.size()-l.sent),send_flags());
        if(n<0)return would_block(net_error());
        if(n==0)return false;
        l.sent+=std::size_t(n);count.tx_bytes+=n;
        if(l.sent==l.tx.size()){l.tx.clear();l.sent=0;}
        return true;
    }
    void clear() {
        for(auto& l:links)l.clear();socket_close(listener);socket_close(udp);listener=udp=invalid_socket;
#ifdef _WIN32
        if(winsock)WSACleanup();
#endif
        winsock=false;people={};state=State::offline;server=false;self_id=0;actual_port=0;clock_valid=false;got_snapshot=false;
    }
    void fail(const std::string& s){clear();failure=s;state=State::error;}
    Participant* by_id(std::uint32_t id){for(auto& p:people)if(p.identity.id==id&&id)return &p;return nullptr;}
    Writer roster()const {
        auto w=control(Control::roster);unsigned n=0;for(const auto& p:people)if(p.identity.id)++n;
        w.u8(n);for(const auto& p:people)if(p.identity.id){w.u32(p.identity.id);w.u8(p.identity.seat);w.text(p.identity.nickname);}return w;
    }
    void broadcast_roster(){auto w=roster();for(auto& l:links)if(l.id&&!l.closing&&!queue(l,w)){l.closing=true;l.closing_since=now;}}
    void drop(Link& l) {
        if(!server){fail("Connessione con l'host interrotta.");return;}
        if(auto* p=by_id(l.id))*p=Participant{};
        const bool changed=l.id!=0;l.clear();if(changed)broadcast_roster();
    }
    void reject(Link& l,const char* why) {auto w=control(Control::error);w.text(why);queue(l,w);l.closing=true;l.closing_since=now;++count.rejected;}
    void hello(Link& l){auto w=control(Control::hello);w.u32(build_id);w.u32(tag_hash(options.content_tag));w.text(options.nickname);if(!queue(l,w))fail("Coda iniziale non disponibile.");}
    bool init(const Options& o,double time,bool hosting) {
        clear();failure.clear();count={};options=o;now=start=time;next_udp=time;server=hosting;local_sequence=0;snapshot_sequence=0;last_snapshot=0;
        next_id=2;session=token=0;local_seat=0;
        if(!std::isfinite(time)||time<0||!valid_nickname(o.nickname)||o.capacity<2||o.capacity>max_players||
           o.send_hz<10||o.send_hz>30||!std::isfinite(o.interpolation)||o.interpolation<.05||o.interpolation>.3||o.content_tag.size()>96){fail("Configurazione online non valida.");return false;}
#ifdef _WIN32
        WSADATA w{};if(WSAStartup(MAKEWORD(2,2),&w)!=0){fail("WSAStartup non riuscito.");return false;}winsock=true;
#endif
        udp=::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
        if(udp==invalid_socket||!nonblocking(udp)){fail("Impossibile creare il socket UDP.");return false;}
        return true;
    }
    bool host(const Options& o,double t) {
        if(!init(o,t,true))return false;
        sockaddr_in local{};if(!address(o.bind_address,o.port,local,true)){fail("BindAddress non e' un IPv4 valido.");return false;}
        listener=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        if(listener==invalid_socket||!nonblocking(listener)){fail("Impossibile creare il socket host.");return false;}
#ifdef _WIN32
        int exclusive=1;setsockopt(listener,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<char*>(&exclusive),sizeof(exclusive));
#endif
        if(::bind(listener,reinterpret_cast<sockaddr*>(&local),sizeof(local))!=0||::listen(listener,int(o.capacity))!=0){fail("Porta TCP occupata o indirizzo host non disponibile.");return false;}
        SockLen len=sizeof(local);if(getsockname(listener,reinterpret_cast<sockaddr*>(&local),&len)!=0){fail("Impossibile leggere la porta host.");return false;}
        actual_port=ntohs(local.sin_port);
        if(::bind(udp,reinterpret_cast<sockaddr*>(&local),sizeof(local))!=0){fail("Porta UDP occupata o indirizzo host non disponibile.");return false;}
        if(!random64(session)){fail("Generatore casuale del sistema non disponibile.");return false;}
        self_id=1;people[0].identity={1,0,o.nickname};state=State::hosting;return true;
    }
    bool join(const Options& o,double t) {
        if(!init(o,t,false))return false;
        if(!o.port||!address(o.host,o.port,destination)){fail("Inserisci un IPv4 host valido e una porta 1-65535.");return false;}
        // Bind ephemeral UDP once. Its endpoint is learned only from a token-
        // bearing datagram whose source IP matches the TCP peer.
        sockaddr_in local{};local.sin_family=AF_INET;
        if(::bind(udp,reinterpret_cast<sockaddr*>(&local),sizeof(local))!=0){fail("Bind UDP client non riuscito.");return false;}
        auto& l=links[0];l.socket=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);l.opened=l.heard=now;l.tcp=destination;
        if(l.socket==invalid_socket||!nonblocking(l.socket)){fail("Impossibile creare il socket client.");return false;}
        tcp_options(l.socket);int r=::connect(l.socket,reinterpret_cast<sockaddr*>(&destination),sizeof(destination));
        if(r<0&&!would_block(net_error())){fail("Connessione rifiutata. Controlla IP, porta e firewall.");return false;}
        state=State::connecting;l.connecting=r<0;if(!l.connecting)hello(l);return true;
    }
    bool message(Link& l,const Bytes& b) {
        Reader r(b);Control c{};if(!control_header(r,c))return false;
        if(c==Control::hello && server && !l.id) {
            auto build=r.u32(),content=r.u32();auto name=r.text(nickname_limit);
            if(!r.done()||!valid_nickname(name))return false;
            if(build!=build_id||content!=tag_hash(options.content_tag)){reject(l,"Versione protocollo o ContentTag differente.");return true;}
            unsigned seat=1;while(seat<options.capacity&&people[seat].identity.id)++seat;
            if(seat>=options.capacity){reject(l,"Sessione piena.");return true;}
            if(next_id==0||!random64(l.token)){reject(l,"Impossibile assegnare una nuova identita'.");return true;}
            l.id=next_id++;people[seat].identity={l.id,std::uint8_t(seat),name};
            auto w=control(Control::welcome);w.u32(l.id);w.u8(seat);w.u64(session);w.u64(l.token);w.u8(options.capacity);
            if(!queue(l,w))return false;broadcast_roster();return true;
        }
        if(c==Control::welcome&&!server&&state==State::connecting&&!self_id) {
            auto id=r.u32();auto seat=r.u8();auto sess=r.u64(),tok=r.u64();auto cap=r.u8();
            if(!r.done()||id<2||seat<1||seat>=cap||cap>max_players||!sess||!tok)return false;
            self_id=id;local_seat=seat;session=sess;token=tok;options.capacity=cap;l.id=1;
            people[seat].identity={id,seat,options.nickname};state=State::joined;return true;
        }
        if(c==Control::roster&&!server&&state==State::joined) {
            unsigned n=r.u8();if(n<1||n>options.capacity)return false;
            std::array<Identity,max_players> ids{};bool found_host=false,found_self=false;
            for(unsigned i=0;i<n;++i){Identity id;id.id=r.u32();id.seat=r.u8();id.nickname=r.text(nickname_limit);
                if(!id.id||id.seat>=options.capacity||ids[id.seat].id||!valid_nickname(id.nickname))return false;
                for(const auto& old:ids)if(old.id==id.id)return false;
                if(id.id==1){if(id.seat!=0)return false;found_host=true;}
                else if(id.seat==0)return false;
                if(id.id==self_id){if(id.seat!=local_seat)return false;found_self=true;}
                ids[id.seat]=id;}
            if(!r.done()||!found_host||!found_self)return false;
            for(unsigned i=0;i<max_players;++i){if(people[i].identity.id!=ids[i].id)people[i]=Participant{};people[i].identity=ids[i];}
            return true;
        }
        if(c==Control::ping&&l.id){auto cookie=r.u64();if(!r.done())return false;auto w=control(Control::pong);w.u64(cookie);return queue(l,w);}
        if(c==Control::pong&&l.id){auto cookie=r.u64();if(!r.done())return false;if(cookie==l.ping_cookie&&l.ping_time>0){
            if(auto* p=by_id(l.id))p->ping_ms=unsigned(std::clamp((now-l.ping_time)*1000.0,0.0,60000.0));l.ping_time=0;}return true;}
        if(c==Control::error&&!server){auto s=r.text(120);if(!r.done())return false;for(unsigned char x:s)if(x<32||x>126)return false;fail(s);return true;}
        if(c==Control::bye&&r.done())return false;
        return false;
    }
    bool read_tcp(Link& l) {
        std::array<char,4096> buf{};int n=::recv(l.socket,buf.data(),int(buf.size()),0);
        if(n==0)return false;
        if(n<0&&!would_block(net_error()))return false;
        if(n>0){count.rx_bytes+=n;if(l.rx.size()+std::size_t(n)>8192)return false;l.rx.insert(l.rx.end(),buf.data(),buf.data()+n);}
        unsigned messages=0;
        while(l.rx.size()>=2&&messages++<32) {
            unsigned len=(unsigned(l.rx[0])<<8)|l.rx[1];if(len<7||len>max_packet)return false;
            if(l.rx.size()<len+2)break;
            Bytes b(l.rx.begin()+2,l.rx.begin()+2+len);l.rx.erase(l.rx.begin(),l.rx.begin()+2+len);
            if(!message(l,b))return false;
            if(state==State::error||state==State::offline)return true;
            l.heard=now;if(l.closing)break;
        }
        return true;
    }
    void accept_tcp() {
        for(unsigned attempts=0;attempts<8;++attempts){sockaddr_in addr{};SockLen len=sizeof(addr);Socket fd=::accept(listener,reinterpret_cast<sockaddr*>(&addr),&len);
            if(fd==invalid_socket)break;
            auto it=std::find_if(links.begin(),links.end(),[](const Link& l){return l.socket==invalid_socket;});
            if(it==links.end()||!nonblocking(fd)){socket_close(fd);continue;}
            tcp_options(fd);it->socket=fd;it->tcp=addr;it->opened=it->heard=now;
        }
    }
    void send_udp(const sockaddr_in& dest,const Writer& w) {
        if(w.bytes.size()>max_packet){++count.rejected;return;}
        int n=::sendto(udp,reinterpret_cast<const char*>(w.bytes.data()),int(w.bytes.size()),send_flags(),reinterpret_cast<const sockaddr*>(&dest),sizeof(dest));
        if(n>0){count.tx_bytes+=n;++count.udp_sent;}
    }
    Writer udp_header(Datagram kind,std::uint64_t key,std::uint32_t seq) {
        Writer w;w.u32(magic);w.u16(version);w.u8(unsigned(kind));w.u64(session);w.u64(key);w.u32(seq);w.u64(millis());return w;
    }
    void receive_udp() {
        // One extra byte detects/trashes oversized datagrams on POSIX; Winsock
        // reports WSAEMSGSIZE. Never parse a silently truncated packet.
        std::array<std::uint8_t,max_packet+1> buf{};
        for(unsigned k=0;k<64;++k){sockaddr_in source{};SockLen len=sizeof(source);
            int n=::recvfrom(udp,reinterpret_cast<char*>(buf.data()),int(buf.size()),0,reinterpret_cast<sockaddr*>(&source),&len);
            if(n<0){if(would_block(net_error()))break;++count.rejected;continue;}
            count.rx_bytes+=n;++count.udp_received;
            if(n>int(max_packet)||n<35){++count.rejected;continue;}
            Reader r(buf.data(),n);auto m=r.u32();auto v=r.u16();auto kind=Datagram(r.u8());auto sess=r.u64(),key=r.u64();auto seq=r.u32();auto stamp=r.u64();
            if(m!=magic||v!=version||sess!=session||stamp>8640000000ull){++count.rejected;continue;}
            if(server&&kind==Datagram::pose){
                Link* owner=nullptr;for(auto& l:links)if(l.id&&!l.closing&&l.token==key&&l.tcp.sin_addr.s_addr==source.sin_addr.s_addr){owner=&l;break;}
                if(!owner){++count.rejected;continue;}
                Pose p=r.pose();if(!r.done()||!valid_pose(p)){++count.rejected;continue;}
                auto* dest=by_id(owner->id);if(!dest)continue;
                if(dest->has_pose&&!newer(p.sequence,dest->latest.sequence))continue;
                owner->udp=source;owner->udp_known=true;owner->last_udp=now;
                dest->latest=p;dest->has_pose=true;dest->received=now;dest->history.push(p,now-start);
            } else if(!server&&state==State::joined&&kind==Datagram::snapshot&&key==token&&same_endpoint(source,destination)){
                if(got_snapshot&&!newer(seq,last_snapshot))continue;
                unsigned entries=r.u8();if(entries>max_players){++count.rejected;continue;}
                struct Entry{std::uint32_t id;std::uint64_t time;Pose p;};std::array<Entry,max_players> list{};bool valid=true;
                for(unsigned i=0;i<entries;++i){list[i].id=r.u32();list[i].time=r.u64();list[i].p=r.pose();
                    if(!list[i].id||list[i].time>stamp||!valid_pose(list[i].p))valid=false;
                    for(unsigned j=0;j<i;++j)if(list[i].id==list[j].id)valid=false;}
                if(!r.done()||!valid){++count.rejected;continue;}
                double observed=now-double(stamp)*.001;
                if(!clock_valid){clock_offset=observed;clock_valid=true;}
                else clock_offset=std::min(observed,clock_offset+.00005); // bounded skew drift, reject jitter spikes
                got_snapshot=true;last_snapshot=seq;
                for(unsigned i=0;i<entries;++i){auto* p=by_id(list[i].id);if(!p||p->identity.id==self_id)continue;
                    if(p->history.push(list[i].p,double(list[i].time)*.001)){p->latest=list[i].p;p->has_pose=true;p->received=now;}}
            }else ++count.rejected;
        }
    }
    void publish() {
        if(now<next_udp)return;next_udp=now+1.0/options.send_hz;
        if(server){++snapshot_sequence;
            for(auto& l:links)if(l.id&&l.udp_known&&!l.closing){auto w=udp_header(Datagram::snapshot,l.token,snapshot_sequence);
                unsigned n=0;for(const auto& p:people)if(p.identity.id&&p.has_pose&&now-p.received<3)++n;w.u8(n);
                for(const auto& p:people)if(p.identity.id&&p.has_pose&&now-p.received<3){w.u32(p.identity.id);w.u64(std::uint64_t(std::max(0.0,p.received-start)*1000));w.pose(p.latest);}send_udp(l.udp,w);}
        }else if(state==State::joined){auto w=udp_header(Datagram::pose,token,0);auto& p=people[local_seat];
            if(!p.has_pose){p.latest.sequence=++local_sequence;p.has_pose=true;}w.pose(p.latest);send_udp(destination,w);}
    }
    void poll(double t) {
        if(state==State::offline||state==State::error)return;
        if(!std::isfinite(t)||t<now)return;now=t;
        if(server)accept_tcp();
        for(auto& l:links){if(l.socket==invalid_socket)continue;
            if(l.connecting){int e=0;if(connection_complete(l.socket,e)){if(e){fail("Connessione rifiutata o host non raggiungibile.");return;}l.connecting=false;hello(l);}
                else if(now-l.opened>10){fail("Timeout connessione. Controlla IP Radmin, porta e firewall.");return;}else continue;}
            if(!flush(l)){drop(l);if(!server)return;continue;}
            if(l.closing){if(l.tx.empty()||now-l.closing_since>2)drop(l);continue;}
            if(!read_tcp(l)){drop(l);if(!server)return;continue;}
            if(state==State::offline||state==State::error)return;
            if((!l.id&&now-l.opened>10)||now-l.heard>15){drop(l);if(!server)return;continue;}
            if(l.id&&now>=l.next_ping){l.next_ping=now+1;l.ping_cookie=millis()+1;l.ping_time=now;
                auto w=control(Control::ping);w.u64(l.ping_cookie);if(!queue(l,w)){drop(l);if(!server)return;}}
            if(!flush(l)){drop(l);if(!server)return;}
        }
        receive_udp();publish();
    }
};
Session::Session():impl_(new Impl){}
Session::~Session(){impl_->clear();}
bool Session::host(const Options& o,double now){return impl_->host(o,now);}
bool Session::join(const Options& o,double now){return impl_->join(o,now);}
void Session::close(){auto w=control(Control::bye);for(auto& l:impl_->links)if(l.socket!=invalid_socket&&!l.connecting){impl_->queue(l,w);impl_->flush(l);}impl_->clear();impl_->failure.clear();}
void Session::poll(double now){impl_->poll(now);}
void Session::set_local_pose(Pose p){auto& i=*impl_;if(!active()||!valid_pose(p))return;p.sequence=++i.local_sequence;
    auto& person=i.people[i.local_seat];person.latest=p;person.has_pose=true;person.received=i.now;
    if(i.server)person.history.push(p,i.now-i.start);}
State Session::state()const{return impl_->state;}
bool Session::active()const{return impl_->state==State::hosting||impl_->state==State::joined;}
bool Session::is_host()const{return impl_->server&&impl_->state==State::hosting;}
std::uint32_t Session::local_id()const{return impl_->self_id;}
const std::array<Participant,max_players>& Session::players()const{return impl_->people;}
bool Session::sample(unsigned seat,double now,Pose& out)const{const auto& i=*impl_;if(seat>=max_players)return false;const auto& p=i.people[seat];
    if(!p.identity.id||!p.has_pose||now-p.received>2.0)return false;
    if(p.identity.id==i.self_id){out=p.latest;return (out.flags&ready)!=0;}
    if(!i.server&&!i.clock_valid)return false;
    double time=i.server?now-i.start:now-i.clock_offset;
    return p.history.sample(time-i.options.interpolation,out)&&(out.flags&ready)!=0;}
const std::string& Session::error()const{return impl_->failure;}
const Counters& Session::counters()const{return impl_->count;}
std::uint16_t Session::bound_port()const{return impl_->actual_port;}
} // namespace usm::online
#endif
