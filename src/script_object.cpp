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

bool modPCSXImageUsable(const uint8_t *bytes, size_t size)
{
    if (bytes == nullptr ||
        size < sizeof(generic_mash_header) + sizeof(script_executable))
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

uint8_t *modPCSXGetOverride(uint32_t nameHash, int *sizeOut)
{
    Mod *mod = getMod(nameHash, MOD_TYPE_PCSX_FILE);
    if (mod == nullptr || mod->Data.empty())
        return nullptr;

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
