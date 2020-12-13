//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// shim out common fallbacks
#ifndef GlobalBuiltInConstructor
#define GlobalBuiltInConstructor(Ctor)
#endif

#ifndef GlobalMathBuiltIn
#define GlobalMathBuiltIn(method)
#endif

#ifndef GlobalBuiltIn
#define GlobalBuiltIn(class, method)
#endif

#ifndef BuiltInRaiseException
#define BuiltInRaiseException(T, id)
#endif

#ifndef BuiltInRaiseException1
#define BuiltInRaiseException1(T, id) BuiltInRaiseException(T, id)
#endif

#ifndef BuiltInRaiseException1
#define BuiltInRaiseException1(T, id) BuiltInRaiseException(T, id)
#endif

#ifndef BuiltInRaiseException2
#define BuiltInRaiseException2(T, id) BuiltInRaiseException(T, id)
#endif

#ifndef BuiltInRaiseException3
#define BuiltInRaiseException3(T, id) BuiltInRaiseException(T, id##_3)
#endif

#ifndef EngineInterfaceBuiltIn2
#define EngineInterfaceBuiltIn2(propID, method)
#endif

#ifndef EngineInterfaceBuiltIn
#define EngineInterfaceBuiltIn(name) EngineInterfaceBuiltIn2(builtIn##name, name)
#endif

GlobalBuiltInConstructor(Boolean)
GlobalBuiltInConstructor(Object)
GlobalBuiltInConstructor(Number)
GlobalBuiltInConstructor(RegExp)
GlobalBuiltInConstructor(String)
GlobalBuiltInConstructor(Date)
GlobalBuiltInConstructor(Error) // TODO(jahorto): consider deleting (currently used by WinRT Promises)
GlobalBuiltInConstructor(Map) // TODO(jahorto): consider deleting (when do we need a Map over an object?)
GlobalBuiltInConstructor(Symbol)

GlobalMathBuiltIn(Abs)
GlobalMathBuiltIn(Floor)
GlobalMathBuiltIn(Pow)

GlobalBuiltIn(JavascriptObject, DefineProperty)
GlobalBuiltIn(JavascriptObject, GetPrototypeOf)
GlobalBuiltIn(JavascriptObject, IsExtensible)
GlobalBuiltIn(JavascriptObject, GetOwnPropertyNames)
GlobalBuiltIn(JavascriptObject, HasOwnProperty)
GlobalBuiltIn(JavascriptObject, Keys)
GlobalBuiltIn(JavascriptObject, Create)
GlobalBuiltIn(JavascriptObject, GetOwnPropertyDescriptor)
GlobalBuiltIn(JavascriptObject, PreventExtensions)

GlobalBuiltIn(JavascriptArray, Push) // TODO(jahorto): consider deleting (trivially implementable in JS)
GlobalBuiltIn(JavascriptArray, Join)
GlobalBuiltIn(JavascriptArray, Map)
GlobalBuiltIn(JavascriptArray, Reduce)
GlobalBuiltIn(JavascriptArray, Slice)
GlobalBuiltIn(JavascriptArray, Concat)

GlobalBuiltIn(JavascriptFunction, Bind)
GlobalBuiltIn(JavascriptFunction, Apply)

GlobalBuiltIn(JavascriptDate, GetDate)
GlobalBuiltIn(JavascriptDate, Now)

GlobalBuiltIn(JavascriptString, Replace)
GlobalBuiltIn(JavascriptString, ToLowerCase)
GlobalBuiltIn(JavascriptString, ToUpperCase)
GlobalBuiltIn(JavascriptString, Split)
GlobalBuiltIn(JavascriptString, Substring)
GlobalBuiltIn(JavascriptString, Repeat)
GlobalBuiltIn(JavascriptString, IndexOf)

GlobalBuiltIn(GlobalObject, IsFinite) // TODO(jahorto): consider switching to Number.isFinite
GlobalBuiltIn(GlobalObject, IsNaN) // TODO(jahorto): consider switching to Number.isNaN
GlobalBuiltIn(GlobalObject, Eval) // TODO(jahorto): consider deleting (currently used by WinRT Promises)

BuiltInRaiseException(TypeError, NeedObject)
BuiltInRaiseException2(TypeError, ObjectIsAlreadyInitialized)
BuiltInRaiseException3(RangeError, OptionValueOutOfRange)
BuiltInRaiseException(RangeError, OptionValueOutOfRange)
BuiltInRaiseException1(TypeError, NeedObjectOrString)
BuiltInRaiseException1(RangeError, LocaleNotWellFormed)
BuiltInRaiseException1(TypeError, This_NullOrUndefined)
BuiltInRaiseException1(TypeError, NotAConstructor)
BuiltInRaiseException1(TypeError, ObjectIsNonExtensible)
BuiltInRaiseException1(TypeError, LengthIsTooBig)
BuiltInRaiseException1(TypeError, NonObjectFromIterable)
BuiltInRaiseException1(TypeError, EmptyArrayAndInitValueNotPresent)
BuiltInRaiseException2(TypeError, NeedObjectOfType)
BuiltInRaiseException1(RangeError, InvalidCurrencyCode)
BuiltInRaiseException(TypeError, MissingCurrencyCode)
BuiltInRaiseException(RangeError, InvalidDate)
BuiltInRaiseException1(TypeError, FunctionArgument_NeedFunction)

EngineInterfaceBuiltIn2(getErrorMessage, GetErrorMessage)
EngineInterfaceBuiltIn2(logDebugMessage, LogDebugMessage)
EngineInterfaceBuiltIn2(tagPublicLibraryCode, TagPublicLibraryCode)
EngineInterfaceBuiltIn(SetPrototype)
EngineInterfaceBuiltIn(GetArrayLength)
EngineInterfaceBuiltIn(RegexMatch)

#undef GlobalBuiltInConstructor
#undef GlobalMathBuiltIn
#undef GlobalBuiltIn
#undef BuiltInRaiseException
#undef BuiltInRaiseException1
#undef BuiltInRaiseException1
#undef BuiltInRaiseException2
#undef BuiltInRaiseException3
#undef EngineInterfaceBuiltIn2
#undef EngineInterfaceBuiltIn
