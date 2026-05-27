#include "variables.h"

#include "../input.h"
#include "../inputsettings.h"

#include <dsound.h>

PolytubeCustomMaterial *& webline_texture = var<PolytubeCustomMaterial *>(0x00958058);


Var<float> dword_151691C = {0x151691C};

Var<float> flt_87EBD4 = {0x87EBD4};

Var<float> flt_86F860 = {0x86F860};

Var<int> dword_975308 = {0x00975308};

Var<bool> byte_959561 = {0x00959561};

Var<int> dword_975314 = {0x00975314};

Var<int> dword_97530C = {0x0097530C};

Var<bool> byte_971F9C = {0x00971F9C};

Var<bool> s_freeze_game_time = {0x009680AC};

Var<bool> g_generating_vtables = {0x0095A6F1};

Var<float> g_tan_half_fov_ratio{0x00921D7C};

Var<bool> byte_965C20{0x00965C20};

Var<bool> g_distance_clipping_enabled{0x0095C718};

Var<int> g_distance_clipping{0x0095C2F8};

Var<int> g_disable_occlusion_culling{0x0095C7AC};

Var<int> globalTextLanguage{0x0096B430};

Var<HWND> g_appHwnd{0x009874C4};

Var<bool> g_is_the_packer{0x009682E4};

Var<bool> g_master_clock_is_up{0x00965EB0};

Var<int> g_TOD{0x0091E000};

Var<char *[14]> dword_965C24 { 0x00965C24 };

char (&g_scene_name)[1024] = var<char[1024]>(0x0095C308);



Var<bool> bExit{0x0095C178};

Var<bool> byte_965BF9 = {0x00965BF9};


IDirectSound8 *& g_directSound = var<IDirectSound8 *>(0x00987518);


Var<HCURSOR> hCursor = {0x0096596C};

Var<HANDLE> hEvent{0x00965C5C};

Var<HANDLE> hObject{0x00965C60};

Var<bool> byte_965BF5 = {0x00965BF5};
Var<bool> byte_965BF6 = {0x00965BF6};

Var<int> dword_922908 = {0x00922908};


Var<int> dword_96BA08 = {0x0096BA08};

Var<bool> byte_922994{0x00922994};

Var<IDirect3DDevice9 *> g_Direct3DDevice = (0x00971F94);

Var<char> byte_965C21 = {0x00965C21};

Var<char> byte_965BF8 = {0x00965BF8};

Var<int> g_cx = {0x00965978};
Var<int> g_cy = {0x00965968};

Var<char *> dword_95C730 = {0x0095C730};
Var<char *> dword_95C72C = {0x0095C72C};

Var<int> dword_91E1D8{0x0091E1D8};

Var<bool> g_indoors{0x0095C87B};

Var<bool> g_player_shadows_enabled = {0x00922C5C};
Var<char> g_enable_stencil_shadows = {0x00921C98};

Var<int> g_cur_shadow_target = {0x00965F44};

Var<bool> byte_975468{0x00975468};

Var<bool> dword_959318{0x00959318};

Var<string_hash> bip01_l_calf = {0x0095BA1C};
Var<string_hash> bip01_r_calf = {0x0095AB1C};
Var<string_hash> bip01_pelvis = {0x0095AAFC};
Var<string_hash> bip01_head = {0x0095B9DC};
Var<string_hash> bip01_spine = {0x0095BA18};

Var<char[260]> byte_9659B8{0x009659B8};

Var<int> nWidth = {0x0093AE84};
Var<int> nHeight = {0x0093AE88};

Var<HWND> g_hWnd = {0x00971F98};

Var<float> flt_965BDC{0x00965BDC};

Var<char> byte_95C718 = {0x0095C718};
Var<int> dword_95C2F8 = {0x0095C2F8};

Var<bool> ChromeEffect = {0x0091E1D4};

Var<int> g_Windowed = {0x00946530};

Var<float> flt_88E518{0x0088E518};

Var<float> flt_88E51C{0x0088E51C};

Var<IDirectSound8 *> pUnkOuter{0x00987518};

Var<bool> cam_target_locked{0x0095C754};

Var<bool> EnableShader{0x00972AB0};
