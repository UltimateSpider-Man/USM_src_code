#include "debug_menu.h"

#include "mstring.h"
#include "script_object.h"
#include "string_hash.h"
#include "vm_executable.h"

#include "os_developer_options.h"
#include "pausemenusystem.h"
#include "func_wrapper.h"

#include "game.h"

#include "ngl.h"
#include "ngl_font.h"
#include "spider_monkey.h"




#include <algorithm>
#include <cassert>
#include <string>

#include <vector>
#include <tuple>


#include <windows.h>

debug_menu *debug_menu::active_menu = nullptr;



const char *to_string(debug_menu_entry_type entry_type)
{
    const char *strings[] = {
        "UNDEFINED",
        "NORMAL",
        "FLOAT_E",
        "POINTER_FLOAT",
        "INTEGER",
        "POINTER_INT",
        "BOOLEAN_E",
        "POINTER_BOOL",
        "POINTER_MENU"
    };

    return strings[entry_type];
}

debug_menu_entry *g_debug_camera_entry {nullptr};

void entry_frame_advance_callback_default(debug_menu_entry *a1) {}

struct debug_menu;

std::string entry_render_callback_default(debug_menu_entry* entry);

std::string entry_render_callback_default(debug_menu_entry* entry)
{
    switch(entry->entry_type)
    {
    case FLOAT_E:
    case POINTER_FLOAT:
    {
		auto val = entry->get_fval();

        char str[64]{}; 
		snprintf(str, 64, "%.2f", val);
		return {str};
	}
    case BOOLEAN_E:
    case POINTER_BOOL:
    {
		bool val = entry->get_bval();

		auto *str = (val ? "True" : "False");
		return {str};
	}
    case BOOLEAN_NUM: {
        bool val = entry->get_bval2();

        auto* str = (val ? "True" : "False");
        return { str };
    }
    case POINTER_MENU: {
        auto* str = (">");
        return { str };
    }
    case INTEGER:
    case POINTER_INT:
    {
		auto val = entry->get_ival();

        char str[100]{}; 
		sprintf(str, "%d", val);
        return {str};
    }
    default:
        break;
    }

    return std::string{""};
}
typedef void (*menu_handler_function)(debug_menu_entry*, custom_key_type key_type);

void close_debug();

debug_menu* current_menu = nullptr;

    debug_menu_entry* debug_menu_entry::alloc_block(debug_menu* m, std::size_t n)
{
    assert(m->count + n <= m->capacity); // stays inside menu’s buffer
    debug_menu_entry* block = &m->entries[m->count];
    m->count += n;
    return block;
}

void script_handler_helper(debug_menu_entry *a2)
{
    if ( a2->field_18 >= 0 && a2->field_14 != nullptr )
    {
        auto *parent = a2->field_14->get_parent();
        auto *exe = parent->get_func(a2->field_18);
        assert(exe != nullptr);

        if ( exe->get_parms_stacksize() == 4 ) {
            a2->field_14->add_thread_for_debug_menu(exe, (const char *)&a2);
        } else {
            a2->field_14->add_thread_for_debug_menu(exe);
        }

        debug_menu::hide();
    }
}

void debug_menu::render_active()
{
    if ( debug_menu::active_menu != nullptr )
    {
        constexpr float menux = 25.0f;
        constexpr float menuy = 45.0f;
        active_menu->render(menux, menuy);
    }

}


int debug_menu_entry::render(int a1, int a2, bool a3)
{
    auto a5 = a3 ? 0xFF00DC00 : 0xFFDCDCDC;

    assert(render_callback != nullptr);

    std::string v10 = this->render_callback(this);
	

    char a2a[256]{};
    auto v14 = v10;
    if ( v14.length() )
    {
        auto *v7 = v14.c_str();
        auto *v4 = this->m_name.c_str();
        snprintf(a2a, 255u, "%s: %s", v4, v7);
    }
    else
    {
        auto *v5 = this->m_name.c_str();
        snprintf(a2a, 255u, "%s", v5);
    }

    uint32_t arg4a, a3a;
    nglGetStringDimensions(nglSysFont(), &arg4a, &a3a, a2a);
    nglListAddString(nglSysFont(), (float)a1, (float)a2, 0.0, a5, a2a);
    auto v11 = arg4a;
    return v11;
}

bool debug_menu_entry::set_script_handler(script_instance *inst, const mString &a3)
{
    auto *v14 = this;

    assert(inst != nullptr);

    auto *v3 = a3.c_str();
    string_hash v8 {v3};

    auto *v5 = inst->get_parent();
    auto v9 = v5->find_func(v8);
    auto v13 = v9;

    bool result;
    if ( v9 >= 0 )
    {
        v14->field_14 = inst;
        v14->field_18 = v13;
        v14->m_game_flags_handler = script_handler_helper;
        result = true;
    }
    else
    {
        auto *v6 = a3.c_str();
        printf("Could not find handler: %s\n", v6);
        result = false;
    }

    return result;
}

bool debug_menu_entry::set_script_handler_from_char2(script_instance *inst, const char* a3, const char* a4, const char* a5, 
                                   const char* a6, const char* a7, const char* a8, const char* a9)
{
    auto *v14 = this;
    assert(inst != nullptr);
    
    // Array of all function name parameters
    const char* func_names[] = { a3, a4, a5, a6, a7, a8, a9 };
    const int num_params = 7;
    
    auto *v5 = inst->get_parent();
    bool result = false;
    int successful_handlers = 0;
    
    // Process each function name parameter
    for (int i = 0; i < num_params; i++) {
        const char* func_name = func_names[i];
        
        // Skip null or empty parameters
        if (func_name == nullptr || func_name[0] == '\0') {
            continue;
        }
        
        string_hash v8 {func_name};
        auto v9 = v5->find_func(v8);
        
        if (v9 >= 0) {
            // Store the handler information
            // Note: This assumes you want to store multiple handlers
            // You may need to adjust based on your actual data structure
            
            if (successful_handlers == 0) {
                // Primary handler
                v14->field_14 = inst;
                v14->field_18 = v9;
                v14->m_game_flags_handler = script_handler_helper;
            } else {
                // Additional handlers - you may need additional fields for these
                // For example:
                // v14->additional_handlers[successful_handlers-1].instance = inst;
                // v14->additional_handlers[successful_handlers-1].func_index = v9;
            }
            
            printf("Successfully set handler %d: %s (index: %d)\n", successful_handlers + 1, func_name, v9);
            successful_handlers++;
            result = true;
        } else {
            printf("Could not find handler: %s\n", func_name);
        }
    }
    
    if (successful_handlers > 0) {
        printf("Total handlers set: %d\n", successful_handlers);
    } else {
        printf("Failed to set any handlers\n");
    }
    
    return result;
}


bool debug_menu_entry::set_script_handler_from_char(script_instance *inst, const char* a3)
{
    auto *v14 = this;

    assert(inst != nullptr);

    auto *v3 = a3;
    string_hash v8 {v3};

    auto *v5 = inst->get_parent();
    auto v9 = v5->find_func(v8);
    auto v13 = v9;

    bool result;
    if ( v9 >= 0 )
    {
        v14->field_14 = inst;
        v14->field_18 = v13;
        v14->m_game_flags_handler = script_handler_helper;
        result = true;
    }
    else
    {
        auto *v6 = a3;
        printf("Could not find handler: %s\n", v6);
        result = false;
    }

    return result;
}

void debug_menu::show()
{
    active_menu = root_menu;
    grab_focus();
}

inline void nglGetStringDimensions2(nglFont* Font, const char* a2, int* a3, int* a4, Float a5, Float a6) {
    CDECL_CALL(0x007798E0, Font, a2, a3, a4, a5, a6);
}
void getStringDimensions2(const char* str, int* width, int* height) {
    nglGetStringDimensions2(*nglSysFont, str, width, height, 1.0, 1.0);
}

int getStringHeight2(const char* str) {
    int height;
    nglGetStringDimensions2(nglSysFont(), str, nullptr, &height, 1.0, 1.0);
    return height;
}

std::string getRealText2(debug_menu_entry* entry) {
    assert(entry->render_callback != nullptr);

    auto v1 = entry->render_callback(entry);

    char a2a[256]{};
    if (v1.size() != 0) {
        auto* v7 = v1.c_str();
        auto* v4 = entry->text;
        snprintf(a2a, 255u, "%s: %s", v4, v7);
    }
    else {
        auto* v5 = entry->text;
        snprintf(a2a, 255u, "%s", v5);
    }

    return { a2a };
}

unsigned int nglColor2(int r, int g, int b, int a)
{
    return ((a << 24) | (r << 16) | (g << 8) | (b & 255));
}

static constexpr DWORD MAX_ELEMENTS_PAGE = 18;


void debug_menu::render_current_debug_menu()
{
    auto UP_ARROW { " ^ ^ ^ " };
    auto DOWN_ARROW { " v v v " };

    int num_elements = std::min((DWORD)MAX_ELEMENTS_PAGE, current_menu->used_slots - current_menu->window_start);
    int needs_down_arrow = ((current_menu->window_start + MAX_ELEMENTS_PAGE) < current_menu->used_slots) ? 1 : 0;


    int cur_width, cur_height;
    int debug_width = 0;
    int debug_height = 0;
	pause_menu_system_ptr->Deactivate();


    auto get_and_update = [&](auto* x) {
        getStringDimensions2(x, &cur_width, &cur_height);
        debug_height += cur_height;
        debug_width = std::max(debug_width, cur_width);
    };

    // printf("new size: %s %d %d (%d %d)\n", x, debug_width, debug_height, cur_width, cur_height);

    get_and_update(current_menu->title);
    get_and_update(UP_ARROW);

    int total_elements_page = needs_down_arrow ? MAX_ELEMENTS_PAGE : current_menu->used_slots - current_menu->window_start;

    for (int i = 0; i < total_elements_page; ++i) {
        debug_menu_entry* entry = &current_menu->entries[current_menu->window_start + i];
        auto cur = getRealText2(entry);
        get_and_update(cur.c_str());
    }

    if (needs_down_arrow) {
        get_and_update(DOWN_ARROW);
    }

    nglQuad quad;

    int menu_x_start = 20, menu_y_start = 40;
    int menu_x_pad = 24, menu_y_pad = 18;

    nglInitQuad(&quad);
    nglSetQuadRect(&quad, menu_x_start, menu_y_start, menu_x_start + debug_width + menu_x_pad, menu_y_start + debug_height + menu_y_pad);
    nglSetQuadColor(&quad, debug_menu::has_focus ? 0xC8141414 : 0x64141414);
    nglSetQuadZ(&quad, 0.5);
    nglListAddQuad(&quad);

    int white_color = nglColor2(255, 255, 255, 255);
    int yellow_color = nglColor2(255, 255, 0, 255);
    int green_color = nglColor2(0, 255, 0, 255);
    int pink_color = nglColor2(255, 0, 255, 255);

    int render_height = menu_y_start;
    render_height += 12;
    int render_x = menu_x_start;
    render_x += 8;
    nglListAddString(nglSysFont(), render_x, render_height, 0.2f, green_color, 1.f, 1.f, current_menu->title);
    render_height += getStringHeight2(current_menu->title);

    if (current_menu->window_start) {
        nglListAddString(*nglSysFont, render_x, render_height, 0.2f, pink_color, 1.f, 1.f, UP_ARROW);
    }

    render_height += getStringHeight2(UP_ARROW);

    for (int i = 0; i < total_elements_page; i++) {

        int current_color = current_menu->cur_index == i ? yellow_color : white_color;

        debug_menu_entry* entry = &current_menu->entries[current_menu->window_start + i];
        auto cur = getRealText2(entry);
        nglListAddString(*nglSysFont, render_x, render_height, 0.2f, current_color, 1.f, 1.f, cur.c_str());
        render_height += getStringHeight2(cur.c_str());
    }

    if (needs_down_arrow) {
        nglListAddString(*nglSysFont, render_x, render_height, 0.2f, pink_color, 1.f, 1.f, DOWN_ARROW);
        render_height += getStringHeight2(DOWN_ARROW);
    }
}

	



debug_menu *debug_menu_entry::remove_menu()
{
    if ( this->m_game_flags_handler != nullptr )
    {
        if ( this->m_value.p_menu != nullptr )
        {
            this->m_value.p_menu->~debug_menu();
        }

        this->m_value.p_menu = nullptr;
        this->m_game_flags_handler(this);
    }

    return this->m_value.p_menu;
}

void debug_menu_entry::on_change(float a3, bool a4)
{
    printf("debug_menu_entry::on_change: text = %s, entry_type = %s, a5 = %d\n", this->text, to_string(this->entry_type), a4);

    switch ( this->entry_type )
    {
    case FLOAT_E:
    case POINTER_FLOAT:
    {
        float v6;
        if ( a4 )
        {
            v6 = this->field_20.m_step_size * this->field_20.m_step_scale;
        }
        else
        {
            v6 = this->field_20.m_step_size;
        }

        auto v5 = this->get_fval() + a3 * v6;
        this->set_fval(v5, true);
        break;
    }
    case BOOLEAN_E:
    case POINTER_BOOL: {
        auto v3 = this->get_bval();
        this->set_bval(!v3, true);
        break;
    }
    case BOOLEAN_NUM: {
        auto v3 = this->get_bval2();
        // Keep track of which entry was last selected (static persists across calls)
// Example debug_menu_entry class

// Keep track of the previously selected entry
        static debug_menu_entry* s_previousEntry = nullptr;

        // If we had a previously selected entry and it’s different from this one, turn it off
        if (s_previousEntry && s_previousEntry != this) {
            s_previousEntry->set_bval2(false, true);
        }

        // Force the newly selected entry to be ON (1)
        this->set_bval2(true, true);

        // Update the static pointer
        s_previousEntry = this;
        break;
    }
    case INTEGER:
    case POINTER_INT:
    {
        float v7 = (a4
                    ? this->field_20.m_step_size * this->field_20.m_step_scale
                    : this->field_20.m_step_size
                    );

        printf("%f\n", v7);
        auto v8 = std::abs(v7);
        if ( v8 < 1.0 )
        {
            v8 = 1.0;
        }

        auto v4 = this->get_ival();
        if ( a3 >= 0.0 )
        {
            this->set_ival((int)(v4 + v8), true);
        }
        else
        {
            this->set_ival((int)(v4 - v8), true);
        }

        break;
    }
    default:
    return;
    }
}

void debug_menu_entry::on_select(float a2)
{
    printf("debug_menu_entry::on_select: text = %s, entry_type = %s\n", this->text, to_string(this->entry_type));

    switch ( this->entry_type )
    {
    case dUNDEFINED:
        if ( this->m_game_flags_handler != nullptr )
        {
            this->m_game_flags_handler(this);
        }

        break;
    case BOOLEAN_E:
    case POINTER_BOOL:
        this->on_change(a2, false);
    case BOOLEAN_NUM:
        this->on_change(a2, false);
        break;
    case POINTER_MENU:
        this->remove_menu();
        if ( this->m_value.p_menu != nullptr )
        {
            current_menu = this->m_value.p_menu;
        }

        break;
    default:
        return;
    }
}


void debug_menu::gather_input(Float a1)
{
    auto *v3 = input_mgr::instance;
    memcpy(previous_input_state, current_input_state, 10);
    memset(current_input_state, 0, sizeof(current_input_state));
    memset(virtual_input_state, 0, sizeof(virtual_input_state));

    if ( v3->get_control_state(69, INVALID_DEVICE_ID) > 0.5 )
    {
        current_input_state[1] = 1;
    }

    if ( v3->get_control_state(68, INVALID_DEVICE_ID) > 0.5 )
    {
        current_input_state[0] = 1;
    }

    if ( v3->get_control_state(76, INVALID_DEVICE_ID) > 0.5 ) {
        current_input_state[6] = 1;
    }
    
    if ( v3->get_control_state(77, INVALID_DEVICE_ID) > 0.5 ) {
        current_input_state[7] = 1;
    }

    if ( v3->get_control_state(74, INVALID_DEVICE_ID) > 0.5 ) {
        current_input_state[2] = 1;
    }

    if ( v3->get_control_state(75, INVALID_DEVICE_ID) > 0.5 ) {
        current_input_state[3] = 1;
    }

    if ( v3->get_control_state(78, INVALID_DEVICE_ID) > 0.5 ) {
        current_input_state[4] = 1;
    }

    if ( v3->get_control_state(79, INVALID_DEVICE_ID) > 0.5 ) {
        current_input_state[5] = 1;
    }

    if ( v3->get_control_state(73, INVALID_DEVICE_ID) > 0.5 ) {
        current_input_state[8] = 1;
    }

    if ( v3->get_control_state(72, INVALID_DEVICE_ID) > 0.5 ) {
        current_input_state[9] = 1;
    }

    if ( v3->get_control_state(70, INVALID_DEVICE_ID) > 0.5 ) {
        current_input_state[3] = 1;
    }

    if ( v3->get_control_state(70, INVALID_DEVICE_ID) < -0.5 ) {
        current_input_state[2] = 1;
    }

    if ( v3->get_control_state(71, INVALID_DEVICE_ID) > 0.5 ) {
        current_input_state[7] = 1;
    }

    if ( v3->get_control_state(71, INVALID_DEVICE_ID) < -0.5 ) {
        current_input_state[6] = 1;
    }

    for (auto i = 0; i < 10; ++i )
    {
        if ( current_input_state[i] == previous_input_state[i] )
        {
            input_state_timer[i] = input_state_timer[i] + a1;
            auto v1 = 0.25f;
            if ( virtual_input_repeating[i] )
            {
                v1 = 0.050000001f;
            }

            if ( input_state_timer[i] > v1 )
            {
                input_state_timer[i] = input_state_timer[i] - v1;
                virtual_input_state[i] = current_input_state[i];
                virtual_input_repeating[i] = true;
            }
        }
        else
        {
            input_state_timer[i] = 0.0;
            virtual_input_repeating[i] = false;
            virtual_input_state[i] = current_input_state[i];
        }
    }
}



void debug_menu::grab_focus()
{
    physics_state_on_exit = !g_game_ptr->is_physics_enabled();
    g_game_ptr->enable_physics(false);
    has_focus = true;
}



void debug_menu::frame_advance(Float a1)
{
    had_menu_this_frame = false;
    gather_input(a1);
    if ( active_menu == nullptr || has_focus )
    {
        if ( virtual_input_state[8] )
        {
            if ( active_menu != nullptr )
            {
                hide();
            }
            else if ( spider_monkey::is_running() ||
                    !os_developer_options::instance->get_flag("ENABLE_ZOOM_MAP") ||
                    g_game_ptr->is_physics_enabled() )
            {
                show();
            }
        }

        if ( active_menu != nullptr )
        {
            had_menu_this_frame = true;
            //active_menu->do_frame_advance(a1);
        }
    }
    else if ( debug_menu::virtual_input_state[9] )
    {
        had_menu_this_frame = true;
        grab_focus();
    }
}


void debug_menu_entry::set_submenu(debug_menu *submenu)
{
    this->entry_type = POINTER_MENU;
    this->m_value.p_menu = submenu;

    if (submenu != nullptr) {
        submenu->m_parent = current_menu;
    }
}

void debug_menu_entry::set_submenu2(debug_menu *submenu)
{
    this->entry_type = dUNDEFINED;
    this->m_value.p_menu = submenu;

    if (submenu != nullptr) {
        submenu->m_parent = current_menu;
    }
}

debug_menu_entry::debug_menu_entry(debug_menu *submenu) : entry_type(POINTER_MENU)
{
    m_value.p_menu = submenu;
    strncpy(this->text, submenu->title, MAX_CHARS_SAFE);
}



void* add_debug_menu_entry(debug_menu* menu, debug_menu_entry* entry)
{
    if (entry->entry_type == POINTER_MENU)
    {
        auto *submenu = entry->m_value.p_menu;
        if (submenu != nullptr) {
            submenu->m_parent = menu;
        }
    }

	if (menu->used_slots < menu->capacity) {
		void* ret = &menu->entries[menu->used_slots];
		memcpy(ret, entry, sizeof(debug_menu_entry));
		++menu->used_slots;

        if (entry->entry_type == NORMAL && menu->used_slots > 1) {
            std::swap(menu->entries[0], menu->entries[menu->used_slots - 1]);
        }

        if (menu->m_sort_mode != debug_menu::sort_mode_t::undefined ) {

            auto begin = menu->entries;
            auto end = begin + menu->used_slots;
            auto find_it = std::find_if(begin, end, [](debug_menu_entry &entry) {
                return entry.entry_type != POINTER_MENU;
            });

            if (find_it != end) {
                auto sort = [mode = menu->m_sort_mode](debug_menu_entry &e0, debug_menu_entry &e1) {
                    auto v7 = e0.get_script_handler();
                    auto v2 = e1.get_script_handler();
                    if (mode == debug_menu::sort_mode_t::ascending) {
                        return v7 < v2;
                    } else { //descending
                        return v7 > v2;
                    }
                };

                std::sort(begin, find_it, sort);

                std::sort(find_it, end, sort);
            }
        }

		return ret;
	}
	else {
		DWORD current_entries_size = sizeof(debug_menu_entry) * menu->capacity;
		DWORD new_entries_size = sizeof(debug_menu_entry) * EXTEND_NEW_ENTRIES;

		void* new_ptr = realloc(menu->entries, current_entries_size + new_entries_size);

		if (new_ptr == nullptr) {
			printf("RIP\n");
			__debugbreak();
		} else {
			menu->capacity += EXTEND_NEW_ENTRIES;
			menu->entries = static_cast<decltype(menu->entries)>(new_ptr);
			memset(&menu->entries[menu->used_slots], 0, new_entries_size);

			return add_debug_menu_entry(menu, entry);
		}
	}
	
	return nullptr;
}

void debug_menu::add_entry(debug_menu_entry *entry)
{
    add_debug_menu_entry(this, entry);
}

debug_menu * create_menu(const char* title, menu_handler_function function, DWORD capacity)
{
    auto *mem = malloc(sizeof(debug_menu));
    debug_menu *menu = new (mem) debug_menu {};

	strncpy(menu->title, title, MAX_CHARS_SAFE);

	menu->capacity = capacity;
	menu->handler = function;
	DWORD total_entries_size = sizeof(debug_menu_entry) * capacity;
	menu->entries = static_cast<decltype(menu->entries)>(malloc(total_entries_size));
	memset(menu->entries, 0, total_entries_size);

	return menu;
}

debug_menu * create_menu(const char* title, debug_menu::sort_mode_t mode)
{
    const auto capacity = 100u;
    auto *mem = malloc(sizeof(debug_menu));
    debug_menu* menu = new (mem) debug_menu {};

	strncpy(menu->title, title, MAX_CHARS_SAFE);

	menu->capacity = capacity;
	DWORD total_entries_size = sizeof(debug_menu_entry) * capacity;
	menu->entries = static_cast<decltype(menu->entries)>(malloc(total_entries_size));
	memset(menu->entries, 0, total_entries_size);

    menu->m_sort_mode = mode;

	return menu;
}

debug_menu_entry *create_menu_entry(const mString &str)
{
    auto *entry = new debug_menu_entry {str};
    return entry;
}

debug_menu_entry *create_menu_entry(debug_menu *menu)
{
    auto *entry = new debug_menu_entry{menu};
    return entry;
}

const char *to_string(custom_key_type key_type)
{
    if (key_type == ENTER)
    {
        return "ENTER";
    }
    else if (key_type == LEFT)
    {
        return "LEFT";
    }
    else if (key_type == RIGHT)
    {
        return "RIGHT";
    }

    return "";
}

void debug_menu::render(int x, int y)
{
    nglQuad a1{};
    nglInitQuad(&a1);
    auto a5 = (float)(unsigned int)(y + menu_height + 10);
    auto a4 = (float)(unsigned int)(x + menu_width + 10);
    nglSetQuadRect(&a1, (float)(x - 5), (float)(y - 5), a4, a5);
    nglSetQuadColor(&a1, has_focus ? 0xC8141414 : 0x64141414);
    nglSetQuadZ(&a1, 0.5);
    nglListAddQuad(&a1);
    menu_width = 0;
    menu_height = 0;
    auto *v3 = this->field_C.c_str();

    uint32_t width, height;
    nglGetStringDimensions(nglSysFont(), &width, &height, v3);
    menu_width = width;
    menu_height += height;

    nglListAddString(nglSysFont(), (float)x, (float)y, 0.0, 0xFFFFFF00, v3);
    auto ya = height + y;
    int v18 = 0;
    int v17 = 0;
    for ( auto *i = this->first; i != nullptr; i = i->next)
    {
        if ( i == this->highlighted)
        {
            v17 = v18;
        }

        ++v18;
    }

    int v15 = 0;
    bool v14 = false;
    bool v13 = false;
    if ( v18 > 20 )
    {
        if ( v17 >= 10 )
        {
            if ( v17 < v18 - 10 )
            {
                v15 = v17 - 10;
                v14 = true;
                v13 = true;
            }
            else
            {
                v15 = v18 - 20;
                v14 = true;
            }
        }
        else
        {
            v15 = 0;
            v13 = true;
        }
    }

    int v12 = 0;
    int v10 = 0;
    for ( auto *entry = this->first; entry != nullptr; entry = entry->next)
    {
        if ( v12 >= v15 && v10 < 20 )
        {
            uint32_t arg4{};
            if ( v10 != 0 || !v14 )
            {
                if ( v10 == 19 && v13 )
                {
                    nglListAddString(nglSysFont(), (float)x, (float)ya, 0.0, 0xFFFF00FF, " v v v ");
                    nglGetStringDimensions(nglSysFont(), &arg4, nullptr, " v v v ");
                }
                else if ( entry == this->highlighted && has_focus )
                {
                    arg4 = entry->render(x, ya, true);
                }
                else
                {
                    arg4 = entry->render(x, ya, false);
                }
            }
            else
            {
                nglListAddString(nglSysFont(), (float)x, (float)ya, 0.0, 0xFFFF00FF, " ^ ^ ^ ");
                nglGetStringDimensions(nglSysFont(), &arg4, nullptr, " ^ ^ ^ ");
            }

            ya += height;
            debug_menu::menu_height += height;
            if ( static_cast<int>(arg4) > debug_menu::menu_width )
            {
                debug_menu::menu_width = arg4;
            }
            
            ++v10;
        }
        
        ++v12;
    }
}



void handle_game_entry(debug_menu_entry *entry, custom_key_type key_type)
{
    printf("handle_game_entry = %s, %s, entry_type = %s\n", entry->text, to_string(key_type), to_string(entry->entry_type));

    if (key_type == ENTER)
    {
        switch(entry->entry_type)
        {
        case dUNDEFINED:
        {    
            if ( entry->m_game_flags_handler != nullptr )
            {
                entry->m_game_flags_handler(entry);
            }
            break;
        }
        case BOOLEAN_E: 
        case POINTER_BOOL:
        {
            auto v3 = entry->get_bval();
            entry->set_bval(!v3, true);
            break;
        } 
        case POINTER_MENU:
        {
            if (entry->m_value.p_menu != nullptr)
            {
                current_menu = entry->m_value.p_menu;
            }
            return;
        }
        default:
            break;
        }
    }
    else if (key_type == LEFT)
    {
        entry->on_change(-1.0, false);
    }
    else if (key_type == RIGHT)
    {
        entry->on_change(1.0, true);
    }
}
