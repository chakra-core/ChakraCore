//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#include "AtomicsOperations.h"

#ifdef _WIN32
#define InterlockedExchangeAdd8 _InterlockedExchangeAdd8
#define InterlockedExchangeAdd16 _InterlockedExchangeAdd16

#define InterlockedAnd8 _InterlockedAnd8
#define InterlockedAnd16 _InterlockedAnd16

#define InterlockedOr8 _InterlockedOr8
#define InterlockedOr16 _InterlockedOr16

#define InterlockedXor8 _InterlockedXor8
#define InterlockedXor16 _InterlockedXor16

#define InterlockedCompareExchange8 _InterlockedCompareExchange8
#define InterlockedCompareExchange16 _InterlockedCompareExchange16

#define InterlockedExchange8 _InterlockedExchange8
#define InterlockedExchange16 _InterlockedExchange16
#endif

#define InterlockedExchangeAdd32 InterlockedExchangeAdd
#define InterlockedAnd32 InterlockedAnd
#define InterlockedOr32 InterlockedOr
#define InterlockedXor32 InterlockedXor
#define InterlockedCompareExchange32 InterlockedCompareExchange
#define InterlockedExchange32 InterlockedExchange

template<typename T> struct ConvertType {};
template<> struct ConvertType<int8> { typedef char _t; };
template<> struct ConvertType<uint8> { typedef char _t; };
template<> struct ConvertType<int16> { typedef short _t; };
template<> struct ConvertType<uint16> { typedef short _t; };
template<> struct ConvertType<int32> { typedef LONG _t; };
template<> struct ConvertType<uint32> { typedef LONG _t; };
template<> struct ConvertType<int64> { typedef LONGLONG _t; };

#define MakeInterLockArgDef1(type) type value
#define MakeInterLockArgDef2(type) type v1, type v2
#define MakeInterLockArgUse1 value
#define MakeInterLockArgUse2 v1, v2
#define _MakeInterlockTemplate(op, argDef, argUse) \
template<typename T> T Interlocked##op##_t(T* target, argDef(T));\
template<> char Interlocked##op##_t(char* target, argDef(char))   { return Interlocked##op##8 (target, argUse); }\
template<> short Interlocked##op##_t(short* target, argDef(short)){ return Interlocked##op##16(target, argUse); }\
template<> LONG Interlocked##op##_t(LONG* target, argDef(LONG))   { return Interlocked##op##32(target, argUse); }\
template<> LONGLONG Interlocked##op##_t(LONGLONG* target, argDef(LONGLONG)) { return Interlocked##op##64(target, argUse); }
#define MakeInterlockTemplate(op, nArgs) _MakeInterlockTemplate(op, MakeInterLockArgDef##nArgs, MakeInterLockArgUse##nArgs)
MakeInterlockTemplate(ExchangeAdd, 1)
MakeInterlockTemplate(And, 1)
MakeInterlockTemplate(Or, 1)
MakeInterlockTemplate(Xor, 1)
MakeInterlockTemplate(Exchange, 1)
MakeInterlockTemplate(CompareExchange, 2)

namespace Js
{
template<typename T> T AtomicsOperations::Load(T* buffer)
{
    // MemoryBarrier only works when the memory size is not greater than the register size
    CompileAssert(sizeof(T) <= sizeof(size_t));
    MemoryBarrier();
    T result = (T)*buffer;
    return result;
}

#if TARGET_32
template<> int64 AtomicsOperations::Load(int64* buffer)
{
    CompileAssert(sizeof(size_t) == 4);
    // Implement 64bits atomic load on 32bits platform with a CompareExchange
    // It is slower, but at least it is garantied to be an atomic operation
    return CompareExchange<int64>(buffer, 0, 0);
}
#endif

template<typename T> T AtomicsOperations::Store(T* buffer, T value)
{
    typedef typename ConvertType<T>::_t convertType;
    InterlockedExchange_t<convertType>((convertType*)buffer, (convertType)value);
    return value;
}

template<typename T> T AtomicsOperations::Add(T* buffer, T value)
{
    typedef typename ConvertType<T>::_t convertType;
    T result = (T)InterlockedExchangeAdd_t<convertType>((convertType*)buffer, (convertType)value);
    return result;
}

template<typename T> T AtomicsOperations::And(T* buffer, T value)
{
    typedef typename ConvertType<T>::_t convertType;
    T result = (T)InterlockedAnd_t<convertType>((convertType*)buffer, (convertType)value);
    return result;
}

template<typename T> T AtomicsOperations::CompareExchange(T* buffer, T comparand, T replacementValue)
{
    typedef typename ConvertType<T>::_t convertType;
    T result = (T)InterlockedCompareExchange_t<convertType>((convertType*)buffer, (convertType)replacementValue, (convertType)comparand);
    return result;
}

template<typename T> T AtomicsOperations::Exchange(T* buffer, T value)
{
    typedef typename ConvertType<T>::_t convertType;
    T result = (T)InterlockedExchange_t<convertType>((convertType*)buffer, (convertType)value);
    return result;
}

template<typename T> T AtomicsOperations::Or(T* buffer, T value)
{
    typedef typename ConvertType<T>::_t convertType;
    T result = (T)InterlockedOr_t<convertType>((convertType*)buffer, (convertType)value);
    return result;
}

template<typename T> T AtomicsOperations::Sub(T* buffer, T value)
{
    typedef typename ConvertType<T>::_t convertType;
    T result = (T)InterlockedExchangeAdd_t<convertType>((convertType*)buffer, -(convertType)value);
    return result;
}

template<typename T> T AtomicsOperations::Xor(T* buffer, T value)
{
    typedef typename ConvertType<T>::_t convertType;
    T result = (T)InterlockedXor_t<convertType>((convertType*)buffer, (convertType)value);
    return result;
}

#define ExplicitImplementation(type) \
    template type AtomicsOperations::Load(type*); \
    template type AtomicsOperations::Store(type*, type); \
    template type AtomicsOperations::Add(type*, type); \
    template type AtomicsOperations::And(type*, type); \
    template type AtomicsOperations::CompareExchange(type*, type, type); \
    template type AtomicsOperations::Exchange(type*, type); \
    template type AtomicsOperations::Or(type*, type); \
    template type AtomicsOperations::Sub(type*, type); \
    template type AtomicsOperations::Xor(type*, type); \

ExplicitImplementation(int8);
ExplicitImplementation(uint8);
ExplicitImplementation(int16);
ExplicitImplementation(uint16);
ExplicitImplementation(int32);
ExplicitImplementation(uint32);
ExplicitImplementation(int64);

};
