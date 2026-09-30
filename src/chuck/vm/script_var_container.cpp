#include "script_var_container.h"

#include "func_wrapper.h"
#include "chuck_str.h"
#include "chunk_file.h"
#include "common.h"
#include "memory.h"
#include "parse_generic_mash.h"
#include "script_library_class.h"
#include "script_manager.h"
#include "slc_manager.h"
#include "trace.h"

#include <cassert>
#include <map>

VALIDATE_SIZE(script_var_container, 0x1Cu);

static constexpr auto SCRIPT_VAR_FLAG_UN_MASHED = 4u;

void script_var_container::un_mash_start(generic_mash_header *a2, void *a3, generic_mash_data_ptrs *a4, void *)
{
    TRACE("script_var_container::un_mash_start");

    assert(( flags & SCRIPT_VAR_FLAG_UN_MASHED ) == 0);
    this->un_mash(a2, a3, a4);

    assert(this->debug_info == nullptr);

    assert(this->field_10 != 0);
}

void script_var_container::script_var_to_addr_t::un_mash(
        void *,
        generic_mash_header *,
        script_var_container *a3,
        void *,
        generic_mash_data_ptrs *a5)
{
    a5->rebase(4);

    int v6 = *a5->get<int>();

    this->name = a5->get<char>(v6);

    sp_log("is_game = %s", a3->is_game() ? "true" : "false");

    int addr = ( a3->is_game()
            ? (int) script_manager::get_game_var_address(this->address)
            : (int) script_manager::get_shared_var_address(this->address)
            );

    sp_log("0x%X", addr);

    this->address = addr;
}

script_var_container::script_var_container()
{
    this->script_var_to_addr = nullptr;
    this->field_10 = 0;
    this->debug_info = nullptr;
    this->flags = 0;
}

void * script_var_container::operator new(size_t size)
{
    return mem_alloc(size);
}

void script_var_container::operator delete(void *ptr, size_t size)
{
    mem_dealloc(ptr, size);
}

void script_var_container::un_mash(generic_mash_header *header, void *a3, generic_mash_data_ptrs *a4)
{
    TRACE("script_var_container::un_mash");

    if constexpr (1) {
        assert(((int)header) % 4 == 0);

        this->script_var_block.un_mash(header, this, a4);

        a4->rebase(4);

        a4->rebase(4);

        this->script_var_to_addr = a4->get<script_var_to_addr_t>(this->field_10);

        a4->rebase(4);

        for ( auto i = 0; i < this->field_10; ++i )
        {
            assert(((int)header) % 4 == 0);

            this->script_var_to_addr[i].un_mash(
                nullptr,
                header,
                this,
                &this->script_var_to_addr[i],
                a4);
        }

        this->flags |= SCRIPT_VAR_FLAG_UN_MASHED;
    } else {
        THISCALL(0x00599440, this, header, a3, a4);
    }
}

void script_var_container::destroy()
{
    THISCALL(0x005A3370, this);
}

int script_var_container::save_script_var_buffer(char *a2)
{
    TRACE("script_var_container::save_script_var_buffer", a2 != nullptr ? "true" : "false");

    if ( a2 != nullptr ) {
        auto v4 = this->script_var_block.size();
        auto *buffer = this->script_var_block.get_buffer();
        memcpy(a2, buffer, v4);
    }

    return this->script_var_block.size();
}

int script_var_container::load_script_var_buffer(char *a2)
{
    TRACE("script_var_container::load_script_var_buffer");

    if ( a2 != nullptr )
    {
        auto v4 = this->script_var_block.size();
        auto *buffer = this->script_var_block.get_buffer();
        memcpy(buffer, a2, v4);
    }

    return this->script_var_block.size();
}

void *script_var_container::get_script_var_address(const char *a2, script_library_class **a3) {
    return (void *) THISCALL(0x005A0520, this, a2, a3);
}

char *script_var_container::get_address(int offset)
{
    TRACE("script_var_container::get_address", std::to_string(offset).c_str());

    assert(this->script_var_block.get_buffer() != nullptr);
    assert(script_var_block.is_address_in_buffer( int( script_var_block.get_buffer() ) + offset ));
    return (this->script_var_block.get_buffer() + offset);
}

void script_var_container::read(chunk_file *file, script_var_container *c)
{
    TRACE("script_var_container::read");

    assert(c != nullptr);

    assert(c->debug_info == nullptr);

    if constexpr (0)
    {
        c->debug_info = new (mem_alloc(8u)) std::decay_t<decltype(*c->debug_info)> {};
        assert(c->debug_info != nullptr);

        auto *mem = mem_alloc(12);
        c->debug_info->var_to_offset = new (mem) _std::map<mString, int> {};
        assert(c->debug_info->var_to_offset != nullptr);

        mem = mem_alloc(12);
        c->debug_info->var_to_slc = new (mem) _std::map<mString, script_library_class *> {};
        assert(c->debug_info->var_to_slc != nullptr);
    }

    std::map<mString, int> v39 {};

    chunk_flavor cf {"UNREG"};
    cf = file->read<chunk_flavor>();
    assert(cf == SCRIPT_MANAGER_VAR_HEADER_CHUNK);

    c->field_10 = file->read<int>();
    
    assert(c->script_var_to_addr == nullptr);

    c->script_var_to_addr = new std::decay_t<decltype(*c->script_var_to_addr)> [c->field_10];
    assert(c->script_var_to_addr != nullptr);

    auto game_var_buffer_size = file->read<int>();
    THISCALL(0x005A3740, &c->script_var_block, game_var_buffer_size);

    auto address = uint32_t(c->script_var_block.get_buffer());
    for ( int i = 0; i < c->field_10; ++i )
    {
        auto v34 = file->read<int>();
        
        auto v33 = file->read<mString>();

        auto type_size = file->read<int>();

        auto a1a = file->read<mString>();
        if ( a1a != "" )
        {
            auto v18 = (int)address;

            using pair_t = std::decay_t<decltype(v39)>::value_type;
            v39.insert(pair_t {a1a, v18});

            auto *my_class = slc_manager::get(v33.c_str());
            sp_log("%s", v33.c_str());
            assert(my_class != nullptr);

            if constexpr (0)
            {
                if ( c->debug_info->var_to_slc != nullptr && my_class != nullptr )
                {
                    assert(type_size == my_class->get_size() && "inconsistent size");

                    using pair_t = std::decay_t<decltype(*c->debug_info->var_to_slc)>::value_type;
                    c->debug_info->var_to_slc->insert(pair_t {a1a, my_class});
                }

                if ( c->debug_info->var_to_offset != nullptr )
                {
                    auto *buffer = c->script_var_block.get_buffer();
                    int v23 = address - ((int) buffer);

                    using pair_t = std::decay_t<decltype(*c->debug_info->var_to_offset)>::value_type;
                    c->debug_info->var_to_offset->insert(pair_t {a1a, v23});
                }
            }
        }

        address += v34 * type_size;
    }

    assert(address == ( (unsigned int)( c->script_var_block.get_buffer() ) + game_var_buffer_size ) 
            && "unexpected type size in some game var type");

    int index = 0;
    for ( auto &v5 : v39 )
    {
        auto dest_buffer_length = v5.first.size() + 1;
        c->script_var_to_addr[index].name = new char[dest_buffer_length];
        assert(c->script_var_to_addr[index].name != nullptr);

        auto v9 = dest_buffer_length;
        auto *v7 = v5.first.c_str();
        chuck_strcpy(c->script_var_to_addr[index].name, v7, v9);
        c->script_var_to_addr[index].address = v5.second;
        ++index;
    }

    //c->field_10 = index;
}
