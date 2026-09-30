#include "entity_viseme_entry.h"

#include "common.h"
#include "mash_info_struct.h"

VALIDATE_SIZE(entity_viseme_entry, 0x8);

entity_viseme_entry::entity_viseme_entry(from_mash_in_place_constructor *a2) : field_0(a2) {}

void entity_viseme_entry::unmash(mash_info_struct *a1, void *)
{
    a1->unmash_class_in_place(this->field_0, nullptr);
}

void entity_viseme_entry::destruct_mashed_class()
{
    this->field_0.destruct_mashed_class();
}
