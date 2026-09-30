#include <gtest/gtest.h>

#include <resource_manager.h>
#include <ngl.h>
#include <tl_system.h>
#include <variables.h>

TEST(ResourceManager, CreateInstance)
{
    g_platform = NL_PLATFORM_PC;

    D3DPRESENT_PARAMETERS d3dpresent_params;
    d3dpresent_params.BackBufferWidth = nWidth;
    d3dpresent_params.BackBufferHeight = nHeight;
    d3dpresent_params.Windowed = g_Windowed;
    d3dpresent_params.BackBufferCount = 1;
    d3dpresent_params.BackBufferFormat = D3DFMT_A8R8G8B8;
    d3dpresent_params.MultiSampleType = D3DMULTISAMPLE_NONE;
    d3dpresent_params.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpresent_params.hDeviceWindow = g_hWnd;
    d3dpresent_params.EnableAutoDepthStencil = true;
    d3dpresent_params.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dpresent_params.Flags = 0;
    d3dpresent_params.PresentationInterval = 1;
    d3dpresent_params.FullScreen_RefreshRateInHz = (g_Windowed ? 0 : 60);

    auto *pD3D = Direct3DCreate9(0x80000020);
    IDirect3D9_CreateDevice(pD3D,
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_REF,
            g_hWnd,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING,
            &d3dpresent_params,
            &g_Direct3DDevice);

    EXPECT_TRUE(g_Direct3DDevice != nullptr);

    nglTextureInit();

    set_tl_system_directories();

    resource_manager::create_inst();
    EXPECT_TRUE(resource_manager::partitions != nullptr);
}
