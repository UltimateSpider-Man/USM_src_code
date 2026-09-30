#pragma once

#include "float.hpp"
#include "game_axis.h"
#include "vector2d.h"
#include "vector3d.h"

#include <cstdint>

struct camera_frame;
struct camera_target_info;
struct spiderman_camera;

struct camera_mode {
    struct {
        int finalize;
        int clone;
        void (__fastcall *activate)(camera_mode *, void *edx, camera_target_info *);
        void (__fastcall *deactivate)(camera_mode *);
        void (__fastcall *frame_advance)(camera_mode *, void *,
                Float , camera_frame *, const camera_target_info *);
        void (__fastcall *request_recenter)(camera_mode *, void *, Float, const camera_target_info *);
        int enable_lookaround;
        void (__fastcall *set_fixedstatic)(
                camera_mode *, void *,
                const vector3d *,
                const vector3d *);
        void (__fastcall *clear_fixedstatic)(camera_mode *);
    } * m_vtbl;
    spiderman_camera *slave;
    camera_mode *field_8;

    camera_mode(spiderman_camera *a2, camera_mode *a3);

    //virtual
    void activate(camera_target_info &a2);
    
    //virtual
    void deactivate();

    //virtual
    void frame_advance(Float a2, camera_frame &a3, const camera_target_info &a4);

    /* virtual */ void request_recenter(Float a2, const camera_target_info &a3);

    void set_fixedstatic(
        const vector3d &a2,
        const vector3d &a3);

    void clear_fixedstatic();
};

struct camera_mode_shake : camera_mode
{
    uint8_t field_C[0x18];
    vector3d frame_fwd;
    vector3d frame_eye;

    void _frame_advance(
            Float a2,
            camera_frame &a3,
            const camera_target_info &a4);
};

struct camera_mode_lookaround : camera_mode {
    bool field_C;
    game_axis field_10;
    game_axis field_40;
    vector2d field_70;

    camera_mode_lookaround(
                    spiderman_camera *a2,
                    camera_mode *a3);

    void _frame_advance(
        Float a2,
        camera_frame &a3,
        const camera_target_info &a4);
};

namespace camera_mode_chase {
    extern void pull_by_target(camera_frame &frame, const camera_target_info &target, Float a3);
}

struct camera_mode_passive : camera_mode {
    float field_C;
    float field_10;
    float field_14;
    float field_18;
    vector3d field_1C;

    //0x004B7390
    camera_mode_passive(
            spiderman_camera *a2,
            camera_mode *a3);

    void _activate(camera_target_info &a2);

    //0x004B7400
    //virtual
    void _frame_advance(
        Float a2,
        camera_frame &frame,
        camera_target_info &target);
};

struct camera_mode_fixedstatic : camera_mode {

    vector3d field_C;
    vector3d field_18;
    bool enabled;

    camera_mode_fixedstatic(
        spiderman_camera *a2,
        camera_mode *a3);

    void _frame_advance(
        Float a2,
        camera_frame &a3,
        const camera_target_info &a4);

    void _set_fixedstatic(
        const vector3d &a2,
        const vector3d &a3);

    void _clear_fixedstatic();
};

struct camera_mode_combat : camera_mode {

    void _frame_advance(
        Float a2,
        camera_frame &a3,
        const camera_target_info &a4);
};

extern void camera_mode_patch();
