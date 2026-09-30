// vector standard header
#pragma once

#include "memory.hpp"
#include <cstdbool>

#include <algorithm>
#include <vector>

//#define _THROW(a0, a1)

#define _STDEXT stdext::

#ifndef _SCL_SECURE_VALIDATE
#define _SCL_SECURE_VALIDATE(a1)
#endif

#ifndef _SCL_SECURE_VALIDATE_RANGE
#define _SCL_SECURE_VALIDATE_RANGE(a1)
#endif

namespace _std {

template<class _Ty, class _Ax = std::allocator<_Ty>>
struct vector;

// base class for vector to hold allocator _Alval
template<class _Ty, class _Alloc>
struct _Vector_val : public _Container_base {
protected:
    _Vector_val(_Alloc _Al = _Alloc()) : _Alval(_Al) { // construct allocator from _Al
    }

    typedef typename std::allocator_traits<_Alloc>::template rebind_alloc<_Ty> _Alty;

    _Alty _Alval; // allocator object for values
};

// varying size array of values
template<class _Ty, class _Ax>
struct vector : public _Vector_val<_Ty, _Ax> {
    typedef vector<_Ty, _Ax> _Myt;
    typedef _Vector_val<_Ty, _Ax> _Mybase;
    typedef typename _Mybase::_Alty _Alloc;
    typedef _Alloc allocator_type;
    typedef typename _Alloc::size_type size_type;
    typedef typename _Alloc::difference_type _Dift;
    typedef _Dift difference_type;
    typedef typename _Alloc::value_type * _Tptr;
    typedef typename _Alloc::value_type const * _Ctptr;
    typedef _Tptr pointer;
    typedef _Ctptr const_pointer;
    typedef typename _Alloc::value_type & _Reft;
    typedef _Reft reference;
    typedef typename _Alloc::value_type const & const_reference;
    typedef typename _Alloc::value_type value_type;

    typedef typename std::vector<_Ty, _Alloc>::iterator iterator;
    typedef typename std::vector<_Ty, _Alloc>::const_iterator const_iterator;

#if 0

    static _VEC_ITER_BASE(iterator it) {
        return it._Myptr;
    }
#else
#define _VEC_ITER_BASE(it) &(*it)
#endif

    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    vector() : _Mybase() { // construct empty vector
        _Buy(0);
    }

    explicit vector(const _Alloc &_Al) : _Mybase(_Al) { // construct empty vector with allocator
        _Buy(0);
    }

    explicit vector(size_type _Count) : _Mybase() { // construct from _Count * _Ty()
        _Construct_n(_Count, _Ty());
    }

    vector(size_type _Count, const _Ty &_Val) : _Mybase() { // construct from _Count * _Val
        _Construct_n(_Count, _Val);
    }

    vector(size_type _Count, const _Ty &_Val, const _Alloc &_Al)
        : _Mybase(_Al) { // construct from _Count * _Val, with allocator
        _Construct_n(_Count, _Val);
    }

    vector(const _Myt &_Right) : _Mybase(_Right._Alval) { // construct by copying _Right
        if (_Buy(_Right.size())) {
            m_last = _Ucopy(_Right.begin(), _Right.end(), m_first);
        }
    }

    template<class _Iter>
    vector(_Iter _First, _Iter _Last) : _Mybase() { // construct from [_First, _Last)
        _Construct(_First, _Last, _Iter_cat(_First));
    }

    template<class _Iter>
    vector(_Iter _First, _Iter _Last, const _Alloc &_Al)
        : _Mybase(_Al) { // construct from [_First, _Last), with allocator
        _Construct(_First, _Last, _Iter_cat(_First));
    }

    template<class _Iter>
    void _Construct(_Iter _Count, _Iter _Val, _Int_iterator_tag) { // initialize with _Count * _Val
        size_type _Size = (size_type) _Count;
        _Construct_n(_Size, (_Ty) _Val);
    }

    template<class _Iter>
    void _Construct(_Iter _First,
                    _Iter _Last,
                    std::input_iterator_tag) { // initialize with [_First, _Last), input iterators
        _Buy(0);
        insert(begin(), _First, _Last);
    }

    void _Construct_n(size_type _Count, const _Ty &_Val) { // construct from _Count * _Val
        if (_Buy(_Count)) {                                // nonzero, fill it
            m_last = _Ufill(m_first, _Count, _Val);
        }
    }

    ~vector() { // destroy the object
        _Tidy();
    }

    _Myt &operator=(const _Myt &_Right) { // assign _Right
        if (this != &_Right) {            // worth doing

            if (_Right.size() == 0)
                clear();                        // new sequence empty, erase existing sequence
            else if (_Right.size() <= size()) { // enough elements, copy new and destroy old
                pointer _Ptr = _STDEXT unchecked_copy(_Right.m_first,
                                                      _Right.m_last,
                                                      m_first); // copy new
                _Destroy(_Ptr, m_last);                         // destroy old
                m_last = m_first + _Right.size();
            } else if (_Right.size() <= capacity()) { // enough room, copy and construct new
                pointer _Ptr = _Right.m_first + size();
                _STDEXT unchecked_copy(_Right.m_first, _Ptr, m_first);
                m_last = _Ucopy(_Ptr, _Right.m_last, m_last);
            } else {                 // not enough room, allocate new array and construct new
                if (m_first != nullptr) { // discard old array
                    _Destroy(m_first, m_last);
                    this->_Alval.deallocate(m_first, m_end - m_first);
                }
                if (_Buy(_Right.size()))
                    m_last = _Ucopy(_Right.m_first, _Right.m_last, m_first);
            }
        }
        return (*this);
    }

    void reserve(size_type _Count) { // determine new minimum length of allocated storage
        if (max_size() < _Count) {
            _Xlen();                      // result too long
        } else if (capacity() < _Count) { // not enough room, reallocate
            pointer _Ptr = this->_Alval.allocate(_Count);

            _Umove(begin(), end(), _Ptr);

            size_type _Size = size();
            if (m_first != nullptr) { // destroy and deallocate old array
                _Destroy(m_first, m_last);
                this->_Alval.deallocate(m_first, m_end - m_first);
            }

            m_end = _Ptr + _Count;
            m_last = _Ptr + _Size;
            m_first = _Ptr;
        }
    }

    // return current length of allocated storage
    size_type capacity() const {
        return (m_first == nullptr ? 0 : m_end - m_first);
    }

    // return iterator for beginning of mutable sequence
    iterator begin() {
        return (iterator(m_first));
    }

    // return iterator for beginning of nonmutable sequence
    const_iterator begin() const {
        return (const_iterator(m_first));
    }

    iterator end() { // return iterator for end of mutable sequence
        return (iterator(m_last));
    }

    const_iterator end() const { // return iterator for end of nonmutable sequence
        return (const_iterator(m_last));
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

    void resize(size_type _Newsize) { // determine new length, padding with _Ty() elements as needed
        resize(_Newsize, _Ty());
    }

    void resize(size_type _Newsize,
                _Ty _Val) { // determine new length, padding with _Val elements as needed
        if (size() < _Newsize)
            _Insert_n(end(), _Newsize - size(), _Val);
        else if (_Newsize < size())
            erase(begin() + _Newsize, end());
    }

    // return length of sequence
    size_type size() const {
        return (m_first == nullptr ? 0 : m_last - m_first);
    }

    // return maximum possible length of sequence
    size_type max_size() const {
        return (this->_Alval.max_size());
    }

    bool empty() const { // test if sequence is empty
        return (size() == 0);
    }

    _Alloc get_allocator() const { // return allocator object for values
        return (this->_Alval);
    }

    const_reference at(size_type _Pos) const { // subscript nonmutable sequence with checking
        if (size() <= _Pos)
            _Xran();
        return (*(begin() + _Pos));
    }

    reference at(size_type _Pos) { // subscript mutable sequence with checking
        if (size() <= _Pos)
            _Xran();
        return (*(begin() + _Pos));
    }

    // subscript nonmutable sequence
    const_reference operator[](size_type _Pos) const {
        return (*(m_first + _Pos));
    }

    // subscript mutable sequence
    reference operator[](size_type _Pos) {
        return (*(m_first + _Pos));
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

    // insert element at end
    void push_back(const _Ty &_Val) {
        if (size() < capacity()) {
            m_last = _Ufill(m_last, 1, _Val);
        }
        else {
            insert(end(), _Val);
        }
    }

    void pop_back() {   // erase element at end
        if (!empty()) { // erase last element
            _Destroy(m_last - 1, m_last);
            --m_last;
        }
    }

    template<class _Iter>
    void assign(_Iter _First, _Iter _Last) { // assign [_First, _Last)
        _Assign(_First, _Last, _Iter_cat(_First));
    }

    template<class _Iter>
    void _Assign(_Iter _Count, _Iter _Val, _Int_iterator_tag) { // assign _Count * _Val
        _Assign_n((size_type) _Count, (_Ty) _Val);
    }

    template<class _Iter>
    void _Assign(_Iter _First,
                 _Iter _Last,
                 std::input_iterator_tag) { // assign [_First, _Last), input iterators
        erase(begin(), end());
        insert(begin(), _First, _Last);
    }

    void assign(size_type _Count, const _Ty &_Val) { // assign _Count * _Val
        _Assign_n(_Count, _Val);
    }

    iterator insert(iterator _Where, const _Ty &_Val) { // insert _Val at _Where
        size_type _Off = size() == 0 ? 0 : _Where - begin();
        _Insert_n(_Where, (size_type) 1, _Val);
        return (begin() + _Off);
    }

    void insert(iterator _Where,
                size_type _Count,
                const _Ty &_Val) { // insert _Count * _Val at _Where
        _Insert_n(_Where, _Count, _Val);
    }

    template<class _Iter>
    void insert(iterator _Where, _Iter _First, _Iter _Last) { // insert [_First, _Last) at _Where
        _Insert(_Where, _First, _Last, _Iter_cat(_First));
    }

    template<class _Iter>
    void _Insert(iterator _Where,
                 _Iter _First,
                 _Iter _Last,
                 _Int_iterator_tag) { // insert _Count * _Val at _Where
        _Insert_n(_Where, (size_type) _First, (_Ty) _Last);
    }

    template<class _Iter>
    void _Insert(iterator _Where,
                 _Iter _First,
                 _Iter _Last,
                 std::input_iterator_tag) { // insert [_First, _Last) at _Where, input iterators
        for (; _First != _Last; ++_First, ++_Where)
            _Where = insert(_Where, *_First);
    }

    template<class _Iter>
    void _Insert(iterator _Where,
                 _Iter _First,
                 _Iter _Last,
                 std::forward_iterator_tag) { // insert [_First, _Last) at _Where, forward iterators

        size_type _Count = 0;
        _Distance(_First, _Last, _Count);
        size_type _Capacity = capacity();

        if (_Count == 0)
            ;
        else if (max_size() - size() < _Count)
            _Xlen();                            // result too long
        else if (_Capacity < size() + _Count) { // not enough room, reallocate
            _Capacity = max_size() - _Capacity / 2 < _Capacity
                ? 0
                : _Capacity + _Capacity / 2; // try to grow by 50%
            if (_Capacity < size() + _Count) {
                _Capacity = size() + _Count;
            }
            pointer _Newvec = this->_Alval.allocate(_Capacity);
            pointer _Ptr = _Newvec;

            _Ptr = _Umove(m_first, _VEC_ITER_BASE(_Where),
                          _Newvec);                        // copy prefix
            _Ptr = _Ucopy(_First, _Last, _Ptr);            // add new stuff
            _Umove(_VEC_ITER_BASE(_Where), m_last, _Ptr);  // copy suffix

            _Count += size();
            if (m_first != 0) { // destroy and deallocate old array
                _Destroy(m_first, m_last);
                this->_Alval.deallocate(m_first, m_end - m_first);
            }

            m_end = _Newvec + _Capacity;
            m_last = _Newvec + _Count;
            m_first = _Newvec;
        } else if ((size_type)(end() - _Where) < _Count) { // new stuff spills off end
            _Umove(_VEC_ITER_BASE(_Where), m_last,
                   _VEC_ITER_BASE(_Where) + _Count); // copy suffix
            _Iter _Mid = _First;
            advance(_Mid, end() - _Where);

            _Ucopy(_Mid, _Last, m_last); // insert new stuff off end

            m_last += _Count;

            _STDEXT unchecked_copy(_First, _Mid,
                                   _VEC_ITER_BASE(_Where)); // insert to old end
        } else {                                            // new stuff can all be assigned
            pointer _Oldend = m_last;
            m_last = _Umove(_Oldend - _Count, _Oldend,
                            m_last); // copy suffix
            _STDEXT _Unchecked_move_backward(_VEC_ITER_BASE(_Where),
                                             _Oldend - _Count,
                                             _Oldend); // copy hole

            _STDEXT unchecked_copy(_First, _Last,
                                   _VEC_ITER_BASE(_Where)); // insert into hole
        }
    }

    iterator erase(iterator _Where) { // erase element at where
        _STDEXT unchecked_copy(_VEC_ITER_BASE(_Where) + 1, m_last, _VEC_ITER_BASE(_Where));
        _Destroy(m_last - 1, m_last);
        --m_last;
        return (_Where);
    }

    iterator erase(iterator _First, iterator _Last) { // erase [_First, _Last)
        if (_First != _Last) {                        // worth doing, copy down over hole

            pointer _Ptr = std::copy(_VEC_ITER_BASE(_Last), m_last, _VEC_ITER_BASE(_First));

            _Destroy(_Ptr, m_last);
            m_last = _Ptr;
        }

        return (_First);
    }

    void clear() { // erase all
        erase(begin(), end());
    }

    void swap(_Myt &_Right) {                // exchange contents with _Right
        if (this->_Alval == _Right._Alval) { // same allocator, swap control information

            std::swap(m_first, _Right.m_first);
            std::swap(m_last, _Right.m_last);
            std::swap(m_end, _Right.m_end);
        } else { // different allocator, do multiple assigns
            _Myt _Ts = *this;
            *this = _Right, _Right = _Ts;
        }
    }

    void _Assign_n(size_type _Count, const _Ty &_Val) { // assign _Count * _Val
        _Ty _Tmp = _Val;                                // in case _Val is in sequence
        erase(begin(), end());
        insert(begin(), _Count, _Tmp);
    }

    // allocate array with _Capacity elements
    bool _Buy(size_type _Capacity) {
        m_first = nullptr, m_last = nullptr, m_end = nullptr;
        if (_Capacity == 0) {
            return (false);
        } else if (max_size() < _Capacity) {
            _Xlen(); // result too long
        } else {     // nonempty array, allocate storage
            m_first = this->_Alval.allocate(_Capacity);
            m_last = m_first;
            m_end = m_first + _Capacity;
        }
        return (true);
    }

    void _Destroy(pointer _First, pointer _Last) { // destroy [_First, _Last) using allocator
        _Destroy_range(_First, _Last, this->_Alval);
    }

    void _Tidy() {           // free all storage
        if (m_first != nullptr) { // something to free, destroy and deallocate it

            _Destroy(m_first, m_last);
            this->_Alval.deallocate(m_first, m_end - m_first);
        }
        m_first = nullptr, m_last = nullptr, m_end = nullptr;
    }

    template<class _Iter>
    pointer _Ucopy(_Iter _First,
                   _Iter _Last,
                   pointer _Ptr) { // copy initializing [_First, _Last), using allocator
        return (_STDEXT unchecked_uninitialized_copy(_First, _Last, _Ptr, this->_Alval));
    }

    template<class _Iter>
    pointer _Umove(_Iter _First,
                   _Iter _Last,
                   pointer _Ptr) { // move initializing [_First, _Last), using allocator
        return (_STDEXT _Unchecked_uninitialized_move(_First, _Last, _Ptr, this->_Alval));
    }

    void _Insert_n(iterator _Where,
                   size_type _Count,
                   const _Ty &_Val) { // insert _Count * _Val at _Where

        _Ty _Tmp = _Val; // in case _Val is in sequence
        size_type _Capacity = capacity();

        if (_Count == 0) {
            ;
        } else if (max_size() - size() < _Count) {
            _Xlen();                              // result too long
        } else if (_Capacity < size() + _Count) { // not enough room, reallocate
            _Capacity = max_size() - _Capacity / 2 < _Capacity
                ? 0
                : _Capacity + _Capacity / 2; // try to grow by 50%
            if (_Capacity < size() + _Count) {
                _Capacity = size() + _Count;
            }
            pointer _Newvec = this->_Alval.allocate(_Capacity);
            pointer _Ptr = _Newvec;

            _Ptr = _Umove(m_first, _VEC_ITER_BASE(_Where),
                          _Newvec);                        // copy prefix
            _Ptr = _Ufill(_Ptr, _Count, _Tmp);             // add new stuff
            _Umove(_VEC_ITER_BASE(_Where), m_last, _Ptr);  // copy suffix

            _Count += size();
            if (m_first != nullptr) { // destroy and deallocate old array
                _Destroy(m_first, m_last);
                this->_Alval.deallocate(m_first, m_end - m_first);
            }

            m_end = _Newvec + _Capacity;
            m_last = _Newvec + _Count;
            m_first = _Newvec;
        } else if ((size_type)(m_last - _VEC_ITER_BASE(_Where)) <
                   _Count) { // new stuff spills off end
            _Umove(_VEC_ITER_BASE(_Where), m_last,
                   _VEC_ITER_BASE(_Where) + _Count); // copy suffix

            _Ufill(m_last,
                   _Count - (m_last - _VEC_ITER_BASE(_Where)),
                   _Tmp); // insert new stuff off end

            m_last += _Count;

            std::fill(_VEC_ITER_BASE(_Where), m_last - _Count,
                      _Tmp); // insert up to old end
        } else {             // new stuff can all be assigned
            pointer _Oldend = m_last;
            m_last = _Umove(_Oldend - _Count, _Oldend,
                            m_last); // copy suffix

            _STDEXT _Unchecked_move_backward(_VEC_ITER_BASE(_Where),
                                             _Oldend - _Count,
                                             _Oldend); // copy hole
            std::fill(_VEC_ITER_BASE(_Where),
                      _VEC_ITER_BASE(_Where) + _Count,
                      _Tmp); // insert into hole
        }
    }

    pointer _Ufill(pointer _Ptr,
                   size_type _Count,
                   const _Ty &_Val) { // copy initializing _Count * _Val, using allocator
        _STDEXT unchecked_uninitialized_fill_n(_Ptr, _Count, _Val, this->_Alval);
        return (_Ptr + _Count);
    }

    static void _Xlen() { // report a length_error
        _THROW(std::length_error, "vector<T> too long");
    }

    static void _Xran() { // report an out_of_range error
        _THROW(std::out_of_range, "invalid vector<T> subscript");
    }

    static void _Xinvarg() { // report an invalid_argument error
        _THROW(std::invalid_argument, "invalid vector<T> argument");
    }

    pointer m_first; // pointer to beginning of array
    pointer m_last;  // pointer to current end of sequence
    pointer m_end;   // pointer to end of array
};

// vector implements a performant swap
template<class _Ty, class _Ax>
struct _Move_operation_category<vector<_Ty, _Ax>> {
public:
    typedef _Swap_move_tag _Move_cat;
};

// vector TEMPLATE FUNCTIONS
template<class _Ty, class _Alloc>
inline bool operator==(const vector<_Ty, _Alloc> &_Left,
                       const vector<_Ty, _Alloc> &_Right) { // test for vector equality
    return (_Left.size() == _Right.size() && equal(_Left.begin(), _Left.end(), _Right.begin()));
}

template<class _Ty, class _Alloc>
inline bool operator!=(const vector<_Ty, _Alloc> &_Left,
                       const vector<_Ty, _Alloc> &_Right) { // test for vector inequality
    return (!(_Left == _Right));
}

template<class _Ty, class _Alloc>
inline bool operator<(const vector<_Ty, _Alloc> &_Left,
                      const vector<_Ty, _Alloc> &_Right) { // test if _Left < _Right for vectors
    return (lexicographical_compare(_Left.begin(), _Left.end(), _Right.begin(), _Right.end()));
}

template<class _Ty, class _Alloc>
inline bool operator>(const vector<_Ty, _Alloc> &_Left,
                      const vector<_Ty, _Alloc> &_Right) { // test if _Left > _Right for vectors
    return (_Right < _Left);
}

template<class _Ty, class _Alloc>
inline bool operator<=(const vector<_Ty, _Alloc> &_Left,
                       const vector<_Ty, _Alloc> &_Right) { // test if _Left <= _Right for vectors
    return (!(_Right < _Left));
}

template<class _Ty, class _Alloc>
inline bool operator>=(const vector<_Ty, _Alloc> &_Left,
                       const vector<_Ty, _Alloc> &_Right) { // test if _Left >= _Right for vectors
    return (!(_Left < _Right));
}

template<class _Ty, class _Alloc>
inline void swap(vector<_Ty, _Alloc> &_Left,
                 vector<_Ty, _Alloc> &_Right) { // swap _Left and _Right vectors
    _Left.swap(_Right);
}

//
// TEMPLATE CLASS vector<bool, Alloc> AND FRIENDS
//
typedef unsigned _Vbase;               // word type for vector<bool> representation
const int _VBITS = 8 * sizeof(_Vbase); // at least CHAR_BITS bits per word

// store information common to reference and iterators
template<class _MycontTy>
struct _Vb_iter_base : public _Ranit<bool, typename _MycontTy::difference_type, bool *, bool> {
    _Vb_iter_base() : _Myptr(0), _Myoff(0) { // construct with null pointer
    }

    _Vb_iter_base(const _Vb_iter_base<_MycontTy> &_Right)
        : _Myptr(_Right._Myptr), _Myoff(_Right._Myoff) { // construct with copy of _Right
    }

    _Vb_iter_base(_Vbase *_Ptr) : _Myptr(_Ptr), _Myoff(0) { // construct with offset and pointer
    }

    _Vbase *_Myptr;
    typename _MycontTy::size_type _Myoff;

    static void _Xlen() { // report a length_error
        _THROW(std::length_error, "vector<bool> too long");
    }

    static void _Xran() { // report an out_of_range error
        _THROW(std::out_of_range, "invalid vector<bool> subscript");
    }

    static void _Xinvarg() { // report an invalid_argument error
        _THROW(std::invalid_argument, "invalid vector<bool> argument");
    }
};

// reference to a bit within a base word
template<class _MycontTy>
struct _Vb_reference : public _Vb_iter_base<_MycontTy> {
    _Vb_reference() { // construct with null pointer
    }

    _Vb_reference(const _Vb_iter_base<_MycontTy> &_Right)
        : _Vb_iter_base<_MycontTy>(_Right) { // construct with base
    }

    _Vb_reference &operator=(
        const _Vb_reference<_MycontTy> &_Right) { // assign _Vb_reference _Right to bit
        return (*this = bool(_Right));
    }

    _Vb_reference<_MycontTy> &operator=(bool _Val) { // assign _Val to bit
        if (_Val)
            *_Getptr() |= _Mask();
        else
            *_Getptr() &= ~_Mask();
        return (*this);
    }

    void flip() { // toggle the bit
        *_Getptr() ^= _Mask();
    }

    bool operator~() const { // test if bit is reset
        return (!bool(*this));
    }

    operator bool() const { // test if bit is set
        return ((*_Getptr() & _Mask()) != 0);
    }

    _Vbase *_Getptr() const { // get pointer to base word

        return (this->_Myptr);
    }

    _Vbase _Mask() const { // convert offset to mask
        return ((_Vbase)(1 << this->_Myoff));
    }
};

template<class _MycontTy>
void swap(_Vb_reference<_MycontTy> _Left,
          _Vb_reference<_MycontTy> _Right) { // swap _Left and _Right vector<bool> elements
    bool _Val = _Left;
    _Left = _Right;
    _Right = _Val;
}

// iterator for nonmutable vector<bool>
template<class _MycontTy>
class _Vb_const_iterator : public _Vb_iter_base<_MycontTy> {
public:
    typedef _Vb_reference<_MycontTy> _Reft;
    typedef bool const_reference;

    typedef std::random_access_iterator_tag iterator_category;
    typedef bool value_type;
    typedef typename _MycontTy::size_type size_type;
    typedef typename _MycontTy::difference_type difference_type;
    typedef const_reference *pointer;
    typedef const_reference reference;

    _Vb_const_iterator() { // construct with null reference
    }

    _Vb_const_iterator(const _Vbase *_Ptr)
        : _Vb_iter_base<_MycontTy>((_Vbase *) _Ptr) { // construct with offset and pointer
    }

    const_reference operator*() const { // return (reference to) designated object
        return (_Reft(*this));
    }

    _Vb_const_iterator<_MycontTy> &operator++() { // preincrement
        _Inc();
        return (*this);
    }

    _Vb_const_iterator<_MycontTy> operator++(int) { // postincrement
        _Vb_const_iterator<_MycontTy> _Tmp = *this;
        ++*this;
        return (_Tmp);
    }

    _Vb_const_iterator<_MycontTy> &operator--() { // predecrement
        _Dec();
        return (*this);
    }

    _Vb_const_iterator<_MycontTy> operator--(int) { // postdecrement
        _Vb_const_iterator<_MycontTy> _Tmp = *this;
        --*this;
        return (_Tmp);
    }

    _Vb_const_iterator<_MycontTy> &operator+=(difference_type _Off) { // increment by integer
        if (_Off == 0)
            return (*this); // early out
        _SCL_SECURE_VALIDATE(this->_Mycont != NULL && this->_Myptr != NULL);
        if (_Off < 0) {
            _SCL_SECURE_VALIDATE_RANGE(this->_My_actual_offset() >= ((size_type) -_Off));
        } else {
            _SCL_SECURE_VALIDATE_RANGE((this->_My_actual_offset() + _Off) <=
                                       ((_MycontTy *) this->_Mycont)->_Mysize);
        }
        if (_Off < 0 && this->_Myoff < 0 - (size_type) _Off) { /* add negative increment */
            this->_Myoff += _Off;
            this->_Myptr -= 1 + ((size_type)(-1) - this->_Myoff) / _VBITS;
            this->_Myoff %= _VBITS;
        } else { /* add non-negative increment */
            this->_Myoff += _Off;
            this->_Myptr += this->_Myoff / _VBITS;
            this->_Myoff %= _VBITS;
        }
        return (*this);
    }

    _Vb_const_iterator<_MycontTy> operator+(difference_type _Off) const { // return this + integer
        _Vb_const_iterator<_MycontTy> _Tmp = *this;
        return (_Tmp += _Off);
    }

    _Vb_const_iterator<_MycontTy> &operator-=(difference_type _Off) { // decrement by integer
        return (*this += -_Off);
    }

    _Vb_const_iterator<_MycontTy> operator-(difference_type _Off) const { // return this - integer
        _Vb_const_iterator<_MycontTy> _Tmp = *this;
        return (_Tmp -= _Off);
    }

    difference_type operator-(
        const _Vb_const_iterator<_MycontTy> &_Right) const { // return difference of iterators

        return (_VBITS * (this->_Myptr - _Right._Myptr) + (difference_type) this->_Myoff -
                (difference_type) _Right._Myoff);
    }

    const_reference operator[](difference_type _Off) const { // subscript
        return (*(*this + _Off));
    }

    bool operator==(const _Vb_const_iterator<_MycontTy> &_Right) const { // test for iterator equality

        return (this->_Myptr == _Right._Myptr && this->_Myoff == _Right._Myoff);
    }

    bool operator!=(
        const _Vb_const_iterator<_MycontTy> &_Right) const { // test for iterator inequality
        return (!(*this == _Right));
    }

    bool operator<(const _Vb_const_iterator<_MycontTy> &_Right) const { // test if this < _Right

        return (this->_Myptr < _Right._Myptr ||
                this->_Myptr == _Right._Myptr && this->_Myoff < _Right._Myoff);
    }

    bool operator>(const _Vb_const_iterator<_MycontTy> &_Right) const { // test if this > _Right
        return (_Right < *this);
    }

    bool operator<=(const _Vb_const_iterator<_MycontTy> &_Right) const { // test if this <= _Right
        return (!(_Right < *this));
    }

    bool operator>=(const _Vb_const_iterator<_MycontTy> &_Right) const { // test if this >= _Right
        return (!(*this < _Right));
    }

    void _Dec() { // decrement bit position
        if (this->_Myoff != 0) {
            --this->_Myoff;
        } else {
            _SCL_SECURE_VALIDATE(this->_Mycont != NULL && this->_Myptr != NULL);
            _SCL_SECURE_VALIDATE_RANGE(this->_Myptr > this->_My_cont_begin());
            --this->_Myptr;
            this->_Myoff = _VBITS - 1;
        }
    }

    // increment bit position
    void _Inc() {
        if (this->_Myoff < _VBITS - 1) {
            ++this->_Myoff;
        } else {
            this->_Myoff = 0, ++this->_Myptr;
        }
    }
};

template<class _MycontTy>
_Vb_const_iterator<_MycontTy> operator+(
    typename _Vb_const_iterator<_MycontTy>::difference_type _Off,
    _Vb_const_iterator<_MycontTy> _Right) { // return _Right + integer
    return (_Right += _Off);
}

// iterator for mutable vector<bool>
template<class _MycontTy>
class _Vb_iterator : public _Vb_const_iterator<_MycontTy> {
public:
    typedef _Vb_reference<_MycontTy> _Reft;
    typedef bool const_reference;

    typedef std::random_access_iterator_tag iterator_category;
    typedef bool value_type;
    typedef typename _MycontTy::size_type size_type;
    typedef typename _MycontTy::difference_type difference_type;
    typedef _Reft *pointer;
    typedef _Reft reference;

    _Vb_iterator() { // construct with null reference
    }

    _Vb_iterator(_Vbase *_Ptr)
        : _Vb_const_iterator<_MycontTy>(_Ptr)

    { // construct with offset and pointer
    }

    reference operator*() const { // return (reference to) designated object
        return (_Reft(*this));
    }

    _Vb_iterator<_MycontTy> &operator++() { // preincrement
        ++*(_Vb_const_iterator<_MycontTy> *) this;
        return (*this);
    }

    _Vb_iterator<_MycontTy> operator++(int) { // postincrement
        _Vb_iterator<_MycontTy> _Tmp = *this;
        ++*this;
        return (_Tmp);
    }

    _Vb_iterator<_MycontTy> &operator--() { // predecrement
        --*(_Vb_const_iterator<_MycontTy> *) this;
        return (*this);
    }

    _Vb_iterator<_MycontTy> operator--(int) { // postdecrement
        _Vb_iterator<_MycontTy> _Tmp = *this;
        --*this;
        return (_Tmp);
    }

    _Vb_iterator<_MycontTy> &operator+=(difference_type _Off) { // increment by integer
        *(_Vb_const_iterator<_MycontTy> *) this += _Off;
        return (*this);
    }

    _Vb_iterator<_MycontTy> operator+(difference_type _Off) const { // return this + integer
        _Vb_iterator<_MycontTy> _Tmp = *this;
        return (_Tmp += _Off);
    }

    _Vb_iterator<_MycontTy> &operator-=(difference_type _Off) { // decrement by integer
        return (*this += -_Off);
    }

    _Vb_iterator<_MycontTy> operator-(difference_type _Off) const { // return this - integer
        _Vb_iterator<_MycontTy> _Tmp = *this;
        return (_Tmp -= _Off);
    }

    difference_type operator-(
        const _Vb_const_iterator<_MycontTy> &_Right) const { // return difference of iterators
        return (*(_Vb_const_iterator<_MycontTy> *) this - _Right);
    }

    reference operator[](difference_type _Off) const { // subscript
        return (*(*this + _Off));
    }
};

template<class _MycontTy>
_Vb_iterator<_MycontTy> operator+(typename _Vb_iterator<_MycontTy>::difference_type _Off,
                                  _Vb_iterator<_MycontTy> _Right) { // return _Right + integer
    return (_Right += _Off);
}

// varying size array of bits
template<class _Alloc>
struct vector<bool, _Alloc> : public _Container_base {
    typedef typename _Alloc::size_type size_type;
    typedef typename _Alloc::difference_type _Dift;
    typedef _std::vector<_Vbase, typename std::allocator_traits<_Alloc>::template rebind_alloc<_Vbase>> _Vbtype;
    typedef _std::vector<bool, _Alloc> _Myt;
    typedef _Dift difference_type;
    typedef bool _Ty;
    typedef _Alloc allocator_type;

    typedef _Vb_reference<_Myt> reference;
    typedef bool const_reference;
    typedef bool value_type;

    typedef reference _Reft;
    typedef _Vb_const_iterator<_Myt> const_iterator;
    typedef _Vb_iterator<_Myt> iterator;

    friend class _Vb_reference<_Myt>;
    friend class _Vb_iter_base<_Myt>;
    friend class _Vb_const_iterator<_Myt>;

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
    void _BConstruct(_Iter _Count, _Iter _Val, _Int_iterator_tag) { // initialize from _Count * _Val
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
    void _Assign(_Iter _Count, _Iter _Val, _Int_iterator_tag) { // assign _Count * _Val
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
                 _Iter _Val,
                 _Int_iterator_tag) { // insert _Count * _Val at _Where
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

        std::swap(m_size, _Right.m_size);
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
