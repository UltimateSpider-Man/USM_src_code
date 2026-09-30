#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "multiplayer_gamepad_win.h"
#if defined(_WIN32) && !defined(OPENUSM_XBPACK_MODE)
// The project globally defines CINTERFACE. Use its COM calling convention
// explicitly here as well, including standalone Windows compile probes.
#ifndef CINTERFACE
#define CINTERFACE
#endif
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <windows.h>
#include <dinput.h>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cwctype>
#include <vector>

namespace usm::mp {
namespace {
struct XiData { WORD buttons;BYTE lt,rt;SHORT lx,ly,rx,ry; };
struct XiState { DWORD packet;XiData pad; };
static_assert(sizeof(XiState)==16,"XInput ABI");
using XiGet=DWORD(WINAPI*)(DWORD,XiState*);
using DiCreate=HRESULT(WINAPI*)(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN);
HMODULE xi_module=nullptr,di_module=nullptr;
XiGet xi_get=nullptr;
IDirectInput8W *di=nullptr;
struct Device {
    GUID guid{};
    IDirectInputDevice8W *object=nullptr;
    pad::Mapping mapping;
    std::array<LONG,8> minimum{},maximum{};
    std::array<bool,8> axis{};
    std::array<bool,4> pov{};
    std::string id,name;
    bool seen=false;
};
std::array<Device,8> devices;
pad::Seats seats;
std::string ini,diagnostic;
bool ready=false;
int backend=0,deadzone=9000; // 0 auto, 1 XInput only, 2 DirectInput only.
HWND owner=nullptr;
double scan_elapsed=1.0;
constexpr DWORD offsets[]{DIJOFS_X,DIJOFS_Y,DIJOFS_Z,DIJOFS_RX,DIJOFS_RY,DIJOFS_RZ,DIJOFS_SLIDER(0),DIJOFS_SLIDER(1)};

HMODULE system_library(const wchar_t *name) {
    wchar_t directory[MAX_PATH]{};
    const UINT n=GetSystemDirectoryW(directory,MAX_PATH);
    if(!n||n>=MAX_PATH-40)return nullptr;
    const auto path=std::wstring(directory)+L"\\"+name;
    return LoadLibraryW(path.c_str());
}
std::string utf8(const wchar_t *text) {
    if(!text||!*text)return {};
    const int n=WideCharToMultiByte(CP_UTF8,0,text,-1,nullptr,0,nullptr,nullptr);
    if(n<=1)return {};
    std::string out(static_cast<std::size_t>(n),'\0');
    WideCharToMultiByte(CP_UTF8,0,text,-1,&out[0],n,nullptr,nullptr);out.pop_back();return out;
}
std::string guid_id(const GUID &g) {
    char b[80]{};
    std::snprintf(b,sizeof(b),"DI:%08lx-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x",
        static_cast<unsigned long>(g.Data1),g.Data2,g.Data3,g.Data4[0],g.Data4[1],g.Data4[2],g.Data4[3],g.Data4[4],g.Data4[5],g.Data4[6],g.Data4[7]);
    return b;
}
std::string setting(const char *section,const char *key,const char *fallback="") {
    char v[128]{};GetPrivateProfileStringA(section,key,fallback,v,sizeof(v),ini.c_str());return v;
}
std::string lower(std::string s){for(auto &c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return s;}
int integer(const char *section,const char *key,int fallback,int minimum,int maximum) {
    const auto text=setting(section,key);if(text.empty())return fallback;
    char *end=nullptr;const long v=std::strtol(text.c_str(),&end,10);
    return end&&end!=text.c_str()&&!*end&&v>=minimum&&v<=maximum?static_cast<int>(v):fallback;
}
void overrides(pad::Mapping &m,const char *section) {
    m.guide_button=integer(section,"Guide",m.guide_button+1,0,128)-1;
    constexpr const char *names[]{"South","East","West","North","L1","R1","Select","Start","L2","R2","L3","R3"};
    for(unsigned n=0;n<12;++n)m.button[n]=integer(section,names[n],m.button[n]+1,0,128)-1;
    constexpr const char *dnames[]{"DpadUp","DpadDown","DpadLeft","DpadRight"};
    for(unsigned n=0;n<4;++n)m.dpad_button[n]=integer(section,dnames[n],m.dpad_button[n]+1,0,128)-1;
    m.axis_x=integer(section,"AxisX",m.axis_x,-1,7);m.axis_y=integer(section,"AxisY",m.axis_y,-1,7);
    m.pov=integer(section,"POV",m.pov,-1,3);
    m.axis_rx=integer(section,"AxisRX",m.axis_rx,-1,7);m.axis_ry=integer(section,"AxisRY",m.axis_ry,-1,7);
    m.invert_rx=integer(section,"InvertRX",m.invert_rx?1:0,0,1)!=0;
    m.invert_ry=integer(section,"InvertRY",m.invert_ry?1:0,0,1)!=0;
    m.invert_x=integer(section,"InvertX",m.invert_x?1:0,0,1)!=0;
    m.invert_y=integer(section,"InvertY",m.invert_y?1:0,0,1)!=0;
}
pad::Mapping mapping(unsigned vid,unsigned pid) {
    char section[48]{};std::snprintf(section,sizeof(section),"DirectInput_%04X_%04X",vid,pid);
    auto layout=lower(setting(section,"Layout"));if(layout.empty())layout=lower(setting("DirectInput","Layout","auto"));
    const bool ps=layout=="playstation"||(layout!="generic"&&pad::sony_id(vid,pid));
    auto out=pad::default_mapping(ps);overrides(out,"DirectInput");overrides(out,section);return out;
}
void release(Device &d){if(d.object){d.object->lpVtbl->Unacquire(d.object);d.object->lpVtbl->Release(d.object);}d=Device{};}
bool contains_ig(const wchar_t *text) {
    std::wstring s=text?text:L"";for(auto &c:s)c=std::towupper(c);return s.find(L"IG_")!=std::wstring::npos;
}
// XUSB controllers enumerate through both APIs. Prefer XInput for those devices.
// Virtual-pad remappers may also expose an unrelated physical HID; their identity
// cannot be inferred reliably. Backend=xinput is the explicit duplicate escape hatch.
bool xinput_device(IDirectInputDevice8W *object,unsigned vid,unsigned pid) {
    if(backend==2||!xi_get)return false;
    DIPROPGUIDANDPATH path{};path.diph.dwSize=sizeof(path);path.diph.dwHeaderSize=sizeof(path.diph);path.diph.dwHow=DIPH_DEVICE;
    if(SUCCEEDED(object->lpVtbl->GetProperty(object, DIPROP_GUIDANDPATH,&path.diph))&&contains_ig(path.wszPath))return true;
    // Fallback for drivers that don't implement DIPROP_GUIDANDPATH.
    UINT count=0;if(GetRawInputDeviceList(nullptr,&count,sizeof(RAWINPUTDEVICELIST))!=0||count>256)return false;
    std::vector<RAWINPUTDEVICELIST> list(count);
    if(count&&GetRawInputDeviceList(list.data(),&count,sizeof(RAWINPUTDEVICELIST))==UINT(-1))return false;
    list.resize(std::min<std::size_t>(list.size(),count));
    for(const auto &r:list) {
        if(r.dwType!=RIM_TYPEHID)continue;
        RID_DEVICE_INFO info{};info.cbSize=sizeof(info);UINT bytes=sizeof(info);
        if(GetRawInputDeviceInfoW(r.hDevice,RIDI_DEVICEINFO,&info,&bytes)==UINT(-1)||info.hid.dwVendorId!=vid||info.hid.dwProductId!=pid)continue;
        wchar_t name[1024]{};UINT chars=1024;
        if(GetRawInputDeviceInfoW(r.hDevice,RIDI_DEVICENAME,name,&chars)!=UINT(-1)&&chars<1024&&contains_ig(name))return true;
    }
    return false;
}
BOOL CALLBACK enumerate(const DIDEVICEINSTANCEW *instance,void*) {
    for(auto &d:devices)if(d.object&&IsEqualGUID(d.guid,instance->guidInstance)){d.seen=true;return DIENUM_CONTINUE;}
    Device *slot=nullptr;for(auto &d:devices)if(!d.object){slot=&d;break;}
    if(!slot)return DIENUM_STOP;
    IDirectInputDevice8W *object=nullptr;
    if(FAILED(di->lpVtbl->CreateDevice(di, instance->guidInstance,&object,nullptr)))return DIENUM_CONTINUE;
    const unsigned vid=LOWORD(instance->guidProduct.Data1),pid=HIWORD(instance->guidProduct.Data1);
    if(xinput_device(object,vid,pid)||FAILED(object->lpVtbl->SetDataFormat(object, &c_dfDIJoystick2))
        ||FAILED(object->lpVtbl->SetCooperativeLevel(object, owner,DISCL_FOREGROUND|DISCL_NONEXCLUSIVE))) {object->lpVtbl->Release(object);return DIENUM_CONTINUE;}
    slot->object=object;slot->guid=instance->guidInstance;slot->seen=true;slot->id=guid_id(slot->guid);
    slot->name=utf8(instance->tszProductName);slot->mapping=mapping(vid,pid);
    if(slot->name.empty())slot->name="DirectInput gamepad";
    char detail[48]{};std::snprintf(detail,sizeof(detail)," [DI %04X:%04X]",vid,pid);slot->name+=detail;
    for(unsigned n=0;n<8;++n) {
        DIDEVICEOBJECTINSTANCEW obj{};obj.dwSize=sizeof(obj);
        if(FAILED(object->lpVtbl->GetObjectInfo(object, &obj,offsets[n],DIPH_BYOFFSET)))continue;
        DIPROPRANGE range{};range.diph.dwSize=sizeof(range);range.diph.dwHeaderSize=sizeof(range.diph);
        range.diph.dwObj=offsets[n];range.diph.dwHow=DIPH_BYOFFSET;range.lMin=-32767;range.lMax=32767;
        // Absolute axes only. A wheel/relative mouse-like axis must not drive movement.
        if(!(obj.dwType&DIDFT_ABSAXIS))continue;
        const HRESULT configured=object->lpVtbl->SetProperty(object, DIPROP_RANGE,&range.diph);
        if(FAILED(configured)&&FAILED(object->lpVtbl->GetProperty(object, DIPROP_RANGE,&range.diph)))continue;
        if(range.lMax<=range.lMin)continue;
        slot->axis[n]=true;slot->minimum[n]=range.lMin;slot->maximum[n]=range.lMax;
    }
    for(unsigned n=0;n<4;++n) {
        DIDEVICEOBJECTINSTANCEW obj{};obj.dwSize=sizeof(obj);
        slot->pov[n]=SUCCEEDED(object->lpVtbl->GetObjectInfo(object,&obj,DIJOFS_POV(n),DIPH_BYOFFSET))&&(obj.dwType&DIDFT_POV)!=0;
    }
    object->lpVtbl->Acquire(object);return DIENUM_CONTINUE;
}
BOOL CALLBACK find_window(HWND window,LPARAM result) {
    DWORD pid=0;GetWindowThreadProcessId(window,&pid);
    if(pid==GetCurrentProcessId()&&IsWindowVisible(window)&&!GetWindow(window,GW_OWNER)){
        *reinterpret_cast<HWND*>(result)=window;return FALSE;
    }
    return TRUE;
}
HWND game_window() {
    HWND window=GetForegroundWindow();DWORD pid=0;GetWindowThreadProcessId(window,&pid);
    if(pid==GetCurrentProcessId())return GetAncestor(window,GA_ROOT);
    window=nullptr;EnumWindows(find_window,reinterpret_cast<LPARAM>(&window));return window;
}
void init() {
    if(ready)return;ready=true;
    const auto choice=lower(setting("Gamepads","Backend","auto"));backend=choice=="xinput"?1:choice=="directinput"?2:0;
    deadzone=integer("Gamepads","Deadzone",9000,2000,25000);
    if(backend!=2)for(const wchar_t *file:{L"xinput1_4.dll",L"xinput1_3.dll",L"xinput9_1_0.dll"}) {
        xi_module=system_library(file);if(!xi_module)continue;
        xi_get=reinterpret_cast<XiGet>(GetProcAddress(xi_module,"XInputGetState"));
        if(xi_get)break;FreeLibrary(xi_module);xi_module=nullptr;
    }
    if(backend!=1) {
        di_module=system_library(L"dinput8.dll");
        if(di_module) {
            auto create=reinterpret_cast<DiCreate>(GetProcAddress(di_module,"DirectInput8Create"));
            if(create&&FAILED(create(GetModuleHandleW(nullptr),DIRECTINPUT_VERSION,IID_IDirectInput8W,reinterpret_cast<void**>(&di),nullptr)))di=nullptr;
        }
    }
    diagnostic="Input: ";diagnostic+=xi_get?"XInput ":"";diagnostic+=di?"DirectInput8":"";
    if(!xi_get&&!di)diagnostic+="unavailable; keyboards remain active";
}
} // namespace
void gamepads_configure(const std::string &path){if(ini!=path){gamepads_shutdown();ini=path;}init();}
int gamepads_deadzone(){return deadzone;}
std::string gamepads_diagnostic(){return diagnostic;}
void gamepads_lock(bool locked){seats.lock(locked);}
std::array<pad::Sample,2> gamepads_poll(bool focused,double elapsed) {
    init();std::vector<pad::Sample> candidates;
    if(xi_get)for(DWORD index=0;index<4;++index) {
        XiState value{};if(xi_get(index,&value)!=ERROR_SUCCESS)continue;
        auto s=pad::map_xinput(value.pad.buttons,value.pad.lx,value.pad.ly,value.pad.rx,value.pad.ry,value.pad.lt,value.pad.rt);
        s.id="XI:"+std::to_string(index);s.name="XInput controller "+std::to_string(index+1);candidates.push_back(s);
    }
    if(di) {
        HWND window=game_window();
        if(window!=owner) {for(auto &d:devices)release(d);owner=window;scan_elapsed=1;}
        scan_elapsed+=std::clamp(elapsed,0.0,1.0);
        if(owner&&scan_elapsed>=1.0) {
            scan_elapsed=0;
            // Remove disconnected entries before enumeration to make room for replacements.
            for(auto &d:devices)if(d.object&&FAILED(di->lpVtbl->GetDeviceStatus(di, d.guid)))release(d);
            for(auto &d:devices)d.seen=false;
            const HRESULT result=di->lpVtbl->EnumDevices(di, DI8DEVCLASS_GAMECTRL,enumerate,nullptr,DIEDFL_ATTACHEDONLY);
            if(SUCCEEDED(result))for(auto &d:devices)if(d.object&&!d.seen)release(d);
        }
        for(auto &d:devices)if(d.object) {
            pad::Sample s;s.id=d.id;s.name=d.name;s.connected=true;
            if(!focused) {d.object->lpVtbl->Unacquire(d.object);candidates.push_back(s);continue;}
            HRESULT poll=d.object->lpVtbl->Poll(d.object);
            if(FAILED(poll)) {d.object->lpVtbl->Acquire(d.object);poll=d.object->lpVtbl->Poll(d.object);} // bounded, no busy reacquire loop
            DIJOYSTATE2 value{};
            if(FAILED(poll)||FAILED(d.object->lpVtbl->GetDeviceState(d.object, sizeof(value),&value))) {
                // A neutral disconnected sample prevents stale held input. Seat identity is retained in matches.
                s.connected=false;candidates.push_back(s);continue;
            }
            pad::DirectSample raw;
            const LONG axes[]{value.lX,value.lY,value.lZ,value.lRx,value.lRy,value.lRz,value.rglSlider[0],value.rglSlider[1]};
            for(unsigned n=0;n<8;++n)if(d.axis[n])raw.axis[n]=pad::normalize_axis(axes[n],d.minimum[n],d.maximum[n]);
            for(unsigned n=0;n<128;++n)raw.button[n]=value.rgbButtons[n];
            for(unsigned n=0;n<4;++n)if(d.pov[n])raw.pov[n]=value.rgdwPOV[n];
            auto mapped=pad::map_direct(raw,d.mapping);mapped.id=d.id;mapped.name=d.name;candidates.push_back(mapped);
        }
    }
    return seats.update(candidates);
}
void gamepads_shutdown() {
    for(auto &d:devices)release(d);
    if(di){di->lpVtbl->Release(di);di=nullptr;}
    if(di_module){FreeLibrary(di_module);di_module=nullptr;}
    if(xi_module){FreeLibrary(xi_module);xi_module=nullptr;}
    xi_get=nullptr;owner=nullptr;ready=false;scan_elapsed=1;seats.clear();
}
} // namespace usm::mp
#endif
