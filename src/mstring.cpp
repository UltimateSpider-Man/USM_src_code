#include "mstring.h"

#include "func_wrapper.h"
#include "log.h"
#include "mash_info_struct.h"
#include "memory.h"
#include "common.h"
#include "mash.h"
#include "trace.h"

#include <cassert>
#include <stdio.h>
#include <windows.h>

VALIDATE_SIZE(mString, 0x10);

Var<int> mString_count {0x00957CEC};

Var<const char *[4]> packfile_ext {0x00936BF0};
Var<const char *[4]> packfile_dir {0x00936BD0};

int mString::npos = -1;

mString::mString(float a1) : mString() {
    char Dest[128];
    sprintf(Dest, "%0.3f", float{a1});

    this->update_guts(Dest, -1);
}

mString::mString([[maybe_unused]] int a2, const char *Format, ...)
    : mString()
{
    char Dest[1024];
    va_list Args;

    va_start(Args, Format);
    vsprintf(Dest, Format, Args);

    this->update_guts(Dest, -1);
}

mString::mString(int a2)
    : mString()

{
    char Dest[32];

    sprintf(Dest, "%d", a2);

    this->guts = (char *) mString::null();

    this->field_C = nullptr;
    ++mString_count();
    this->update_guts(Dest, -1);
}

mString &mString::operator=(const char *a2) {
    this->update_guts(a2, -1);
    return (*this);
}

mString &mString::operator=(const mString &a2) {
    if (this != (&a2)) {
        this->update_guts(a2.guts, a2.size());
    }

    return (*this);
}

mString::~mString() {
    finalize(0);
}

mString mString::to_lower() {
    strlwr(this->guts);
    return *this;
}

mString mString::to_upper() {
    strupr(this->guts);
    return *this;
}

char mString::at(int i) const
{
    return (*this)[i];
}

char mString::operator[](int i) const {
    assert(guts != nullptr);
    assert(i <= (int) m_size);

    assert(i >= 0);

    return this->guts[i];
}

void mString::initialize() {
    this->set_size(0);
    this->field_0 = 0;
    this->field_C = nullptr;

    this->guts = (char *) mString::null();
    ++mString_count();
}

mString::mString(const char *a2)
    : mString()
{
    if (a2 != nullptr) {
        this->update_guts(a2, -1);
    }
}

mString mString::from_float(float a2) {
    mString a1{a2};

    return a1;
}

int mString::find(const char *str, int a3) const {
    char *v1 = strstr(&this->guts[a3], str);
    if (v1 != nullptr) {
        return v1 - this->guts;
    }

    return mString::npos;
}

mString *mString::operator+=(const char *Source) {
    this->append(Source, -1);
    return this;
}

mString mString::substr(int a3, int Count) const {
    mString v7;
    v7.append(&this->guts[a3], Count);

    mString result;
    result.update_guts(v7.guts, -1);

    return result;
}

int mString::find(mString::pos_t a2, char a3) const {
    char *v3 = this->guts;
    char v4 = v3[a2];
    char *v5 = &v3[a2];
    if (!v4) {
        return -1;
    }

    while (v4 != a3) {
        v4 = (v5++)[1];
        if (!v4) {
            return -1;
        }
    }

    return v5 - v3;
}

mString mString::slice(int start, int end) {
    if (start < 0) {
        start += this->size();
    }

    if (end < 0) {
        end += this->size();
    }

    assert(start <= end);

    mString result = substr(start, end - start);
    return result;
}

int mString::rfind(const char *str) const {
    assert(guts && str);
    int v5 = std::strlen(str);
    for (auto i = this->size() - v5; i >= 0; --i) {
        int j;
        for (j = 0; j < v5 && this->guts[j + i] == str[j]; ++j) {
            ;
        }

        if (j == v5) {
            return i;
        }
    }

    return -1;
}

int mString::rfind(char a2, int a3) const {
    if (!this->size()) {
        return -1;
    }

    int v5 = a3;
    if (a3 == -1) {
        v5 = this->size() - 1;
    }

    char *v6 = this->guts;
    char *v7 = &v6[v5];
    if (*v7 == a2) {
        return v7 - v6;
    }

    while (--v7 >= v6) {
        if (*v7 == a2) {
            return v7 - v6;
        }
    }

    return -1;
}

void mString::finalize(int) {
    this->destroy_guts();
    --mString_count();
}

mString::mString()
    : mContainer(), guts(""),

      field_C(nullptr) {

    ++(mString_count());
}

mString::mString([[maybe_unused]] mString::fmtd fmt, const char *Format, ...) {
    char Dest[1024];
    va_list Args;

    va_start(Args, Format);
    this->set_size(0);
    this->field_0 = 0;
    vsprintf(Dest, Format, Args);

    this->guts = (char *) mString::null();
    ++mString_count();

    this->field_C = nullptr;
    this->update_guts(Dest, -1);
}

mString::mString(const mString &a2)
    : mString()

{
    if constexpr (1) {
        this->update_guts(a2.guts, -1);
    } else {
        THISCALL(0x00421220, this, &a2);
    }
}

mString mString::from_int(int a2) {
    mString str = mString{a2};

    return str;
}

int mString::to_int() const {
    return std::atoi(this->guts);
}

double mString::to_float() const {
    return std::atof(this->guts);
}

static const char *sub_1067BA0(const char *a1, int a2) {
    if (!a1) {
        return nullptr;
    }

    const char *i;
    for (i = a1; *i && *i != a2; ++i) {
        ;
    }

    const char *result = nullptr;

    if (*i) {
        result = i;
    }

    return result;
}

mString mString::remove_leading(const char *a1) {
    int start;
    for (start = 0; start < this->size() && sub_1067BA0(a1, this->guts[start]) != nullptr; ++start) {
        ;
    }

    auto end = this->size();
    auto v5 = this->slice(start, end);
    *this = v5;

    return *this;
}

mString mString::remove_trailing(const char *a2) {
    int end;
    for (end = this->size(); end > 0 && sub_1067BA0(a2, this->guts[end - 1]) != nullptr; --end) {
        ;
    }

    auto a1 = this->slice(0, end);
    *this = a1;

    return *this;
}

mString mString::remove_surrounding_whitespace() {
    this->remove_leading(" \n\t\r");

    this->remove_trailing(" \n\t\r");
    return *this;
}

bool operator==(const mString &a1, const mString &a2) {
    auto *v2 = a2.c_str();
    return (a1 == v2);
}

bool operator!=(const mString &a1, const mString &a2) {
    return !(a1 == a2);
}

bool operator<(const mString &a1, const mString &a2) {
    auto *v2 = a2.c_str();
    return a1.compare(v2) == 1;
}

bool operator>(const mString &a1, const mString &a2) {
    auto *v2 = a2.c_str();

    return a1.compare(v2) == -1;
}

const char *mString::c_str() const {
    return guts;
}

char *mString::data() {
    return guts;
}

void *dialog_box_formatting(mString *out_string, mString a2, int a3, int a4) {
    //sp_log("dialog_box_formatting: %s", a2.guts);

    return (void *) CDECL_CALL(0x0064DF30, out_string, a2, a3, a4);
}

void mString::append(char a3) {
    char a1[2];
    a1[0] = a3;
    a1[1] = 0;
    this->append(a1, -1);
}

void mString::append(const char *from_string, int from_string_length) {
    if (from_string_length == -1) {
        from_string_length = strlen(from_string);
    }

    if (from_string_length) {
        assert(from_string_length > 0);
        assert(((uint32_t) from_string_length) < MAX_MSTRING_LENGTH);

        size_t v6 = from_string_length + this->size();

        {
            auto v5 = this->field_C;
            if (v5 != nullptr && v6 < v5->m_size) {
                this->set_size(v6);
                strncat(this->guts, from_string, from_string_length);
                this->guts[v6] = 0;
            } else {
                slab_allocator::slab_t *v12 = nullptr;

                auto *v8 = static_cast<char *>(mem_alloc(v6 + 1));

                if (this->size() <= 0) {
                    v8[0] = 0;
                } else {
                    strncpy(v8, this->guts, this->size());
                    v8[this->size()] = 0;
                }

                strncat(v8, from_string, from_string_length);

                this->guts = v8;
                this->set_size(v6);
                this->field_C = v12;
                this->guts[v6] = 0;
            }
        }
    }
}

bool mString::operator==(const char *a2) const {
    assert(guts != nullptr);

    return strncmp(this->guts, a2, 65535u) == 0;
}

void mString::destroy_guts() {
    if constexpr (1) {
        auto *v2 = this->guts;
        if (v2 != mString::null()) {
            if ((int) v2 < (int) this || (int) v2 > (int) this + this->field_0) {
                if (this->field_C == nullptr) {
                    delete[](v2);
                } else {
                    slab_allocator::deallocate(v2, this->field_C);
                }
            }

            this->guts = (char *) mString::null();

            this->field_C = nullptr;
        }

    } else {
        THISCALL(0x0041F970, this);
    }
}

mString operator+(const char *a2, const mString &a3) {
    mString v5;

    if (a2 != nullptr) {
        v5.update_guts(a2, -1);
    }

    v5.append(a3.guts, 0xFFFFFFFF);

    mString a1{};

    a1.update_guts(v5.guts, -1);

    return a1;
}

mString operator+(const mString &arg4, const char *Source) {
    char *v3 = arg4.guts;

    mString v6;
    v6.update_guts(v3, -1);

    v6.append(Source, 0xFFFFFFFF);
    char *v4 = v6.guts;

    mString a1 {};

    a1.update_guts(v4, -1);

    return a1;
}

mString mString::get_standalone_filename(const mString &arg4, _nlPlatformEnum a3) {
    const char *vec[] = {".PAK", "_XB.PAK", "_GC.PAK", "_PC.PAK"};

    mString v9 = mString{vec[a3]};

    mString v8 = mString{"packs\\"};

    mString v2 = v8 + arg4;
    mString res = v2 + v9;

    return res;
}

mString *mString::operator+=(const mString &a2) {
    this->append(a2.guts, 0xFFFFFFFF);
    return this;
}

mString operator+(const mString &a2, const mString &a3) {
    const char *v3 = a2.guts;
    mString v7;
    v7.update_guts(v3, -1);

    const char *v4 = a3.guts;
    v7.append(v4, 0xFFFFFFFF);
    const char *v5 = v7.guts;

    mString a1{};
    a1.update_guts(v5, -1);

    return a1;
}

mString mString::truncate(int a2) {
    int v4; // edx

    auto *result = this;
    auto v3 = this->size();
    if (v3) {
        v4 = a2;
        if (a2 < 0) {
            v4 = 0;
        }

        if (v4 > v3) {
            v4 = v3;
        }

        result->guts[v4] = 0;
        result->set_size(v4);
    }

    return *result;
}

int mString::compare(const char *str) const {
    assert(str != nullptr);
    assert(guts != nullptr);

    auto *v3 = this->guts;

    uint32_t i;
    for (i = 0; i < this->m_size; ++i) {
        if (str[i] == '\0') {
            return -1;
        }

        if (v3[i] != str[i]) {
            if (str[i] > v3[i]) {
                return 1;
            }

            if (str[i] < v3[i]) {
                return -1;
            }
        }
    }

    return str[i] != '\0';
}

bool mString::is_equal(const char *a2) const {
    assert(guts != nullptr);
    return strncmp(this->guts, a2, 65535) == 0;
}

void mString::update_guts(const char *from_string, int n) {
    if constexpr (1) {
 //       assert(from_string != nullptr);

        if (n == -1) {
            n = strlen(from_string);
        }

        assert(n >= 0 && ((uint32_t) n) <= MAX_MSTRING_LENGTH - 1);

        if (n > static_cast<int>(m_size)) {
            this->destroy_guts();

            this->guts = (n < 176)
                ? (static_cast<char *>(slab_allocator::allocate(n + 1, &this->field_C)))
                : (new char[n + 1]);
        }

        if (n > 0) {
            std::memcpy(this->guts, from_string, n + 1);
            this->m_size = n;
        } else {
            this->destroy_guts();

            this->m_size = n;

            this->guts = (char *) mString::null();
        }
    } else {
        THISCALL(0x0041F9D0, this, from_string, n);
    }
}

void mString::unmash(mash_info_struct *a1, void *a2)
{
    this->custom_unmash(a1, a2);
}

void mString::custom_unmash(mash_info_struct *a1, void *a2)
{
    TRACE("mString::custom_unmash");

#ifdef TARGET_XBOX
    assert(guts == (char *)mash::CUSTOM_MASH_SENTRY);
#endif

    if constexpr (1)
    {
        auto size = this->size();
        if (size <= 0) {
            this->guts = (char *) mString::null();
        } else {
            a1->align_buffer(
#ifdef TARGET_XBOX
                mash::NORMAL_BUFFER,
#endif 
                    1);

            this->guts = (char *) a1->read_from_buffer(
#ifdef TARGET_XBOX
                mash::NORMAL_BUFFER,
#endif 
                this->m_size + 1, 1);
;
            a1->align_buffer(
#ifdef TARGET_XBOX
                mash::NORMAL_BUFFER,
#endif 
                    4);
        }

#ifndef TARGET_XBOX
        this->field_0 = (int) &a1->mash_image_ptr[a1->buffer_size_used[0] - (uint32_t)this];
#endif
    }
    else
    {
        THISCALL(0x004209F0, this, a1, a2);
    }

    sp_log("%d %s", this->m_size, this->c_str());
}
