#include "fe_mission_text.h"

#include "common.h"
#include "fetext.h"
#include "func_wrapper.h"
#include "panelanimfile.h"
#include "panelquad.h"
#include "utility.h"

VALIDATE_SIZE(fe_mission_text, 0xB8);
VALIDATE_OFFSET(fe_mission_text, detail, 0x10);
VALIDATE_OFFSET(fe_mission_text, gradient_text, 0x9C);
VALIDATE_OFFSET(fe_mission_text, field_B0, 0xB0);

fe_mission_text::fe_mission_text()
{

}

void fe_mission_text::draw_v10()
{
    if (this->panel == nullptr) {
        return;
    }

    if (!this->shown && !this->anim->field_2D) {
        return;
    }

    this->back_1->Draw();
    this->back_2->Draw();
    this->detail->Draw();

    switch (this->field_B0) {
    case 1:
        this->gradient_hints->Draw();
        break;
    case 2:
        this->gradient_reward->Draw();
        break;
    default:
        this->gradient_text->Draw();
        break;
    }

    if (!this->anim->field_2D) {
        this->text->Draw();
    }
}

void fe_mission_text::set_flavor(int a2) {
    this->field_B0 = a2;
}

void fe_mission_text::set_text(string a2) {
    sp_log("fe_mission_text::set_text: %s", a2.data);

    THISCALL(0x0060D440, this, a2);
}

void fe_mission_text::SetShown(bool a2) {
    sp_log("fe_mission_text::SetShown: %d", a2);

    THISCALL(0x0061AA00, this, a2);
}

#ifdef OPENUSM_XBPACK_V10
void fe_mission_text_v10_patch()
{
    {
        FUNC_ADDRESS(address, &fe_mission_text::draw_v10);
        REDIRECT(0x00635AAD, address);
    }
}
#endif

void fe_mission_text_patch() {
    {
        FUNC_ADDRESS(address, &fe_mission_text::set_text);
        REDIRECT(0x00672EBC, address);
    }

    {
        FUNC_ADDRESS(address, &fe_mission_text::SetShown);
        REDIRECT(0x00672ECB, address);
    }
}
