#include "resource_pack_streamer.h"

#include "binary_search_array_cmp.h"
#include "common.h"
#include "debugutil.h"
#include "filespec.h"
#include "func_wrapper.h"
#include "limited_timer.h"
#include "log.h"
#include "mesh_file_resource_handler.h"
#include "nfl_system.h"
#include "osassert.h"
#include "os_developer_options.h"
#include "parse_generic_mash.h"
#include "resource_directory.h"
#include "resource_manager.h"
#include "resource_pack_header.h"
#include "resource_pack_location.h"
#include "resource_pack_token.h"
#include "return_address.h"
#include "trace.h"
#include "utility.h"
#include "worldly_pack_slot.h"

#include <cassert>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

using list_t = _std::list<resource_pack_queue_entry>;

#ifndef TEST_CASE
VALIDATE_SIZE(list_t, 12u);
VALIDATE_SIZE(list_t::m_head, 4u);
VALIDATE_SIZE(list_t::m_size, 4u);
VALIDATE_SIZE((*(list_t::m_head)), 56u);

VALIDATE_SIZE(_std::_Container_base, 1u);

VALIDATE_OFFSET(list_t, m_head, 4u);
VALIDATE_OFFSET(list_t, m_size, 8u);

VALIDATE_SIZE(list_t::_Mybase, 3u);
VALIDATE_SIZE(list_t::_Myt, 12u);
#endif

using List_ptr_t = _std::_List_ptr<list_t::value_type, std::allocator<list_t::value_type>>;

VALIDATE_SIZE(List_ptr_t, 2u);

VALIDATE_SIZE(resource_pack_streamer, 0x90);

VALIDATE_SIZE(resource_pack_queue_entry, 0x30u);

resource_pack_queue_entry::resource_pack_queue_entry() : field_0(), field_28() {}

resource_pack_streamer::resource_pack_streamer()
{
    if constexpr (1)
    {
        this->currently_streaming = false;
        this->active = true;
        this->clear();
    }
    else
    {
        THISCALL(0x0053E040, this);
    }

}



resource_pack_streamer::~resource_pack_streamer()
{
    if constexpr (0)
    {
        this->clear();

        THISCALL(0x00504B80, &this->field_6C);
        operator delete(this->field_6C.m_head);
        this->field_6C.m_head = nullptr;
    }
    else
    {
        THISCALL(0x00537C00, this);
    }
}

void resource_pack_streamer::init(
        resource_partition *a2,
        _std::vector<resource_pack_slot *> *slots)
{
    assert(!currently_streaming);

    this->clear();
    this->field_68 = (int)a2;

    assert(slots != nullptr);

    this->pack_slots = slots;
    this->field_7C = 0.0;
    this->m_data_size = 0;
}

resource_directory *g_resource_directory = nullptr;

void resource_pack_streamer::load_internal(const char *a2,
                                           int which_slot_idx,
                                           bool (*cb)(resource_pack_slot::callback_enum,
                                                      resource_pack_streamer *,
                                                      resource_pack_slot *,
                                                      limited_timer *),
                                           const resource_pack_token &token)
{
    TRACE("resource_pack_streamer::load_internal", a2);

    if constexpr (1)
    {
        assert(!currently_streaming);

        this->field_8 = resource_key {string_hash {a2}, RESOURCE_KEY_TYPE_PACK};

        assert(pack_slots != nullptr);
        assert(which_slot_idx >= 0 && ((uint32_t) which_slot_idx) < pack_slots->size());

        this->m_slot_index = which_slot_idx;
        this->field_7C = 0.0;
        this->m_data_size = 0;
        this->curr_slot = this->pack_slots->at(which_slot_idx);
      //  assert(curr_slot != nullptr && curr_slot->is_empty());

        resource_pack_location pack_location{};

        if (!resource_manager::get_pack_file_stats(this->field_8, &pack_location, nullptr, nullptr)) {
            auto *str = this->field_8.m_hash.to_string();
            sp_log("Packfile %s not found in amalgapak.  Perhaps you need to repack it?", str);
            assert(0);
        }

        this->curr_loc = pack_location;
        this->field_78 = nullptr;

        auto v12 = (resource_manager::using_amalgapak()
                ? resource_manager::amalgapak_id
                : resource_manager::open_pack(a2));

        this->curr_file_id = v12;

        nflRequestParams params{};
        
        if (auto *slot = this->curr_slot;
                pack_location.loc.m_size > slot->get_slot_size())
        {
            int size = pack_location.loc.m_size;

            int budget = slot->get_slot_size();

            int v13 = size - budget;

            auto *v10 = this->field_8.m_hash.to_string();
            sp_log("%s's memory is over-budget by %d bytes (budget is %u size is %u)",
                   v10,
                   v13,
                   budget,
                   size);
        }

        params.field_18 = nullptr;
        params.dataSize = 0;
        params.field_24 = nullptr;
        params.field_14 = pack_location.loc.m_offset;
        params.streamID = 0;
        params.field_10 = 2;
        params.field_20 = 0;
        params.fileID = v12;

        params.m_callback = stream_request_callback;

        params.field_C = NFL_REQUEST_TYPE_READ;
        params.field_18 = this->curr_slot->get_header_mem_addr();

        assert(pack_location.field_28 == 0);
        params.dataSize = pack_location.loc.m_size - pack_location.field_28;

        assert((params.dataSize % 2048) == 0);

        params.field_24 = this;

        assert(curr_stream_request_id == NFL_REQUEST_ID_INVALID);

        this->curr_stream_request_id = nflAddRequest(&params);
        assert(curr_stream_request_id != NFL_REQUEST_ID_INVALID);

        this->m_data_size = params.dataSize;
        this->curr_slot->notify_load_started(this->field_8, this->m_data_size, cb, token);
        this->currently_streaming = true;

        if (os_developer_options::instance->get_flag(mString {"SHOW_STREAMER_SPAM"}))
        {
            auto *str = this->field_8.m_hash.to_string();

            debug_print_va("Streamer load start %s", str);
        }
    } else {
        THISCALL(0x0054C580, this, a2, which_slot_idx, cb, &token);
    }
}

void resource_pack_streamer::unload(resource_pack_slot *s)
{
    assert(pack_slots != nullptr);

    for ( uint32_t i {0}; i < this->pack_slots->size(); ++i )
    {
        if ( this->pack_slots->at(i) == s )
        {
            this->unload(i);
            return;
        }
    }

    assert(0);
}

void resource_pack_streamer::unload(int which_slot_idx)
{
    assert(pack_slots != nullptr);
    assert(which_slot_idx >= 0 && ((uint32_t) which_slot_idx) < pack_slots->size());

    auto *which_slot = pack_slots->at(which_slot_idx);
    assert(which_slot != nullptr);

    if (!which_slot->is_empty()) {
        assert(which_slot->is_pack_ready());

        this->unload_internal(which_slot_idx);
    }
}

bool resource_pack_streamer::can_cancel_load(int a2) const
{
    return this->currently_streaming && this->m_slot_index == a2;
}

void resource_pack_streamer::cancel_load(int which_slot_idx)
{
    assert(can_cancel_load( which_slot_idx ));
    if ( this->curr_stream_request_id != NFL_REQUEST_ID_INVALID)
    {
        nflCancelRequest(this->curr_stream_request_id);
        this->curr_stream_request_id = NFL_REQUEST_ID_INVALID;
    }

    if ( this->field_88 != NFL_REQUEST_ID_INVALID )
    {
        nflCancelRequest(this->field_88);
        this->field_88 = NFL_REQUEST_ID_INVALID;
    }

    if ( !resource_manager::using_amalgapak() )
    {
        nflCloseFile(this->curr_file_id);
        this->curr_file_id = NFL_FILE_ID_INVALID;
    }

    assert(curr_slot != nullptr);

    this->curr_slot->notify_load_cancelled();

    this->currently_streaming = false;
    this->field_8 = {};

    this->m_slot_index = -1;
    this->curr_slot = nullptr;
}

void resource_pack_streamer::stream_request_callback(nflRequestState a1,
                                                     nflRequestID a2,
                                                     resource_pack_streamer *which_streamer)
{
    TRACE("resource_pack_streamer::stream_request_callback");

    if constexpr (1)
    {
        assert(which_streamer != nullptr);

        if (a1 == NFS_REQUEST_STATE_WAITING
                && which_streamer->currently_streaming
                && a2 == which_streamer->curr_stream_request_id)
        {
            nflRequestID v3 = which_streamer->field_88;

            which_streamer->curr_stream_request_id = NFL_REQUEST_ID_INVALID;

            if (v3 == NFL_REQUEST_ID_INVALID) {
                which_streamer->finish_data_read();
            }
        }
    } else {
        CDECL_CALL(0x00542970, a1, a2, which_streamer);
    }

}

void resource_pack_streamer::clear() {

    if constexpr (1)
    {
        assert(!currently_streaming && "Attempting to clear a streamer while it is still actively streaming");
        this->currently_streaming = false;

        this->field_8 = {};

        this->m_slot_index = -1;
        this->curr_slot = nullptr;
        this->pack_slots = nullptr;
        this->curr_stream_request_id = NFL_REQUEST_ID_INVALID;
        this->field_88 = NFL_REQUEST_ID_INVALID;
        this->curr_file_id = NFL_FILE_ID_INVALID;

        THISCALL(0x00504B80, &this->field_6C);
    }
    else
    {
        THISCALL(0x00531B70, this);
    }

}

bool resource_pack_streamer::all_slots_idle() const
{
    if (this->pack_slots == nullptr) {
        return true;
    }

    auto &slots = (*this->pack_slots);
    for (auto &slot : slots)
    {
        if ( !(slot->is_empty() || slot->is_pack_ready()) ) {
            return false;
        }
    }

    return true;
}

bool resource_pack_streamer::is_idle() const {
    return this->is_disk_idle() && this->all_slots_idle();
}

bool resource_pack_streamer::is_disk_idle() const
{
    return !this->currently_streaming && this->field_6C.empty();
}

void resource_pack_streamer::flush(void (*a2)(void)) {
    this->flush(a2, Float{0.02f});
}


void resource_pack_streamer::flush(void (*a2)(void), Float a3) {
    TRACE("resource_pack_streamer::flush");

    if constexpr (1)
    {
        assert(this->active);

        limited_timer timer{a3};

        while (this->currently_streaming
                || !this->field_6C.empty()
                || !this->all_slots_idle())
        {
            if (a2 != nullptr) {
                a2();
            }

            nflUpdate();

            timer.reset();
            this->frame_advance({0.0}, &timer);

#ifdef TARGET_XBOX
    if (g_resource_directory != nullptr)
    {
        auto *tlres = g_resource_directory->get_tlresource(to_hash("mini_map_frame"), TLRESOURCE_TYPE_MESH_FILE);
        sp_log("0x%08X", tlres);
        assert(tlres != nullptr);
    }
#endif

        }

    }
    else
    {
        THISCALL(0x00551200, this, a2, a3);
    }
}

//bool (*)(resource_pack_slot::callback_enum,resource_pack_streamer *,resource_pack_slot *,limited_timer *)

void resource_pack_streamer::frame_advance_idle([[maybe_unused]] Float a2)
{
    TRACE("resource_pack_streamer::frame_advance_idle");

    if constexpr (1) {
        if (!this->field_6C.empty()) {
            resource_pack_queue_entry &v3 = this->field_6C.front();

            auto *str = v3.field_0.to_string();
            int idx = v3.field_20;
            auto *cb = v3.m_callback;

            resource_pack_token a5 = v3.field_28;

            this->load_internal(str, idx, cb, a5);

            if constexpr (0) {
                this->field_6C.pop_front();
            } else {
                auto *m_head = this->field_6C.m_head;
                auto *v6 = m_head->_Next;
                if ( m_head->_Next != m_head ) {
                    v6->_Prev->_Next = v6->_Next;
                    v6->_Next->_Prev = v6->_Prev;
                    CDECL_CALL(0x0082207C);
                    //operator delete(v6);
                    --this->field_6C.m_size;
                }
            }
        }
    } else {
        THISCALL(0x0054C820, this, a2);
    }
}

void resource_pack_streamer::load(const char *a2,
                                  int which_slot_idx,
                                  bool (*cb)(resource_pack_slot::callback_enum,
                                             resource_pack_streamer *,
                                             resource_pack_slot *,
                                             limited_timer *),
                                  const resource_pack_token *a5)
{
    TRACE("resource_pack_streamer::load", a2);

    assert(this->pack_slots != nullptr);
    assert(which_slot_idx >= 0 && ((uint32_t) which_slot_idx) < this->pack_slots->size());

    auto *which_slot = this->pack_slots->at(which_slot_idx);

   // assert(which_slot != nullptr);

  //  assert(which_slot->is_empty());

    assert(!this->currently_streaming);

    for (auto &entry : this->field_6C) {
        auto *str = entry.field_0.to_string();
        
        if (strcmpi(str, a2) == 0)
        {
            error("Tried to queue %s for load twice.", a2);
        }
        
        if (entry.field_20 == which_slot_idx)
        {
            error("Tried to queue %s for load in a slot that is already queued for load.", a2);
        }
    }

    if constexpr (1)
    {
        string_hash v2 {a2};

        auto &v7 = (*this->pack_slots);
        for (size_t i = 0; i < v7.size(); ++i) {
            auto *v10 = v7[i];
            if (v10 != nullptr) {
                if (!v10->is_empty()) {
                    auto v3 = v10->get_name_key().m_hash;
                    if (v2 == v3) {
                        sp_log("Trying to load pack %s when it is already loaded", a2);
                 //       assert(0);
                    }
                }
            }
        }

        resource_pack_queue_entry v16{};

        fixedstring<8> v15{a2};
        static_assert(sizeof(v15) == 32);

        v16.field_0 = v15;
        v16.field_20 = which_slot_idx;
        v16.m_callback = cb;
        if (a5 != nullptr) {
            v16.field_28 = *a5;
        }

#ifndef TEST_CASE
        {
            auto iterator = this->field_6C.end();
            auto *_Pnode = iterator._Mynode();

            using list_t = _std::list<resource_pack_queue_entry>;

            static_assert(std::is_same_v<decltype(_Pnode), list_t::_Nodeptr>);
            static_assert(std::is_same_v<decltype(_Pnode->_Prev), list_t::_Nodeptr>);

            list_t::_Nodeptr (__fastcall *Buynode)(void *, void *, list_t::_Nodeptr, list_t::_Nodeptr, resource_pack_queue_entry *) = CAST(Buynode, 0x00566AB0);

            auto *_Newnode =
                Buynode(&this->field_6C, nullptr, _Pnode, _Pnode->_Prev, &v16);

            void (__fastcall *Incsize)(void *, void *, uint32_t) = CAST(Incsize, 0x00566AE0);

            Incsize(&this->field_6C, nullptr, 1u);
            list_t::_Prevnode(_Pnode) = _Newnode;
            list_t::_Nextnode(list_t::_Prevnode(_Newnode)) = _Newnode;
        }
#else
        this->field_6C.push_back(v16);
#endif

        this->frame_advance_idle(0.0);
    }
    else
    {
        THISCALL(0x00550F90, this, a2, which_slot_idx, cb, a5);
    }

}

//FIXME
void resource_pack_streamer::frame_advance(Float a2, limited_timer *a3)
{
    TRACE("resource_pack_streamer::frame_advance");
    if constexpr (0)
    {
        if (this->active) {
            if (this->currently_streaming) {
                this->frame_advance_streaming(a2);
            } else {
                this->frame_advance_idle(a2);
            }

            if (this->pack_slots != nullptr)
            {
                auto &pack_slots = (*this->pack_slots);

                std::vector<resource_pack_slot *> v15{};

                if (!pack_slots.empty())
                {
                    v15.reserve(pack_slots.size());

                    for (auto &slot : pack_slots)
                    {
                        slot->frame_advance(a2, a3);
                        if (slot->is_pack_unloading() && slot->is_empty()) {
                            v15.push_back(slot);
                        }
                    }
                }

                for (auto &slot : v15) {
                    auto client_done = slot->try_callback((resource_pack_slot::callback_enum) 6, nullptr);
                    assert(client_done);
                }
            }
        }
    } else {
        THISCALL(0x005510F0, this, a2, a3);
    }
}

void resource_pack_streamer::frame_advance_streaming(Float a2)
{
    TRACE("resource_pack_streamer::frame_advance_streaming");

    if constexpr (1)
    {
        this->field_7C += a2;
        assert(this->curr_slot != nullptr);

        if (this->curr_stream_request_id == NFL_REQUEST_ID_INVALID &&
            this->field_88 == NFL_REQUEST_ID_INVALID) {
            this->curr_slot->notify_load_finished();

            if (os_developer_options::instance->get_flag(mString {"SHOW_RESOURCE_SPAM"})) {
                auto &res_dir = this->curr_slot->get_resource_directory();
                res_dir.debug_print();
            }

            this->currently_streaming = false;

            this->field_8 = {};

            this->m_slot_index = -1;
            this->curr_slot = nullptr;
        }
    } else {
        THISCALL(0x0051EF70, this, a2);
    }
}

void resource_pack_streamer::unload_all() {
    TRACE("resource_pack_streamer::unload_all");

    if constexpr (0)
    {
        assert(!currently_streaming);

        if constexpr (1)
        {
            THISCALL(0x00504B80, &this->field_6C);
        }
        else
        {
            this->field_6C.clear();
        }

        assert(pack_slots != nullptr);

        for (size_t i = 0; i < pack_slots->size(); ++i) {
            auto *slot = pack_slots->at(i);
            assert(slot != nullptr);

            assert(slot->is_empty() || slot->is_pack_ready());

            if (slot->is_pack_ready()) {
                this->unload(i);
            }
        }
    } else {
        THISCALL(0x0053E150, this);
    }
}

void resource_pack_streamer::set_active(bool a2) {
  //  assert(!currently_streaming);

    this->active = a2;
}

void resource_pack_streamer::unload_internal(int which_slot_idx)
{
    TRACE("resource_pack_streamer::unload_internal", std::to_string(which_slot_idx).c_str());

    if constexpr (1)
    {
        assert(pack_slots != nullptr);
        assert(which_slot_idx >= 0 && ((uint32_t) which_slot_idx) < pack_slots->size());

        resource_pack_slot *which_slot = pack_slots->at(which_slot_idx);
        assert(which_slot != nullptr);
        assert(which_slot->is_pack_ready());

        which_slot->notify_unload_started();

        if (os_developer_options::instance->get_flag(mString {"SHOW_STREAMER_SPAM"}))
        {
            auto *str = which_slot->get_name_key().m_hash.to_string();

            debug_print_va("Streamer unload start %s", str);
        }

    } else {
        THISCALL(0x00537C60, this, which_slot_idx);
    }
}

void resource_pack_streamer::finish_data_read()
{
    TRACE("resource_pack_streamer::finish_data_read",
                this->field_8.get_platform_string(g_platform).c_str());

    if constexpr (1)
    {
        assert(this->curr_slot != nullptr);

        auto *pack_file_header = bit_cast<resource_pack_header *>(this->curr_slot->get_header_mem_addr());
        sp_log("header_mem_addr = 0x%08X", pack_file_header);

        assert(pack_file_header != nullptr);

        pack_file_header->verify(this->field_8);

        auto *header =
            bit_cast<generic_mash_header *>(this->curr_slot->get_header_mem_addr() +
                                               pack_file_header->directory_offset);
        assert(header == CAST(header, bit_cast<char *>(pack_file_header) + 0x30));

        resource_directory *directory = nullptr;
        auto alloced_mem =
            parse_generic_object_mash(directory, header, nullptr, nullptr, nullptr, 0, 0, nullptr);
        assert(!alloced_mem && "This should NOT allocate anything!");

        for (const auto &i : directory->type_start_idxs)
        {
            assert(i >= 0 && i <= directory->resource_locations.size());
        }

        auto *header_mem_addr = this->curr_slot->get_header_mem_addr();

        directory
            ->constructor_common(this->curr_slot,
                                 &header_mem_addr[pack_file_header->res_dir_mash_size],
                                 this->field_78,
                                 pack_file_header->field_20 - pack_file_header->res_dir_mash_size,
                                 pack_file_header->field_24);

        assert(directory->parents.size() >= curr_loc.prerequisite_count);

        if (this->curr_loc.prerequisite_count != 0) {
            for (int i = 0; i < this->curr_loc.prerequisite_count; ++i) {
                const auto prereq_idx = i + this->curr_loc.prerequisite_offset;

                auto *parent_name = resource_manager::get_prerequisiste(prereq_idx);
                assert(parent_name != nullptr);

                directory->parents.m_data[i] = resource_manager::get_resource_directory(
                    *parent_name);
                if (directory->parents.m_data[i] == nullptr) {
                    auto str1 = parent_name->m_hash.to_string();

                    auto str0 = this->field_8.m_hash.to_string();

                    sp_log("%s's parent '%s' is not loaded", str0, str1);
                }
            }
        }

        this->curr_slot->set_resource_directory(directory);

        if (!resource_manager::using_amalga) {
            nflCloseFile(this->curr_file_id);
        }

    }
    else
    {
        THISCALL(0x0053E1A0, this);
    }
}

void resource_pack_streamer_patch() {

    {
        FUNC_ADDRESS(address, &resource_pack_streamer::load_internal);
        REDIRECT(0x0054C89C, address);
    }

    {
        FUNC_ADDRESS(address, &resource_pack_streamer::frame_advance);
        REDIRECT(0x0055127A, address);
    }

    {
        FUNC_ADDRESS(address, &resource_pack_streamer::frame_advance_streaming);
        REDIRECT(0x00551124, address);
    }

    {
        FUNC_ADDRESS(address, &resource_pack_streamer::frame_advance_idle);
        REDIRECT(0x005510D3, address);
        REDIRECT(0x00551132, address);
    }

    {
        FUNC_ADDRESS(address, &resource_pack_streamer::unload_internal);
        SET_JUMP(0x00537C60, address);
    }

    {
        FUNC_ADDRESS(address, &resource_pack_streamer::cancel_load);
        REDIRECT(0x0055148D, address);
    }
    return;

    {
        void (resource_pack_streamer::*flush)(void (*a2)(void),
                                              Float) = &resource_pack_streamer::flush;

        FUNC_ADDRESS(address, flush);
        SET_JUMP(0x00551200, address);
    }

    {
        FUNC_ADDRESS(address, &resource_pack_streamer::unload_all);
        REDIRECT(0x00558A21, address);
    }

    {
        FUNC_ADDRESS(address, &resource_pack_streamer::finish_data_read);
        REDIRECT(0x005429A2, address);
    }

    {
        FUNC_ADDRESS(address, &resource_pack_streamer::load);
        SET_JUMP(0x00550F90, address);
    }
}
