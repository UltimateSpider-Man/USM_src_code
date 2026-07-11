#include "als_request_data.h"

#include "common.h"
#include "func_wrapper.h"

namespace als {
VALIDATE_SIZE(request_data, 0x14);

request_data::request_data()
{
    clear();
}


request_data::request_data(const request_data &a2) {
    this->did_transition_occur = a2.did_transition_occur;
    this->field_1 = a2.field_1;
    this->field_2 = a2.field_2;
    this->field_3 = a2.field_3;
    this->field_4 = a2.field_4;
    this->field_8 = a2.field_8;
    this->field_C = a2.field_C;
    this->field_10 = a2.field_10;
}
 
void request_data::clear()
{
    this->did_transition_occur = false;
    this->field_1 = false;
    this->field_2 = false;
    this->field_3 = false;
    this->field_4 = false;
    this->field_8 = {0};
    this->field_C = 0;
    this->field_10 = scripted_trans_group::IMPLICIT;
}

void request_data::operator=(const request_data &a2)
{
    // Converted from 0x004ADF40: plain 0x14-byte request-data copy.
    this->did_transition_occur = a2.did_transition_occur;
    this->field_1 = a2.field_1;
    this->field_2 = a2.field_2;
    this->field_3 = a2.field_3;
    this->field_4 = a2.field_4;
    this->field_8 = a2.field_8;
    this->field_C = a2.field_C;
    this->field_10 = a2.field_10;
}

}
