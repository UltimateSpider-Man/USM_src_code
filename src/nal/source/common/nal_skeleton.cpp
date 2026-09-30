#include <nal_skeleton.h>

#include <func_wrapper.h>
#include <vtbl.h>

#include <trace.h>

#include <nal_list.h>
#include <nal_system.h>
#include <tl_instance_bank.h>
#include <tl_system.h>
#include <tlresource_location.h>
#include <utility/mod.h>
#include "mod_nal_overrides.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "quaternion.h"
#include "vector3d.h"
#include "vector4d.h"
#include "matrix4x4.h"

namespace
{
bool g_xbox_v10_skeleton_fixups_enabled = false;

constexpr std::uint32_t NAL_GENERIC_SKELETON_V10 = 0x00010200u;
constexpr std::size_t NAL_GENERIC_INLINE_DATA = 0xE0u;
constexpr std::size_t NAL_COMPONENT_INFO_SIZE = 0x30u;
constexpr std::size_t NAL_GENERIC_MAX_SERIALIZED_SPAN = 64u * 1024u * 1024u;

std::uint32_t read_pcskel_u32(const std::uint8_t *bytes, std::size_t offset)
{
    std::uint32_t value = 0;
    std::memcpy(&value, bytes + offset, sizeof(value));
    return value;
}

bool advance_pcskel_offset(std::size_t &offset, std::size_t amount)
{
    if (offset > NAL_GENERIC_MAX_SERIALIZED_SPAN ||
        amount > NAL_GENERIC_MAX_SERIALIZED_SPAN - offset)
        return false;

    offset += amount;
    return true;
}

bool align_pcskel_offset4(std::size_t &offset)
{
    if (offset > NAL_GENERIC_MAX_SERIALIZED_SPAN - 3u)
        return false;

    offset = (offset + 3u) & ~std::size_t(3u);
    return true;
}

bool looks_like_pcskel_name(const std::uint8_t *name)
{
    // tlFixedString has a 28-byte, zero-terminated ASCII payload.  Component
    // names in the supplied skeletons use only letters, digits and '_'.
    for (std::size_t i = 0; i < 28u; ++i)
    {
        const unsigned char c = name[i];
        if (c == 0)
            return i >= 3u;

        const bool valid =
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '_';
        if (!valid)
            return false;
    }

    return false;
}

bool component_table_is_valid(
    const std::uint8_t *table,
    std::uint32_t component_count,
    std::uint32_t total_tracks,
    std::uint32_t pose_data_size)
{
    for (std::uint32_t i = 0; i < component_count; ++i)
    {
        const auto *info = table + std::size_t(i) * NAL_COMPONENT_INFO_SIZE;
        const std::uint32_t type_hash = read_pcskel_u32(info, 0x00u);
        const std::uint32_t first_track = read_pcskel_u32(info, 0x24u);
        const std::uint32_t track_count = read_pcskel_u32(info, 0x28u);
        const std::uint32_t pose_offset = read_pcskel_u32(info, 0x2Cu);

        if (type_hash == 0u || !looks_like_pcskel_name(info + 0x04u))
            return false;

        if (first_track > total_tracks ||
            track_count > total_tracks - first_track ||
            pose_offset > pose_data_size)
            return false;
    }

    return true;
}

bool normalize_xbox_v10_generic_component_table(void *image)
{
    if (image == nullptr)
        return false;

    auto *bytes = static_cast<std::uint8_t *>(image);

    if (read_pcskel_u32(bytes, 0x04u) != NAL_GENERIC_SKELETON_V10 ||
        std::memcmp(bytes + 0x2Cu, "generic\0", 8u) != 0)
        return false;

    const std::uint32_t string_bytes = read_pcskel_u32(bytes, 0x64u);
    const std::uint32_t hierarchy_bytes = read_pcskel_u32(bytes, 0x6Cu);
    const std::uint32_t bone_count = read_pcskel_u32(bytes, 0x74u);
    const std::uint32_t track_info_count = read_pcskel_u32(bytes, 0x7Cu);
    const std::uint32_t total_tracks = read_pcskel_u32(bytes, 0x80u);
    const std::uint32_t component_count = read_pcskel_u32(bytes, 0x88u);
    const std::uint32_t pose_data_size = read_pcskel_u32(bytes, 0x90u);

    // Reject damaged headers before performing the same offset arithmetic as
    // nalGenericSkeleton::Process.  These limits are far above retail data.
    if (string_bytes > 8u * 1024u * 1024u ||
        hierarchy_bytes > 8u * 1024u * 1024u ||
        bone_count > 4096u ||
        track_info_count > 65536u ||
        total_tracks > 65536u ||
        component_count == 0u || component_count > 128u ||
        pose_data_size > 16u * 1024u * 1024u)
        return false;

    std::size_t component_table_offset = NAL_GENERIC_INLINE_DATA;
    if (!advance_pcskel_offset(component_table_offset, string_bytes) ||
        !advance_pcskel_offset(component_table_offset, hierarchy_bytes) ||
        !align_pcskel_offset4(component_table_offset) ||
        !advance_pcskel_offset(
            component_table_offset, std::size_t(bone_count) * 0x30u) ||
        !align_pcskel_offset4(component_table_offset) ||
        !advance_pcskel_offset(
            component_table_offset, std::size_t(track_info_count) * 0x28u) ||
        !align_pcskel_offset4(component_table_offset))
        return false;

    const std::size_t component_table_bytes =
        std::size_t(component_count) * NAL_COMPONENT_INFO_SIZE;
    if (component_table_offset > NAL_GENERIC_MAX_SERIALIZED_SPAN - 4u ||
        component_table_bytes >
            NAL_GENERIC_MAX_SERIALIZED_SPAN - component_table_offset - 4u)
        return false;

    auto *component_table = bytes + component_table_offset;

    // PC's generic Process expects the first tlFixedString at field_8C.
    // Xbox v10 inserts one 32-bit word before this table.  In ped_fem and
    // ped_male, Process therefore points at 0xCD4 while the first descriptor
    // starts at 0xCD8.  Every descriptor is then read four bytes early:
    // hashes, component vtables, first-track indices and track counts all
    // become invalid, producing the exploded pedestrian skinning matrices.
    //
    // Convert only when the normal table is invalid and the +4-byte view is a
    // complete, internally consistent descriptor table.  This also makes the
    // operation idempotent if a cached skeleton image is constructed twice.
    if (component_table_is_valid(
            component_table, component_count, total_tracks, pose_data_size) ||
        !component_table_is_valid(
            component_table + 4u,
            component_count,
            total_tracks,
            pose_data_size))
        return false;

    std::memmove(
        component_table,
        component_table + 4u,
        component_table_bytes);

    const auto *name = bit_cast<const tlFixedString *>(bytes + 0x08u);
    sp_log(
        "[xbpack] normalized Xbox v10 generic PCSKEL \"%s\": "
        "moved %u pose-component descriptors by 4 bytes",
        name->to_string(),
        static_cast<unsigned>(component_count));
    return true;
}
}

void nalEnableXboxV10SkeletonFixups()
{
    g_xbox_v10_skeleton_fixups_enabled = true;
}

void nalPrepareSkeletonImage(void *image)
{
    if (g_xbox_v10_skeleton_fixups_enabled)
        normalize_xbox_v10_generic_component_table(image);
}

void *nalConstructSkeleton(void *a1)
{
    TRACE("nalConstructSkeleton");

    // mods/<name>.PCSKEL replaces the packed skeleton wholesale. The caller
    // stores whatever pointer this returns (skeleton_resource_handler does
    // a3->field_8 = nalConstructSkeleton(a3->field_8)), so swapping the
    // blob is supported by the call contract. Both the file image and the
    // runtime object carry the skeleton's tlFixedString name at +0x8.
    if (a1 != nullptr)
    {
        modScanNalOverrides();

        const auto *name = bit_cast<const tlFixedString *>(
            static_cast<const char *>(a1) + 0x8);

        bool configured = false;
        void *selected = modNalConfiguredSkeleton(a1, &configured);
        if (selected) a1 = selected;
        Mod *mod = configured ? nullptr : getMod(name->m_hash, TLRESOURCE_TYPE_SKELETON);
        if (mod && modNalConfiguredSource(mod->Path)) mod = nullptr;
        if (mod != nullptr && !mod->Data.empty())
        {
            void *copy = tlMemAlloc((uint32_t)mod->Data.size(), 16u, 0x2000000u);
            if (copy != nullptr)
            {
                std::memcpy(copy, mod->Data.data(), mod->Data.size());

                sp_log("[mod] skeleton \"%s\" overridden from \"%s\" (%u bytes)",
                       name->to_string(),
                       mod->Path.filename().string().c_str(),
                       (unsigned)mod->Data.size());

                a1 = copy;
            }
        }
    }

    // Normalize the selected packed or loose skeleton before the native
    // constructor resolves its component types and allocates pose storage.
    nalPrepareSkeletonImage(a1);

    if constexpr (0)
    {
        struct {
            std::intptr_t m_vtbl;
            int Version;
            tlFixedString field_8;
            tlHashString field_28;
            int field_48;
            int field_4C;
            int field_50;

        } *skel = static_cast<decltype(skel)>(a1);

#ifdef TARGET_XBOX
        tlFixedString str = *bit_cast<tlFixedString *>(&skel->field_28);
#else
        const tlHashString &str = skel->field_28;
#endif

        auto *instance = nalTypeInstanceBank.Search(str);
        assert(instance != nullptr && "unable to find skeleton type in type instance bank");

        auto *v1 = static_cast<nalInitListAnimType *>(instance->field_20);

        auto vtbl = v1->skel_vtbl_ptr;
        skel->m_vtbl = vtbl;
        sp_log("0x%08X", vtbl);

        bool (__fastcall *CheckVersion)(void *) = CAST(CheckVersion, get_vfunc(skel->m_vtbl, 0x10));
        if ( !CheckVersion(a1) )
        {
#ifdef TARGET_XBOX
            auto v3 = skel->Version;
            auto *v5 = skel->field_8.to_string();
            sp_log("Unsupported skeleton version %x (%s).\n", v3, v5);
            assert(0);
#endif
        }

        void (__fastcall *Process)(void *) = CAST(Process, get_vfunc(skel->m_vtbl, 0x8));
        Process(a1);

        skel->field_50 = 0;
        return a1;

    } else {
        return (void *) CDECL_CALL(0x0078DC80, a1);
    }
}

namespace inverse_kinematics {

    // let distance d = ||T - P|| and precomputed coefficients,
    //    cos0 = a0 * d + b0 / d
    //    cos1 = a1 * d + b1 / d
    //    sin_i = sqrt(1 - cos_i^2)
    void __cdecl nalIKSolve2D(
        matrix4x4* hinge,
        vector3d* root,
        vector3d* target,
        float b0a_len,
        float b1a_len,
        float b0b_len,
        float b1b_len,
        vector3d* proj_point,
        vector3d* bone_axis_dir,
        float* sin0,
        float* cos0,
        float* sin1,
        float* cos1)
    {
        vector3d tmpProj;
        ProjectPointOntoLineXform(&tmpProj, root, hinge);
        *proj_point = tmpProj;

        vector3d diff;
        diff.x = target->x - proj_point->x;
        diff.y = target->y - proj_point->y;
        diff.z = target->z - proj_point->z;

        // ||T - P||
        float bend_radius = std::sqrt(
            diff.x * diff.x +
            diff.y * diff.y +
            diff.z * diff.z);

        float inv_radius = 1.0f / bend_radius;

        // (T - P) / ||T - P||
        bone_axis_dir->x = diff.x * inv_radius;
        bone_axis_dir->y = diff.y * inv_radius;
        bone_axis_dir->z = diff.z * inv_radius;

        float cos0_raw = bend_radius * b0a_len + inv_radius * b0b_len;
        float cos1_raw = bend_radius * b1a_len + inv_radius * b1b_len;

        // clamp -1,1
        float c0 = cos0_raw;
        if (c0 > 1.0f)
            c0 = 1.0f;
        else if (c0 < -1.0f)
            c0 = -1.0f;
        *cos0 = c0;

        float c1 = cos1_raw;
        if (c1 > 1.0f)
            c1 = 1.0f;
        else if (c1 < -1.0f)
            c1 = -1.0f;
        *cos1 = c1;

        // sin = sqrt(1 - cos^2)
        *sin0 = std::sqrt(1.0f - (*cos0) * (*cos0));
        *sin1 = std::sqrt(1.0f - (*cos1) * (*cos1));
    }

    void __cdecl nalIKMap2DTo3D(
        float chain_scale,
        float sin0,
        float cos0,
        float sin1,
        float cos1,
        vector3d* origin,
        vector3d* bone_axis_dir,
        vector4d* bend_dir,
        float chain_sin0,
        float chain_cos0,
        matrix4x4* joint0,
        matrix4x4* joint1)
    {
        const vector3d bone = *bone_axis_dir;
        const vector3d bend{
            bend_dir->x,
            bend_dir->y,
            bend_dir->z
        };

        // N = normalize(bone x bend)
        vector3d normal = vector3d::cross(bone, bend).normalized();
        vector3d tangent = vector3d::cross(normal, bone);

        // pack basis
        vector4d axisX{ bone.x,   bone.y,   bone.z,   0.0f };
        vector4d axisY{ tangent.x,tangent.y,tangent.z,0.0f };
        vector4d axisZ{ normal.x, normal.y, normal.z, 0.0f };

        vector4d origin4{
            origin->x,
            origin->y,
            origin->z,
            1.0f
        };

        matrix4x4 hinge_space;
        hinge_space.compose_from_basis(&axisX, &axisY, &axisZ, &origin4);

        // j0

        const float neg_chain_sin0 = -chain_sin0;

        vector4d j0_x{ // r00
            cos0,
            sin0 * chain_cos0,
            sin0 * chain_sin0,
            0.0f
        };

        vector4d j0_y{ // r01
            -sin0,
            cos0 * chain_cos0,
            cos0 * chain_sin0,
            0.0f
        };

        vector4d j0_z{ // r02
            0.0f,
            neg_chain_sin0,
            chain_cos0,
            0.0f
        };

        vector4d j0_pos{ 0,0,0,1 }; // r03

        matrix4x4 joint0_local;
        joint0_local.compose_from_basis(&j0_x, &j0_y, &j0_z, &j0_pos);

        local_to_world(joint0, &joint0_local, &hinge_space);

        // j1

        float off_sin0 = chain_scale * sin0;

        vector4d j1_pos{ // r03
            chain_scale * cos0,
            chain_cos0 * off_sin0,
            chain_sin0 * off_sin0,
            1.0f
        };

        vector4d j1_z{ // same as j0_z
            0.0f,
            neg_chain_sin0,
            chain_cos0,
            0.0f
        };

        vector4d j1_y{ // r01
            sin1,
            cos1 * chain_cos0,
            cos1 * chain_sin0,
            0.0f
        };

        vector4d j1_x{ // r00
            cos1,
            -(sin1 * chain_cos0),
            -(sin1 * chain_sin0),
            0.0f
        };

        matrix4x4 joint1_local;
        joint1_local.compose_from_basis(&j1_x, &j1_y, &j1_z, &j1_pos);

        local_to_world(joint1, &joint1_local, &hinge_space);
    }

    inline void flip_chain_basis(matrix4x4* m) {
        vector4d r0 = m->arr[0],
            r1 = m->arr[1],
            r2 = m->arr[2];

        auto negate = [](vector4d v) {
            v.x = -v.x;
            v.y = -v.y;
            v.z = -v.z;
            v.w = -v.w;
            return v;
        };

        r0 = negate(r0);
        r1 = negate(r1);
        r2 = negate(r2);

        std::swap(r1, r2);

        m->arr[0] = r0;
        m->arr[1] = r1;
        m->arr[2] = r2;
    }

    vector4d* __cdecl compute_bend_plane_normal(
        vector4d* out,
        float*     /*unused*/,
        matrix4x4* m,
        float      axis_x,
        float      axis_y,
        float      axis_z)
    {
        vector4d *vector4d_1 = out;
        // axis x effector Y
        out->x = axis_y * m->arr[1].z - axis_z      * m->arr[1].y;
        out->y = axis_z * m->arr[1].x - m->arr[1].z * axis_x;
        out->z = axis_x * m->arr[1].y - axis_y      * m->arr[1].x;
        return vector4d_1;
    }

    vector3d* __cdecl compute_arm_elbow_bend_direction(
        vector3d* out,
        matrix4x4* m,
        matrix4x4* /*ent*/,
        float      dirX,
        float      dirY,
        float      dirZ)
    {
        const vector3d row0{ m->arr[0].x, m->arr[0].y, m->arr[0].z };
        const vector3d row1{ m->arr[1].x, m->arr[1].y, m->arr[1].z };
        const vector3d row2{ m->arr[2].x, m->arr[2].y, m->arr[2].z };
        const vector3d dir{ dirX, dirY, dirZ };

        const vector3d cross = vector3d::cross(dir, row1);
        const float    sign = dirX * row1.x + dirY * row1.y + dirZ * row1.z;

        vector3d result;

        if (sign < 0.0f)
        {
            const float a = sign + 1.0f;
            const float b = -sign;
            const vector3d diag{ -row0.x - row2.x,
                                 -row0.y - row2.y,
                                 -row0.z - row2.z };
            result.x = cross.x * a + diag.x * b;
            result.y = cross.y * a + diag.y * b;
            result.z = cross.z * a + diag.z * b;
        }
        else
        {
            const float a = 1.0f - sign;
            const float b = sign;
            const vector3d diag{ row2.x - row0.x,
                                 row2.y - row0.y,
                                 row2.z - row0.z };
            result.x = cross.x * a + diag.x * b;
            result.y = cross.y * a + diag.y * b;
            result.z = cross.z * a + diag.z * b;
        }

        out->x = result.x;
        out->y = result.y;
        out->z = result.z;
        return out;
    }

    vector3d* __cdecl compute_arm_elbow_bend_direction_mirrored(
        vector3d* out,
        matrix4x4* m,
        matrix4x4* /*ent*/,
        float      dirX,
        float      dirY,
        float      dirZ)
    {
        const vector3d row0{ m->arr[0].x, m->arr[0].y, m->arr[0].z };
        const vector3d row1{ m->arr[1].x, m->arr[1].y, m->arr[1].z };
        const vector3d row2{ m->arr[2].x, m->arr[2].y, m->arr[2].z };
        const vector3d dir{ dirX, dirY, dirZ };

        // mirrored "up" axis
        const vector3d up_m{ -row1.x, -row1.y, -row1.z };

        const vector3d cross = vector3d::cross(dir, up_m);
        const float    sign = dirX * up_m.x + dirY * up_m.y + dirZ * up_m.z;

        vector3d result;

        if (sign < 0.0f)
        {
            const float a = sign + 1.0f;
            const float b = -sign;
            const vector3d diag{ -row0.x - row2.x,
                                 -row0.y - row2.y,
                                 -row0.z - row2.z };
            result.x = cross.x * a + diag.x * b;
            result.y = cross.y * a + diag.y * b;
            result.z = cross.z * a + diag.z * b;
        }
        else
        {
            const float a = 1.0f - sign;
            const float b = sign;
            const vector3d diag{ row2.x - row0.x,
                                 row2.y - row0.y,
                                 row2.z - row0.z };
            result.x = cross.x * a + diag.x * b;
            result.y = cross.y * a + diag.y * b;
            result.z = cross.z * a + diag.z * b;
        }

        out->x = result.x;
        out->y = result.y;
        out->z = result.z;
        return out;
    }

    void __cdecl solve_two_bone(
        matrix4x4* j0,
        matrix4x4* j1,
        matrix4x4* line_xform,
        vector3d* root,
        matrix4x4* effector,
        ik_bone_chain_t* chain,
        get_bend_dir_t get_bend_dir)
    {
        vector3d target{ effector->arr[3].x,effector->arr[3].y, effector->arr[3].z };
        float    sin0 = 0.0f;
        float    sin1 = 0.0f;
        float    cos0 = 1.0f;
        float    cos1 = 1.0f;
        vector3d proj_point{};
        vector3d bone_axis_dir{};

        inverse_kinematics::nalIKSolve2D(
            line_xform,
            root,
            &target,
            chain->b0a_len,
            chain->b0b_len,
            chain->b1a_len,
            chain->b1b_len,
            &proj_point,
            &bone_axis_dir,
            &sin0,
            &cos0,
            &sin1,
            &cos1);

        vector3d tmp_bend{};
        vector3d* bend_dir = get_bend_dir(&tmp_bend, line_xform, effector, bone_axis_dir.x, bone_axis_dir.y, bone_axis_dir.z);
        vector4d bone_bend_dir{ bend_dir->x, bend_dir->y, bend_dir->z, 0.0f };

        inverse_kinematics::nalIKMap2DTo3D(
            chain->chain_scale,
            sin0,
            cos0,
            sin1,
            cos1,
            &proj_point,
            &bone_axis_dir,
            &bone_bend_dir,
            0.0f,   // chain_sin0
            1.0f,   // chain_cos0
            j0,
            j1);

        flip_chain_basis(j0);
        flip_chain_basis(j1);
    }

    void __cdecl DecomposeIKSpin(
        matrix4x4* joint0,
        matrix4x4* joint1,
        matrix4x4* hinge,
        vector3d* root,
        matrix4x4* effector,
        ik_bone_chain_t* chain,
        get_bend_dir_t get_bend_dir,
        float twistAngle)
    {
        vector3d target{ effector->arr[3].x, effector->arr[3].y, effector->arr[3].z };

        float    sin0 = 0.0f;
        float    sin1 = 0.0f;
        float    cos0 = 1.0f;
        float    cos1 = 1.0f;
        vector3d proj_point{};
        vector3d axis_dir{}; // hinge space

        inverse_kinematics::nalIKSolve2D(
            hinge,
            root,
            &target,
            chain->b0a_len,
            chain->b0b_len,
            chain->b1a_len,
            chain->b1b_len,
            &proj_point,
            &axis_dir,
            &sin0,
            &cos0,
            &sin1,
            &cos1);

        vector3d tmp{};
        vector3d* bend_vec = get_bend_dir(&tmp, hinge, effector, axis_dir.x, axis_dir.y, axis_dir.z);
        vector4d bend_dir{ bend_vec->x, bend_vec->y, bend_vec->z, 0.0f };

        // apply twist
        float chain_cos0 = std::cos(twistAngle);
        float chain_sin0 = std::sin(twistAngle);

        inverse_kinematics::nalIKMap2DTo3D(
            chain->chain_scale,
            sin0,
            cos0,
            sin1,
            cos1,
            &proj_point,
            &axis_dir,
            &bend_dir,
            chain_sin0,
            chain_cos0,
            joint0,
            joint1);

        flip_chain_basis(joint0);
        flip_chain_basis(joint1);
    }

    quaternion* __cdecl quat_blend(
        quaternion* quat,
        quaternion* quatA,
        float* weightA,
        quaternion* quatB,
        vector4d* weights)
    {
        const float a = *weightA;
        const float b = weights->y;

        const float x = a * quatA->arr[0] + b * quatB->arr[0];
        const float y = a * quatA->arr[1] + b * quatB->arr[1];
        const float z = a * quatA->arr[2] + b * quatB->arr[2];
        const float w = a * quatA->arr[3] + b * quatB->arr[3];

        quat->arr[0] = x;
        quat->arr[1] = y;
        quat->arr[2] = z;
        quat->arr[3] = w;

        return quat;
    }

    namespace
    {
    struct legs_ik_skel_v10
    {
        vector3d offsets[8];
        float left_chain[6];
        float right_chain[6];
        uint32_t bone_indices[8];
        uint32_t parent_index;
        uint32_t padding[3];
    };

struct legs_ik_pose_v10
    {
        quaternion foot_quats[2];
        quaternion foot_targets[2];
        vector3d foot_positions[2];
        float knee_spin[2];
    };

    static_assert(sizeof(legs_ik_skel_v10) == 0xC0);
    static_assert(sizeof(legs_ik_pose_v10) == 0x60);

    void make_legs_ik_matrix(
        matrix4x4 &matrix, const quaternion &rotation, const vector3d &position)
    {
        const quaternion pc_rotation {
            rotation.arr[3], rotation.arr[0], rotation.arr[1], rotation.arr[2]};
        pc_rotation.to_matrix(matrix);
        matrix.w = vector4d {position, 1.0f};
    }
    }

    int __stdcall LegsIK_BuildBoneMatrices_v10(
        matrix4x4 *matrices, int, void *skel_data, void *pose_data)
    {
        auto *skel = static_cast<legs_ik_skel_v10 *>(skel_data);
        auto *pose = static_cast<legs_ik_pose_v10 *>(pose_data);

        for (int i = 0; i < 2; ++i) {
            make_legs_ik_matrix(
                matrices[skel->bone_indices[i + 2]],
                pose->foot_targets[i],
                pose->foot_positions[i]);
        }

        const matrix4x4 &parent = matrices[skel->parent_index];
        matrix4x4 parent_rotation = parent;
        parent_rotation.w = vector4d {0.0f, 0.0f, 0.0f, 1.0f};

        auto *chain = reinterpret_cast<ik_bone_chain_t *>(skel->left_chain);
        auto heuristic = reinterpret_cast<get_bend_dir_t>(&compute_bend_plane_normal);

        DecomposeIKSpin(
            &matrices[skel->bone_indices[4]],
            &matrices[skel->bone_indices[5]],
            &parent_rotation,
            &skel->offsets[4],
            &matrices[skel->bone_indices[2]],
            chain,
            heuristic,
            pose->knee_spin[0]);

        DecomposeIKSpin(
            &matrices[skel->bone_indices[6]],
            &matrices[skel->bone_indices[7]],
            &parent_rotation,
            &skel->offsets[6],
            &matrices[skel->bone_indices[3]],
            chain,
            heuristic,
            pose->knee_spin[1]);

        for (int i = 2; i < 8; ++i) {
            auto &matrix = matrices[skel->bone_indices[i]];
            matrix.w.x += parent.w.x;
            matrix.w.y += parent.w.y;
            matrix.w.z += parent.w.z;
            matrix.w.w = 1.0f;
        }

        for (int i = 0; i < 2; ++i) {
            auto &matrix = matrices[skel->bone_indices[i]];
            make_legs_ik_matrix(matrix, pose->foot_quats[i], skel->offsets[i]);
            local_to_world(
                &matrix,
                &matrix,
                &matrices[skel->bone_indices[i + 2]]);
        }

        return 0;
    }
    int CalcIKTrackDataSize(int mask)
    {
        int num_tracks = 0;

        if (mask & 1) num_tracks += 3;
        if (mask & 2) num_tracks += 3;
        if (mask & 4) num_tracks += 7;
        if (mask & 8) num_tracks += 7;

        return num_tracks;
    }

}
  
