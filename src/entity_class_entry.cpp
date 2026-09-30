#include "entity_class_entry.h"

#include "common.h"
#include "entity_viseme_entry.h"
#include "mash_info_struct.h"

VALIDATE_SIZE(entity_class_entry, 0x88);

entity_class_entry::entity_class_entry()
{
    this->field_24 = false;
    this->field_25 = false;

    this->initialize(mash::ALLOCATED);
}

void entity_class_entry::initialize(mash::allocation_scope scope)
{
    if ( scope == mash::ALLOCATED )
    {
        this->field_18 = {0};
        this->field_24 = false;
        this->field_25 = false;
        this->field_40 = 0;
    }
}

void entity_class_entry::unmash(mash_info_struct *a1, void *)
{
    a1->unmash_class_in_place(this->field_0, this);
    a1->unmash_class_in_place(this->field_4, this);
    a1->unmash_class_in_place(this->field_8, this);
    a1->unmash_class_in_place(this->field_18, this);
    a1->unmash_class_in_place(this->field_1C, this);
    a1->unmash_class_in_place(this->field_20, this);
    a1->unmash_class_in_place(this->field_44, this);
    a1->unmash_class_in_place(this->field_54, this);
    a1->unmash_class_in_place(this->field_64, this);

    a1->unmash_class_in_place(this->field_74, this);
}

void entity_class_entry::destruct_mashed_class()
{
    this->field_0.destruct_mashed_class();
    this->field_4.destruct_mashed_class();
    this->field_8.destruct_mashed_class();
    this->field_18.destruct_mashed_class();
    this->field_1C.destruct_mashed_class();
    this->field_20.destruct_mashed_class();
    this->field_44.destruct_mashed_class();
    this->field_54.destruct_mashed_class();
    this->field_64.destruct_mashed_class();
    this->field_74.destruct_mashed_class();
}
