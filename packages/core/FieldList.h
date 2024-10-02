/*
 * Copyright (c) 2021, University of Washington
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the University of Washington nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY OF WASHINGTON AND CONTRIBUTORS
 * “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE UNIVERSITY OF WASHINGTON OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __field_list__
#define __field_list__

/******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include "OsApi.h"
#include "LuaEngine.h"
#include "Field.h"

/******************************************************************************
 * CLASS
 ******************************************************************************/

template <class T>
class FieldList: public Field
{
    public:

        /*--------------------------------------------------------------------
         * Methods
         *--------------------------------------------------------------------*/

                        FieldList   (void);
                        FieldList   (std::initializer_list<T> init_list);
                        FieldList   (size_t size, const T& default_value);
                        FieldList   (const FieldList<T>& list);
        virtual         ~FieldList  (void) override = default;

        long            append      (const T& v);
        void            clear       (void);

        long            length      (void) const override;
        const Field*    get         (long i) const override;

        FieldList<T>&   operator=   (const FieldList<T>& list);
        FieldList<T>&   operator=   (std::initializer_list<T> init_list);
        T               operator[]  (int i) const;
        T&              operator[]  (int i);

        string          toJson      (void) const override;
        int             toLua       (lua_State* L) const override;
        int             toLua       (lua_State* L, long key) const override;
        void            fromLua     (lua_State* L, int index) override;

        /*--------------------------------------------------------------------
         * Data
         *--------------------------------------------------------------------*/

        vector<T> values;

    private:

        /*--------------------------------------------------------------------
         * Methods
         *--------------------------------------------------------------------*/

        void copy (const FieldList<T>& list);
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

template <class T>
inline string convertToJson(const FieldList<T>& v) {
    return v.toJson();
}

template <class T>
inline int convertToLua(lua_State* L, const FieldList<T>& v) {
    return v.toLua(L);
}

template <class T>
inline void convertFromLua(lua_State* L, int index, FieldList<T>& v) {
    v.fromLua(L, index);
}

inline uint32_t toEncoding(const FieldList<bool>& v)     { (void)v; return Field::NESTED_LIST | Field::BOOL;   };
inline uint32_t toEncoding(const FieldList<int8_t>& v)   { (void)v; return Field::NESTED_LIST | Field::INT8;   };
inline uint32_t toEncoding(const FieldList<int16_t>& v)  { (void)v; return Field::NESTED_LIST | Field::INT16;  };
inline uint32_t toEncoding(const FieldList<int32_t>& v)  { (void)v; return Field::NESTED_LIST | Field::INT32;  };
inline uint32_t toEncoding(const FieldList<int64_t>& v)  { (void)v; return Field::NESTED_LIST | Field::INT64;  };
inline uint32_t toEncoding(const FieldList<uint8_t>& v)  { (void)v; return Field::NESTED_LIST | Field::UINT8;  };
inline uint32_t toEncoding(const FieldList<uint16_t>& v) { (void)v; return Field::NESTED_LIST | Field::UINT16; };
inline uint32_t toEncoding(const FieldList<uint32_t>& v) { (void)v; return Field::NESTED_LIST | Field::UINT32; };
inline uint32_t toEncoding(const FieldList<uint64_t>& v) { (void)v; return Field::NESTED_LIST | Field::UINT64; };
inline uint32_t toEncoding(const FieldList<float>& v)    { (void)v; return Field::NESTED_LIST | Field::FLOAT;  };
inline uint32_t toEncoding(const FieldList<double>& v)   { (void)v; return Field::NESTED_LIST | Field::DOUBLE; };
inline uint32_t toEncoding(const FieldList<time8_t>& v)  { (void)v; return Field::NESTED_LIST | Field::TIME8;  };
inline uint32_t toEncoding(const FieldList<string>& v)   { (void)v; return Field::NESTED_LIST | Field::STRING; };

/******************************************************************************
 * METHODS
 ******************************************************************************/

/*----------------------------------------------------------------------------
 * Constructor
 *----------------------------------------------------------------------------*/
template <class T>
FieldList<T>::FieldList():
    Field(LIST, getImpliedEncoding<T>())
{
}

/*----------------------------------------------------------------------------
 * Constructor
 *----------------------------------------------------------------------------*/
template <class T>
FieldList<T>::FieldList(std::initializer_list<T> init_list):
    Field(LIST, getImpliedEncoding<T>())
{
    for(T element: init_list)
    {
        values.push_back(element);
    }
}

/*----------------------------------------------------------------------------
 * Constructor
 *----------------------------------------------------------------------------*/
template <class T>
FieldList<T>::FieldList(size_t size, const T& default_value):
    Field(LIST, getImpliedEncoding<T>()),
    values(size, default_value)
{
}

/*----------------------------------------------------------------------------
 * Copy Constructor
 *----------------------------------------------------------------------------*/
template <class T>
FieldList<T>::FieldList(const FieldList<T>& list):
    Field(LIST, getImpliedEncoding<T>())
{
    copy(list);
}

/*----------------------------------------------------------------------------
 * append
 *----------------------------------------------------------------------------*/
template<class T>
long FieldList<T>::append(const T& v)
{
    values.push_back(v);
    return static_cast<long>(values.size());
}

/*----------------------------------------------------------------------------
 * clear
 *----------------------------------------------------------------------------*/
template<class T>
void FieldList<T>::clear(void)
{
    values.clear();
}

/*----------------------------------------------------------------------------
 * length
 *----------------------------------------------------------------------------*/
template<class T>
long FieldList<T>::length(void) const
{
    return static_cast<long>(values.size());
}

/*----------------------------------------------------------------------------
 * get
 *----------------------------------------------------------------------------*/
template<class T>
const Field* FieldList<T>::get(long i) const
{
    if constexpr(std::is_same<T, bool>::value)
    {
        /*
         * The problem with std::vector<bool> is that it doesn't return a reference to a bool when you use the [] operator.
         * Instead, it returns a proxy object that acts like a bool, but isn't a real bool.
         * This is because the proxy object is a reference to a single bit, not a byte.
         * std::vector<bool> packs multiple bool values into a smaller memory footprint, usually packing 8 bool values into a single byte.
         * This is for space efficiency.
         */

        return NULL; // I propose we don't support bools in FieldList, use int_8 instead
    }
    else
    {
        return reinterpret_cast<const Field*>(&values[i]);  // For non-bool types
    }
}

/*----------------------------------------------------------------------------
 * operator=
 *----------------------------------------------------------------------------*/
template <class T>
FieldList<T>& FieldList<T>::operator=(const FieldList<T>& list)
{
    if(this == &list) return *this;
    copy(list);
    return *this;
}

/*----------------------------------------------------------------------------
 * operator=
 *----------------------------------------------------------------------------*/
template <class T>
FieldList<T>& FieldList<T>::operator=(std::initializer_list<T> init_list)
{
    values.clear();
    for(T element: init_list)
    {
        values.push_back(element);
    }
    return *this;
}

/*----------------------------------------------------------------------------
 * operator[] - rvalue
 *----------------------------------------------------------------------------*/
template <class T>
T FieldList<T>::operator[](int i) const
{
    return values[i];
}

/*----------------------------------------------------------------------------
 * operator[] - lvalue
 *----------------------------------------------------------------------------*/
template <class T>
T& FieldList<T>::operator[](int i)
{
    return values[i];
}

/*----------------------------------------------------------------------------
 * toJson
 *----------------------------------------------------------------------------*/
template <class T>
string FieldList<T>::toJson (void) const
{
    const size_t size = values.size();
    string str("[");
    for(size_t i = 0; i < size; i++)
    {
        str += convertToJson(values[i]);
        if(i < size - 1) str += ",";
    }
    str += "]";
    return str;
}

/*----------------------------------------------------------------------------
 * toLua
 *----------------------------------------------------------------------------*/
template <class T>
int FieldList<T>::toLua (lua_State* L) const
{
    lua_newtable(L);
    for(size_t i = 0; i < values.size(); i++)
    {
        convertToLua(L, values[i]);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

/*----------------------------------------------------------------------------
 * toLua
 *----------------------------------------------------------------------------*/
template <class T>
int FieldList<T>::toLua (lua_State* L, long key) const
{
    if(key >= 0 && key < static_cast<long>(values.size()))
    {
        convertToLua(L, values[key]);
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}

/*----------------------------------------------------------------------------
 * fromLua
 *----------------------------------------------------------------------------*/
template <class T>
void FieldList<T>::fromLua (lua_State* L, int index)
{
    T value;

    // clear the list of values
    values.clear();

    // convert all elements from lua
    const int num_elements = lua_rawlen(L, index);
    for(int i = 0; i < num_elements; i++)
    {
        lua_rawgeti(L, index, i + 1);
        convertFromLua(L, -1, value);
        lua_pop(L, 1);
        values.push_back(value);
    }
}

/*----------------------------------------------------------------------------
 * copy
 *----------------------------------------------------------------------------*/
template <class T>
void FieldList<T>::copy(const FieldList<T>& list)
{
    for(size_t i = 0; i < values.size(); i++)
    {
        values[i] = list.values[i];
    }
    encoding = list.encoding;
}

#endif  /* __field_list__ */
