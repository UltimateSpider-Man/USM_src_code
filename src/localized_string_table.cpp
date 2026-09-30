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

#include <array>
#include <cassert>

namespace {
constexpr int PC_GLOBAL_TEXT_COUNT = 478;
#ifdef OPENUSM_XBPACK_V10
constexpr int PACK_GLOBAL_TEXT_COUNT = 446;
constexpr int V10_SCRIPT_TEXT_COUNT = 281;
#else
constexpr int PACK_GLOBAL_TEXT_COUNT = PC_GLOBAL_TEXT_COUNT;
#endif

int expected_global_text_count()
{
    return g_platform == NL_PLATFORM_XBOX ? PACK_GLOBAL_TEXT_COUNT : PC_GLOBAL_TEXT_COUNT;
}

#ifdef OPENUSM_XBPACK_V10
int v10_global_text_index(int idx)
{
    if (idx >= 0 && idx <= 23) {
        return idx;
    }
    if (idx >= 25 && idx <= 50) {
        return idx - 1;
    }
    if (idx >= 52 && idx <= 242) {
        return idx - 2;
    }
    if (idx >= 244 && idx <= 265) {
        return idx - 3;
    }
    if (idx >= 273 && idx <= 291) {
        return idx - 7;
    }
    if (idx >= 294 && idx <= 301) {
        return idx - 8;
    }
    if (idx >= 303 && idx <= 307) {
        return idx - 9;
    }
    if (idx >= 309 && idx <= 314) {
        return idx - 10;
    }
    if (idx >= 408 && idx <= 414) {
        return idx - 14;
    }
    if (idx >= 419 && idx <= 424) {
        return idx - 13;
    }
    if (idx >= 449 && idx <= 452) {
        return idx - 25;
    }

    switch (idx) {
    case 243: return 240;
    case 266: return 262;
    case 267:
    case 268:
    case 269:
    case 270: return 263;
    case 271: return 264;
    case 272: return 265;
    case 293: return 285;
    case 302: return 293;
    case 315: return 305;
    case 316: return 306;
    case 325: return 313;
    case 326: return 320;
    case 328: return 327;
    case 346:
    case 367: return 357;
    case 348: return 334;
    case 360:
    case 369: return 349;
    case 365: return 346;
    case 368: return 355;
    case 370: return 359;
    case 372: return 351;
    case 373: return 361;
    case 374: return 362;
    case 375: return 364;
    case 376: return 384;
    case 377: return 366;
    case 378: return 368;
    case 379: return 370;
    case 380: return 372;
    case 381: return 374;
    case 382: return 376;
    case 383: return 378;
    case 384: return 380;
    case 385: return 382;
    case 386: return 386;
    case 387: return 388;
    case 388: return 390;
    case 389: return 341;
    case 390: return 342;
    case 391: return 343;
    case 393: return 345;
    case 415: return 401;
    case 416: return 402;
    case 417: return 403;
    case 418: return 404;
    case 426: return 412;
    case 427:
    case 428: return 413;
    case 429: return 414;
    case 430: return 415;
    case 431: return 416;
    case 432: return 417;
    case 433: return 418;
    case 434: return 419;
    case 435: return 420;
    case 436: return 423;
    case 453: return 443;
    case 454: return 444;
    case 455:
    case 456: return 429;
    case 457: return 430;
    case 458: return 431;
    case 459: return 432;
    case 461: return 445;
    case 462: return 433;
    case 463: return 434;
    case 464:
    case 465:
    case 475: return 435;
    case 466: return 436;
    case 467: return 437;
    case 468: return 438;
    case 470: return 440;
    case 471: return 441;
    case 472: return 442;
    case 473: return 443;
    case 474: return 444;
    case 476: return 433;
    case 477: return 432;
    default: return -1;
    }
}

void convert_v10_global_text(localized_string_table *table, const char **strings)
{
    assert(table->field_4 - table->scripttext_number == PACK_GLOBAL_TEXT_COUNT);
    assert(table->scripttext_number == V10_SCRIPT_TEXT_COUNT);

    static std::array<const char *, PC_GLOBAL_TEXT_COUNT + V10_SCRIPT_TEXT_COUNT> pc_strings;
    for (int i = 0; i < PC_GLOBAL_TEXT_COUNT; ++i) {
        const int xb_idx = v10_global_text_index(i);
        pc_strings[i] = xb_idx >= 0 && strings[xb_idx] != nullptr ? strings[xb_idx] : "";

        char key[4];
        itoa(i, key, 10);
        if (auto *override_text = get_msg(g_fileUSM(), key); override_text != nullptr) {
            pc_strings[i] = override_text;
        }
    }

    for (int i = 0; i < table->scripttext_number; ++i) {
        pc_strings[PC_GLOBAL_TEXT_COUNT + i] = strings[PACK_GLOBAL_TEXT_COUNT + i];
    }

    table->field_0 = reinterpret_cast<localized_string_table::internal *>(pc_strings.data());
    table->field_4 = PC_GLOBAL_TEXT_COUNT + table->scripttext_number;
}
#endif

const char **localized_strings(localized_string_table *table)
{
    if (table == nullptr || table->field_0 == nullptr) {
        return nullptr;
    }

    return reinterpret_cast<const char **>(table->field_0);
}

int localized_global_text_count(localized_string_table *table)
{
    if (table == nullptr) {
        return expected_global_text_count();
    }

    const int count = table->field_4 - table->scripttext_number;
    return count > 0 ? count : expected_global_text_count();
}

const char *localized_error_string(localized_string_table *table)
{
    const char **strings = localized_strings(table);
    if (strings != nullptr && table->field_4 > 0 && strings[0] != nullptr) {
        return strings[0];
    }

    return "";
}
}

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
        v5.append(g_platform == NL_PLATFORM_XBOX ? "_XBOX" : "_PS2");

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

    const int global_text_count = this->field_4 - this->scripttext_number;
    const int expected_count = expected_global_text_count();
    if (global_text_count < 0) {
        sp_log(
            "localized strings table has invalid counts: total=%d script=%d global=%d.",
            this->field_4,
            this->scripttext_number,
            global_text_count);
        assert(0);
        return;
    }

    if (global_text_count != expected_count) {
        sp_log(
            "localized strings table global count mismatch: expected=%d actual=%d total=%d script=%d.",
            expected_count,
            global_text_count,
            this->field_4,
            this->scripttext_number);

        if (g_platform != NL_PLATFORM_XBOX) {
            assert(0);
        }
    }

    const char **strings = localized_strings(this);
    assert(strings != nullptr);

    if (this->field_4 > 0) {
        for (int i = 0; i < this->field_4; ++i) {
            char DstBuf[4];
            itoa(i, DstBuf, 10);
#ifdef OPENUSM_XBPACK_V10
            auto *v6 = g_platform == NL_PLATFORM_XBOX ? nullptr : get_msg(g_fileUSM(), DstBuf);
#else
            auto *v6 = get_msg(g_fileUSM(), DstBuf);
#endif
            if (v6 != nullptr) {
                strings[i] = v6;
            } else if (strings[i] != nullptr) {
                strings[i] += this->field_8;
            }

            auto v7 = (uint8_t *) strings[i];
            if (v7 != nullptr && *v7) {
                do {
                    if (*v7 == 160) {
                        *v7 = ' ';
                    }
                } while (*++v7);
            }
        }
    }

#ifdef OPENUSM_XBPACK_V10
    if (g_platform == NL_PLATFORM_XBOX) {
        convert_v10_global_text(this, strings);
    }
#endif
}

const char *localized_string_table::lookup_scripttext_string(int num) {
    if (num < 0 || num >= this->scripttext_number) {
        sp_log("localized scripttext lookup out of range: num=%d script_count=%d.", num, this->scripttext_number);
        assert(g_platform == NL_PLATFORM_XBOX);
        return localized_error_string(this);
    }

    const char **strings = localized_strings(this);
    assert(strings != nullptr);

    const int global_text_count = localized_global_text_count(this);
    auto *result = strings[global_text_count + num];

    //    sp_log("lookup_scripttext_string: %s %d", result, num);

    return result != nullptr ? result : localized_error_string(this);
}

const char *localized_string_table::lookup_localized_string(global_text_enum num)
{
    const int idx = static_cast<int>(num);
    const int global_text_count = localized_global_text_count(this);

    if (idx < 0 || idx >= global_text_count) {
        sp_log("localized global text lookup out of range: num=%d global_count=%d.", idx, global_text_count);
        assert(g_platform == NL_PLATFORM_XBOX);
        return localized_error_string(this);
    }

    const char **strings = localized_strings(this);
    assert(strings != nullptr);

    auto *result = strings[idx];

    return result != nullptr ? result : localized_error_string(this);
}

void localized_string_table_patch() {

    SET_JUMP(0x0062EF10, localized_string_table::load_localizer);

    {
        FUNC_ADDRESS(address, &localized_string_table::lookup_localized_string);
        SET_JUMP(0x0060BDC0, address);
    }

    {
        FUNC_ADDRESS(address, &localized_string_table::lookup_scripttext_string);
        SET_JUMP(0x0060BDD0, address);
    }

    //REDIRECT(0x006732E8, dialog_box_formatting);
}
