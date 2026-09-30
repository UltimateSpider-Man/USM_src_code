#if (defined(_WIN32) || defined(USM_MULTIPLAYER_NET_TEST)) && !defined(OPENUSM_XBPACK_MODE)
#if defined(USM_MULTIPLAYER_NET_TEST)
#include "multiplayer_posix_test_adapter.h"
#else
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h> // Must precede windows.h, including indirect inclusions.
#include <ws2tcpip.h>
#include <windows.h>
#endif
#include "multiplayer_lan.h"
#include "multiplayer_protocol.h"
#include <algorithm>
#include <deque>
#include <limits>
#include <vector>

namespace usm::mp {
namespace {
SOCKET connection=INVALID_SOCKET,listener=INVALID_SOCKET;
LanState state=LanState::off;
bool wsa=false,host_role=false,hello_received=false,local_ready=false,remote_ready=false;
Settings chosen;
std::string error;
std::vector<std::uint8_t> outgoing;
wire::Decoder decoder;
std::deque<wire::Packet> incoming;
ULONGLONG last_activity=0;
std::uint32_t remote_next=0,sent_frame=std::numeric_limits<std::uint32_t>::max();
Input sent_input{};
void close_sockets() {
    if(connection!=INVALID_SOCKET){closesocket(connection);connection=INVALID_SOCKET;}
    if(listener!=INVALID_SOCKET){closesocket(listener);listener=INVALID_SOCKET;}
    if(wsa){WSACleanup();wsa=false;}
}
void fail(const std::string &why){error=why;state=LanState::error;close_sockets();}
bool nonblocking(SOCKET s){u_long yes=1;return ioctlsocket(s,FIONBIO,&yes)==0;}
bool enqueue(wire::Packet p) {
    if(outgoing.size()+wire::packet_size>4096){fail("LAN send queue exceeded its bound.");return false;}
    p.player=host_role?0:1;const auto b=wire::encode(p);outgoing.insert(outgoing.end(),b.begin(),b.end());return true;
}
void connected() {
    int yes=1;setsockopt(connection,IPPROTO_TCP,TCP_NODELAY,reinterpret_cast<const char*>(&yes),sizeof(yes));
    state=LanState::handshake;last_activity=GetTickCount64();wire::Packet p;p.kind=wire::Kind::hello;enqueue(p);
}
void check_ready(){if(local_ready&&remote_ready){state=LanState::playing;last_activity=GetTickCount64();}}
bool receive_packet(const wire::Packet &p) {
    if(p.player!=(host_role?1:0)){fail("LAN peer sent an invalid player role.");return false;}
    if(p.kind==wire::Kind::bye){fail("The other player left the match.");return false;}
    if(p.kind==wire::Kind::hello) {
        if(hello_received||state!=LanState::handshake){fail("Unexpected LAN handshake.");return false;}
        hello_received=true;
        if(host_role){wire::Packet settings;settings.kind=wire::Kind::settings;settings.settings=chosen;
            if(!enqueue(settings))return false;
            state=LanState::assets;}
        return true;
    }
    if(!hello_received){fail("LAN message arrived before the handshake.");return false;}
    if(p.kind==wire::Kind::settings) {
        if(host_role||state!=LanState::handshake){fail("Unexpected LAN match settings.");return false;}
        chosen=p.settings;state=LanState::assets;return true;
    }
    if(p.kind==wire::Kind::ready) {
        if(remote_ready||(state!=LanState::assets&&state!=LanState::waiting)){fail("Unexpected LAN ready message.");return false;}
        remote_ready=true;check_ready();return true;
    }
    if(p.kind==wire::Kind::input) {
        if(state!=LanState::playing||p.frame!=remote_next||incoming.size()>=4){fail("LAN input sequence is invalid or too far ahead.");return false;}
        incoming.push_back(p);++remote_next;return true;
    }
    fail("Unknown LAN message.");return false;
}
void flush() {
    while(!outgoing.empty()&&connection!=INVALID_SOCKET) {
        int n=send(connection,reinterpret_cast<const char*>(outgoing.data()),static_cast<int>(outgoing.size()),0);
        if(n>0){outgoing.erase(outgoing.begin(),outgoing.begin()+n);continue;}
        if(n<0&&WSAGetLastError()==WSAEWOULDBLOCK)return;
        fail("LAN send failed or the peer disconnected.");return;
    }
}
}
void lan_close() {
    // No blocking drain at teardown. Closing TCP is sufficient; the peer detects EOF.
    close_sockets();state=LanState::off;outgoing.clear();incoming.clear();decoder.reset();
    hello_received=local_ready=remote_ready=false;remote_next=0;sent_frame=std::numeric_limits<std::uint32_t>::max();error.clear();
}
bool lan_begin(bool host,const std::string &ipv4,std::uint16_t port,const Settings &settings) {
    lan_close();if(!port){error="LAN port must be between 1 and 65535.";state=LanState::error;return false;}
    WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data)!=0){error="Winsock initialization failed.";state=LanState::error;return false;}
    wsa=true;host_role=host;chosen=settings;chosen.sanitize();last_activity=GetTickCount64();
    SOCKET s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(s==INVALID_SOCKET){fail("Cannot create a LAN socket.");return false;}
    if(!nonblocking(s)){closesocket(s);fail("Cannot make the LAN socket nonblocking.");return false;}
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(port);
    if(host) {
        listener=s;BOOL exclusive=TRUE;
        setsockopt(s,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<const char*>(&exclusive),sizeof(exclusive));
        address.sin_addr.s_addr=htonl(INADDR_ANY);
        if(bind(s,reinterpret_cast<sockaddr*>(&address),sizeof(address))==SOCKET_ERROR||listen(s,1)==SOCKET_ERROR){fail("Cannot listen on this LAN port. Another process may be using it.");return false;}
        state=LanState::listening;
    } else {
        connection=s;
        // Literal IPv4 only: no synchronous DNS or unbounded name resolution in the game thread.
        if(InetPtonA(AF_INET,ipv4.c_str(),&address.sin_addr)!=1){fail("HostIP in multiplayer.ini must be a literal IPv4 address.");return false;}
        int status=connect(s,reinterpret_cast<sockaddr*>(&address),sizeof(address));
        if(status==0)connected();
        else if(WSAGetLastError()==WSAEWOULDBLOCK||WSAGetLastError()==WSAEINPROGRESS)state=LanState::connecting;
        else {fail("Cannot connect to the LAN host.");return false;}
    }
    return true;
}
void lan_pump() {
    if(state==LanState::off||state==LanState::error)return;
    if(state==LanState::listening) {
        connection=accept(listener,nullptr,nullptr);
        if(connection==INVALID_SOCKET){if(WSAGetLastError()!=WSAEWOULDBLOCK)fail("LAN accept failed.");return;}
        closesocket(listener);listener=INVALID_SOCKET;
        if(!nonblocking(connection)){fail("Cannot configure the accepted LAN socket.");return;}connected();
    }
    if(state==LanState::connecting) {
        fd_set write_set,except_set;FD_ZERO(&write_set);FD_ZERO(&except_set);FD_SET(connection,&write_set);FD_SET(connection,&except_set);timeval wait{};
        int n=select(0,nullptr,&write_set,&except_set,&wait);
        if(n==SOCKET_ERROR){fail("LAN connection check failed.");return;}
        if(n>0){int result=0,len=sizeof(result);
            if(getsockopt(connection,SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&result),&len)!=0||result){fail("LAN connection was refused.");return;}connected();}
        else if(GetTickCount64()-last_activity>10000){fail("LAN connection timed out after 10 seconds.");return;}
        else return;
    }
    flush();if(state==LanState::error)return;
    std::uint8_t buffer[512];
    for(int budget=0;budget<4096;budget+=static_cast<int>(sizeof(buffer))) {
        int n=recv(connection,reinterpret_cast<char*>(buffer),sizeof(buffer),0);
        if(n==0){fail("The LAN connection was closed.");return;}
        if(n<0){if(WSAGetLastError()!=WSAEWOULDBLOCK)fail("LAN receive failed.");break;}
        last_activity=GetTickCount64();
        if(!decoder.append(buffer,static_cast<std::size_t>(n),receive_packet)) {
            if(state!=LanState::error)fail("LAN packet rejected: incompatible version/rules or malformed data.");
            return;
        }
    }
    if(state!=LanState::error&&GetTickCount64()-last_activity>(state==LanState::playing?15000u:120000u))fail("The LAN peer stopped responding.");
    if(state!=LanState::error)flush();
}
LanState lan_state(){return state;}
const std::string &lan_error(){return error;}
const Settings &lan_settings(){return chosen;}
bool lan_is_host(){return host_role;}
void lan_ready() {
    if(local_ready||(state!=LanState::assets&&state!=LanState::waiting))return;
    wire::Packet p;p.kind=wire::Kind::ready;if(!enqueue(p))return;
    local_ready=true;state=LanState::waiting;last_activity=GetTickCount64();check_ready();flush();
}
bool lan_exchange(std::uint32_t frame,Input local,std::uint32_t checksum,std::array<Input,2> &input) {
    if(state!=LanState::playing)return false;
    if(sent_frame!=frame) {
        if(sent_frame!=std::numeric_limits<std::uint32_t>::max()){fail("Local LAN tick sequence changed while waiting for input.");return false;}
        wire::Packet p;p.kind=wire::Kind::input;p.frame=frame;p.checksum=checksum;p.buttons=local.held&valid_buttons;
        if(!enqueue(p))return false;
        sent_input.held=p.buttons;sent_frame=frame;flush();
    }
    if(incoming.empty())return false;
    const auto p=incoming.front();
    if(p.frame!=frame||p.checksum!=checksum){fail("LAN simulation desynchronized. Both PCs must use the same source build and match rules.");return false;}
    input[host_role?0:1]=sent_input;input[host_role?1:0].held=p.buttons;incoming.pop_front();
    sent_frame=std::numeric_limits<std::uint32_t>::max();return true;
}
} // namespace usm::mp
#endif
