#include "femanager.h"

#include "common.h"
#include "fe_controller_disconnect.h"
#include "femenusystem.h"
#include "frontendmenusystem.h"
#include "func_wrapper.h"
#include "igofrontend.h"
#include "igozoomoutmap.h"
#include "log.h"
#include "memory.h"
#include "ngl.h"
#include "os_developer_options.h"
#include "pausemenusystem.h"
#include "resource_manager.h"
#include "trace.h"
#include "utility.h"
#include "variable.h"
#include "vector3d.h"
#include "vector4d.h"

#include "resource_directory.h"

#include <cassert>

VALIDATE_OFFSET(FEManager, IGO, 0x18u);
VALIDATE_OFFSET(FEManager, field_2A, 0x2A);

FEManager & g_femanager = var<FEManager>(0x00937B00);

const char *FEManager::font_name_array[5] = {"nglSysFont",
                                             "i_upupandaway",
                                             "badaboom",
                                             "i_button_icons",
                                             "damnnoisykids"};

void FEManager::InitIGO()
{
    TRACE("FEManager::InitGO");

    if constexpr (0)
    {
        auto *mem = mem_alloc(sizeof(PauseMenuSystem));
        this->m_pause_menu_system = new (mem) PauseMenuSystem{static_cast<font_index>(1)};

        this->IGO = new IGOFrontEnd{};
        this->IGO->Init();
    } else {
        THISCALL(0x00648FF0, this);
    }
}

void FEManager::LoadFont(font_index a2)
{
    tlFixedString a1 {font_name_array[a2]};
    nglLoadTexture(a1);
    a1 = {font_name_array[a2]};
    this->field_4[a2] = nglLoadFont(a1);
}

void FEManager::LoadFonts() {
    TRACE("FEManager::LoadFonts");

    if constexpr (0)
    {
        if (!this->field_2A)
        {
            for (auto i = 0u; i < 5u; ++i) {
                this->LoadFont(static_cast<font_index>(i));
            }

            this->field_2A = true;
        }
    } else {
        THISCALL(0x00618CA0, this);
    }
}

void FEManager::Draw() {
    if constexpr (1) {
        fe_controller_disconnect::draw();
        if (os_developer_options::instance->get_flag(static_cast<os_developer_options::flags_t>(144)))
        {
            nglListBeginScene(static_cast<nglSceneParamType>(1));
            nglSetClearFlags(0);
            nglSetOrthoMatrix(0.0, 1000.0);

            vector3d v3;
            v3[0] = 0.003125;
            v3[1] = -0.0041666669;
            v3[2] = 1.0;

            matrix4x4 v5;
            v5.make_scale(v3);

            v5.arr[3] = vector4d{-1.0, 1.0, 0.0, 1.0};

            nglSetWorldToViewMatrix({v5});
            auto *v2 = this->m_pause_menu_system;

            if (v2->m_index < 0) {
                this->IGO->Draw();
            } else {
                auto *vtbl = bit_cast<fastcall_call(*)[9]>(v2->m_vtbl);

                void (__fastcall *func)(void *) = CAST(func, (*vtbl)[8]);
                func(v2);
                //v6->Draw(this->field_1C);
            }

            nglListEndScene();
        }
    } else {
        THISCALL(0x00640E30, this);
    }
}

void FEManager::Update(Float a2)
{
    if constexpr (1)
	{
        if (fe_controller_disconnect::update()) {
            auto *v3 = this->m_pause_menu_system;
            if (v3->m_index < 0) {
                this->IGO->Update(a2);
            } else {
                auto *vtbl = bit_cast<fastcall_call(*)[6]>(v3->m_vtbl);

                void (__fastcall *Update)(void *, void *, Float) = CAST(Update, (*vtbl)[5]);

                //sp_log("FEManager::Update(): 0x%08X", func);

                if (bit_cast<std::intptr_t>(Update) == 0x0062F0C0) {
                    PauseMenuSystem *pause_menu_system = CAST(pause_menu_system, v3);

                    pause_menu_system->Update(a2);

                } else {
                    Update(this->m_pause_menu_system, nullptr, a2);
                }

                //this->field_1C->Update(this->field_1C, a2);
            }
        }
    } else {
        THISCALL(0x00642B30, this, a2);
    }
}

void FEManager::ReleaseFrontEnd() {
    THISCALL(0x0060B920, this);
}


// Plays a UI/interface sound effect
// out: Contains the sound hash code and receives instance ID
void FEManager__PlayInterfaceSound(sound_instance_id2* out) {
    if (out == nullptr) {
        return;
    }
    
    // In the actual game, this would:
    // 1. Look up sound asset by hash code in out->field_0
    // 2. Create a sound instance
    // 3. Play the sound with specified volume/pitch (from stack params)
    // 4. Return instance ID in out structure for potential stop/modify
    
    // The function signature from assembly shows additional params on stack:
    // - float volume (typically 1.0f = 0x3F800000)
    // - float pitch (typically 1.0f = 0x3F800000)
    
    // For stub: just acknowledge the sound would play
    // Real implementation would interface with audio engine
}

// Wrapper for class method
void FEManager::PlayInterfaceSound(sound_instance_id2* out) {
    FEManager__PlayInterfaceSound(out);
}

void FEManager::ReleaseFont(font_index idx) {
    auto **fonts = this->field_4;
    auto &font = fonts[idx];

    if (font != nullptr) {
        nglReleaseFont(font);
    }
}

nglFont *FEManager::GetFont(font_index idx) {
    if (idx != 5 && idx != 6) {
        return this->field_4[idx];
    }

    assert(0);

    return nullptr;
}

PanelQuad *FEManager::GetDefaultPQ() {
    return (PanelQuad *) THISCALL(0x00638180, this);
}

void FEManager::LoadFrontEnd() {
    if constexpr (1) {
        auto v2 = resource_manager::get_best_context(RESOURCE_PARTITION_MISSION);
        auto *__old_context = resource_manager::push_resource_context(v2);

        auto *mem = mem_alloc(sizeof(FrontEndMenuSystem));
        this->m_fe_menu_system = new (mem) FrontEndMenuSystem{};
        resource_manager::pop_resource_context();

        assert(resource_manager::get_resource_context() == __old_context);
    } else {
        THISCALL(0x00648AB0, this);
    }
}

#ifdef OPENUSM_XBPACK_MODE
void __fastcall xbpack_load_frontend(FEManager *self, void *)
{
    if (self->m_fe_menu_system != nullptr) {
        return;
    }

    static Var<bool> first_time_through{0x00937B78};
    first_time_through() = true;
    THISCALL(0x00648AB0, self);
}
#endif

void FEManager::RenderLoadMeter(bool a2) {
    if (this->m_fe_menu_system != nullptr) {
        this->m_fe_menu_system->RenderLoadMeter(a2);
    }
}

void FEManager::ReleaseIGO()
{
    THISCALL(0x00642E40, this);
}

void FEManager::ReleaseFonts()
{
    for (int i = 0; i < 5; ++i) {
        this->ReleaseFont(static_cast<font_index>(i));
    }
}

void RenderLoadMeter()
{
    if constexpr (1)
    {
        auto *system = g_femanager.m_fe_menu_system;

        if (system != nullptr) {
            system->RenderLoadMeter(false);
        }
    } else {
        CDECL_CALL(0x006299B0);
    }
}

void FEManager_patch() {
    {
        FUNC_ADDRESS(address, &FEManager::LoadFonts);
        REDIRECT(0x00552E61, address);
    }

    {
        FUNC_ADDRESS(address, &FEManager::InitIGO);
        REDIRECT(0x00552E6B, address);
    }
}
