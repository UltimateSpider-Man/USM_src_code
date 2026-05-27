#pragma once

#include "mstring.h"
#include "float.hpp"


#include <algorithm>
#include <cassert>
#include <string>

#include <cstdint>
#include <windows.h>

constexpr auto EXTEND_NEW_ENTRIES = 20;

constexpr auto MAX_CHARS_SAFE = 63;
constexpr auto MAX_CHARS = MAX_CHARS_SAFE + 1;

enum debug_menu_entry_type {
    dUNDEFINED = 0,
    NORMAL,
    FLOAT_E,
    POINTER_FLOAT,
    INTEGER,
    POINTER_INT,
    BOOLEAN_E,
    POINTER_BOOL,
    POINTER_MENU,
    BOOLEAN_NUM
};



// Add this to global scope or debug_menu namespace:


enum menu_type {
DEBUG_MENU_ENTRY_TYPE_FLOAT = 4
};

				

extern const char *to_string(debug_menu_entry_type entry_type);

enum custom_key_type {
	LEFT,
	RIGHT,
	ENTER
};

struct debug_menu_entry;

extern void entry_frame_advance_callback_default(debug_menu_entry *a1);

struct debug_menu;

extern std::string entry_render_callback_default(debug_menu_entry* entry);

struct script_instance;

struct additional_handler_info {
    script_instance* instance;
    int func_index;
    void* handler_func;
};
struct debug_menu_entry {
	char text[MAX_CHARS];
	debug_menu_entry_type entry_type;
	menu_type type;
	    union {
        bool* fptr;
		bool* bptr;
		bool bval;
    } contents;
    union {
        float fval;
        float *p_fval;
        bool bval;
        bool *p_bval;
        int ival;
        int *p_ival;
        debug_menu *p_menu;
    } m_value;
	void* data;
	void* data1;
    uint16_t m_id {0};

    std::string (*render_callback)(debug_menu_entry *) = entry_render_callback_default;
    void (*m_game_flags_handler)(debug_menu_entry *) = nullptr;
    void (*frame_advance_callback)(debug_menu_entry *) = entry_frame_advance_callback_default;
    script_instance *field_14;
    int field_18;
    struct {
        float m_min_value;
        float m_max_value;
        float m_step_size;
        float m_step_scale;
    } field_20 {0.f, 1.f, 0.1f, 10.f};
    bool m_value_initialized {false};
    void *m_data = nullptr;
	mString m_name;
        bool m_checked{false};		
		void (*m_script_handler)(debug_menu_entry*,script_instance* a3, const mString* a4) = nullptr;

        void set_checked(bool state) { m_checked = state; }
    bool is_checked() const { return m_checked; }
	    debug_menu_entry *prev;
    debug_menu_entry *next;
		const char* m_name2;


const char* get_name() const {
    return this->m_name2;
}


    void set_step_size(float a2)
    {
        this->field_20.m_step_size = a2;
    }

    void set_step_scale(float a2)
    {
        this->field_20.m_step_scale = a2;
    }

    void set_data(void *a2)
    {
        this->m_data = a2;
    }

    void * get_data()
    {
        return this->m_data;
    }
	
 bool set_script_handler_from_char2(script_instance *inst, const char* a3,const char* a4,const char* a5,const char* a6,const char* a7,const char* a8,const char* a9);


 bool set_script_handler_from_char(script_instance *inst, const char* a3);

    std::string get_script_handler()
    {
        return std::string {this->text};
    }

    void set_id(int id)
    {
        m_id = id;
    }

    auto get_id() const
    {
        return m_id;
    }

    void set_frame_advance_cb(void (*a2)(debug_menu_entry *))
    {
        this->frame_advance_callback = a2;
    }

    void set_submenu(debug_menu *submenu);
	
	    void set_submenu2(debug_menu *submenu);

    void on_select(float a2);

    bool set_script_handler(script_instance *inst, const mString &a3);

    debug_menu *remove_menu();

    void on_change(float a3, bool a4);

    void set_fval(float a2, bool a3)
    {
        if ( !this->is_value_initialized() )
        {
            if ( a2 > this->field_20.m_max_value )
            {
                a2 = this->field_20.m_max_value;
            }

            if ( this->field_20.m_min_value > a2 )
            {
                a2 = this->field_20.m_min_value;
            }

            auto v3 = this->entry_type;
            if ( v3 == FLOAT_E )
            {
                this->m_value.fval = a2;
            }
            else if ( v3 == POINTER_FLOAT )
            {
                *this->m_value.p_fval = a2;
            }
            else
            {
                assert(0);
            }

            if ( this->m_game_flags_handler != nullptr && a3 )
            {
                this->m_game_flags_handler(this);
            }
        }

        this->get_fval();
    }

    bool get_bval2() const
    {
        auto v2 = this->entry_type;
        if (v2 == BOOLEAN_NUM)
        {
            return this->m_value.bval;
        }

        assert(0);
        return 0;
    }

        bool set_bval2(bool a2, bool a3)
    {
        if (!this->is_value_initialized()) {
            auto v4 = this->entry_type;
            if (v4 == BOOLEAN_NUM) {
                this->m_value.bval = a2;
            } else {

                assert(0);
            }

            if (this->m_game_flags_handler != nullptr && a3) {
                this->m_game_flags_handler(this);
            }
        }

        return this->get_bval2();
    }

    float get_fval()
    {
        auto v2 = this->entry_type;
        if (v2 == FLOAT_E) {
            return this->m_value.fval;
        }

        if (v2 == POINTER_FLOAT) {
            return *this->m_value.p_fval;
        }

        assert(0);
        return 0.0;
    }

    bool get_bval() const
    {
        auto v2 = this->entry_type;
        if ( v2 == BOOLEAN_E )
        {
            return this->m_value.bval;
        }

        if ( v2 == POINTER_BOOL )
        {
            return *this->m_value.p_bval;
        }

        assert(0);
        return 0;
    }

    int get_ival()
    {
        auto v2 = this->entry_type;
        if ( v2 == INTEGER )
        {
            return this->m_value.ival;
        }

        if ( v2 == POINTER_INT )
        {
            return *this->m_value.p_ival;
        }

        assert(0);
        return 0;
    }

    bool is_value_initialized() const
    {
        return this->m_value_initialized;
    }

    void set_value_initialized(bool a2)
    {
        m_value_initialized = a2;
    }

    int set_ival(int a2, bool a3)
    {
        printf("debug_menu_entry::set_ival: a2 = %d\n", a2);

        if ( !this->is_value_initialized() )
        {
            if ( a2 > this->field_20.m_max_value ) {
                a2 = this->field_20.m_max_value;
            }

            if ( this->field_20.m_min_value > a2 ) {
                a2 = this->field_20.m_min_value;
            }

            auto v4 = this->entry_type;
            if ( v4 == INTEGER )
            {
                this->m_value.ival = a2;
            }
            else if ( v4 == POINTER_INT )
            {
                *this->m_value.p_ival = a2;
            }
            else
            {
                assert(0);
            }

            if ( this->m_game_flags_handler != nullptr && a3 )
            {
                this->m_game_flags_handler(this);
            }
        }

        return this->get_ival();
    }

    void set_p_ival(int *a2)
    {
        this->entry_type = POINTER_INT;
        this->m_value.p_ival= a2;
    }

    void set_pt_fval(float *a2)
    {
        this->entry_type = POINTER_FLOAT;
        this->m_value.p_fval = a2;
    }

    void set_min_value(float a2)
    {
        this->field_20.m_min_value = a2;
    }

    void set_max_value(float a2)
    {
        this->field_20.m_max_value = a2;
    }
	
	void set_type() {
    m_value.bval = false;  // Initialize to default value
}
	
	    void set_float_value(float a2)
    {
        this->field_20.m_max_value = a2;

    }
	
	int render(int a1, int a2, bool a3);

    void set_bval(bool a2)
    {
        this->entry_type = BOOLEAN_E;
        this->m_value.bval = a2;
		
    }
    bool set_bval2(bool a2)
    {
        this->entry_type = BOOLEAN_NUM;
        this->m_value.bval = a2;
    }

    bool set_bval(bool a2, bool a3)
    {
        if ( !this->is_value_initialized() )
        {
            auto v4 = this->entry_type;
            if ( v4 == BOOLEAN_E )
            {
                this->m_value.bval = a2;
            }
            else if ( v4 == POINTER_BOOL )
            {
                *this->m_value.p_bval = a2;
            }
            else
            {
                assert(0);
            }

            if ( this->m_game_flags_handler != nullptr && a3 )
            {
                this->m_game_flags_handler(this);
            }
        }

        return this->get_bval();
    }

    void set_pt_bval(bool *a2)
    {
        this->entry_type = POINTER_BOOL;
        this->m_value.p_bval = a2;
    }

    void set_ival(int a2)
    {
        this->entry_type = INTEGER;
        this->m_value.ival= a2;
    }

    void set_fl_values(const float *a2)
    {
        auto &v2 = this->field_20;
        v2.m_min_value = a2[0];
        v2.m_max_value = a2[1];
        v2.m_step_size = a2[2];
        v2.m_step_scale = a2[3];
    }

      debug_menu_entry* alloc_block(debug_menu* m, std::size_t n);

    void set_game_flags_handler(void (*a2)(debug_menu_entry *))
    {
        this->m_game_flags_handler = a2;
    }
	
    void set_script_entry_handler(void (*a2)(debug_menu_entry * ,script_instance * a3, mString a4))
    {
        this->m_script_handler;
    }

    void set_render_cb(std::string (*a2)(debug_menu_entry *))
    {
        this->render_callback = a2;
    }

    debug_menu_entry() = default;

    debug_menu_entry(const char *p_text) : entry_type(dUNDEFINED)
    {
        m_value.p_ival = nullptr;
        strncpy(this->text, p_text, MAX_CHARS_SAFE);
    }

    debug_menu_entry(const mString &p_text) : debug_menu_entry(p_text.c_str())
    {
        strncpy(this->text, p_text.c_str(), MAX_CHARS_SAFE);
    }



    debug_menu_entry(debug_menu *submenu);
	
};

extern debug_menu_entry *g_debug_camera_entry;

typedef void (*menu_handler_function)(debug_menu_entry*, custom_key_type key_type);


extern void close_debug();

extern debug_menu* current_menu;

struct debug_menu {
    enum class sort_mode_t {
        undefined = 0,
        ascending = 1,
        descending = 2,
    };

	char title[MAX_CHARS];
	DWORD capacity;
	DWORD used_slots;
	DWORD window_start;
	DWORD cur_index;
	menu_handler_function handler;
	debug_menu_entry* entries;
    debug_menu *m_parent {nullptr};
    sort_mode_t m_sort_mode;
    size_t count;
	    debug_menu_entry *first;
    debug_menu_entry *last;
    debug_menu_entry *highlighted;
    mString field_C;
			int has_editable_values;
		void* is_user_list;



    void add_entry(debug_menu_entry *entry);

    void add_entry(debug_menu *a1)
    {
        debug_menu_entry entry {a1};
        this->add_entry(&entry);
    }

    void render_current_debug_menu();

    static void hide()
    {
        close_debug();
    }

    void go_back()
    {
        if (this->m_parent != nullptr)
        { 
            current_menu = this->m_parent;
            return;
        }
        
        close_debug();
    }

    static void init();
	


    static inline int menu_height = 0;

    static inline int menu_width = 0;

    static inline bool has_focus = false;
    
    static inline bool physics_state_on_exit = true;

    static inline bool had_menu_this_frame = false;

    static inline bool previous_input_state[10]{};

    static inline bool virtual_input_state[10]{};

    static inline bool current_input_state[10]{};

    static inline bool virtual_input_repeating[10]{};

    static inline float input_state_timer[10]{};

	static void frame_advance(Float a1);
	
	    static void grab_focus();
		
		  static void show();
		  
		  
	void render(int x, int y);	  


void render_active();



    static void gather_input(Float a1);




    static inline debug_menu *root_menu = nullptr;


static debug_menu* active_menu;


		
};

struct debug_build_info {
    std::string version;
    std::string build_machine;
    std::string code_path;
    std::string build_time;
    std::string archive_assets;
    std::string display_info;
    
    // Default initialization with your project info
    void set_defaults() {
        version         = "PKG:usm_mod-CL00000001-DEBUG";
        build_machine   = "DEV_LOCAL";
        code_path       = "usm/debug/code, @000001";
        build_time      = "2025/01/21 12:00:00 -0000";
        archive_assets  = "usm_assets, @000001";
        display_info    = "DEBUG BUILD";
    }
};

extern debug_build_info  g_build_info;



extern void* add_debug_menu_entry(debug_menu* menu, debug_menu_entry* entry);

extern debug_menu * create_menu(const char* title, menu_handler_function function, DWORD capacity);


extern debug_menu * create_menu(const char* title, debug_menu::sort_mode_t mode = debug_menu::sort_mode_t::undefined);

extern debug_menu_entry *create_menu_entry(const mString &str);

extern debug_menu_entry *create_menu_entry(debug_menu *menu);

extern const char *to_string(custom_key_type key_type);

extern void handle_game_entry(debug_menu_entry *entry, custom_key_type key_type);


