#include "script_object.h"

#include "chunk_file.h"

#include "func_wrapper.h"
#include "memory.h"
#include "parse_generic_mash.h"
#include "script_executable.h"
#include "script_executable_entry.h"
#include "script_manager.h"
#include "vm_executable.h"
#include "vm_thread.h"
#include "common.h"
#include "trace.h"
#include "utility.h"

#include <cassert>
#include <cctype>
#include <cstdio>
#include <limits>
#include <unordered_set>

VALIDATE_SIZE(script_object, 0x34);
VALIDATE_SIZE(script_object::function, 0x10);

VALIDATE_SIZE(script_instance, 0x44);

VALIDATE_SIZE(vm_symbol, 0x4C);

script_object::script_object()
{
    this->instances = nullptr;
    this->flags = 0;
    this->constructor_common();
}

void script_object::constructor_common() {
    TRACE("script_object::constructor_common");

    assert(instances == nullptr);

    if constexpr (1) {
        auto *mem = mem_alloc(sizeof(*this->instances));
        this->instances = new (mem) simple_list<script_instance> {};
    } else {
        THISCALL(0x005A0750, this);
    }

    assert(instances != nullptr);
}

script_object::~script_object()
{
    if ( (this->flags & 2) != 0 ) {
        this->destructor_common();
    } else {
        this->destroy();
    }
}

void * script_object::operator new(size_t size)
{
    return mem_alloc(size);
}

void script_object::operator delete(void *ptr, size_t size)
{
    mem_dealloc(ptr, size);
}

void script_object::release_mem()
{
    TRACE("script_object::release_mem");

    this->destructor_common();
}

void script_object::destructor_common()
{
    TRACE("script_object::destructor_common");

    if ( this->instances != nullptr )
    {
        while ( !this->instances->empty() )
        {
            auto v5 = this->instances->begin();
            this->instances->common_erase(v5._Ptr);

            auto *v3 = v5._Ptr;
            if ( v3 != nullptr ) {
                delete v3;
            }
        }

        mem_dealloc(this->instances, sizeof(*this->instances));
        this->instances = nullptr;
    }

    this->global_instance = nullptr;
}

void script_object::destroy()
{
    if ( debug_info != nullptr )
    {
        this->debug_info->~debug_info_t();
        ::operator delete(debug_info);
        this->debug_info = nullptr;
    }

    if ( this->funcs != nullptr )
    {
        for ( auto i = 0; i < this->total_funcs; ++i )
        {
            auto &v5 = this->funcs[i];
            if ( v5 != nullptr ) {
                v5->~vm_executable();
                ::operator delete(v5);
            }
        }

        operator delete[](this->funcs);
        this->funcs = nullptr;
        this->total_funcs = 0;
    }

    this->destructor_common();
}

void script_object::create_destructor_instances()
{
	if constexpr (1)
    {
		this->global_instance = nullptr;
		if ( this->instances != nullptr )
		{
			for ( auto &v2 : (*this->instances) )
			{
				v2.massacre_threads(nullptr, nullptr);
				if ( v2.field_28 != nullptr )
				{
					if ( !v2.field_28->is_from_mash() )
					{
						v2.field_28->~vm_executable();
						::operator delete(v2.field_28);
					}

					v2.field_28 = nullptr;
				}
			}
		}

		if ( this->field_28 != -1
				&& this->field_28 < this->total_funcs
				&& this->instances != nullptr )
		{
			for ( auto &v3 : (*this->instances) ) {
				this->add_thread(&v3, this->field_28);
			}
		}
	}
    else
    {
		THISCALL(0x005AF320, this);
	}
}

void script_object::quick_un_mash()
{
    this->constructor_common();
    if ( this->is_global_object() ) {
        this->create_auto_instance(0.0);
    }
}

simple_list<vm_thread>::iterator script_instance::delete_thread(
        simple_list<vm_thread>::iterator a3)
{
    TRACE("script_instance::delete_thread");

    if constexpr (0)
    {
        auto *condemned = &(*a3);
        assert(condemned != nullptr);

        for ( auto &it : this->threads )
        {
            auto *v9 = &it;
            if ( v9 != condemned && v9->field_14 == condemned ) {
                v9->field_14 = nullptr;
            }
        }

        auto v8 = this->threads.erase(condemned);

        if ( condemned != nullptr ) {
            delete condemned;
            condemned = nullptr;
        }

        return v8;

    }
    else
    {
        using iterator_t = simple_list<vm_thread>::iterator;

        iterator_t it {};

        void (__fastcall *func)(void *, void *edx, iterator_t *, iterator_t) = CAST(func, 0x005AAE60);
        func(this, nullptr, &it, a3);
        return it;
    }
}

void script_instance::dump_threads_to_file(FILE *a2)
{
    TRACE("script_instance::dump_threads_to_file");

    for ( auto &v7 : this->threads )
    {
        if ( !v7.is_suspended() )
        {
            auto *exec = v7.get_executable();
            auto &name = exec->get_name();
            auto *v4 = name.to_string();
            auto *v3 = this->name.to_string();
            fprintf(a2, "%s %s\n", v3, v4);
        }
    }
}

void script_instance::run(bool a2)
{
    TRACE("script_instance::run");

    this->flags |= 2u;
    this->build_parameters();
    auto it = this->threads.begin();
    auto end = this->threads.end();
    while (it != end)
    {
        auto *t = it._Ptr;
        assert(t != nullptr);

        if ( (a2 || !t->is_suspended()) && t->run() ) {
            it = this->delete_thread(it);
        } else {
            ++it;
        }
    }
}

void script_instance::run_callbacks(
        script_instance_callback_reason_t a2,
        vm_thread *a3)
{
    TRACE("script_instance::run_callbacks");

    for ( auto &v1 : this->field_38 ) {
        this->m_callback(a2, this, a3, v1);
    }
}

void script_instance::build_parameters()
{
    TRACE("script_instance::build_parameters");

    if constexpr (1)
    {
        if ( this->field_28 != nullptr )
        {
            assert(parent != nullptr);

            static const string_hash inst_name {"__parms_builder"};
            auto *v6 = parent->add_instance(inst_name, nullptr, nullptr);
            vm_thread t {v6, this->field_28};
            t.run();
            this->parent->remove_instance(v6);
            t.inst = nullptr;
            t.ex = nullptr;
            auto *v12 = t.get_data_stack().buffer;
            assert(threads.size() == 1);

            auto *v11 = this->threads._first_element;
            auto constructor_parmsize = this->parent->get_constructor_parmsize();
            auto v4 = v12;
            auto &stack = v11->get_data_stack();
            stack.push(v4, constructor_parmsize);
            if ( !this->field_28->is_from_mash() )
            {
                auto *v10 = this->field_28;
                auto *v9 = v10;
                if ( v10 != nullptr ) {
                    v9->~vm_executable();
                    delete(v9);
                }
            }

            this->field_28 = nullptr;
        }
    }
    else
    {
        THISCALL(0x005AF500, this);
    }
}

int script_object::get_constructor_parmsize() {
    auto *func = this->get_func(0);
    auto v2 = func->get_parms_stacksize();
    if ( !func->is_static() ) {
        v2 -= 4;
    }

    return v2;
}

void script_object::run(bool a2)
{
    TRACE("script_object::run");

    for (auto &v1 : (*this->instances) ) {
        v1.run(a2);
    }
}

bool script_object::has_threads() const
{
    if (this->instances != nullptr)
	{
        for (auto &v1 : (*this->instances)) {
            if (v1.has_threads()) {
                return true;
            }
        }
    }

    return false;
}

void script_object::dump_threads_to_file(FILE *a2)
{
    TRACE("script_object::dump_threads_to_file");

    if ( this->instances != nullptr ) {
        for ( auto &v2 : (*this->instances) ) {
            v2.dump_threads_to_file(a2);
        }
    }
}

script_instance * script_object::add_instance(string_hash a2, char *a3, vm_thread **a4)
{
    TRACE("script_object::add_instance");

    assert(!this->is_global_object()
                && "please don't create global object instances with this method");

    if constexpr (1)
    {
        auto *inst = new script_instance {a2, this->data_blocksize, 0u};
        assert(inst != nullptr);

        this->add(inst);
        auto *con = this->get_func(0);
        assert(con->get_name() == name);

        auto *v9 = inst->add_thread(con);
        auto &stack = v9->get_data_stack();
        stack.push((char *) &inst, 4);

        auto v10 = con->get_parms_stacksize();
        if ( !con->is_static() ) {
            v10 -= 4;
        }

        if ( a3 != nullptr )
        {
            auto &stack = v9->get_data_stack();
            stack.push(a3, v10);
        }

        if ( a4 != nullptr ) {
            *a4 = v9;
        }

        return inst;
    } else {
        return (script_instance *) THISCALL(0x005AB120, this, a2, a3, a4);
    }
}

script_instance * script_object::add_instance(string_hash a2, vm_executable *parms_builder)
{
    TRACE("script_object::add_instance", a2.to_string());

    assert(!is_global_object() && "please don't create global object instances with this method");

    //assert(parms_builder->is_from_mash() && "this function should only be used for mashed parms_builders");

    if constexpr (0)
    {
        auto *inst = new script_instance {a2, this->data_blocksize, 0};
        assert(inst != nullptr);

        this->add(inst);

        auto *func = this->get_func(0);
        assert(func->get_name() == this->name);

        auto *t = inst->add_thread(func);
        auto &stack = t->get_data_stack();
        stack.push((const char *) &inst, 4);

        inst->field_28 = parms_builder;
        assert(parent != nullptr);

        if ( inst->field_28 != nullptr ) {
            inst->field_28->link(this->parent);
        }

        return inst;
    }
    else
    {
        script_instance * (__fastcall *func)(void *, void *edx, string_hash a2, vm_executable *parms_builder) = CAST(func, 0x005AB200);
        return func(this, nullptr, a2, parms_builder);
    }
}

void script_object::remove_instance(script_instance *a2)
{
    TRACE("script_object::remove_instance");

	assert(this->instances != nullptr);

	if constexpr (1)
	{
		for ( auto &v7 : (*this->instances) )
		{
			if ( (&v7) == a2 )
			{
				auto *v2 = &v7;
				if ( v2 == this->global_instance ) {
					this->global_instance = nullptr;
				}

				this->instances->common_erase({v2});

				delete v2;
				return;
			}
		}

		assert(0);
	}
    else
    {
		THISCALL(0x005ADC60, this, a2);
	}
}

script_instance * script_object::add_game_init_instance(string_hash a2, int a3)
{
    TRACE("script_object::add_game_init_instance");

    auto *inst = new script_instance {a2, this->data_blocksize, a3 | 4u};
    assert(inst != nullptr);

    this->add(inst);
    return inst;
}

void script_object::add(script_instance *a2)
{
    TRACE("script_object::add");

    if constexpr (1)
    {
        assert(instances != nullptr);

        a2->set_parent(this);
        this->instances->emplace_back(a2);
    }
    else
    {
        THISCALL(0x0059ECC0, this, a2);
    }
}

void script_object::link(const script_executable *a2)
{
    TRACE("script_object::link");

    for ( auto i = 0; i < this->total_funcs; ++i )
    {
        auto &x = this->funcs[i];
        x->link(a2);
    }
}

void script_object::un_mash(generic_mash_header *header, void *a3, void *a4, generic_mash_data_ptrs *a5)
{
    TRACE("script_object::un_mash");

    if constexpr (1)
    {
        this->parent = static_cast<script_executable *>(a3);
        assert(((int)header) % 4 == 0);

        this->static_data.un_mash(header, &this->static_data, a5);

        rebase(a5->field_0, 4u);

        this->funcs = a5->get<vm_executable *>(this->total_funcs);
        for ( auto i = 0; i < this->total_funcs; ++i )
        {
            rebase(a5->field_0, 4u);

            this->funcs[i] = a5->get<vm_executable>();

            assert(((int)header) % 4 == 0);
            this->funcs[i]->un_mash(header, this, this->funcs[i], a5);
        }

        this->constructor_common();
        if ( this->is_global_object() ) {
            this->create_auto_instance(Float{0.0});
        }

    }
    else
    {
        THISCALL(0x005AB350, this, header, a3, a4, a5);
    }

    sp_log("flags = 0x%08X", this->flags);
    //assert(this->debug_info == nullptr);
}

void script_object::create_auto_instance(Float a2)
{
    TRACE("script_object::create_auto_instance");

    if constexpr (1)
    {
        assert(this->instances != nullptr);

        auto &con = *this->get_func(0);
        assert(con.get_name() == name);

        if ( con.get_parms_stacksize() == 4 )
        {
            static string_hash auto_inst_name {int(to_hash("__auto"))};

            auto *inst = new script_instance {auto_inst_name, this->data_blocksize, 0};

            assert(inst != nullptr);
            inst->set_parent(this);

            if ( this->is_global_object() )
            {
                assert(global_instance == nullptr);

                this->instances->emplace_back(inst);
                this->global_instance = inst;
            }
            else
            {
                this->instances->push_back(inst);
            }

            auto *new_thread = inst->add_thread(&con);
            if ( this->is_global_object() ) {
                auto &stack = new_thread->get_data_stack();
                stack.push(a2);
            } else {
                auto &stack = new_thread->get_data_stack();
                stack.push((const char *)&inst, 4);
            }
        }
    }
    else
    {
        THISCALL(0x005AAEF0, this, a2);
    }
}

vm_executable * script_object::get_func(int i)
{
    assert(funcs != nullptr);

    assert(i >= 0);

    assert(i < total_funcs);

    return this->funcs[i];
}

int script_object::get_size_instances() const
{
    return ( this->instances == nullptr
            ? 0
            : this->instances->size()
            );
}

int script_object::find_func(string_hash a2) const
{
    TRACE("script_object::find_func", a2.to_string());

    if constexpr (1) {
        const auto v14 = a2.source_hash_code % 20;

        auto idx = v14;
        auto v12 = 0x7FFFFFFF;
        auto lru_index = -1;
        while ( function_cache()[idx].field_8 != -1 ) {
            if ( function_cache()[idx].field_0 == this 
                && function_cache()[idx].field_4 == a2 )
            {
                static int dword_1597B60 {0};
                ++dword_1597B60;
                function_cache()[idx].field_C = usage_counter()++;
                auto v4 = function_cache()[idx].field_8;
                return v4;
            }

            if ( v12 > function_cache()[idx].field_C ) {
                v12 = function_cache()[idx].field_C;
                lru_index = idx;
            }

            if ( (int)++idx >= 20 ) {
                idx = 0;
            }

            if ( idx == v14 ) {
                goto LABEL_12;
            }
        }

        lru_index = idx;
        LABEL_12:

        static int dword_1597B64 {0};
        ++dword_1597B64;
        for ( auto i = 0; i < this->total_funcs; ++i ) {
            auto &v9 = this->funcs[i];
            auto v3 = v9->get_fullname();
            if ( v3 == a2 ) {
                assert(lru_index != -1);
                function_cache()[lru_index].field_0 = this;
                function_cache()[lru_index].field_8 = i;
                function_cache()[lru_index].field_4 = a2;
                function_cache()[lru_index].field_C = usage_counter()++;
                return i;
            }
        }

        return -1;

    } else {
        return THISCALL(0x0058EF80, this, a2);
    }
}

int script_object::find_func_short(string_hash a2) const
{
    for ( int i = 0; i < this->total_funcs; ++i )
    {
        auto v3 = this->funcs[i]->get_name();
        if ( v3 == a2 ) {
            return i;
        }
    }

    return -1;
}

int script_object::find_function_by_address(const uint16_t *a2) const
{
    //TRACE("script_object::find_function_by_address");

    for ( auto i = 0; i < this->total_funcs; ++i )
    {
        auto &v4 = this->funcs[i];
        if ( v4 != nullptr )
        {
            if ( a2 >= v4->get_start() )
            {
                auto *v2 = v4->get_start();
                if ( a2 < v2 + v4->get_size() ) {
                    return i;
                }
            }
        }
    }

    return -1;
}

void vm_symbol::read(chunk_file *file) {
    this->field_0 = file->read<mString>();
    this->field_C = file->read<mString>();

    this->field_30 = file->read<int>();
    this->field_34 = file->read<int>();

    this->field_38 = file->read<bool>();

    this->field_18 = file->read<mString>();
    this->field_24 = file->read<mString>();
}

void script_object::read(chunk_file *file, script_object *so)
{
    TRACE("script_object::load");

    auto *mem = mem_alloc(sizeof(debug_info_t));
    so->debug_info = new (mem) debug_info_t{}; 
    assert(so->debug_info != nullptr);

    assert(so->parent != nullptr);

    chunk_flavor cf = file->read<chunk_flavor>();

    sp_log("so->flags = 0x%08X", so->flags);
    if ( cf == CHUNK_EXTERNAL ) {
        so->flags |= SCRIPT_OBJECT_FLAG_EXTERNAL;
        cf = file->read<chunk_flavor>();
    }

    if ( cf == CHUNK_GLOBAL ) {
        so->flags |= SCRIPT_OBJECT_FLAG_GLOBAL;
    }

    if ( cf == CHUNK_STANDARD ) {
        cf = file->read<chunk_flavor>();
        if ( cf == CHUNK_PARENT )
        {
            auto v39 = file->read<uint32_t>();
            auto *system_string = so->parent->get_system_string(v39);
            so->debug_info->field_0 = string_hash {system_string};
            cf = file->read<chunk_flavor>();
        }
    } else {
        cf = file->read<chunk_flavor>();
    }

    if ( cf == CHUNK_NSTATIC ) {
        auto i = file->read<int>();
        while ( i ) {
            vm_symbol v38{};
            v38.read(file);

            so->debug_info->field_4.push_back(v38);
            --i;
        }

        cf = file->read<chunk_flavor>();
    }

    sp_log("%s", cf.field_0);
    assert(cf == CHUNK_STATIC_BLOCKSIZE);

    so->static_data = file->read<int>();
    cf = file->read<chunk_flavor>();

    while ( cf == CHUNK_STAT_INIT ) {
        auto v36 = file->read<int>();
        auto v35 = file->read<int>();
        auto *buffer = so->static_data.get_buffer();
        auto *v34 = (float *) &buffer[v36];
        if ( v35 != 0 )
        {
            if ( v35 == 1 )
            {
                auto v31 = file->read<float>();
                *v34 = v31;
            }
            else if ( v35 == 2 )
            {
                auto v30 = file->read<int>();
                auto *str = so->parent->get_system_string(v30);
                auto *pso = so->parent->find_object(string_hash {str}, nullptr);
                assert(pso);

                v30 = file->read<int>();
                [[maybe_unused]] auto *v27 = pso->parent->get_system_string(v30);
                auto v26 = 0;
                chunk_flavor v25 {"UNREG"};
                v25 = file->read<chunk_flavor>();
                if ( v25 == CHUNK_PARMS )
                {
                    //v26 = pso->sub_6870D3(string_hash {v27}, file, nullptr);
                }
                else if ( v25 == CHUNK_NULL )
                {
                    //v26 = (string_hash *)pso->add_instance(string_hash {v27}, this, nullptr);
                }
                else if ( v25 == CHUNK_GAME_INIT )
                {
                    //v26 = (string_hash *)pso->sub_68757E(string_hash {v27}, nullptr);
                }
                else
                {
                    assert(0 && "bad sx file");
                }

                *bit_cast<DWORD *>(v34) = v26;
            }
        }
        else
        {
            auto v33 = file->read<int>();
            auto *permanent_string = so->parent->get_permanent_string(v33);
            *(DWORD *)v34 = (int)permanent_string;
        }

        cf = file->read<chunk_flavor>();
    }

    if ( cf == CHUNK_NDATA ) {
        auto i = file->read<int>();
        while ( i != 0 ) {
            vm_symbol v23{};
            v23.read(file);
            so->debug_info->field_10.push_back(v23);
            --i;
        }

        cf = file->read<chunk_flavor>();
    }

    assert(cf == CHUNK_DATA_BLOCKSIZE);

    so->data_blocksize = file->read<int>();
    cf = file->read<chunk_flavor>();
    if ( cf == chunk_flavor {"desidx"} ) {
        so->field_28 = file->read<int>();
        cf = file->read<chunk_flavor>();
    } else {
        so->field_28 = -1;
    }

    assert(cf == CHUNK_FUNCS);
    so->total_funcs = file->read<int>();
    sp_log("so->total_funcs = %d", so->total_funcs);
    if ( so->total_funcs > 0 ) {
        so->funcs = (vm_executable **)operator new(4 * so->total_funcs);
        assert(so->funcs != nullptr);

        for ( auto i = 0; i < so->total_funcs; ++i )
        {
            auto *x = new vm_executable {so};
            assert(x != nullptr);

            vm_executable::read(file, x);
            so->funcs[i] = x;
        }
    }

    if ( so->is_global_object() ) {
        so->create_auto_instance(0.0);
    }
}

script_instance::script_instance(
        string_hash a2,
        int size,
        unsigned int a4) : name(a2),
                            data(size),
                            field_28(nullptr),
                            parent(nullptr),
                            flags(a4)
{
    TRACE("script_instance::script_instance", a2.to_string());
    sp_log("%d", size);
}

script_instance::~script_instance()
{
    TRACE("script_instance::~script_instance");

    this->run_callbacks(static_cast<script_instance_callback_reason_t>(0), nullptr);

    while ( !this->threads.empty() )
    {
        auto *t = &(*this->threads.begin());
        this->threads.common_erase(t);

        t->~vm_thread();
        vm_thread::pool().remove(t);
    }
}

void * script_instance::operator new(size_t size) {
    return mem_alloc(size);
}

void script_instance::operator delete(void *ptr, size_t size) {
    mem_dealloc(ptr, size);
}

bool script_instance::run_single_thread(vm_thread *a2, bool a3)
{
    TRACE("script_instance::run_single_thread");

    if constexpr (0)
    {
        this->flags |= 2u;
        bool v4 = false;
        auto *inst = a2->get_instance();
        auto *so = inst->get_parent();
        auto *parent = so->get_parent();
        auto *entry = script_manager::find_entry(parent);
        assert(entry != nullptr);

        script_manager::run_callbacks(static_cast<script_manager_callback_reason>(10), parent, entry->field_8);
        if ( (a3 || !a2->is_suspended()) && a2->run() )
        {
            auto end = this->threads.end();
            for (auto it = this->threads.begin(); it != end; ++it )
            {
                if ( &(*it) == a2 ) {
                    this->delete_thread(it);
                }
            }

            v4 = true;
        }

        script_manager::run_callbacks(static_cast<script_manager_callback_reason>(11), parent, entry->field_8);
        return v4;
    }
    else
    {
        return THISCALL(0x005AF100, this, a2, a3);
    }
}

void script_instance::register_callback(
    void (*cb)(script_instance_callback_reason_t, script_instance *, vm_thread *, void *),
    void *user_data)
{
    assert(cb != nullptr);

    if constexpr (0)
    {
        this->m_callback = cb;

        decltype(this->field_38)::ret_t ret;

        void (__fastcall *func)(void *, void *edx, decltype(ret) *, void **) = CAST(func, 0x005B50E0);

        func(&this->field_38, nullptr,
           &ret,
           &user_data);

        assert(ret.second && "tried to insert user_data more than once!!!");
    }
    else
    {
        THISCALL(0x005A33F0, this, cb, user_data);
    }
}

vm_thread *script_instance::add_thread(const vm_executable *ex, const char *parms)
{
    TRACE("script_instance::add_thread");

    auto *nt = this->add_thread(ex);
    assert(nt != nullptr);

    if ( parms != nullptr ) {
        auto v5 = ex->get_parms_stacksize();
        nt->get_data_stack().push(parms, v5);
    }

    nt->PC = ex->buffer;
    return nt;
}

void script_instance::add_thread(void *a2, const vm_executable *a3, const char *a4)
{
    if constexpr (0)
    {
        auto *nt = new vm_thread {this, a3, a2};

        assert(nt != nullptr);

        this->threads.emplace_back(nt);

        if ( (this->flags & 1) != 0 ) {
            nt->set_suspended(true);
        }

        if ( a4 != nullptr )
        {
            auto parms_stacksize = a3->get_parms_stacksize();
            auto &data_stack = nt->get_data_stack();

            data_stack.push(a4, parms_stacksize);
        }

        nt->PC = a3->get_start();
    }
    else
    {
        THISCALL(0x005AAD50, this, a2, a3, a4);
    }
}

void script_instance::recursive_massacre_threads(vm_thread *root)
{
    assert(root != nullptr);

    auto it = this->threads.begin();
    auto end = this->threads.end();
    while (it != end) 
    {
        auto *t = &(*it);
        assert(t != nullptr);

        if ( t->field_14 == root )
        {
            assert(t != root);

            this->recursive_massacre_threads(t);
            it = this->delete_thread(it);
        }
        else
        {
            ++it;
        }
    }
}

void script_instance::massacre_threads(const vm_executable *a2, const vm_thread *a3)
{
    TRACE("script_instance::massacre_threads");

    if constexpr (1)
    {
        if ( a2 != nullptr )
        {
            auto it = this->threads.begin();
            auto end = this->threads.end();
            while ( it != end )
            {
                auto *t = &(*it);
                assert(t != nullptr);

                bool v9 = false;
                if ( t != a3 )
                {
                    auto v8 = t->get_executable()->get_name();
                    if ( a2->get_name() == v8 ) {
                        v9 = true;
                    }
                }

                if ( v9 )
                {
                    this->recursive_massacre_threads(t);
                    it = this->delete_thread(it);
                }
                else
                {
                    ++it;
                }
            }
        }
        else
        {
            auto it = this->threads.begin();
            auto end = this->threads.end();
            while (it != end) 
            {
                auto *t = &(*it);
                assert(t != nullptr);

                if ( t == a3 ) {
                    ++it;
                } else {
                    it = this->delete_thread(it);
                }
            }
        }
    } else {
        THISCALL(0x005ADB80, this, a2, a3);
    }
}

void script_instance::kill_thread(const vm_executable *a2, const vm_thread *a3)
{
    TRACE("script_instance::kill_thread");

    if constexpr (1)
    {
        auto it = this->threads.begin();
        auto end = this->threads.end();
        while ( it != end )
        {
            auto *t = &(*it);
            assert(t != nullptr);

            bool v7 = false;
            if ( t->get_instance() == this && t != a3 )
            {
                auto v6 = a2->get_name();
                if ( t->get_executable()->get_name() == v6 ) {
                    v7 = true;
                }
            }

            if ( v7 ) {
                it = this->delete_thread({t});
            } else {
                ++it;
            }
        }
    }
    else
    {
        THISCALL(0x005AD8D0, this, a2, a3);
    }
}

vm_thread *script_instance::add_thread(const vm_executable *a2)
{
    TRACE("script_instance::add_thread");

    if constexpr (1)
    {
        auto *nt = new vm_thread {this, a2};
        assert(nt != nullptr);

        this->threads.emplace_back(nt);

        if ( (this->flags & 1) != 0 ) {
            nt->set_suspended(true);
        }

        return nt;
    }
    else
    {
        vm_thread * (__fastcall *func)(void *, void *edx, const vm_executable *a2) = CAST(func, 0x005AAC20);
        return func(this, nullptr, a2);
    }
}

vm_thread *script_object::add_thread(script_instance *a2, int fidx)
{
	assert(fidx < this->total_funcs);

	auto *t = a2->add_thread(this->funcs[fidx]);
	auto &stack = t->get_data_stack();
	stack.push((const char *)&a2, 4);
	return t;
}

bool script_instance::has_threads() const
{
    TRACE("script_instance::has_threads");

    return (this->threads.size() != 0);
}

// ---------------------------------------------------------------------------
// .PCSX mod overrides (see script_object.h for the contract)
//
// A .pcsx image is the same generic-mash blob a retail pack serves for
// RESOURCE_KEY_TYPE_SCRIPT: 16-byte generic_mash_header, then the
// script_executable object image, then the mashed data with the exec code
// image embedded inside — the blob is fully self-contained (permanent
// strings, script objects, vm_executables and bytecode all travel in it).
// The override rides resource_manager::get_resource, the one funnel
// script_manager::load fetches script blobs through, swapping the byte
// pointer + size before parse_generic_object_mash consumes them.
//
// Retail re-streams a script's pack bytes on every load_world, so a retail
// script image is always pristine when it is parsed. The override has to
// reproduce that, because un_mash/link rewrite the image IN PLACE and bake
// absolute addresses into it: OP_ARG_LFR bakes script_library_class function
// pointers, OP_ARG_CLV bakes find_instance() results and case 17 bakes the
// game/shared var addresses (vm_executable::link_un_mash), while
// SCRIPT_EXECUTABLE_FLAG_LINKED lands in the image's own flags word. All of
// those pointees die with the level (slc_manager::kill, destroy_game_var),
// and script_manager::link() skips any exec whose is_linked() is already
// set -- so re-serving a once-linked image on the next level would run
// bytecode full of freed pointers.
//
// Hence: one buffer per hash, re-stamped from the pristine master bytes on
// every fetch that is not for an already-loaded exec. That makes each load a
// full un_mash + link over virgin bytes, exactly like a pack re-stream, and
// keeps IN_USE clear at parse time so parse_generic_mash_init never takes
// its clone branch (which would break script_manager::load's
// assert(!allocated_mem)). A fetch for a script that IS currently loaded --
// script_manager::is_loadable probing it, or a second load under a different
// context key -- must NOT disturb the bytes the live exec is using, so it
// gets the buffer as-is; that mirrors retail serving the same pack bytes.
// ---------------------------------------------------------------------------

// True while script_manager holds a loaded exec for this script name, i.e.
// while the served image is in use and must not be re-stamped.
static bool modPCSXIsLoaded(uint32_t nameHash)
{
    auto *execs = script_manager::get_exec_list();
    if (execs == nullptr)
        return false;

    for (auto &entry : (*execs))
    {
        if (entry.first.field_0.m_hash.source_hash_code == nameHash)
            return true;
    }

    return false;
}

namespace {

constexpr uint32_t MOD_PS2SX_MARKER = 0xA1A1A1A1u;

struct modPS2SXTranslationStats
{
    unsigned int vmRecords = 0;
    unsigned int globalCalls = 0;
    unsigned int localCalls = 0;
    unsigned int methodOffsets = 0;
};

struct modPS2SXRemapRegion
{
    uint16_t first;
    uint16_t last;
    int delta;
};

// Derived from the beta/PC counterpart corpus used by the local
// ps2sx_to_pcsx translator.  The low ranges are deliberately identity
// regions; indices above the calibrated 0x1F0 ceiling are left untouched.
constexpr modPS2SXRemapRegion MOD_PS2SX_GLOBAL_REGIONS[] = {
    {0x0000, 0x0011,  0}, {0x0012, 0x006B,  4},
    {0x006C, 0x0086,  5}, {0x0087, 0x0095,  6},
    {0x0096, 0x00E1,  4}, {0x00E2, 0x00EC,  5},
    {0x00ED, 0x00EE,  8}, {0x00EF, 0x00FA,  9},
    {0x00FB, 0x00FE, 11}, {0x00FF, 0x011D, 12},
    {0x011E, 0x0129, 13}, {0x012A, 0x0148, 12},
    {0x0149, 0x0152, 13}, {0x0153, 0x0162, 14},
    {0x0163, 0x0191, 15}, {0x0192, 0x01F0, 16},
};

constexpr modPS2SXRemapRegion MOD_PS2SX_LOCAL_REGIONS[] = {
    {0x0000, 0x0030, 0}, {0x0031, 0x004F, 1},
    {0x0050, 0x0054, 2}, {0x0055, 0x006C, 3},
    {0x006D, 0x00B5, 2}, {0x00B6, 0x00BB, 3},
    {0x00BC, 0x00C2, 4}, {0x00C3, 0x00ED, 5},
    {0x00EE, 0x01F0, 6},
};

static std::map<uint32_t, std::string> &modPS2SXNames()
{
    static std::map<uint32_t, std::string> names;
    return names;
}

static std::unordered_set<const void *> &modPS2SXRuntimeImages()
{
    static std::unordered_set<const void *> images;
    return images;
}

static bool modPS2SXReadU16(const uint8_t *bytes, size_t size,
                            size_t offset, uint16_t *value)
{
    if (value == nullptr || offset > size || size - offset < sizeof(*value))
        return false;
    std::memcpy(value, bytes + offset, sizeof(*value));
    return true;
}

static bool modPS2SXReadU32(const uint8_t *bytes, size_t size,
                            size_t offset, uint32_t *value)
{
    if (value == nullptr || offset > size || size - offset < sizeof(*value))
        return false;
    std::memcpy(value, bytes + offset, sizeof(*value));
    return true;
}

static bool modPS2SXReadI32(const uint8_t *bytes, size_t size,
                            size_t offset, int32_t *value)
{
    uint32_t raw = 0;
    if (!modPS2SXReadU32(bytes, size, offset, &raw))
        return false;
    std::memcpy(value, &raw, sizeof(raw));
    return true;
}

static bool modPS2SXWriteU16(std::vector<uint8_t> &bytes, size_t offset,
                             uint16_t value)
{
    if (offset > bytes.size() || bytes.size() - offset < sizeof(value))
        return false;
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
    return true;
}

static bool modPS2SXWriteU32(std::vector<uint8_t> &bytes, size_t offset,
                             uint32_t value)
{
    if (offset > bytes.size() || bytes.size() - offset < sizeof(value))
        return false;
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
    return true;
}

static bool modPS2SXAlignInput(size_t *cursor, size_t alignment, size_t size)
{
    if (cursor == nullptr || alignment == 0 || *cursor > size)
        return false;
    const size_t remainder = *cursor % alignment;
    const size_t padding = remainder == 0 ? 0 : alignment - remainder;
    if (padding > size - *cursor)
        return false;
    *cursor += padding;
    return true;
}

static void modPS2SXAlignOutput(std::vector<uint8_t> &out, size_t alignment)
{
    while ((out.size() % alignment) != 0)
        out.push_back(0xE3);
}

static bool modPS2SXAppend(const uint8_t *bytes, size_t size,
                           size_t *cursor, size_t count,
                           std::vector<uint8_t> &out)
{
    if (cursor == nullptr || *cursor > size || count > size - *cursor)
        return false;
    out.insert(out.end(), bytes + *cursor, bytes + *cursor + count);
    *cursor += count;
    return true;
}

static bool modPS2SXCountBytes(int32_t count, size_t stride, size_t size,
                               size_t *byteCount)
{
    if (byteCount == nullptr || count < 0 || stride == 0)
        return false;
    const size_t n = static_cast<size_t>(count);
    if (n > size / stride)
        return false;
    *byteCount = n * stride;
    return true;
}

static uint16_t modPS2SXRemapIndex(
        uint16_t index, const modPS2SXRemapRegion *regions, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        if (index >= regions[i].first && index <= regions[i].last)
            return static_cast<uint16_t>(index + regions[i].delta);
    }
    return index;
}

static bool modPS2SXOpcodeSupported(uint8_t opcode)
{
    if (opcode <= 16 || opcode == 18 || (opcode >= 20 && opcode <= 35)
        || opcode == 37 || opcode == 38
        || (opcode >= 43 && opcode <= 65))
        return true;
    return false;
}

static int modPS2SXOperandWords(uint8_t argumentType)
{
    switch (argumentType)
    {
    case 0: return 0;
    case 1: case 2: case 3: return 2;
    case 4: case 5: case 6: case 7: return 1;
    case 8: case 9: case 10: case 11:
    case 15: case 16: case 17: return 2;
    default: return -1;
    }
}

static bool modPS2SXTranslateCode(std::vector<uint8_t> &out,
                                  size_t codeStart, size_t codeSize,
                                  modPS2SXTranslationStats *stats,
                                  std::string *reason)
{
    if ((codeSize & 1u) != 0 || codeStart > out.size()
        || codeSize > out.size() - codeStart)
    {
        if (reason != nullptr) *reason = "invalid beta bytecode range";
        return false;
    }

    const size_t codeEnd = codeStart + codeSize;
    size_t cursor = codeStart;
    while (cursor < codeEnd)
    {
        const size_t instruction = cursor;
        uint16_t opword = 0;
        if (!modPS2SXReadU16(out.data(), out.size(), cursor, &opword))
            return false;
        cursor += sizeof(opword);

        const uint8_t opcode = static_cast<uint8_t>(opword >> 8);
        const uint8_t rawArgument = static_cast<uint8_t>(opword & 0xFFu);
        const uint8_t argumentType = rawArgument & 0x7Fu;
        const int operandWords = modPS2SXOperandWords(argumentType);
        if (!modPS2SXOpcodeSupported(opcode) || operandWords < 0)
        {
            if (reason != nullptr)
            {
                char message[96] {};
                std::snprintf(message, sizeof(message),
                              "unsupported opcode/argument 0x%02X/0x%02X at 0x%X",
                              opcode, argumentType,
                              static_cast<unsigned int>(instruction - codeStart));
                *reason = message;
            }
            return false;
        }

        uint16_t dataSize = 4;
        if ((rawArgument & 0x80u) != 0)
        {
            if (!modPS2SXReadU16(out.data(), out.size(), cursor, &dataSize)
                || cursor + sizeof(dataSize) > codeEnd)
                return false;
            cursor += sizeof(dataSize);
        }

        const size_t operandBytes = static_cast<size_t>(operandWords) * 2u;
        if (operandBytes > codeEnd - cursor)
        {
            if (reason != nullptr) *reason = "truncated beta bytecode operand";
            return false;
        }

        // OP_BSL / OP_ARG_LFR stores {SLC group, function index}.  Group 0
        // addresses the global table; group 7 addresses the local table.
        if (opcode == 4 && argumentType == 10 && operandBytes == 4)
        {
            uint16_t group = 0;
            uint16_t index = 0;
            modPS2SXReadU16(out.data(), out.size(), cursor, &group);
            modPS2SXReadU16(out.data(), out.size(), cursor + 2, &index);
            uint16_t remapped = index;
            if (group == 0)
            {
                remapped = modPS2SXRemapIndex(
                        index, MOD_PS2SX_GLOBAL_REGIONS,
                        sizeof(MOD_PS2SX_GLOBAL_REGIONS)
                            / sizeof(MOD_PS2SX_GLOBAL_REGIONS[0]));
                if (remapped != index && stats != nullptr)
                    ++stats->globalCalls;
            }
            else if (group == 7)
            {
                remapped = modPS2SXRemapIndex(
                        index, MOD_PS2SX_LOCAL_REGIONS,
                        sizeof(MOD_PS2SX_LOCAL_REGIONS)
                            / sizeof(MOD_PS2SX_LOCAL_REGIONS[0]));
                if (remapped != index && stats != nullptr)
                    ++stats->localCalls;
            }
            if (remapped != index)
                modPS2SXWriteU16(out, cursor + 2, remapped);
        }

        // Beta OP_PSH/VAR records with dsize 0x0C use a method-table offset
        // four bytes earlier than the PC executable for the calibrated band.
        if (opcode == 29 && rawArgument == 0x91u && dataSize == 0x0Cu
            && operandBytes == 4)
        {
            uint32_t value = 0;
            modPS2SXReadU32(out.data(), out.size(), cursor, &value);
            if (value >= 0x100u && value < 0x300u)
            {
                modPS2SXWriteU32(out, cursor, value + 4u);
                if (stats != nullptr) ++stats->methodOffsets;
            }
        }

        cursor += operandBytes;
    }

    if (cursor != codeEnd)
    {
        if (reason != nullptr) *reason = "beta bytecode does not end cleanly";
        return false;
    }
    return true;
}

static bool modPS2SXAppendVM(const uint8_t *bytes, size_t size,
                             size_t *cursor, uint32_t codeSize,
                             std::vector<uint8_t> &out,
                             modPS2SXTranslationStats *stats,
                             std::string *reason)
{
    if (!modPS2SXAlignInput(cursor, 4, size))
        return false;
    modPS2SXAlignOutput(out, 4);
    if (*cursor > size || size - *cursor < 0x28)
        return false;

    uint32_t debugInfo = 0;
    uint32_t extraWord = 0;
    uint32_t flags = 0;
    uint32_t sentinel = 0;
    uint32_t codeOffset = 0;
    int32_t codeWords = 0;
    modPS2SXReadU32(bytes, size, *cursor + 0x18, &debugInfo);
    modPS2SXReadU32(bytes, size, *cursor + 0x1C, &extraWord);
    modPS2SXReadU32(bytes, size, *cursor + 0x20, &flags);
    modPS2SXReadU32(bytes, size, *cursor + 0x24, &sentinel);
    modPS2SXReadU32(bytes, size, *cursor + 0x10, &codeOffset);
    modPS2SXReadI32(bytes, size, *cursor + 0x14, &codeWords);

    if (debugInfo != 0 || extraWord != 0
        || (flags != VM_EXECUTABLE_FLAG_FROM_MASH
            && flags != (VM_EXECUTABLE_FLAG_FROM_MASH
                         | VM_EXECUTABLE_FLAG_STATIC))
        || sentinel != 0xCDCDCDCDu)
    {
        if (reason != nullptr)
            *reason = "mixed or unsupported PS2 vm_executable record layout";
        return false;
    }
    if (codeWords < 0 || codeOffset > codeSize
        || 2u * static_cast<uint32_t>(codeWords) > codeSize - codeOffset)
    {
        if (reason != nullptr)
            *reason = "PS2 vm_executable code range is outside the bytecode image";
        return false;
    }

    // PS2: common fields, debug ptr, extra zero, flags, CD sentinel (0x28).
    // PC:  common fields, debug ptr, flags,      CD sentinel (0x24).
    out.insert(out.end(), bytes + *cursor, bytes + *cursor + 0x1C);
    out.insert(out.end(), bytes + *cursor + 0x20,
               bytes + *cursor + 0x28);
    *cursor += 0x28;
    if (stats != nullptr) ++stats->vmRecords;
    return true;
}

static bool modPS2SXExtractName(const uint8_t *bytes, size_t size,
                                std::string *name)
{
    if (name == nullptr || size < 0x30)
        return false;
    size_t length = 0;
    while (length < 32 && bytes[0x10 + length] != 0)
    {
        const unsigned char c = bytes[0x10 + length];
        if (c < 0x20 || c > 0x7E)
            return false;
        ++length;
    }
    if (length == 0 || length == 32)
        return false;
    name->assign(reinterpret_cast<const char *>(bytes + 0x10), length);
    return true;
}

static bool modPS2SXConvert(const uint8_t *bytes, size_t size,
                            std::vector<uint8_t> *converted,
                            modPS2SXTranslationStats *stats,
                            std::string *reason)
{
    if (bytes == nullptr || converted == nullptr || size < 0x70
        || size > static_cast<size_t>(std::numeric_limits<int>::max()))
    {
        if (reason != nullptr) *reason = "file is too small or too large";
        return false;
    }

    const auto *header = bit_cast<const generic_mash_header *>(bytes);
    uint32_t marker = 0;
    uint32_t scriptFlags = 0;
    uint32_t codeSize = 0;
    int32_t objectCount = 0;
    int32_t stringCount = 0;
    int32_t infoCount = 0;
    modPS2SXReadU32(bytes, size, 0x6C, &marker);
    modPS2SXReadU32(bytes, size, 0x60, &scriptFlags);
    modPS2SXReadU32(bytes, size, 0x34, &codeSize);
    modPS2SXReadI32(bytes, size, 0x40, &objectCount);
    modPS2SXReadI32(bytes, size, 0x4C, &stringCount);
    modPS2SXReadI32(bytes, size, 0x68, &infoCount);

    if (header->field_4 != 0 || header->field_8 != static_cast<int>(size)
        || header->class_id != 0xFFFF || header->field_E != 0
        || header->safety_key != header->generate_safety_key()
        || marker != MOD_PS2SX_MARKER
        || scriptFlags != script_executable::SCRIPT_EXECUTABLE_FLAG_FROM_MASH
        || codeSize > size - 0x70)
    {
        if (reason != nullptr) *reason = "invalid PS2SX mash header or marker";
        return false;
    }

    static constexpr size_t POINTER_FIELDS[] = {
        0x30, 0x38, 0x3C, 0x44, 0x48, 0x50, 0x58, 0x5C, 0x64
    };
    for (const size_t offset : POINTER_FIELDS)
    {
        uint32_t value = 0;
        if (!modPS2SXReadU32(bytes, size, offset, &value) || value != 0)
        {
            if (reason != nullptr)
                *reason = "PS2SX is a live/dumped image instead of virgin pack data";
            return false;
        }
    }

    size_t objectPointerBytes = 0;
    size_t stringPointerBytes = 0;
    size_t infoBytes = 0;
    if (!modPS2SXCountBytes(objectCount, sizeof(uint32_t), size,
                            &objectPointerBytes)
        || !modPS2SXCountBytes(stringCount, sizeof(uint32_t), size,
                               &stringPointerBytes)
        || !modPS2SXCountBytes(infoCount, sizeof(script_executable::info_t),
                               size, &infoBytes))
    {
        if (reason != nullptr) *reason = "invalid PS2SX object/string counts";
        return false;
    }

    converted->clear();
    converted->reserve(size);
    converted->insert(converted->end(), bytes, bytes + 0x6C);
    converted->insert(converted->end(), bytes + 0x70,
                      bytes + 0x70 + codeSize);
    if (!modPS2SXTranslateCode(*converted, 0x6C, codeSize, stats, reason))
        return false;

    size_t cursor = 0x70 + static_cast<size_t>(codeSize);
    if (!modPS2SXAlignInput(&cursor, 4, size))
        return false;
    modPS2SXAlignOutput(*converted, 4);
    if (!modPS2SXAppend(bytes, size, &cursor, objectPointerBytes, *converted))
        return false;

    for (int32_t objectIndex = 0; objectIndex < objectCount; ++objectIndex)
    {
        if (!modPS2SXAlignInput(&cursor, 8, size))
            return false;
        modPS2SXAlignOutput(*converted, 8);
        const size_t objectOffset = cursor;
        if (!modPS2SXAppend(bytes, size, &cursor, sizeof(script_object),
                            *converted))
            return false;

        int32_t staticDataSize = 0;
        int32_t functionCount = 0;
        if (!modPS2SXReadI32(bytes, size, objectOffset + 0x10,
                             &staticDataSize)
            || !modPS2SXReadI32(bytes, size, objectOffset + 0x24,
                                &functionCount)
            || staticDataSize < 0)
        {
            if (reason != nullptr) *reason = "invalid PS2 script_object record";
            return false;
        }

        size_t functionPointerBytes = 0;
        if (!modPS2SXCountBytes(functionCount, sizeof(uint32_t), size,
                                &functionPointerBytes))
            return false;

        if (!modPS2SXAlignInput(&cursor, 4, size))
            return false;
        modPS2SXAlignOutput(*converted, 4);
        if (!modPS2SXAppend(bytes, size, &cursor,
                            static_cast<size_t>(staticDataSize), *converted))
            return false;

        if (!modPS2SXAlignInput(&cursor, 4, size))
            return false;
        modPS2SXAlignOutput(*converted, 4);
        if (!modPS2SXAppend(bytes, size, &cursor, functionPointerBytes,
                            *converted))
            return false;

        for (int32_t function = 0; function < functionCount; ++function)
        {
            if (!modPS2SXAppendVM(bytes, size, &cursor, codeSize,
                                  *converted, stats, reason))
                return false;
        }
    }

    if (!modPS2SXAlignInput(&cursor, 4, size))
        return false;
    modPS2SXAlignOutput(*converted, 4);
    if (!modPS2SXAppend(bytes, size, &cursor, objectPointerBytes, *converted))
        return false;

    if (!modPS2SXAlignInput(&cursor, 4, size))
        return false;
    modPS2SXAlignOutput(*converted, 4);
    if (!modPS2SXAppend(bytes, size, &cursor, stringPointerBytes, *converted))
        return false;

    for (int32_t stringIndex = 0; stringIndex < stringCount; ++stringIndex)
    {
        if (!modPS2SXAlignInput(&cursor, 4, size))
            return false;
        modPS2SXAlignOutput(*converted, 4);
        uint32_t length = 0;
        if (!modPS2SXReadU32(bytes, size, cursor, &length)
            || length > size - cursor - sizeof(length)
            || !modPS2SXAppend(bytes, size, &cursor,
                               sizeof(length) + static_cast<size_t>(length),
                               *converted))
            return false;
    }

    if (!modPS2SXAlignInput(&cursor, 4, size))
        return false;
    modPS2SXAlignOutput(*converted, 4);
    if (!modPS2SXAlignInput(&cursor, 4, size))
        return false;
    modPS2SXAlignOutput(*converted, 4);

    const size_t infoOffset = cursor;
    if (!modPS2SXAppend(bytes, size, &cursor, infoBytes, *converted))
        return false;
    if (!modPS2SXAlignInput(&cursor, 4, size))
        return false;
    modPS2SXAlignOutput(*converted, 4);

    for (int32_t infoIndex = 0; infoIndex < infoCount; ++infoIndex)
    {
        int32_t kind = 0;
        if (!modPS2SXReadI32(bytes, size,
                             infoOffset
                                 + static_cast<size_t>(infoIndex)
                                       * sizeof(script_executable::info_t)
                                 + 0x10,
                             &kind))
            return false;
        if (kind == -1
            && !modPS2SXAppendVM(bytes, size, &cursor, codeSize,
                                 *converted, stats, reason))
            return false;
    }

    // Regular beta mashes end here with at most alignment/fill bytes.  The
    // known QUEENS_COMBAT_TOUR2 anomaly fails earlier because it mixes the
    // compact and expanded VM layouts and advertises more info records than
    // physically exist; rejecting it is safer than manufacturing pointers.
    if (cursor > size || size - cursor > 12)
    {
        if (reason != nullptr) *reason = "unexpected PS2SX tail data";
        return false;
    }

    // Retail pack payloads end on a 16-byte boundary.  The parser ignores
    // this final fill, but retaining the allocator contract keeps field_8
    // and any subsequent shared-data pointer naturally aligned.
    modPS2SXAlignOutput(*converted, 16);
    if (converted->size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        return false;
    auto *pcHeader = bit_cast<generic_mash_header *>(converted->data());
    pcHeader->field_8 = static_cast<int>(converted->size());
    pcHeader->safety_key = pcHeader->generate_safety_key();
    return true;
}

} // namespace

// Some loose PCSX extractors write the total file size into generic_mash_header::field_8
// but leave the original pack safety key untouched.  In a valid mash field_8 is the
// offset of the shared-data area, not the file length.  A strict safety-key check would
// therefore reject an otherwise pristine pack-produced script before it can be used as
// an external-only resource.  Recover the authenticated offset algebraically, but only
// for this narrow stale-header shape (field_8 == file size) and only when every other
// script-mash invariant still matches.  This keeps malformed/runtime-dumped images out.
static bool modPCSXNormalizeLooseMashHeader(std::vector<uint8_t> &bytes,
                                            const std::filesystem::path &path)
{
    if (bytes.size() < sizeof(generic_mash_header) + sizeof(script_executable))
        return false;

    auto *header = bit_cast<generic_mash_header *>(bytes.data());
    if (header->safety_key == header->generate_safety_key())
        return true;

    constexpr uint32_t kMask = 0x0FFFFFFFu;
    constexpr uint32_t kSafetyBase = 0x7BADBA5Du;

    // Generic mash safety keys always live in the 0x7xxxxxxx range.  Restrict
    // repair to ordinary script mashes and the extractor-specific stale-size case.
    if ((header->safety_key & 0xF0000000u) != 0x70000000u
        || header->class_id != 0xFFFFu
        || header->is_flagged(0x40000000u)
        || header->field_8 != static_cast<int>(bytes.size()))
        return false;

    const uint32_t wantedLow = header->safety_key & kMask;
    const uint32_t recovered =
        (wantedLow
         - (kSafetyBase & kMask)
         + (header->field_4 & kMask)
         - static_cast<uint32_t>(header->class_id)
         - static_cast<uint32_t>(header->field_E)) & kMask;

    // Shared mash data must begin inside the image and remain naturally aligned.
    if (recovered < sizeof(generic_mash_header)
        || recovered >= bytes.size()
        || (recovered & 3u) != 0u)
        return false;

    const int staleOffset = header->field_8;
    header->field_8 = static_cast<int>(recovered);
    if (header->safety_key != header->generate_safety_key())
    {
        header->field_8 = staleOffset;
        return false;
    }

    sp_log("[mod] pcsx \"%s\": repaired extracted mash header shared-data offset "
           "0x%X -> 0x%X using its safety key",
           path.filename().string().c_str(), staleOffset, recovered);
    return true;
}

bool modPCSXImageUsable(const uint8_t *bytes, size_t size)
{
    if (bytes == nullptr ||
        size < sizeof(generic_mash_header) + sizeof(script_executable))
        return false;

    // PS2SX starts its actual code after a platform marker and uses wider VM
    // records.  It can satisfy the generic header checks below but must first
    // pass through modPS2SXRegister's structural translator.
    uint32_t platformMarker = 0;
    if (size >= 0x70
        && modPS2SXReadU32(bytes, size, 0x6C, &platformMarker)
        && platformMarker == MOD_PS2SX_MARKER)
        return false;

    const auto *header = bit_cast<const generic_mash_header *>(bytes);

    // the header authenticates itself; IN_USE (0x80000000) and the vtable
    // flag (0x40000000) sit outside the checksummed low 28 bits
    if (header->safety_key != header->generate_safety_key())
        return false;

    // script images are plain object mashes: no vtable word, class_id
    // 0xFFFF. mash_was_allocated/release_generic_mash (entity_mash.cpp)
    // demand exactly this shape on every un_load, so anything else would
    // blow up later even if it parsed now.
    if (header->is_flagged(0x40000000) || header->class_id != 0xFFFF)
        return false;

    // shared mash data lives at header + field_8, inside the image
    if (header->field_8 < (int)sizeof(generic_mash_header) ||
        (size_t)header->field_8 > size)
        return false;

    // The script_executable object image starts right after the header. Its
    // flags word must be virgin: an image dumped out of a running process
    // carries UN_MASHED (and LINKED), which would send the very first load
    // down script_executable::quick_un_mash over pointer fields still
    // holding the donor process's absolute addresses -- and the per-function
    // VM_EXECUTABLE_FLAG_UN_MASHED bits buried in the mash data would
    // likewise defeat vm_executable::un_mash. Only packer output is
    // supportable, so reject the rest here rather than crash later.
    const auto *exec = bit_cast<const script_executable *>(
            bytes + sizeof(generic_mash_header));
    if ((exec->flags & (script_executable::SCRIPT_EXECUTABLE_FLAG_UN_MASHED
                        | script_executable::SCRIPT_EXECUTABLE_FLAG_LINKED)) != 0)
        return false;

    return true;
}

// ---------------------------------------------------------------------------
// Chunk-format .PCSX (the form a file on disk actually has)
//
// chunk_file derives from text_file, so a compiler-emitted .pcsx is a TEXT
// file whose first whitespace-delimited token is "scrobjs"
// (CHUNK_SCRIPT_OBJECTS). It is not self-contained: script_executable::load
// also needs the string tables and the executable code image beside it --
// <name>.pcsst, <name>.pcpst and <name>.pcsxl -- and asserts on any that are
// missing. So all four are required before a drop is accepted, and the
// engine's own loader does the parsing (see script_manager::load, which
// routes these keys down the old-fashioned path with Mod::Path's directory).
// ---------------------------------------------------------------------------

bool modPCSXIsChunkImage(const uint8_t *bytes, size_t size)
{
    if (bytes == nullptr)
        return false;

    // First non-whitespace token, bounded by chunk_flavor's buffer.
    size_t i = 0;
    while (i < size && std::isspace((unsigned char)bytes[i]))
        ++i;

    size_t n = 0;
    char token[CHUNK_FLAVOR_SIZE] {};
    while (i < size && n < sizeof(token) - 1 &&
           !std::isspace((unsigned char)bytes[i]))
        token[n++] = (char)bytes[i++];

    return chunk_flavor {token} == CHUNK_SCRIPT_OBJECTS;
}

// Where a chunk-format .pcsx for this script lives: the directory (with
// trailing separator) script_executable::load must use instead of "scripts\",
// and the file's stem. The stem matters because load() would otherwise derive
// the filename from string_hash::to_string(), which falls back to a synthetic
// 12-char rendering for any hash the engine's string table does not know --
// exactly the case for a brand-new script that exists only as a mod drop.
bool modPCSXGetChunkDir(uint32_t nameHash, std::string *dirOut,
                        std::string *stemOut)
{
    const Mod *mod = getMod(nameHash, MOD_TYPE_PCSX_CHUNK);
    if (mod == nullptr)
        return false;

    if (dirOut != nullptr)
    {
        std::string dir = mod->Path.parent_path().string();
        if (!dir.empty() && dir.back() != '\\' && dir.back() != '/')
            dir += '\\';
        *dirOut = dir;
    }

    if (stemOut != nullptr)
        *stemOut = mod->Path.stem().string();

    return true;
}

// Accept a chunk-format drop only with its whole set of siblings present.
static bool modPCSXRegisterChunk(const std::filesystem::path &path,
                                 const std::string &stem)
{
    static const char *kSiblings[] = { ".pcsst", ".pcpst", ".pcsxl" };

    std::error_code ec;
    for (const char *ext : kSiblings)
    {
        std::filesystem::path sibling = path;
        sibling.replace_extension(ext);
        if (!std::filesystem::is_regular_file(sibling, ec))
        {
            sp_log("[mod] pcsx \"%s\": chunk-format script is missing its "
                   "\"%s\" companion - .pcsx, .pcsst, .pcpst and .pcsxl must "
                   "all be dropped together, ignored",
                   path.filename().string().c_str(), ext);
            return false;
        }
    }

    const uint32_t hash = to_hash(stem.c_str());

    // Re-registration (enumerate_mods() reruns): replace, don't stack.
    {
        auto range = Mods.equal_range(hash);
        for (auto it = range.first; it != range.second; )
            it = (it->second.Type == MOD_TYPE_PCSX_CHUNK) ? Mods.erase(it)
                                                          : std::next(it);
    }

    sp_log("[mod] registered chunk-format pcsx script \"%s\" -> \"%s\" "
           "(key 0x%08X, dir \"%s\")",
           path.filename().string().c_str(), stem.c_str(), hash,
           path.parent_path().string().c_str());

    // No Data: the engine reads all four files off disk itself. Keeping the
    // vector empty is also what stops modPCSXGetOverride from ever handing
    // these bytes to the mash parser.
    Mods.emplace(hash, Mod{path, MOD_TYPE_PCSX_CHUNK, {}});

    return true;
}

bool modPCSXRegister(const std::filesystem::path &path,
                     std::vector<uint8_t> &&fileData)
{
    const std::string stem = transformToLower(path.stem().string());

    // Two on-disk forms share the .pcsx extension: the compiler's text chunk
    // format, and a mash image extracted from a pack. Tell them apart by
    // content rather than trusting the name.
    if (modPCSXIsChunkImage(fileData.data(), fileData.size()))
        return modPCSXRegisterChunk(path, stem);

    // Accept pristine pack mashes directly.  For loose files extracted by tools that
    // replaced field_8 with the total byte count, restore the authenticated shared-data
    // offset before validation.  This is what allows a standalone .PCSX to be used even
    // when no RESOURCE_KEY_TYPE_SCRIPT entry exists in any loaded PCPACK.
    modPCSXNormalizeLooseMashHeader(fileData, path);

    if (!modPCSXImageUsable(fileData.data(), fileData.size()))
    {
        sp_log("[mod] pcsx \"%s\": neither a \"scrobjs\" chunk script nor a "
               "packer-produced virgin mash image, ignored",
               path.filename().string().c_str());
        return false;
    }

    // A dumped-from-memory image may carry a live IN_USE flag; the flag is
    // outside the checksummed bits, so clearing it keeps the header valid.
    auto *header = bit_cast<generic_mash_header *>(fileData.data());
    header->field_4 &= ~_MASH_FLAG_IN_USE;

    const uint32_t hash = to_hash(stem.c_str());

    // Re-registration (enumerate_mods() reruns): replace, don't stack.
    {
        auto range = Mods.equal_range(hash);
        for (auto it = range.first; it != range.second; )
            it = (it->second.Type == MOD_TYPE_PCSX_FILE) ? Mods.erase(it) : std::next(it);
    }

    sp_log("[mod] registered pcsx override \"%s\" -> \"%s\" (%u bytes, key 0x%08X)",
           path.filename().string().c_str(), stem.c_str(),
           (unsigned)fileData.size(), hash);

    Mods.emplace(hash, Mod{path, MOD_TYPE_PCSX_FILE, std::move(fileData)});

    // Hash-named drops ("extra/0x1189AB87.pcsx") also bind under the literal
    // value, mirroring the mesh/texture/wav/ent stem convention.
    if (uint32_t literal = 0;
        modParseLiteralHash(stem, &literal) && literal != hash)
    {
        const Mod *just = getMod(hash, MOD_TYPE_PCSX_FILE);
        if (just != nullptr)
        {
            Mods.emplace(literal, Mod{just->Path, MOD_TYPE_PCSX_FILE, just->Data});
            sp_log("[mod] pcsx \"%s\" also bound as literal hash 0x%08X",
                   stem.c_str(), literal);
        }
    }

    return true;
}

bool modPS2SXRegister(const std::filesystem::path &path,
                      std::vector<uint8_t> &&fileData)
{
    std::string embeddedName;
    if (!modPS2SXExtractName(fileData.data(), fileData.size(), &embeddedName))
    {
        sp_log("[mod] ps2sx \"%s\": embedded script name is invalid, ignored",
               path.filename().string().c_str());
        return false;
    }

    const uint32_t embeddedHash = to_hash(embeddedName.c_str());
    const std::string stem = transformToLower(path.stem().string());
    uint32_t filenameHash = 0;
    const bool literalStem = modParseLiteralHash(stem, &filenameHash);
    if ((!literalStem && to_hash(stem.c_str()) != embeddedHash)
        || (literalStem && filenameHash != embeddedHash))
    {
        sp_log("[mod] ps2sx \"%s\": filename key does not match embedded "
               "script \"%s\" (0x%08X), ignored",
               path.filename().string().c_str(), embeddedName.c_str(),
               embeddedHash);
        return false;
    }

    modPS2SXTranslationStats stats {};
    std::vector<uint8_t> converted;
    std::string reason;
    if (!modPS2SXConvert(fileData.data(), fileData.size(), &converted,
                         &stats, &reason)
        || !modPCSXImageUsable(converted.data(), converted.size()))
    {
        sp_log("[mod] ps2sx \"%s\" (%s, 0x%08X): %s, ignored",
               path.filename().string().c_str(), embeddedName.c_str(),
               embeddedHash,
               reason.empty() ? "translated image failed PC mash validation"
                              : reason.c_str());
        return false;
    }

    auto &names = modPS2SXNames();
    const auto oldName = names.find(embeddedHash);
    if (oldName != names.end()
        && transformToLower(oldName->second)
               != transformToLower(embeddedName))
    {
        sp_log("[mod] ps2sx hash collision 0x%08X: \"%s\" vs \"%s\", "
               "new file ignored", embeddedHash, oldName->second.c_str(),
               embeddedName.c_str());
        return false;
    }

    if (const Mod *existing = getMod(embeddedHash, MOD_TYPE_PS2SX_FILE))
    {
        if (existing->Data == converted)
        {
            sp_log("[mod] ps2sx \"%s\": byte-identical duplicate of \"%s\", "
                   "ignored", path.filename().string().c_str(),
                   existing->Path.string().c_str());
            return true;
        }

        sp_log("[mod] ps2sx \"%s\": conflicts with \"%s\" for script "
               "\"%s\" (0x%08X), first file retained",
               path.filename().string().c_str(), existing->Path.string().c_str(),
               embeddedName.c_str(), embeddedHash);
        return false;
    }
    names[embeddedHash] = embeddedName;

    // Re-enumeration and byte-identical copies from several PS2 packs should
    // leave one deterministic override entry, not a chain of aliases.
    auto range = Mods.equal_range(embeddedHash);
    for (auto it = range.first; it != range.second; )
        it = it->second.Type == MOD_TYPE_PS2SX_FILE ? Mods.erase(it)
                                                    : std::next(it);

    sp_log("[mod] registered ps2sx \"%s\" -> \"%s\" (0x%08X): "
           "%u -> %u bytes, %u VM records, remapped %u global/%u local "
           "calls and %u method offsets",
           path.filename().string().c_str(), embeddedName.c_str(),
           embeddedHash, static_cast<unsigned int>(fileData.size()),
           static_cast<unsigned int>(converted.size()), stats.vmRecords,
           stats.globalCalls, stats.localCalls, stats.methodOffsets);

    Mods.emplace(embeddedHash,
                 Mod{path, MOD_TYPE_PS2SX_FILE, std::move(converted)});
    return true;
}

bool modPS2SXHashToString(uint32_t nameHash, std::string *nameOut)
{
    const auto found = modPS2SXNames().find(nameHash);
    if (found == modPS2SXNames().end())
        return false;
    if (nameOut != nullptr)
        *nameOut = found->second;
    return true;
}

const char *modPS2SXHashName(uint32_t nameHash)
{
    const auto found = modPS2SXNames().find(nameHash);
    return found == modPS2SXNames().end() ? nullptr
                                          : found->second.c_str();
}

void modPS2SXResetRegistry()
{
    modPS2SXNames().clear();
}

bool modPS2SXOverrideSelected(uint32_t nameHash)
{
    return getMod(nameHash, MOD_TYPE_PCSX_FILE) == nullptr
           && getMod(nameHash, MOD_TYPE_PS2SX_FILE) != nullptr;
}

bool modPS2SXIsRuntimeImage(const void *image)
{
    return image != nullptr
           && modPS2SXRuntimeImages().count(image) != 0;
}

uint8_t *modPCSXGetOverride(uint32_t nameHash, int *sizeOut)
{
    Mod *mod = getMod(nameHash, MOD_TYPE_PCSX_FILE);
    bool fromPS2SX = false;
    if (mod == nullptr)
    {
        mod = getMod(nameHash, MOD_TYPE_PS2SX_FILE);
        fromPS2SX = mod != nullptr;
    }
    if (mod == nullptr || mod->Data.empty())
        return nullptr;

    // enumerate_mods runs before the engine creates its hash dictionary.
    // Register the embedded name lazily now, when resource loading is live,
    // so string_hash::to_string resolves 0x1189AB87 as ULTIMATE_SPIDERMAN
    // instead of returning the synthetic hexadecimal fallback.
    if (fromPS2SX)
    {
        std::string embeddedName;
        if (modPS2SXHashToString(nameHash, &embeddedName))
        {
            const string_hash registeredName {embeddedName.c_str()};
            (void)registeredName;
        }
    }

    // One 16-aligned (mash images are laid out against a 16-byte base; parse
    // rebases are 4/8-byte) writable buffer per mod image. The bytes in Mods
    // stay pristine as the master copy and are re-stamped over the buffer
    // whenever no loaded exec is using it, so every load un_mashes and links
    // virgin bytes -- see the block comment above for why re-serving a
    // once-linked image would run freed pointers.
    struct pcsxImage { const Mod *source; void *buffer; size_t size; };
    static std::unordered_map<uint32_t, pcsxImage> s_images;

    auto &slot = s_images[nameHash];

    // Re-registration (enumerate_mods() reruns) can hand us a different-sized
    // image; a still-loaded exec keeps pointers into the old buffer, so that
    // one is abandoned rather than freed or resized.
    if (slot.buffer == nullptr || slot.source != mod ||
        slot.size != mod->Data.size())
    {
        void *buffer = arch_memalign(16u, mod->Data.size());
        if (buffer == nullptr)
            return nullptr;
        slot.source = mod;
        slot.buffer = buffer;
        slot.size = mod->Data.size();
        std::memcpy(slot.buffer, mod->Data.data(), slot.size);
        if (fromPS2SX)
            modPS2SXRuntimeImages().insert(slot.buffer);
    }
    else if (!modPCSXIsLoaded(nameHash))
    {
        std::memcpy(slot.buffer, mod->Data.data(), slot.size);
    }

    if (sizeOut != nullptr)
        *sizeOut = (int)slot.size;
    return static_cast<uint8_t *>(slot.buffer);
}

void script_instance_patch()
{
    {
        FUNC_ADDRESS(address, &script_instance::run);
        SET_JUMP(0x005AF660, address);
    }

    {
        FUNC_ADDRESS(address, &script_object::create_auto_instance);
        SET_JUMP(0x005AAEF0, address);
    }

    {
        FUNC_ADDRESS(address, &script_object::find_func);
        SET_JUMP(0x0058EF80, address);
    }
}
