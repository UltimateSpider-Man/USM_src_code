#include "localized_string_table.h"

#include "femanager.h"
#include "fileusm.h"
#include "func_wrapper.h"
#include "game.h"
#include "os_developer_options.h"
#include "resource_directory.h"
#include "resource_manager.h"
#include "settings.h"
#include "trace.h"
#include "utility.h"
#include "variables.h"

#include <cassert>



const char *localized_string_table::find(const char *key)
{
    auto len = strlen(key);

    auto *search = static_cast<char *>(malloc(len + 2));
    strcpy(search, key);
    search[len] = '=';
    search[len + 1] = '\0';

    if (this->field_0 && this->field_4 && this->field_8 > 0 && this->scripttext_number > 0)
    {
        for (int i = 0; i < this->scripttext_number; ++i)
        {
            auto *entry = this->field_0->field_0[i];
            if (strncmp(entry, search, len + 1) == 0)
            {
                free(search);
                return entry + len + 1;
            }
        }
    }

    free(search);
    return nullptr;
}

const char *localized_string_table::lookup(localized_string_table *inst, const char *key)
{
    if (inst != nullptr) {
        return inst->find(key);
    }

    return nullptr;
}

void localized_string_table::load_localizer()
{
    TRACE("localized_string_table::load_localizer");

    if constexpr (1)
    {
        [[maybe_unused]] auto a3 = os_developer_options::instance->get_string(os_developer_options::strings_t::SKU);
        globalTextLanguage() = 0;

        switch (g_settings()->sub_81D010("Settings\\Language", 0)) {
        case 1:
            globalTextLanguage() = 1;
            break;
        case 2:
            globalTextLanguage() = 2;
            break;
        case 3:
            globalTextLanguage() = 3;
            break;
        case 4:
            globalTextLanguage() = 4;
            break;
        default:
            globalTextLanguage() = 0;
            break;
        }

        auto *my_partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_LANG);
        assert(my_partition != nullptr);
        assert(my_partition->get_pack_slots().size() == 1);

        resource_pack_slot *my_slot = my_partition->get_pack_slots().front();
        assert(my_slot != nullptr);

        auto *my_streamer = my_partition->get_streamer();
        assert(my_streamer != nullptr);

        static const char *globalTextLangFileNames[] { "globaltext_ENGLISH",
                                                        "globaltext_FRENCH",
                                                        "globaltext_GERMAN",
                                                        "globaltext_SPANISH",
                                                        "globaltext_ITALIAN" };

        const auto *textLangFileName = globalTextLangFileNames[globalTextLanguage()];

        my_streamer->load(textLangFileName, 0, nullptr, nullptr);
        my_streamer->flush(RenderLoadMeter);

        mString v5{textLangFileName};
#ifdef TARGET_XBOX
        v5.append("_XBOX");
#else
        v5.append("_PS2");
#endif

        resource_key res_key = create_resource_key_from_path(v5.c_str(), RESOURCE_KEY_TYPE_LANGUAGE);
        localized_string_table *string_localizer =
            CAST(string_localizer, my_slot->get_resource(res_key, nullptr, nullptr));
        assert(string_localizer != nullptr);

        string_localizer->sub_60BD30();
        g_game_ptr->field_7C = string_localizer;
    }
    else
    {
        CDECL_CALL(0x0062EF10);
    }
}

void localized_string_table::sub_60BD30() {
    this->field_0 = (internal *) ((char *) this + (unsigned int) this->field_0);
    this->field_8 += (int) this;

    if (this->field_4 != this->scripttext_number + 478) {
        sp_log(
            "localized strings table does not have the correct number of entries (%d) it has (%d) "
            "instead.",
            this->scripttext_number + 478,
            this->field_4);
        assert(0);
    }

    if (this->field_4 > 0) {
        for (int i = 0; i < this->field_4; ++i) {
            char DstBuf[4];
            itoa(i, DstBuf, 10);
            auto *v6 = get_msg(g_fileUSM(), DstBuf);
            if (v6 != nullptr) {
                this->field_0->field_0[i] = v6;
            } else {
                this->field_0->field_0[i] += this->field_8;
            }

            auto v7 = (uint8_t *) this->field_0->field_0[i];
            if (*v7) {
                do {
                    if (*v7 == 160) {
                        *v7 = ' ';
                    }
                } while (*++v7);
            }
        }
    }
}

const char *localized_string_table::lookup_scripttext_string(int num) {
    assert(num >= 0);
    assert(num < this->scripttext_number);

    auto *result = this->field_0->field_778[num];

    //    sp_log("lookup_scripttext_string: %s %d", result, num);

    return result;
}

const char *localized_string_table::lookup_localized_string(global_text_enum num)
{
    static constexpr auto GT_LAST = 479;

    assert(num >= 0);
    assert(num < GT_LAST);

    auto *result = this->field_0->field_0[num];

    return result;
}

void localized_string_table_patch() {

    SET_JUMP(0x0062EF10, localized_string_table::load_localizer);
    return;

    {
        FUNC_ADDRESS(address, &localized_string_table::lookup_localized_string);
        REDIRECT(0x005A7ED6, address);

        /*
        REDIRECT(0x00455F2B, address);
        REDIRECT(0x005A76DA, address);
        REDIRECT(0x005A7A66, address);
        REDIRECT(0x005A7DB1, address);
        */
    }

    if constexpr (0) {
        {
            FUNC_ADDRESS(address, &localized_string_table::lookup_scripttext_string);
            REDIRECT(0x006732B6, address);
            REDIRECT(0x00673327, address);
            REDIRECT(0x00672C5D, address);
            REDIRECT(0x00672DB4, address);
        }
    }

    //REDIRECT(0x006732E8, dialog_box_formatting);
}
