#include "multiplayer_online_interfaces.h"
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
#include <iphlpapi.h>
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#endif
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <memory>
namespace usm::online {
std::vector<LocalAddress> local_ipv4_addresses(){
    std::vector<LocalAddress> result;
    auto append=[&](const std::string& name,const sockaddr* address){
        if(!address||address->sa_family!=AF_INET)return;
        const auto* in=reinterpret_cast<const sockaddr_in*>(address);
        auto value=ntohl(in->sin_addr.s_addr);if(value==0||(value>>24)==127)return;
        char ip[16]{};std::snprintf(ip,sizeof(ip),"%u.%u.%u.%u",unsigned(value>>24),unsigned((value>>16)&255),unsigned((value>>8)&255),unsigned(value&255));
        if(std::find_if(result.begin(),result.end(),[&](const LocalAddress& a){return a.ipv4==ip;})==result.end())result.push_back({name,ip});
    };
#ifdef _WIN32
    ULONG size=15000;
    for(unsigned attempt=0;attempt<3&&size<=1024*1024;++attempt){
        // Aligned storage for the OS's linked structs, not an unaligned byte array.
        std::unique_ptr<std::max_align_t[]> storage(new std::max_align_t[(size+sizeof(std::max_align_t)-1)/sizeof(std::max_align_t)]);
        auto* first=reinterpret_cast<IP_ADAPTER_ADDRESSES*>(storage.get());
        auto code=GetAdaptersAddresses(AF_INET,GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|GAA_FLAG_SKIP_DNS_SERVER,nullptr,first,&size);
        if(code==ERROR_BUFFER_OVERFLOW)continue;
        if(code!=NO_ERROR)break;
        for(auto* a=first;a;a=a->Next){if(a->OperStatus!=IfOperStatusUp)continue;
            std::string name;const wchar_t* wide=a->FriendlyName;
            if(wide)for(unsigned i=0;i<40&&wide[i];++i)name.push_back(wide[i]>=32&&wide[i]<127?char(wide[i]):'?');
            for(auto* u=a->FirstUnicastAddress;u;u=u->Next)append(name,u->Address.lpSockaddr);}
        break;
    }
#else
    ifaddrs* first=nullptr;
    if(getifaddrs(&first)==0){for(auto* a=first;a;a=a->ifa_next)if(a->ifa_flags&IFF_UP)append(a->ifa_name?a->ifa_name:"",a->ifa_addr);freeifaddrs(first);}
#endif
    // Prefer an explicitly named Radmin interface, never guess it from an IP
    // prefix. Other valid LAN/VPN addresses remain selectable.
    auto rank=[](std::string name){for(auto& c:name)if(c>='A'&&c<='Z')c=char(c+32);return name.find("radmin")!=std::string::npos;};
    std::stable_sort(result.begin(),result.end(),[&](const LocalAddress& a,const LocalAddress& b){return rank(a.adapter)>rank(b.adapter);});
    return result;
}
}
#endif
