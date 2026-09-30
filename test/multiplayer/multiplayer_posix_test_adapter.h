#pragma once
// TEST ONLY: exercise the production nonblocking state machine with real POSIX
// sockets. This is not evidence that the Windows ABI or Winsock build compiles.
#ifndef USM_MULTIPLAYER_NET_TEST
#error This adapter is only for the standalone transport tests.
#endif
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
using SOCKET=int;
using BOOL=int;
using ULONGLONG=std::uint64_t;
struct WSADATA{};
constexpr SOCKET INVALID_SOCKET=-1;
constexpr int SOCKET_ERROR=-1,TRUE=1;
constexpr int WSAEWOULDBLOCK=EWOULDBLOCK,WSAEINPROGRESS=EINPROGRESS;
// An unsupported option leaves POSIX's default exclusive bind behavior intact.
constexpr int SO_EXCLUSIVEADDRUSE=0x7fff;
inline int MAKEWORD(int low,int high){return low|(high<<8);}
inline int WSAStartup(int,WSADATA*){return 0;}
inline int WSACleanup(){return 0;}
inline int WSAGetLastError(){return errno;}
inline int closesocket(SOCKET s){return ::close(s);}
inline int ioctlsocket(SOCKET s,unsigned long op,unsigned long *value){return ::ioctl(s,op,value);}
inline int InetPtonA(int af,const char *text,void *address){return ::inet_pton(af,text,address);}
inline ULONGLONG GetTickCount64(){return static_cast<ULONGLONG>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
inline int test_select(int,fd_set *read,fd_set *write,fd_set *except,timeval *wait){
    int high=-1;
    for(int fd=0;fd<FD_SETSIZE;++fd)
        if((read&&FD_ISSET(fd,read))||(write&&FD_ISSET(fd,write))||(except&&FD_ISSET(fd,except)))high=fd;
    return ::select(high+1,read,write,except,wait);
}
inline int test_getsockopt(SOCKET s,int level,int option,char *value,int *length){
    socklen_t len=static_cast<socklen_t>(*length);const int result=::getsockopt(s,level,option,value,&len);*length=static_cast<int>(len);return result;
}
inline int test_send(SOCKET s,const char *data,int size,int flags){return static_cast<int>(::send(s,data,static_cast<std::size_t>(size),flags|MSG_NOSIGNAL));}
#define select test_select
#define getsockopt test_getsockopt
#define send test_send
