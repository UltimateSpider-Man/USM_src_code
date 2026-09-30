#pragma once
// TEST ONLY. These are deterministic OS/engine substitutes, NOT Windows SDK
// declarations and NOT proof of native binary/hardware compatibility.
#include <array>
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
struct LARGE_INTEGER {long long QuadPart=0;};
struct POINT {long x=0,y=0;};
struct RECT {long left=0,top=0,right=0,bottom=0;};
constexpr unsigned MAX_PATH=260;
constexpr int VK_UP=38,VK_DOWN=40,VK_LEFT=37,VK_RIGHT=39,VK_RETURN=13,VK_ESCAPE=27,VK_SPACE=32,VK_LBUTTON=1;
constexpr int VK_NUMPAD0=96,VK_NUMPAD1=97,VK_NUMPAD2=98,VK_NUMPAD3=99,VK_NUMPAD4=100,VK_NUMPAD5=101;
namespace mode_test {
inline std::array<bool,256> keys{};
inline bool focused=true,quit_dialog=false;
inline long long clock=1000000;
inline POINT mouse{};
}
inline HWND GetForegroundWindow(){return reinterpret_cast<HWND>(std::uintptr_t(mode_test::focused?1:2));}
inline DWORD GetCurrentProcessId(){return 1;}
inline DWORD GetWindowThreadProcessId(HWND w,DWORD *pid){*pid=w==reinterpret_cast<HWND>(1)?1:2;return 1;}
inline short GetAsyncKeyState(int n){return mode_test::keys[n]?short(0x8000):0;}
inline bool QueryPerformanceCounter(LARGE_INTEGER *v){v->QuadPart=mode_test::clock;return true;}
inline bool QueryPerformanceFrequency(LARGE_INTEGER *v){v->QuadPart=1000000;return true;}
inline DWORD GetModuleFileNameA(void*,char*,DWORD){return 0;}
inline unsigned GetPrivateProfileIntA(const char*,const char*,int fallback,const char*){return static_cast<unsigned>(fallback);}
inline DWORD GetPrivateProfileStringA(const char*,const char*,const char *value,char *out,DWORD count,const char*) {
    if(count){std::strncpy(out,value,count-1);out[count-1]=0;}return static_cast<DWORD>(std::strlen(value));
}
inline void OutputDebugStringA(const char*){}
inline bool GetCursorPos(POINT *p){*p=mode_test::mouse;return true;}
inline bool ScreenToClient(HWND,POINT*){return true;}
inline bool GetClientRect(HWND,RECT *r){r->right=960;r->bottom=600;return true;}
inline int nglGetScreenWidth(){return 960;}
inline int nglGetScreenHeight(){return 600;}
inline bool byte_922994(){return mode_test::quit_dialog;}
struct FEMenu {};
struct PanelAnimFile {bool field_2D=false;};
struct main_menu_options : FEMenu {
    short field_104=0,field_106=0;
    bool field_108=false,field_109=false;
    PanelAnimFile *field_E4=nullptr;
    int highlights=0;
    void update_highlight(){++highlights;}
};
struct actor {};
