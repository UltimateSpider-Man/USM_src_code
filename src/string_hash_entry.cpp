#include "string_hash_entry.h"

#include "string_hash.h"

#include "common.h"
#ifdef OPENUSM_XBPACK_V10
#include "exe_allocator.h"
#endif
#include "func_wrapper.h"
#include "mash_info_struct.h"

VALIDATE_SIZE(string_hash_entry, 20u);

#ifdef OPENUSM_XBPACK_V10
void *string_hash_entry::operator new(std::size_t size)
{
    assert(size == sizeof(string_hash_entry));
    return exe_allocator<string_hash_entry> {}.allocate(1);
}

void string_hash_entry::operator delete(void *ptr, std::size_t size) noexcept
{
    (void) size;
    exe_allocator<string_hash_entry> {}.deallocate(
        static_cast<string_hash_entry *>(ptr), 1);
}
#endif

string_hash_entry::string_hash_entry()
{
    this->initialize(mash::ALLOCATED, nullptr, nullptr);
}

string_hash_entry::string_hash_entry(const char *a2, const string_hash *a3)
{
    this->initialize(mash::ALLOCATED, a2, a3);
}

void string_hash_entry::initialize(mash::allocation_scope, const char *a2, const string_hash *a3)
{
    if (a2 != nullptr) {
        this->field_4 = a2;
    }

    if (a3 != nullptr) {
        this->field_0 = *a3;
    }
}

mString string_hash_entry::generate_text(const char *a3) const
{
    if constexpr (1) {
        mString a1{0, "0x%08x\t%s%s", this->field_0.source_hash_code, this->field_4.c_str(), a3};

        return a1;
    } else {
        mString result;
        THISCALL(0x0050DBC0, this, &result, a3);

        return result;
    }
}

void string_hash_entry::unmash(mash_info_struct *a1, void *a2)
{
    this->custom_unmash(a1, a2);
}

void string_hash_entry::custom_unmash(mash_info_struct *a2, void *)
{
    a2->unmash_class_in_place(this->field_0, this);
    a2->unmash_class_in_place(this->field_4, this);
}
