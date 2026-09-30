#include "usm_anim_player.h"

#include "common.h"
#include "func_wrapper.h"
#include "nal_anim.h"
#include "nal_system.h"
#include "utility.h"

using type = usm_anim_player<nalAnimClass<nalAnyPose>, 3>;

VALIDATE_SIZE(type, 0x2C);

template<>
void usm_anim_player<nalAnimClass<nalAnyPose>, 3>::PlayModifier(
        nalAnimClass<nalAnyPose> *a2,
        usm_anim_player_modifier_type a3,
        Float a4,
        nalPlayMethod *a5,
        Float a6,
        int a7,
        Float a8,
        void *a9,
        bool a10,
        void *a11)
{
    THISCALL(0x004B0530, this, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
}

template<>
void usm_anim_player<nalAnimClass<nalAnyPose>, 3>::sub_4B06A0(Float a2)
{
    THISCALL(0x004B06A0, this, a2);
}

template<>
void usm_anim_player<nalAnimClass<nalAnyPose>, 3>::sub_4B0860(nalAnyPose &a2)
{
    THISCALL(0x004B0860, this, &a2);
}

template<>
usm_anim_player<nalAnimClass<nalAnyPose>, 3>::nalAnimState *usm_anim_player<nalAnimClass<nalAnyPose>, 3>::Advance(
        Float a2)
{
    return (nalAnimState *) THISCALL(0x004AD7F0, this, a2);
}

bool __fastcall sub_4B0020(void *self, void *, int a2, Float a3)
{
    struct {
        char field_0[0x10];
        struct {
            int m_vtbl;
        } *field_10;
    } *v1 = static_cast<decltype(v1)>(self);
    sp_log("0x%08X", v1->field_10->m_vtbl);

    return THISCALL(0x004B0020, self, a2, a3);
}

void __fastcall sub_4AD850(
        void *self,
        void *,
        nalAnimClass<nalAnyPose> *a2,
        int a3,
        Float a4,
        usm_anim_player<nalAnimClass<nalAnyPose>, 3>::nalPlayMethod *a5,
        Float a6,
        void *a7,
        int a8,
        int a9,
        void *a10,
        bool a11,
        int a12)
{
    sp_log("0x%08X", a5->m_vtbl);
    sp_log("0x%08X", a2->m_vtbl);

    THISCALL(0x004AD850, self, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
}

void usm_anim_player_patch()
{
    // Keep the original call to 0x004B0020.  The diagnostic wrapper assumes
    // state->play_method is non-null, although the game supports a null state.

    if constexpr (0)
    {
        REDIRECT(0x004AFF2E, sub_4AD850);
        REDIRECT(0x004B063A, sub_4AD850);
    }
}
