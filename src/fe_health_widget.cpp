#include "fe_health_widget.h"

#include "common.h"
#include "func_wrapper.h"
#include "panelfile.h"
#include "resource_directory.h"
#include "resource_manager.h"

VALIDATE_SIZE(fe_health_widget, 0x58);
VALIDATE_OFFSET(fe_health_widget, field_30, 0x30);

namespace {
PanelFile *carnage_panel = nullptr;
PanelFile *venom_panel = nullptr;
fe_health_widget *hero_widget = nullptr;
}

void carnage_hud_resource_changed(fe_health_widget *widget, bool carnage, bool unloading) {
    if (carnage) {
        if (unloading) {
            if (widget && carnage_panel && widget->field_0[4] == carnage_panel)
                widget->DeInit(4);
            carnage_panel = nullptr;
        }
        return;
    }
    hero_widget = widget;
    venom_panel = !unloading && widget ? widget->field_0[4] : nullptr;
}

bool fe_health_widget::SelectCarnagePanel(bool selected) {
#if !defined(OPENUSM_XBPACK_MODE)
    if (hero_widget != this) {
        hero_widget = this;
        venom_panel = field_0[4] != carnage_panel ? field_0[4] : nullptr;
    }
    if (!selected) {
        if (carnage_panel && field_0[4] == carnage_panel) field_0[4] = venom_panel;
        return true;
    }
    if (field_0[4] && field_0[4] != carnage_panel) venom_panel = field_0[4];
    if (!carnage_panel) {
        const resource_key game_key{string_hash{"GAME"}, RESOURCE_KEY_TYPE_PACK};
        auto *directory = resource_manager::get_resource_directory(game_key);
        const resource_key panel_key{string_hash{"HG_HERO_CARNAGE"}, RESOURCE_KEY_TYPE_PANEL};
        if (!directory || !directory->get_resource(panel_key, nullptr, nullptr)) return false;
        // The new panel and its mesh/textures live in GAME, independently of
        // whichever hero pack is currently being removed or selected.
        resource_manager::push_resource_context(directory->pack_slot);
        Init(4, "HG_HERO_CARNAGE", false);
        resource_manager::pop_resource_context();
        carnage_panel = field_0[4];
    } else {
        field_0[4] = carnage_panel;
        field_55 = false;
        clear_bars();
    }
    return true;
#else
    return !selected;
#endif
}

fe_health_widget::fe_health_widget() {
    this->field_30 = 0;
    this->field_34 = 6;
    auto v9 = 0;
    do {
        this->field_0[v9++] = nullptr;
    } while (v9 < this->field_34);

    this->field_54 = 0;
    this->field_55 = 0;
    this->field_38 = this->field_34;
    this->field_40 = nullptr;
    this->field_44 = nullptr;
    this->field_48 = nullptr;
    this->field_4C = 1.0;
    this->field_50 = 1.0;
}

void fe_health_widget::SetShown(bool a2) {
    THISCALL(0x0061A3F0, this, a2);
}

void fe_health_widget::UpdateMasking() {
    THISCALL(0x0061A5A0, this);
}

char fe_health_widget::clear_bars() {
    return static_cast<char>(THISCALL(0x0063B170, this));
}

void fe_health_widget::Init(int a2, const char *a3, bool a4) {
    this->field_0[a2] = PanelFile::UnmashPanelFile(a3, static_cast<panel_layer>(7));
    this->field_3C = a4;
    this->field_55 = false;
    this->clear_bars();
}

void fe_health_widget::DeInit(int a2) {
    this->field_0[a2] = nullptr;
    if (a2 == this->field_38) {
        this->field_54 = 0;
        this->field_38 = this->field_34;
    }
}
