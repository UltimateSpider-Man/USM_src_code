#include "mash_info_struct.h"

#include "ai_interaction_data.h"
#include "base_ai_res_state_graph.h"
#include "als_animation_logic_system_shared.h"
#include "als_res_data.h"
#include "core_ai_resource.h"
#include "cut_scene.h"
#if defined(OPENUSM_XBPACK_MODE) && !defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)
#include "entity_mash.h"
#endif
#include "gab_manager.h"
#include "path_graph.h"
#include "token_def_list.h"
#include "skeleton_interface.h"
#include "sound_alias_database.h"
#include "panelfile.h"
#include "trace.h"

#include "func_wrapper.h"

template<>
void mash_info_struct::construct_class<mAvlTree<string_hash_entry>>(mAvlTree<string_hash_entry> *&a1)
{
    auto *v1 = a1;
    if ( a1 != nullptr )
    {
        THISCALL(0x00420EF0, v1, nullptr);
        a1 = v1;
    }
    else
    {
        a1 = nullptr;
    }
}

template<>
void mash_info_struct::construct_class(PanelFile *&a1)
{
    TRACE("mash_info_struct::construct_class<PanelFile>");
    if ( a1 != nullptr )
    {
        void (__fastcall *func)(void *, int edx, void *) = CAST(func, 0x00642FA0);
        func(a1, 0, nullptr);
    }
}

template<>
void mash_info_struct::construct_class(sound_alias_database *&a1)
{
    if ( a1 != nullptr )
    {
        void (__fastcall *func)(void *, int edx, void *) = CAST(func, 0x005D9040);
        func(a1, 0, nullptr);
    }
}

template<>
void mash_info_struct::construct_class(token_def_list *&a1)
{
    TRACE("mash_info_struct::construct_class<token_def_list>");
    if ( a1 != nullptr )
    {
        if constexpr (0) {
            void (__fastcall *func)(void *, int edx, void *) = CAST(func, 0x005DEDA0);
            func(a1, 0, nullptr);
        } else {
            new (a1) token_def_list {nullptr};
        }
    }
}

template<>
void mash_info_struct::construct_class(path_graph *&a1)
{
    if ( a1 != nullptr )
    {
        void (__fastcall *func)(void *, int edx, void *) = CAST(func, 0x005DE080);
        func(a1, 0, nullptr);
    }
}

template<>
void mash_info_struct::construct_class(ai::state_graph *&a1)
{
    if ( a1 != nullptr )
    {
        void (__fastcall *func)(void *, int edx, void *) = CAST(func, 0x006DA190);
        func(a1, 0, nullptr);
    }
}

template<>
void mash_info_struct::construct_class(gab_database *&a1)
{
    if ( a1 != nullptr )
    {
        void (__fastcall *func)(void *, int edx, void *) = CAST(func, 0x005E0E80);
        func(a1, 0, nullptr);
    }
}

template<>
void mash_info_struct::construct_class(als::animation_logic_system_shared *&a1)
{
    TRACE("mash_info_struct::construct_class<als::animation_logic_system_shared>");

    if ( a1 != nullptr )
    {
        if constexpr (0) {
            a1 = new (a1) als::animation_logic_system_shared {nullptr};
        } else {
            void (__fastcall *func)(void *, int edx, void *) = CAST(func, 0x004AC000);
            func(a1, 0, nullptr);
        }
    }
}

template<>
void mash_info_struct::construct_class(ai::core_ai_resource *&a1)
{
    if ( a1 != nullptr )
    {
        void (__fastcall *func)(void *, int edx, void *) = CAST(func, 0x006D9A10);
        func(a1, 0, nullptr);
    }
}

template<>
void mash_info_struct::construct_class(cut_scene *&a1)
{
    if ( a1 != nullptr )
    {
        void (__fastcall *func)(void *, int edx, void *) = CAST(func, 0x00742890);
        func(a1, 0, nullptr);
    }
}

template<>
void mash_info_struct::construct_class(ai_interaction_data *&a1)
{
    if ( a1 != nullptr )
    {
        void (__fastcall *func)(void *, int edx, void *) = CAST(func, 0x006B65B0);
        func(a1, 0, nullptr);
    }
}

template<>
void mash_info_struct::construct_class(als_res_data *&a1)
{
    TRACE("mash_info_struct::construct_class<als_res_data>");

    if ( a1 != nullptr )
    {
        void (__fastcall *func)(void *, int edx, void *) = CAST(func, 0x004ABF80);
        func(a1, 0, nullptr);
    }
}

template<>
void mash_info_struct::construct_class(skeleton_interface *&a1)
{
    if ( a1 != nullptr )
    {
#if defined(OPENUSM_XBPACK_MODE) && !defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)
        // Xbox V14 stores the type hash in the serialized vtable slot.  The
        // stock PC conglomerate teardown later invokes a virtual teardown slot
        // through this value.  Relink it, but mark it non-owning: the object is
        // embedded in the nested mash image and must not be passed to the PC
        // heap's deleting destructor.
        a1->m_vtbl = ifc_v_table_lookup()[6];
#endif

        auto func = [](skeleton_interface *self, int a2, int a3) {
            self->field_4 = CAST(self->field_4, a3);
            self->field_8 = ( a2 == 1 );
        };

#if defined(OPENUSM_XBPACK_MODE) && !defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)
        func(a1, 0, 0);
#else
        func(a1, 1, 0);
#endif
    }
}
