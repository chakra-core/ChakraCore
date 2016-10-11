//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

// forward declaration
class JITTypeHandler;

class JITType
{
public:
    JITType();
    JITType(TypeIDL * data);

    bool IsShared() const;
    Js::TypeId GetTypeId() const;
    intptr_t GetPrototypeAddr() const;
    intptr_t GetAddr() const;

    const JITTypeHandler* GetTypeHandler() const;

    TypeIDL * GetData();

    static void BuildFromJsType(__in Js::Type * jsType, __out JITType * jitType);

private:
    TypeIDL m_data;
};

class JITTypeHolder
{
public:
    JITType * t;

    JITTypeHolder();
    JITTypeHolder(JITType * t);
    const JITType* operator->() const;
    bool operator== (const JITTypeHolder& p) const;
    bool operator!= (const JITTypeHolder& p) const;
    bool operator> (const JITTypeHolder& p) const;
    bool operator>= (const JITTypeHolder& p) const;
    bool operator< (const JITTypeHolder& p) const;
    bool operator<= (const JITTypeHolder& p) const;
    void operator =(const JITTypeHolder &p);
    bool operator== (const std::nullptr_t &p) const;
    bool operator!= (const std::nullptr_t &p) const;

private:
    // prevent implicit conversion
    template<typename T> bool operator== (T p) const;
    template<typename T> bool operator!= (T p) const;
    template<typename T> bool operator>= (T p) const;
    template<typename T> bool operator> (T p) const;
    template<typename T> bool operator< (T p) const;
    template<typename T> bool operator<= (T p) const;
    template<typename T> void operator =(T p);
    template<typename T> T* operator->();
};
