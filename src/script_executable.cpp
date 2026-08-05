#include "script_executable.h"

#include "entity_mash.h"
#include "func_wrapper.h"

#include "filespec.h"
#include "chunk_file.h"
#include "chuck_str.h"
#include "memory.h"
#include "script_manager.h"
#include "script_object.h"
#include "trace.h"
#include "common.h"
#include "parse_generic_mash.h"
#include "resource_key.h"
#include "script_library_class.h"
#include "script_executable_allocated_stuff_record.h"
#include "utility.h"
#include "variables.h"
#include "vm_executable.h"

#include <cassert>

VALIDATE_SIZE(script_executable, 0x5Cu);

script_executable::script_executable()
{
	this->constructor_common();
}

void * script_executable::operator new(size_t size) {
    return mem_alloc(size);
}

void script_executable::operator delete(void *ptr, size_t size) {
    mem_dealloc(ptr, size);
}

void script_executable::constructor_common()
{
	assert(script_allocated_stuff_map == nullptr);

	if constexpr (1) {
		auto *v2 = mem_alloc(sizeof(*script_allocated_stuff_map));
		if ( v2 != nullptr ) {
			decltype(script_allocated_stuff_map) (__fastcall *sub_5B8490)(void *) = CAST(sub_5B8490, 0x005B8490);
			this->script_allocated_stuff_map = sub_5B8490(v2);
		}
	} else {
		THISCALL(0x005AFC50, this);
	}

	assert(script_allocated_stuff_map != nullptr);
}

bool script_executable::has_threads() const
{
	if constexpr (1)
	{
		for ( int i = 0; i < this->total_script_objects; ++i )
		{
			auto &so = this->script_objects[i];
			if ( so->has_threads() ) {
				return true;
			}
		}

		return false;
	} else {
		return (bool) THISCALL(0x0059BBA0, this);
	}
}

bool script_executable::is_from_mash() const {
    return (this->flags & SCRIPT_EXECUTABLE_FLAG_FROM_MASH) != 0;
}

void script_executable::quick_un_mash()
{
    TRACE("script_executable::quick_un_mash");

    assert(( flags & SCRIPT_EXECUTABLE_FLAG_UN_MASHED ) != 0);

    assert(( flags & SCRIPT_EXECUTABLE_FLAG_LINKED ) != 0);

    assert(( flags & SCRIPT_EXECUTABLE_FLAG_FROM_MASH ) != 0);

    this->constructor_common();
    for ( auto i = 0; i < this->total_script_objects; ++i ) {
        this->script_objects[i]->quick_un_mash();
    }
}

#if 0
bool compare(const script_object *a, const script_object *b)
{
    assert(a != nullptr);
    assert(b != nullptr);

    if (a->name != b->name) {
        sp_log("script_object::name: 0x%08X %s != %s", a->name.source_hash_code, a->name.to_string(), b->name.to_string());
        return false;
    }

    assert(a->data_blocksize == b->data_blocksize);
    assert(a->total_funcs == b->total_funcs);
    assert(a->field_28 == b->field_28);

    if (a->is_global_object() != b->is_global_object()) {
        sp_log("is_global_object(): %d != %d", a->is_global_object(), b->is_global_object());
        return false;
    }

    return true;
}

bool script_executable::compare(const script_executable &a, const script_executable &b)
{
    if (!(a.field_0 == b.field_0)) {
        sp_log("field_0: %s != %s", a.field_0.to_string(), b.field_0.to_string());
        return false;
    }

    if (a.sx_exe_image_size != b.sx_exe_image_size) {
        sp_log("sx_exe_image_size: %d != %d", a.sx_exe_image_size, b.sx_exe_image_size);
        //return false;
    }

    if (a.total_script_objects != b.total_script_objects) {
        sp_log("total_script_objects: %d != %d", a.total_script_objects, b.total_script_objects);
        return false;
    }

    for (auto i = 0; i < a.total_script_objects; ++i)
    {
        auto *so0 = a.script_objects[i];
        auto *so1 = b.script_objects[i];

        assert(::compare(so0, so1));
    }

    assert(a.permanent_string_table_size == b.permanent_string_table_size);

    return true;
}
#else
bool script_executable::compare(const script_executable &, const script_executable &) {
    return false;
}
#endif

void script_executable::un_mash_start(generic_mash_header *a2, void *a3, generic_mash_data_ptrs *a4, [[maybe_unused]] void *a5)
{
    TRACE("script_executable::un_mash_start");

    if constexpr (1)
    {
        this->un_mash(a2, a3, a4);
    }
    else
    {
        void (__fastcall *func)(void *,void *edx, generic_mash_header *, void *, generic_mash_data_ptrs *, void *) = CAST(func, 0x005B04D0);
        func(this, nullptr, a2, a3, a4, a5);
    }

    printf("field_58 = %d\n", this->field_58);

    if constexpr (0)
    {
        sp_log("%d", this->system_string_table_size);
        if (this->field_0 == fixedstring<8>{"CITY_ARENA"}) {
            assert(0);
        }
    }

    if constexpr (0) {
        assert(this->script_object_dummy_list == nullptr);

        if (!(this->field_0 == fixedstring<8>{"INIT_GV"})) {
            return;
        }

        g_is_the_packer() = true;

        script_executable se{};
        se.load(resource_key {string_hash {this->field_0.to_string()}, RESOURCE_KEY_TYPE_SCRIPT});

        assert(script_executable::compare(*this, se));

        g_is_the_packer() = false;
    }
}

script_object *script_executable::find_object(int index) const
{
    TRACE("script_executable::find_object", std::to_string(index).c_str());

    assert(index >= 0);

    assert(index < total_script_objects);

    assert(script_objects_by_name != nullptr);

    return this->script_objects_by_name[index];
}

void script_executable::info_t::un_mash(
        generic_mash_header *header,
        script_executable *a3,
        void *,
        generic_mash_data_ptrs *a5)
{
    assert(so_name == chuck_str_t::INVALID_STRING_HASH);
    auto *owner = a3->find_object(this->field_18);
    assert(owner != nullptr);

    if ( this->field_10 == -1 )
    {
        rebase(a5->field_0, 4u);

        this->field_8 = bit_cast<vm_executable *>(a5->field_0);
        a5->field_0 += sizeof(vm_executable);

        assert(((int)header) % 4 == 0);

        this->field_8->un_mash(header, owner, this->field_8, a5);
    }
}

void script_executable::un_mash(generic_mash_header *header, void *a3, generic_mash_data_ptrs *a4)
{
    TRACE("script_executable::un_mash");

    if constexpr (1)
    {
        if ( this->is_un_mashed() )
        {
            this->quick_un_mash();
        }
        else 
        {
            assert(script_object_dummy_list == nullptr);
            
            rebase(a4->field_0, 4u);

            static auto *start_debug = a4->field_0;

            [this, &a4]() {
                if constexpr (1)
                {
                    filespec v98 {mString {this->field_0.to_string()}};
                    v98.m_dir = mString {"scripts\\"};
                    v98.m_ext = mString {".pc"} + "sxl";

                    os_file v85 {};
                    v85.open(v98.fullname(), os_file::FILE_READ);
                    if ( v85.is_open() )
                    {
                        sp_log("found pcsxl file %s", v98.fullname().c_str());
                        this->sx_exe_image_size = v85.get_size();
                        this->sx_exe_image = new uint16_t[this->sx_exe_image_size / 2];
                        assert(sx_exe_image != nullptr);

                        v85.read(this->sx_exe_image, this->sx_exe_image_size);
                        return;
                    }
                }

                this->sx_exe_image = CAST(this->sx_exe_image, a4->field_0);
            }();

            a4->field_0 += this->sx_exe_image_size;

            rebase(a4->field_0, 4u);

            sp_log("0x%08X", sx_exe_image_size);
            sp_log("offset = 0x%08X", a4->field_0 - start_debug);

            this->script_objects = a4->get<script_object *>(this->total_script_objects);

            sp_log("offset = 0x%08X", a4->field_0 - start_debug);
            for ( auto i = 0; i < this->total_script_objects; ++i )
            {
                rebase(a4->field_0, 8u);

                rebase(a4->field_0, 4u);
                
                this->script_objects[i] = a4->get<script_object>();

                assert(((int) header) % 4 == 0);
                auto &so = this->script_objects[i];
                so->un_mash(header, this, so, a4);
            }


            this->global_script_object = this->script_objects[0];

            rebase(a4->field_0, 4u);

            this->script_objects_by_name = a4->get<script_object *>(this->total_script_objects);
            for ( auto i = 0; i < this->total_script_objects; ++i ) {
                this->script_objects_by_name[i] = this->script_objects[(int)this->script_objects_by_name[i]];
            }

            rebase(a4->field_0, 4u);

            this->permanent_string_table = a4->get<char *>(this->permanent_string_table_size);

            for ( auto i = 0; i < this->permanent_string_table_size; ++i )
            {
                rebase(a4->field_0, 4u);

                auto v21 = *a4->get<uint32_t>();

                this->permanent_string_table[i] = (char *) a4->field_0;
                a4->field_0 += v21;
            }

            this->system_string_table = nullptr;
            this->system_string_table_size = 0;

            rebase(a4->field_0, 4u);

            rebase(a4->field_0, 4u);

            this->field_54 = a4->get<info_t>(this->field_58);

            rebase(a4->field_0, 4u);

            for (int i = 0; i < this->field_58; ++i)
            {
                assert(((int)header) % 4 == 0);

                if constexpr (1) {
                    this->field_54[i].un_mash(header, this, &this->field_54[i], a4);
                } else {
                    auto *info = this->field_54 + i;
                    
                    assert(info->so_name == string_hash::INVALID_STRING_HASH);

                    auto *owner = this->find_object(info->field_18);
                    assert(owner != nullptr);

                    if ( info->field_10 == -1 )
                    {
                        rebase(a4->field_0, 4u);

                        info->field_8 = a4->get<vm_executable>();

                        assert(((int) header) % 4 == 0);
                        info->field_8->un_mash(header, owner, info->field_8, a4);
                    }
                }
            }

            this->constructor_common();
            this->flags |= SCRIPT_EXECUTABLE_FLAG_UN_MASHED;
        }
    }
    else
    {
        THISCALL(0x005B01F0, this, header, a3, a4);
    }
}

vm_executable *script_executable::find_function_by_address(const uint16_t *a2) const
{
    //TRACE("script_executable::find_function_by_address");

    if constexpr (1)
    {
        for ( auto i = 0; i < this->total_script_objects; ++i )
        {
            auto &so = this->script_objects[i];
            if ( so != nullptr )
            {
                auto a1 = so->find_function_by_address(a2);
                if ( a1 != -1 ) {
                    return so->get_func(a1);
                }
            }
        }

        return nullptr;
    } else {
        return (vm_executable *) THISCALL(0x0058F280, this, a2);
    }
}

vm_executable *script_executable::find_function_by_name(string_hash a2) const
{
	TRACE("script_executable::find_function_by_name");
	if constexpr (1)
	{
		for ( int v3 = 0; ++v3 < this->total_script_objects; ++v3 )
		{
			auto &so = this->script_objects[v3];
			if ( auto idx = so->find_func(a2);
					idx != -1 )
			{
				auto *func = so->get_func(idx);
				auto *owner = func->get_owner();
				if ( owner->is_global_object() ) {
					return func;	
				}
			}
		}

		return nullptr;
	} else {
		return (vm_executable *) THISCALL(0x0058F310, this, a2);
	}
}

void script_executable::register_allocated_stuff_callback(int a2, void (*a3)(script_executable *, _std::list<uint32_t> &, _std::list<mString> &))
{
    TRACE("script_executable::register_allocated_stuff_callback");

    if constexpr (1) {
        assert(script_allocated_stuff_map != nullptr);
        script_executable_allocated_stuff_record a2a {};
        a2a.field_0 = a3;

        this->script_allocated_stuff_map->insert({a2, a2a});
    } else {
        THISCALL(0x005AF460, this, a2, a3);
    }
}

int script_executable::get_total_allocated_stuff(int a2)
{
    TRACE("script_executable::get_total_allocated_stuff");
    assert(this->script_allocated_stuff_map != nullptr);

    auto it = this->script_allocated_stuff_map->find(a2);
    assert(it != script_allocated_stuff_map->end() && "you need to register first");

    return it->second.stuff.size();
}

void script_executable::add_object_by_name(script_object *a1, int a3)
{
    TRACE("script_executable::add_object_by_name");

    int i;
    for ( i = 0; i < a3; ++i )
    {
        auto v4 = this->script_objects_by_name[i]->name;
        if ( v4 > a1->name ) {
            break;
        }
    }

    if ( i == a3 ) {
        this->script_objects_by_name[a3] = a1;
    } else {
        for ( auto j = a3; j > i; --j ) {
            this->script_objects_by_name[j] = this->script_objects_by_name[j - 1];
        }

        this->script_objects_by_name[i] = a1;
    }

    if (!std::is_sorted(this->script_objects_by_name, this->script_objects_by_name + a3 + 1))
    //if constexpr (0)
    {
        std::for_each(this->script_objects_by_name, this->script_objects_by_name + a3 + 1, [](auto &so) {
            printf("0x%08X\n", so->name.source_hash_code);
        });
        //assert(0);
    }

}

void script_executable::add_object(script_object *so, int &a3)
{
    if ( so->is_global_object() ) {
        this->global_script_object = so;
        this->script_objects[0] = so;
    } else {
        this->script_objects[a3++] = so;
    }

#ifdef TARGET_XBOX
    struct local_slc_t : script_library_class {
        local_slc_t(const char *s) : script_library_class(s, 4, nullptr, true) {}
    };

    auto *mem = mem_alloc(sizeof(local_slc_t));
    auto *v3 = so->name.to_string();
    auto *v6 = new (mem) local_slc_t {v3};

    mString a1 {v3};

    struct {
        mString field_0;
        script_library_class *field_C;
    } a2a = {a1, v6};
    //sub_6963C1((void *)v12->script_object_dummy_list, (int)v11, (int)a2a);
#endif

}

static constexpr auto CHUCK_STR_MAX_LENGTH = 47u;

void script_executable::load(const resource_key &resource_id)
{
    this->load(resource_id, mString {"scripts\\"});
}

void script_executable::load(const resource_key &resource_id, const mString &dir,
                            const mString &name_override)
{
    TRACE("script_executable::load", resource_id.get_platform_string(3).c_str());

    script_manager::run_callbacks((script_manager_callback_reason)3, this, nullptr);

    filespec v98 {mString {resource_id.m_hash.to_string()}};
    //if ( v98.m_dir == mString::null() && script_manager::using_chuck_old_fashioned() )
    {
        v98.m_dir = dir;
    }

    if ( name_override.size() > 0 ) {
        v98.m_name = name_override;
    }

    this->field_0 = fixedstring<8>{v98.m_name.c_str()};
    
    auto get_platform_extension = []() -> mString {
        if constexpr (0) {
            return mString {".xb"};
        } else {
            return mString {".pc"};
        }
    };

    v98.m_ext = get_platform_extension() + "sst";
    chunk_file v97 {};
    v97.open(v98.fullname(), os_file::FILE_READ);

    if ( !v97.is_open() ) {
        auto v23 = "unable to open " + v98.fullname() + " for reading";
        script_manager::run_callbacks((script_manager_callback_reason)4, nullptr, v23.c_str());
    }

    auto cf = v97.read<chunk_flavor>();
    assert(cf == chunk_flavor {"strngtbl"});

    this->system_string_table_size = v97.read<int>();
    this->system_string_table = new char *[this->system_string_table_size];
    assert(system_string_table != nullptr);

    for ( auto i = 0; i < this->system_string_table_size; ++i ) {
        mString v94 = v97.read<mString>();

        auto &sst = this->system_string_table[i];
        sst = static_cast<char *>(operator new(v94.size() + 1));
        assert(sst != nullptr);

        chuck_strcpy(sst, v94.c_str(), v94.size() + 1);
    }
    
    chunk_file source{};

    v98.m_ext = get_platform_extension() + "pst";
    source.open(v98.fullname(), os_file::FILE_READ);
    assert(source.is_open());

    cf = source.read<chunk_flavor>();
    assert(cf == chunk_flavor {"strngtbl"});

    this->permanent_string_table_size = source.read<int>();
    this->permanent_string_table = new char *[this->permanent_string_table_size];
    assert(permanent_string_table != nullptr);

    for ( auto i = 0; i < this->permanent_string_table_size; ++i ) {
        mString v90 = source.read<mString>();

        auto &pst = this->permanent_string_table[i];
        pst = static_cast<char *>(operator new(v90.size() + 1));

        assert(pst != nullptr);

        chuck_strcpy(pst, v90.c_str(), v90.size() + 1);
    }

    chunk_file io{};
    chunk_flavor v88 {"UNREG"};
    
    v98.m_ext = get_platform_extension() + "sx";
    io.open(v98.fullname(), os_file::FILE_READ);
    assert(io.is_open());

    {
        auto v87 = v98.fullname() + "l";

        assert(sx_exe_image == nullptr);

        filespec v86 {v87};
        os_file v85 {};
        v85.open(v86.fullname(), os_file::FILE_READ);
        if ( v85.is_open() ) {
            auto file_size = v85.get_size();
            this->sx_exe_image_size = file_size;
            this->sx_exe_image = new uint16_t[file_size / 2];
            assert(sx_exe_image != nullptr);

            v85.read(this->sx_exe_image, file_size);
        }

        if ( this->sx_exe_image == nullptr )
        {
            auto a1g = v87 + ": executable code file is missing; this is now required with the sx file";
            script_manager::run_callbacks((script_manager_callback_reason)4, this, a1g.c_str());
        }
    }

    cf = io.read<chunk_flavor>();
    if ( cf != CHUNK_SCRIPT_OBJECTS )
    {
        auto a2d = v98.fullname() + ": bad format; script objects were expected";
        script_manager::run_callbacks((script_manager_callback_reason)4, this, a2d.c_str());
    }

    this->total_script_objects = io.read<int>();
    assert(total_script_objects > 0);

    assert(script_objects == nullptr);

    this->script_objects = new script_object *[this->total_script_objects];
    this->script_objects_by_name = new script_object *[this->total_script_objects];


    assert(script_objects != nullptr);
    assert(script_objects_by_name != nullptr);

    auto *mem = mem_alloc(12);
    this->script_object_dummy_list = new (mem) _std::map<mString, script_library_class *>{};

    assert(script_object_dummy_list != nullptr);

    [[maybe_unused]] int v84 = 1;
    int v83 = 0;
    while ( v83 < this->total_script_objects )
    {
        cf = io.read<chunk_flavor>();
        if ( cf == CHUNK_SCRIPT_OBJECT )
        {
            auto *so = new script_object{};
            assert(so != nullptr && "Couldn't create a script object");

            auto v81 = io.read<int>();
            auto *my_name = this->system_string_table[v81];
            sp_log("my_name = %s", my_name);
            assert(strlen( my_name ) < CHUCK_STR_MAX_LENGTH && "the CHUCK compiler should disallow this case");

            so->name = string_hash {my_name};
            so->set_parent(this);

            this->add_object_by_name(so, v83);
            io.read(so);
            this->add_object(so, v84);

            ++v83;
        }
        else
        {
            auto a2e = v98.fullname() + ": bad format; script object was expected";
            auto *v20 = a2e.c_str();
            script_manager::run_callbacks((script_manager_callback_reason)4, this, v20);
        }
    }

    cf = io.read<chunk_flavor>();
    assert(cf == CHUNK_END);

    script_manager::run_callbacks((script_manager_callback_reason)2, this, nullptr);
}

vm_thread *script_executable::sub_5AB510(Float a2)
{
	assert(this->global_script_object != nullptr);

	if constexpr (1)
    {
		auto *inst = this->global_script_object->get_global_instance();
		assert(inst != nullptr && "where did the global instance go?");

		char v1[4] {};
		memcpy(v1, &a2, 4);
		return inst->add_thread(this->global_script_object->get_func(0), v1);
	} else {
		return (vm_thread *) THISCALL(0x005AB510, this, a2);
	}
}

static bool g_cant_create_threads {};

void script_executable::un_load(bool a2)
{
    TRACE("script_executable::un_load");

    if constexpr (1)
	{
        script_manager::run_callbacks((script_manager_callback_reason)1, this, nullptr);
        for ( auto i = 0; i < 20; ++i )
		{
            if ( script_object::function_cache()[i].field_0 != nullptr ) {
                if ( script_object::function_cache()[i].field_0->get_parent() == this ) {
                    script_object::function_cache()[i].field_0 = nullptr;
                    script_object::function_cache()[i].field_4 = string_hash {0};
                    script_object::function_cache()[i].field_8 = -1;
                    script_object::function_cache()[i].field_C = 0;
                }
            }
        }

        if ( a2 )
		{
            for ( auto j = 0; j < this->total_script_objects; ++j ) {
                auto &so = this->script_objects[j];
                so->create_destructor_instances();
            }

            g_cant_create_threads = true;
            bool v12 = false;
            int v11 = 0;
            while ( !v12 ) {
                v12 = true;
                for ( auto k = this->total_script_objects - 1; k >= 0; --k ) {
                    auto &v9 = this->script_objects[k];

                    if ( v9->get_size_instances() > 0 )
                    {
                        v9->run(true);
                        if ( v9->has_threads() ) {
                            v12 = false;
                        }
                    }
                }

                if ( ++v11 > 50000 ) {
                    script_manager::dump_threads_to_file();
                    sp_log("Script destructors never finished executing. Talk to martin or the script author.");
                    assert(0);
                }
            }
        }

        assert(this->script_allocated_stuff_map != nullptr);

        for ( auto &v2 : (*this->script_allocated_stuff_map) ) {
            if ( v2.second.stuff.size() != 0 ) {
                auto &v6 = v2.second.debug_stuff_descriptions;
                auto &v5 = v2.second.stuff;
                v2.second.field_0(this, v5, v6);
            }
        }

        g_cant_create_threads = false;
        script_manager::run_callbacks((script_manager_callback_reason) 0, this, nullptr);
    } else {
        THISCALL(0x005AF780, this, a2);
    }
}

void script_executable::release_mem()
{
    TRACE("script_executable::release_mem");

    if constexpr (1) {
        for ( auto i = 0; i < this->total_script_objects; ++i ) {
            this->script_objects[i]->release_mem();
        }

        if ( script_allocated_stuff_map != nullptr ) {
            THISCALL(0x005B8460, this->script_allocated_stuff_map);
            mem_dealloc(script_allocated_stuff_map, sizeof(*script_allocated_stuff_map));
            this->script_allocated_stuff_map = nullptr;
        }
            
        release_generic_mash(this);

    } else {
        THISCALL(0x005B0470, this);
    }
}

script_library_class * script_executable::find_library_class(const mString &a2) const {
    TRACE("script_executable::find_library_class", a2.c_str());

    assert(this->script_object_dummy_list != nullptr);
    auto it = script_object_dummy_list->find(a2);
    auto end = script_object_dummy_list->end();
    if ( it != end ) {
        return it->second;
    } 

    return nullptr;
}

const char *script_executable::get_permanent_string(unsigned int index) const
{
    assert(permanent_string_table != nullptr && "We should still have the string table around any time we're doing a lookup");

    assert((int)index < permanent_string_table_size && "Index out of bounds... bad juju man");

    return this->permanent_string_table[index];
}

script_object *script_executable::find_object(
        const string_hash &a2,
        int *a3) const
{
    TRACE("script_executable::find_object", a2.to_string());

    if constexpr (0) {
        int v8 = 0;
        int v7 = this->total_script_objects - 1;
        int v6 = this->total_script_objects / 2;
        script_object *v5 = nullptr;
        while ( 1 )
        {
            v5 = this->script_objects_by_name[v6];
            if ( v5->name == a2 ) {
                break;
            }

            auto v4 = v6;
            if ( v5->name == a2 )
            {
                v8 = v6 + 1;
                if ( v8 >= this->total_script_objects ) {
                    return nullptr;
                }
            }
            else
            {
                v7 = v6 - 1;
                if ( v7 < 0 ) {
                    return nullptr;
                }
            }

            if ( v8 <= v7 )
            {
                v6 = (v8 + v7) / 2;
                if ( v4 != v6 ) {
                    continue;
                }
            }

            return nullptr;
        }

        if ( a3 != nullptr ) {
            *a3 = v6;
        }

        return v5;
    } else {
       auto *so = (script_object *) THISCALL(0x0058F1C0, this, &a2, a3);
       return so;
    }
}

uint32_t script_executable::get_system_string_index(const std::set<string_hash> &set, const string_hash &p)
{
    assert(p != string_hash {0});
    assert(set.size() != 0);

    auto begin = set.begin();
    auto end = set.end();
    auto it_find = std::find_if(begin, end, [&p](auto &name) {
        return name == p;
    });

    assert(it_find != end);

    return std::distance(begin, it_find);
}

const char *script_executable::get_system_string(unsigned int index) const {

    assert(system_string_table != nullptr && "We should still have the string table around any time we're doing a lookup");

    assert((int)index < system_string_table_size && "Index out of bounds... bad juju man");

    return this->system_string_table[index];
}

uint16_t * script_executable::get_exec_code(unsigned int offset) {

    TRACE("script_executable::get_exec_code", std::to_string(offset).c_str());

    assert(sx_exe_image != nullptr && "We should have loaded the executable code from the sxl or sxb file");

    assert(offset < (uint32_t) sx_exe_image_size && "offset into exec code is out of bounds... bad juju man");

    return sx_exe_image + (offset >> 1);
}

void script_executable::link()
{
    TRACE("script_executable::link");

    assert(( ( flags & SCRIPT_EXECUTABLE_FLAG_LINKED ) == 0 ) && "trying to link the same exec more than once!!!");

    script_manager::run_callbacks((script_manager_callback_reason)6, this, nullptr);
    for ( auto i = 0; i < this->total_script_objects; ++i ) {
        auto &so = this->script_objects[i];
        so->link(this);
    }

    if ( this->is_from_mash() ) {
        assert(system_string_table == nullptr);

        assert(system_string_table_size == 0);
    } else {
        for ( auto j = 0; j < this->system_string_table_size; ++j ) {
            ::delete(this->system_string_table[j]);
        }

        operator delete[](this->system_string_table);
        this->system_string_table = nullptr;
        this->system_string_table_size = 0;
    }

    this->flags |= SCRIPT_EXECUTABLE_FLAG_LINKED;
    script_manager::run_callbacks((script_manager_callback_reason)7, this, nullptr);
}

void script_executable::dump_threads_to_file(FILE *a2) {
    TRACE("script_executable::dump_threads_to_file");

    for ( auto i = 0; i < this->total_script_objects; ++i ) {
        this->script_objects[i]->dump_threads_to_file(a2);
    }
}

void script_executable::run(Float a2, bool a3)
{
    TRACE("script_executable::run");

    if constexpr (1)
    {
        for ( auto i = 0; i < this->total_script_objects; ++i ) {

            auto &so = this->script_objects[i];
            if (so == nullptr) {
                sp_log("%d", i);
                assert(0);
            }

            if ( so->get_size_instances() != 0 ) {
                so->run(a3);
            }
        }
    }
    else
    {
        THISCALL(0x005AF990, this, a2, a3);
    }
}

void script_executable::first_run(Float a2, bool a3)
{
    TRACE("script_executable::first_run");

    if constexpr (1)
    {
        if ( !this->is_from_mash() ) {
            return;
        }

        this->global_script_object->get_static_data().set_to_zero();
        for (auto i = 0; i < this->field_58; ++i)
        {
            auto &info = this->field_54[i];
            assert(info.so_name == string_hash::INVALID_STRING_HASH);

            auto *so = this->find_object(info.field_18);
            assert(so != nullptr);

            sp_log("name = %s", so->get_name().to_string());
            sp_log("%d", info.field_C);

            auto &buffer = *bit_cast<int *>(this->global_script_object->get_static_data_buffer() + info.field_C);
            uint32_t v14 = info.field_10;
            switch (v14) {
            case 0xFFFFFFFD:
            case 0xFFFFFFFF:
                buffer = (int)so->add_instance(info.field_0, info.field_8);
                break;
            case 0xFFFFFFFE:
                buffer = (int)so->add_game_init_instance(info.field_0, 0);
                break;
            case 0xFFFFFFFC:
                buffer = info.field_14;
                break;
            default:
                buffer = (int)this->get_permanent_string(v14);
                break;
            }
        }
    }
    else
    {
        THISCALL(0x005AB440, this, a2, a3);
    }
}

void script_executable::add_allocated_stuff(
        int a2,
        uint32_t a3,
        const mString &a1)
{
    TRACE("script_executable::add_allocated_stuff");

    assert(this->script_allocated_stuff_map != nullptr);

    auto it = this->script_allocated_stuff_map->find(a2);
    assert(it != this->script_allocated_stuff_map->end() && "you need to register first");

    auto &v5 = (*it);
    v5.second.stuff.push_front(a3);
    v5.second.debug_stuff_descriptions.push_front(a1);
}



void script_executable::remove_allocated_stuff(int a2, uint32_t a3)
{
    TRACE("script_executable::remove_allocated_stuff");

    assert(this->script_allocated_stuff_map != nullptr);

    auto it = this->script_allocated_stuff_map->find(a2);
    assert(it != script_allocated_stuff_map->end() && "you need to register first");

    assert(it->second.stuff.size() == it->second.debug_stuff_descriptions.size());

    auto dsc_it = it->second.debug_stuff_descriptions.begin();
    for ( auto stuff_it = it->second.stuff.begin(), stuff_end = it->second.stuff.end();
            stuff_it != stuff_end;
            ++dsc_it, ++stuff_it
            ) {
        if ( (*stuff_it) == a3 ) {
            it->second.stuff.erase(stuff_it);
            it->second.debug_stuff_descriptions.erase(dsc_it);
            return;
        }
    }
}

void script_executable_patch()
{
    {
        FUNC_ADDRESS(address, &script_executable::get_total_allocated_stuff); 
        SET_JUMP(0x005A07F0, address);
    }

    {
        FUNC_ADDRESS(address, &script_executable::un_mash);
        REDIRECT(0x005B0850, address);
    }

    {
        FUNC_ADDRESS(address, &script_executable::register_allocated_stuff_callback);
        REDIRECT(0x005AF460, address);
    }

    {
        FUNC_ADDRESS(address, &script_executable::add_allocated_stuff);
        SET_JUMP(0x005A34B0, address);
    }

    {
        FUNC_ADDRESS(address, &script_executable::remove_allocated_stuff);
        SET_JUMP(0x005A0790, address);
    }
}
