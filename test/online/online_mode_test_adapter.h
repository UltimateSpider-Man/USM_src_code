#pragma once
// Explicit test collaborators. NOT a Windows SDK or native engine ABI test.
#include <array>
#include <map>
#include <cstdint>
#include <cstring>
#include <string>
#ifdef __fastcall
#undef __fastcall
#endif
#ifdef __thiscall
#undef __thiscall
#endif
#define __fastcall
#define __thiscall
using Float=float;
using DWORD=std::uint32_t;
using HWND=void*;
struct LARGE_INTEGER{long long QuadPart=0;};
struct POINT{long x=0,y=0;};
struct RECT{long left=0,top=0,right=0,bottom=0;};
constexpr unsigned MAX_PATH=260;
constexpr int VK_UP=38,VK_DOWN=40,VK_LEFT=37,VK_RIGHT=39,VK_RETURN=13,VK_ESCAPE=27,VK_SPACE=32,VK_LBUTTON=1;
constexpr int VK_F6=117,VK_SHIFT=16,VK_BACK=8,VK_DELETE=46,VK_NUMPAD0=96,VK_NUMPAD9=105,VK_OEM_PERIOD=190,VK_DECIMAL=110,VK_OEM_MINUS=189,VK_SUBTRACT=109;
namespace online_test {
inline std::array<bool,256> keys{};inline bool focused=true,quit_dialog=false;
inline long long clock=1000000;inline POINT mouse{};
inline std::map<std::string,std::string> ini;
}
inline HWND GetForegroundWindow(){return reinterpret_cast<HWND>(std::uintptr_t(online_test::focused?1:2));}
inline DWORD GetCurrentProcessId(){return 1;}
inline DWORD GetWindowThreadProcessId(HWND w,DWORD* p){*p=w==reinterpret_cast<HWND>(1)?1:2;return 1;}
inline short GetAsyncKeyState(unsigned k){return online_test::keys[k]?short(0x8000):0;}
inline bool QueryPerformanceCounter(LARGE_INTEGER* v){v->QuadPart=online_test::clock;return true;}
inline bool QueryPerformanceFrequency(LARGE_INTEGER* v){v->QuadPart=1000000;return true;}
inline DWORD GetModuleFileNameA(void*,char*,DWORD){return 0;}
inline unsigned GetPrivateProfileIntA(const char*,const char*,int v,const char*){return unsigned(v);}
inline DWORD GetPrivateProfileStringA(const char*,const char*,const char* v,char* out,DWORD n,const char*){if(n){std::strncpy(out,v,n-1);out[n-1]=0;}return DWORD(std::strlen(v));}
inline bool WritePrivateProfileStringA(const char* section,const char* key,const char* value,const char*){online_test::ini[std::string(section)+"/"+key]=value;return true;}
inline void OutputDebugStringA(const char*){}
inline bool GetCursorPos(POINT* p){*p=online_test::mouse;return true;}
inline bool ScreenToClient(HWND,POINT*){return true;}
inline bool GetClientRect(HWND,RECT* r){r->right=1280;r->bottom=960;return true;}
inline int nglGetScreenWidth(){return 1280;}
inline int nglGetScreenHeight(){return 960;}
inline bool byte_922994(){return online_test::quit_dialog;}
struct FEMenu{};
struct PanelAnimFile{bool field_2D=false;};
struct main_menu_options:FEMenu{
    short field_104=0,field_106=0;bool field_108=false,field_109=false,field_10A=false;
    PanelAnimFile* field_E4=nullptr;int highlights=0;void update_highlight(){++highlights;}
};
struct InputState {
    int field_0=0;std::uint8_t m_flags=0;char field_5=0;
    std::uint8_t m_jump=0,m_stick_to_walls=0,m_punch=0,m_kick=0,m_black_button=0,m_throw_web=0,field_C=0,field_D=0;
    int field_10=0,field_14=0,field_18=0,field_1C=0;
};
