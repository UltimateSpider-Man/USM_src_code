#pragma once

template<typename T0, typename T1>
struct aarect {
    T1 field_0[2];

    T1 & mini()
    {
        return this->field_0;
    }

    bool sub_560880() const;

    aarect<T0, T1> & operator+=(const T1 &a1)
    {
        this->field_0[0] += a1;
        this->field_0[1] += a1;
        return *this;
    }

    bool operator==(const aarect<T0, T1> &a2) const
    {
        return this->field_0[0] == a2.field_0[0] &&
               this->field_0[1] == a2.field_0[1];
    }
};
