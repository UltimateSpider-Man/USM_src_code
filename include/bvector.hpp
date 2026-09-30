#pragma once


#include <algorithm>
#include <vector>

namespace _std {

template<class _Ty, class _Ax = std::allocator<_Ty>>
struct vector;

struct vector<bool, _Alloc> : public _Container_base {
    typedef typename _Alloc::size_type size_type;
    typedef typename _Alloc::difference_type _Dift;
    typedef _std::vector<_Vbase, typename std::allocator_traits<_Alloc>::template rebind_alloc<_Vbase>> _Vbtype;
    typedef _std::vector<bool, _Alloc> _Myt;
    typedef _Dift difference_type;
    typedef bool _Ty;
    typedef _Alloc allocator_type;

    typedef typename std::vector<bool, _Alloc>::reference reference;
    typedef bool const_reference;
    typedef bool value_type;

    typedef reference _Reft;
    typedef typename std::vector<bool, _Alloc>::const_iterator const_iterator;
    typedef typename std::vector<bool, _Alloc>::iterator iterator;

    typedef iterator pointer;
    typedef const_iterator const_pointer;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    vector() : m_size(0), _Myvec() { // construct empty vector
    }

    explicit vector(const _Alloc &_Al)
        : m_size(0), _Myvec(_Al) { // construct empty vector, with allocator
    }

    explicit vector(size_type _Count, bool _Val = false)
        : m_size(0), _Myvec(_Nw(_Count), (_Vbase)(_Val ? -1 : 0)) { // construct from _Count * _Val
        _Trim(_Count);
    }

    vector(size_type _Count, bool _Val, const _Alloc &_Al)
        : m_size(0), _Myvec(_Nw(_Count),
                            (_Vbase)(_Val ? -1 : 0),
                            _Al) { // construct from _Count * _Val, with allocator
        _Trim(_Count);
    }

    template<class _Iter>
    vector(_Iter _First, _Iter _Last) : m_size(0), _Myvec() { // construct from [_First, _Last)
        _BConstruct(_First, _Last, _Iter_cat(_First));
    }

    template<class _Iter>
    vector(_Iter _First, _Iter _Last, const _Alloc &_Al)
        : m_size(0), _Myvec(_Al) { // construct from [_First, _Last), with allocator
        _BConstruct(_First, _Last, _Iter_cat(_First));
    }

    template<class _Iter>
    void _BConstruct(_Iter _Count, _Iter _Val) { // initialize from _Count * _Val
        size_type _Num = (size_type) _Count;
        _Myvec.assign(_Num, (_Ty) _Val ? -1 : 0);
        _Trim(_Num);
    }

    template<class _Iter>
    void _BConstruct(_Iter _First,
                     _Iter _Last,
                     std::input_iterator_tag) { // initialize from [_First, _Last), input iterators
        insert(begin(), _First, _Last);
    }

    ~vector() { // destroy the object
        m_size = 0;
    }

    void reserve(size_type _Count) { // determine new minimum length of allocated storage
        _Myvec.reserve(_Nw(_Count));
    }

    size_type capacity() const { // return current length of allocated storage
        return (_Myvec.capacity() * _VBITS);
    }

    iterator begin() { // return iterator for beginning of mutable sequence
        return (iterator(_VEC_ITER_BASE(_Myvec.begin())));
    }

    const_iterator begin() const { // return iterator for beginning of nonmutable sequence
        return (const_iterator(_VEC_ITER_BASE(_Myvec.begin())));
    }

    iterator end() { // return iterator for end of mutable sequence
        iterator _Tmp = begin();
        if (0 < m_size)
            _Tmp += m_size;
        return (_Tmp);
    }

    const_iterator end() const { // return iterator for end of nonmutable sequence
        const_iterator _Tmp = begin();
        if (0 < m_size)
            _Tmp += m_size;
        return (_Tmp);
    }

    reverse_iterator rbegin() { // return iterator for beginning of reversed mutable sequence
        return (reverse_iterator(end()));
    }

    const_reverse_iterator rbegin()
        const { // return iterator for beginning of reversed nonmutable sequence
        return (const_reverse_iterator(end()));
    }

    reverse_iterator rend() { // return iterator for end of reversed mutable sequence
        return (reverse_iterator(begin()));
    }

    const_reverse_iterator rend() const { // return iterator for end of reversed nonmutable sequence
        return (const_reverse_iterator(begin()));
    }

    void resize(size_type _Newsize,
                bool _Val = false) { // determine new length, padding with _Val elements as needed
        if (size() < _Newsize)
            _Insert_n(end(), _Newsize - size(), _Val);
        else if (_Newsize < size())
            erase(begin() + _Newsize, end());
    }

    size_type size() const { // return length of sequence
        return (m_size);
    }

    size_type max_size() const { // return maximum possible length of sequence
        const size_type _Maxsize = _Myvec.max_size();
        return (_Maxsize < (size_type)(-1) / _VBITS ? _Maxsize * _VBITS : (size_type)(-1));
    }

    bool empty() const { // test if sequence is empty
        return (size() == 0);
    }

    _Alloc get_allocator() const { // return allocator object for values
        return (_Myvec.get_allocator());
    }

    const_reference at(size_type _Off) const { // subscript nonmutable sequence with checking
        if (size() <= _Off)
            _Xran();
        return (*(begin() + _Off));
    }

    reference at(size_type _Off) { // subscript mutable sequence with checking
        if (size() <= _Off)
            _Xran();
        return (*(begin() + _Off));
    }

    const_reference operator[](size_type _Off) const { // subscript nonmutable sequence
        return (*(begin() + _Off));
    }

    reference operator[](size_type _Off) { // subscript mutable sequence
        return (*(begin() + _Off));
    }

    reference front() { // return first element of mutable sequence
        return (*begin());
    }

    const_reference front() const { // return first element of nonmutable sequence
        return (*begin());
    }

    reference back() { // return last element of mutable sequence
        return (*(end() - 1));
    }

    const_reference back() const { // return last element of nonmutable sequence
        return (*(end() - 1));
    }

    void push_back(bool _Val) { // insert element at end
        insert(end(), _Val);
    }

    void pop_back() { // erase element at end
        if (!empty())
            erase(end() - 1);
    }

    template<class _Iter>
    void assign(_Iter _First, _Iter _Last) { // assign [_First, _Last)
        _Assign(_First, _Last, _Iter_cat(_First));
    }

    template<class _Iter>
    void _Assign(_Iter _Count, _Iter _Val) { // assign _Count * _Val
        _Assign_n((size_type) _Count, (bool) _Val);
    }

    template<class _Iter>
    void _Assign(_Iter _First,
                 _Iter _Last,
                 std::input_iterator_tag) { // assign [_First, _Last), input iterators
        erase(begin(), end());
        insert(begin(), _First, _Last);
    }

    void assign(size_type _Count, bool _Val) { // assign _Count * _Val
        _Assign_n(_Count, _Val);
    }

    iterator insert(iterator _Where, bool _Val) { // insert _Val at _Where
        size_type _Off = _Where - begin();
        _Insert_n(_Where, (size_type) 1, _Val);
        return (begin() + _Off);
    }

    void insert(iterator _Where, size_type _Count, bool _Val) { // insert _Count * _Val at _Where
        _Insert_n(_Where, _Count, _Val);
    }

    template<class _Iter>
    void insert(iterator _Where, _Iter _First, _Iter _Last) { // insert [_First, _Last) at _Where
        _Insert(_Where, _First, _Last, _Iter_cat(_First));
    }

    template<class _Iter>
    void _Insert(iterator _Where,
                 _Iter _Count,
                 _Iter _Val) { // insert _Count * _Val at _Where
        _Insert_n(_Where, (size_type) _Count, (bool) _Val);
    }

    template<class _Iter>
    void _Insert(iterator _Where,
                 _Iter _First,
                 _Iter _Last,
                 std::input_iterator_tag) { // insert [_First, _Last) at _Where, input iterators
        size_type _Off = _Where - begin();

        for (; _First != _Last; ++_First, ++_Off)
            insert(begin() + _Off, *_First);
    }

    template<class _Iter>
    void _Insert(iterator _Where,
                 _Iter _First,
                 _Iter _Last,
                 std::forward_iterator_tag) { // insert [_First, _Last) at _Where, forward iterators

        size_type _Count = 0;
        _Distance(_First, _Last, _Count);

        size_type _Off = _Insert_x(_Where, _Count);
        std::copy(_First, _Last, begin() + _Off);
    }

    iterator erase(iterator _Where) { // erase element at _Where
        size_type _Off = _Where - begin();

        std::copy(_Where + 1, end(), _Where);

        _Trim(m_size - 1);
        return (begin() + _Off);
    }

    iterator erase(iterator _First, iterator _Last) { // erase [_First, _Last)
        size_type _Off = _First - begin();

        iterator _Next = std::copy(_Last, end(), _First);
        _Trim(_Next - begin());

        return (begin() + _Off);
    }

    void clear() { // erase all elements
        erase(begin(), end());
    }

    void flip() { // toggle all elements
        for (typename _Vbtype::iterator _Next = _Myvec.begin(); _Next != _Myvec.end(); ++_Next)
            *_Next = (_Vbase) ~*_Next;
        _Trim(m_size);
    }

    void swap(_Myt &_Right) { // exchange contents with right

        std::swap(m_size, _Right._Mysize);
        _Myvec.swap(_Right._Myvec);
    }

    static void swap(reference _Left,
                     reference _Right) { // swap _Left and _Right vector<bool> elements
        bool _Val = _Left;
        _Left = _Right;
        _Right = _Val;
    }

    void _Assign_n(size_type _Count, bool _Val) { // assign _Count * _Val
        erase(begin(), end());
        _Insert_n(begin(), _Count, _Val);
    }

    void _Insert_n(iterator _Where, size_type _Count, bool _Val) { // insert _Count * _Val at _Where
        size_type _Off = _Insert_x(_Where, _Count);
        fill(begin() + _Off, begin() + (_Off + _Count), _Val);
    }

    size_type _Insert_x(iterator _Where,
                        size_type _Count) { // make room to insert _Count elements at _Where
        size_type _Off = _Where - begin();

        if (_Count == 0)
            ;
        else if (max_size() - size() < _Count)
            _Xlen(); // result too long
        else {       // worth doing
            _Myvec.resize(_Nw(size() + _Count), 0);
            if (size() == 0)
                m_size += _Count;
            else { // make room and copy down suffix
                iterator _Oldend = end();
                m_size += _Count;
                copy_backward(begin() + _Off, _Oldend, end());
            }
        }
        return (_Off);
    }

    static size_type _Nw(size_type _Count) { // return number of base words from number of bits
        return ((_Count + _VBITS - 1) / _VBITS);
    }

    void _Trim(size_type _Size) { // trim base vector to exact length in bits
        if (max_size() < _Size)
            _Xlen(); // result too long
        size_type _Words = _Nw(_Size);

        if (_Words < _Myvec.size())
            _Myvec.erase(_Myvec.begin() + _Words, _Myvec.end());
        m_size = _Size;
        _Size %= _VBITS;
        if (0 < _Size)
            _Myvec[_Words - 1] &= (_Vbase)((1 << _Size) - 1);
    }

    void _Xlen() const { // report a length_error
        _THROW(std::length_error, "vector<bool> too long");
    }

    void _Xran() const { // throw an out_of_range error
        _THROW(std::out_of_range, "invalid vector<bool> subscript");
    }

    size_type m_size;  // current length of sequence
    _Vbtype _Myvec;    // base vector of words
};


typedef vector<bool, std::allocator<bool>> _Bvector;
} // namespace _std
