#include "polytube.h"

#include "common.h"
#include "game.h"
#include "func_wrapper.h"
#include "memory.h"
#include "oldmath_po.h"
#include "us_pcuv_shader.h"
#include "slab_allocator.h"
#include "string_hash.h"
#include "trace.h"
#include "vtbl.h"

#include <cassert>

#if MOD_MESH_SUPPORT && !defined(TARGET_XBOX)
#include "base_ai_core.h"
#include "conglom.h"
#include "exe_allocator.h"
#include "mod_polytube_attachment.h"
#include "tentacle_interface.h"
#include "wds.h"

extern bool modMeshAttachmentDelta(conglomerate *, entity_base *, vector3d &);

namespace {
struct ModNativeTentacleRecord {
    std::uint32_t prefix[14];
    entity_base *resolvedEntity;
    mashable_vector<std::uint64_t> controlHashes;
    mashable_vector<entity_base *> controlEntities;
};
static_assert(sizeof(ModNativeTentacleRecord) == 0x4c, "PC tentacle definition size");
static_assert(offsetof(ModNativeTentacleRecord, controlEntities) == 0x44, "PC tentacle point entities");

bool modMatchTentacleOwner(entity *candidate, polytube *tube,
                          conglomerate *&owner, entity_base *&base)
{
    if (!candidate || !candidate->is_a_conglomerate()) return false;
    auto *conglom = static_cast<conglomerate *>(candidate);
    auto *ifc = conglom->field_124;
    if (!ifc || ifc->field_4 != candidate) return false;
    modmesh::attachment::NativeTentacleInterface view;
    std::memcpy(&view, ifc, sizeof(view));
    auto *tubes = reinterpret_cast<const std::uint32_t *>(view.tubes);
    const int index = modmesh::attachment::matchingTubeIndex(view, tubes,
                          static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(tube)));
    if (index < 0) return false;
    const auto &record = reinterpret_cast<const ModNativeTentacleRecord *>(view.definitions)[index];
    if (record.controlEntities.m_size > 4096) return false;
    // initialize_polytubes fills control point zero from exactly this entity.
    // The optional source entity is only a fallback for an absent first point.
    base = record.controlEntities.m_data && record.controlEntities.m_size
        ? record.controlEntities.m_data[0] : nullptr;
    if (!base) base = record.resolvedEntity;
    if (!base) return false;
    owner = conglom;
    return true;
}

bool modResolveTentacleOwner(polytube *tube, conglomerate *&owner, entity_base *&base)
{
    auto *info = tube->field_130;
    if (info && info->tentacle == tube && info->my_ai && info->base_node) {
        auto *actor = info->my_ai->field_64;
        if (actor && actor->is_a_conglomerate()) {
            owner = static_cast<conglomerate *>(actor);
            base = info->base_node;
            return true;
        }
    }
    auto *world = g_world_ptr;
    if (!world) return false;
    // A tube created by tentacle_interface has no AI info or parent. Resolve
    // ownership by exact tube-array membership, never by distance or appearance.
    if (world->get_num_players() > 0 &&
        modMatchTentacleOwner(world->get_hero_ptr(0), tube, owner, base)) return true;
    std::size_t blocks = 0, entities = 0;
    for (auto &block : world->ent_mgr.entities.field_0) {
        if (++blocks > 4096 || block.size() > 100000) return false;
        for (auto *candidate : block) {
            if (++entities > 100000) return false;
            if (modMatchTentacleOwner(candidate, tube, owner, base)) return true;
        }
    }
    return false;
}

bool modRenderAttachedPolytube(polytube *tube, Float distance)
{
    // Endpoint-driven webs rebuild their entire line inside native render.
    if (!tube->field_78 || tube->field_11C || tube->field_120) return false;
    auto &spline = tube->the_spline;
    std::size_t count = 0;
    if (!modmesh::attachment::vectorCount(spline.control_pts, 4096, count) ||
        !modmesh::attachment::hasExtendedSpan(spline.control_pts.m_first, count) ||
        (tube->field_79 && (spline.field_30 < 0 || spline.field_30 > 256 ||
            (count + 2) * std::size_t(spline.field_30) > 65536))) return false;
    // Check original controls before resolving/repositioning an attachment or
    // copying its cached curve: collapsed native tubes must remain collapsed.
    conglomerate *owner = nullptr;
    entity_base *base = nullptr;
    if (!modResolveTentacleOwner(tube, owner, base)) return false;
    vector3d worldDelta;
    if (!modMeshAttachmentDelta(owner, base, worldDelta)) return false;
    if (!modmesh::attachment::finitePoint(worldDelta)) return false;

    const po &world = tube->get_abs_po();
    const vector3d localDelta = world.non_affine_inverse_xform(worldDelta);
    if (!modmesh::attachment::finitePoint(localDelta)) return false;

    // Retail rebuild frees arrays through 0x82207C and allocates at 0x822046.
    // Every shadow array therefore uses that same heap. The original spline,
    // including all pointers/cache flags, is restored before this scope exits.
    modmesh::attachment::ScopedSplineShadow<::spline, exe_allocator> shadow(spline);
    if (!shadow.valid()) return false;
    if (tube->field_79 && spline.need_rebuild) spline.rebuild_helper();
    auto &points = tube->field_79 ? spline.curve_pts : spline.control_pts;
    if (!modmesh::attachment::vectorCount(points, 65536, count) ||
        !modmesh::attachment::shiftRenderPoints(points.m_first, count, localDelta)) return false;
    // At 0x5A5D19 native render selects this array, then copies its positions
    // into scratch vertices synchronously. No queued node borrows these points.
    THISCALL(0x005A5B10, tube, distance);
    static polytube *reported[8] = {};
    static unsigned reportedCount = 0;
    bool seen = false;
    for (unsigned i = 0; i < reportedCount; ++i) seen |= reported[i] == tube;
    if (!seen && reportedCount < 8) {
        reported[reportedCount++] = tube;
        sp_log("[modmesh] attachment draw owner=%s tube=%p base-bone=%u points=%u delta=(%.6f %.6f %.6f)",
               owner->get_id().to_string(), tube, unsigned(base->get_bone_idx()),
               unsigned(count), worldDelta.x, worldDelta.y, worldDelta.z);
    }
    return true;
}
}
#endif

VALIDATE_SIZE(polytube, 0x178u);
VALIDATE_SIZE(polytube_pt_anim, 0x2C);

VALIDATE_SIZE(PolytubeCustomVertex::Iterator, 0x4Cu);

polytube_pt_anim::polytube_pt_anim() : field_0(0), 
                    field_4(ZEROVEC),
                    field_10(ZEROVEC),
                    field_1C(0.0),
                    field_20(0.0),
                    field_24(0.0),
                    field_28(0.0)
{
}

polytube::polytube(const string_hash &a2, uint32_t a3) : entity(a2, a3)
{
    static Var<bool> g_generating_vtables = (0x0095A6F1);

    this->field_79 = 0;
    this->field_7A = 0;
    this->field_7B = 0;
    this->the_spline = spline{};
    this->field_11C = 0;
    this->field_120 = 0;
    this->field_128 = 0;
    this->field_144 = 0;
    this->field_148 = 0;
    this->field_14C = 0;
    if (!g_generating_vtables()) {
        this->field_68 = nullptr;
        this->field_6C = nullptr;
        this->init();
    }
}

int polytube::get_num_control_pts() {
    return this->the_spline.get_num_control_pts();
}

void polytube::build(int a1, spline::eSplineType a2) {
    this->the_spline.build(a1, a2);
}

void polytube::init_offsets() {
    assert(num_sides > 0);
    assert(tube_radius > 0);

    if constexpr (0) {
    } else {
        THISCALL(0x00B96220, this);
    }
}

void polytube::init() {
    this->field_D0 = nullptr;
    this->field_D8 = nullptr;
    this->field_D4 = nullptr;
    this->field_E4 = 0;
    this->num_sides = 2;
    this->tube_radius = 0.025f;
    this->tiles_per_meter = 1.0;
    this->field_79 = 1;
    this->field_7A = 0;
    this->max_length = -1.0;
    this->field_4 |= 0x100;
    this->field_7A = 0;
    this->field_104 = 0.0;
    this->field_100 = 0.0;
    this->field_128 = -1;
    this->field_DC = 0;
    this->field_E0 = 0;
    this->field_11C = 0;
    this->field_120 = 0;
    this->field_124 = 1.0;
    this->field_7B = 0;
    this->field_7C = 0;
    this->field_108 = 0;

    string_hash v5 {"c_alpha"};
    this->set_material(v5);
    this->field_78 = 0;
    this->field_74 = nullptr;
    this->field_70 = nullptr;
    this->field_130 = nullptr;

    std::memset(this->field_15C, 0, sizeof(this->field_15C));

    this->field_8 = (this->field_8 & 0x8002041F) | 0xF;
    this->field_144 = 0;
    this->field_148 = 0;
    this->field_14C = 0;
    this->field_142 = 0;
    this->field_150 = 0;
    this->field_154 = 0;
    this->field_158 = 0;
    this->field_140 = -1;
}

void polytube::_render(Float a2)
{
    TRACE("polytube::render");
#if MOD_MESH_SUPPORT && !defined(TARGET_XBOX)
    try {
        if (modRenderAttachedPolytube(this, a2)) return;
    } catch (const std::bad_alloc &) {
        // Allocation failure leaves native spline storage intact.
    }
#endif
    THISCALL(0x005A5B10, this, a2);
}

void polytube::set_control_pt(int index, const vector3d &a2) {
    this->the_spline.set_control_pt(index, a2);
}

vector3d polytube::get_control_pt(int a3) {
    auto &v4 = this->the_spline.get_control_pt(a3);

    auto a2 = this->get_abs_po().slow_xform(v4);
    return a2;
}

void polytube::rebuild_helper() {
    if (this->the_spline.need_rebuild) {
        this->the_spline.rebuild_helper();
    }
}

void polytube::set_abs_control_pt(int index, const vector3d &a3) {
    auto &abs_po = this->get_abs_po();

    auto v4 = abs_po.inverse_xform(a3);
    this->set_control_pt(index, v4);
}

void polytube::set_max_length(Float a2) {
    this->max_length = a2;
}

void polytube::frame_advance_all_polytubes(Float a1)
{
    TRACE("polytube::frame_advance_all_polytubes");

    CDECL_CALL(0x0059B490, a1);
}

void polytube::set_material(string_hash a2)
{
    auto *v3 = this->field_D0;
    if (v3 != nullptr)
    {
        void (__fastcall *finalize)(void *, void *, bool) = CAST(finalize, get_vfunc(v3->m_vtbl, 0x0));
        finalize(v3, nullptr, false);

        mem_dealloc(v3, sizeof(*v3));
        this->field_D0 = nullptr;
    }

    auto *mem = mem_alloc(sizeof(PCUV_ShaderMaterial));
    nglTexture *v5 = nglGetTexture(a2.source_hash_code);
    auto *v4 = new (mem) PCUV_ShaderMaterial {v5, static_cast<nglBlendModeType>(2), 0, 72};
    v4->field_1C = &v5->field_60;
    v4->m_vtbl = 0x0087E698;
    this->field_D0 = v4;
}

void polytube::set_material(PolytubeCustomMaterial *a2)
{
    THISCALL(0x005A2460, this, a2);
}

void polytube::set_tiles_per_meter(Float a2)
{
    this->tiles_per_meter = a2;
    assert(tiles_per_meter > 0.0f);
}

void polytube::check_anims(bool a2)
{
    if ( !this->pt_anims.empty() || a2 )
    {
        auto anim_size = this->pt_anims.size();
        auto pt_size = this->get_num_control_pts();
        assert(anim_size <= pt_size);

        while ( anim_size < pt_size )
        {
            polytube_pt_anim pt_anim {};

            this->pt_anims.push_back(pt_anim);

            ++anim_size;

            assert(anim_size == pt_anims.size());
        }

        assert(pt_anims.size() == get_num_control_pts());
    }
}

void polytube::add_control_pt(const vector3d &a2)
{
    this->the_spline.add_control_pt(a2);
    this->check_anims(false);

    if (this->field_130)
    {
        this->destroy_tentacle_info();
        this->create_tentacle_info();
    }
}

void polytube::destroy_tentacle_info()
{
    THISCALL(0x005A29B0, this);
}

void polytube::create_tentacle_info()
{
    THISCALL(0x005A2930, this);
}

void PolytubeCustomVertex::Iterator::Write(
        const vector3d &a2,
        const vector3d &a3)
{
    TRACE("PolytubeCustomVertex::Iterator::Write");

    if constexpr (0)
    {
        auto a3a = a3;
        vector3d v52 {};
        auto v49 = this->field_48;
        this->field_8->field_4 = 0;
        if ( this->field_0 > 0 )
        {
            auto a2a = a2 - this->field_18;
            auto len = a2a.length();
            a2a = a2a / len;
            if ( this->field_0 == 1 )
            {
                auto v14 = vector3d::cross(a2a, a3a);
                v14.normalize();
                this->field_30 = v14;
                this->field_24 = vector3d::cross(this->field_30, a2a);
                this->field_3C = this->field_48;
            }

            vector3d v52 = vector3d::cross(a2a, a3a);
            v52.normalize();

            a3a = vector3d::cross(v52, a2a);

            auto v16 = this->field_8->field_8;
            auto v17 = this->field_3C - len * this->field_44;
            float v54 = v16 - 1;
            float v53 = 0.0;
            auto v49 = v17;
            float v18 = (v16 - 1);
            if ( (int)(v16 - 1) < 0 ) {
                v18 += 4.2949673e9;
            }

            v54 = 1.0f / v18;

            this->field_C.BeginStrip(2 * v16);
            this->field_8->field_4 = 0;

            auto v19 = v53;
            while ( this->field_8->field_4 < this->field_8->field_8 )
            {
                vector3d v55[2] {};
                auto v20 = this->field_8;
                auto v21 = v20->field_4;

                auto v22 = v20->field_0[4 * v21];
                auto v23 = v20->field_0[4 * v21 + 1];

                auto v24 = v23 * this->field_24;
                auto v26 = v22 * this->field_30;
                auto v61 = v26 + this->field_18;
                v55[0] = v61 + v24;

                auto v57 = v23 * a3a;
                auto v63 = v22 * v52;
                auto v59 = a2 + v63;
                v55[1] = v59 + v57;

                auto sub_67B1C5 = [](
                        nglVertexDef_MultipassMesh<nglVertexDef_PCUV_Base>::Iterator *self,
                        const vector3d &a2,
                        int a3,
                        float a4,
                        float a5) -> void
                {
                    static auto sub_681610 = [](nglVertexDef *a1)
                    {
                        struct {
                            vector3d field_0;
                            float field_C[2];
                            int field_14;
                        } *v1;
                        return bit_cast<decltype(v1)>(a1->field_4->field_4C + int(a1->field_4->field_3C.m_vertexData));
                    };

                    auto *v5 = sub_681610(self->field_4) + self->field_8;
                    v5->field_0 = a2;
                    v5->field_14 = a3;
                    v5->field_C[0] = a4;
                    v5->field_C[1] = a5;
                };

                sub_67B1C5(&this->field_C, v55[0], this->field_40, 0.0f, this->field_3C);
                ++this->field_C.field_8;

                sub_67B1C5(&this->field_C, v55[1], this->field_40, 0.0f, v49);
                ++this->field_C.field_8;

                ++this->field_8->field_4;
            }
        }

        this->field_18 = a2;
        this->field_24 = a3a;
        this->field_30 = v52;

        this->field_3C = v49;
    }
    else
    {
        THISCALL(0x00403BE0, this, &a2, &a3);
    }
}

void mod_polytube_attachment_patch()
{
#if MOD_MESH_SUPPORT && !defined(TARGET_XBOX)
    FUNC_ADDRESS(address, &polytube::_render);
    set_vfunc(0x0088F46C, address);
#endif
}

void polytube_patch()
{
    mod_polytube_attachment_patch();

    REDIRECT(0x005584E8, polytube::frame_advance_all_polytubes);

    {
        FUNC_ADDRESS(address, &PolytubeCustomVertex::Iterator::Write);
        REDIRECT(0x005A60A6, address);
        REDIRECT(0x005A61E4, address);
        REDIRECT(0x005A6404, address);
        REDIRECT(0x005A658F, address);
    }
}
