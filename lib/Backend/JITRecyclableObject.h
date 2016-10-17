//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class JITRecyclableObject
{
private:
    intptr_t remoteVTable;
    Js::TypeId * typeId;
public:
    static bool Is(Js::Var var)
    {
        return !Js::TaggedNumber::Is(var);
    }

    Js::TypeId GetTypeId() const
    {
        return *typeId;
    }
};

class JITJavascriptString : JITRecyclableObject
{
private:
    const char16* m_pszValue;
    charcount_t m_charLength;
public:
    const char16* GetString() const
    {
        return m_pszValue;
    }

    charcount_t GetLength() const
    {
        return m_charLength;
    }

    static bool Equals(Js::Var aLeft, Js::Var aRight)
    {
        return Js::JavascriptStringHelpers<JITJavascriptString>::Equals(aLeft, aRight);
    }

    static bool Is(Js::Var var)
    {
        if (!JITRecyclableObject::Is(var))
        {
            return false;
        }
        JITRecyclableObject * jitObj = reinterpret_cast<JITRecyclableObject*>(var);
        return jitObj->GetTypeId() == Js::TypeIds_String;
    }

    static JITJavascriptString * FromVar(Js::Var var)
    {
        Assert(offsetof(JITJavascriptString, m_pszValue) == Js::JavascriptString::GetOffsetOfpszValue());
        Assert(offsetof(JITJavascriptString, m_charLength) == Js::JavascriptString::GetOffsetOfcharLength());
        Assert(Is(var));

        return reinterpret_cast<JITJavascriptString*>(var);
    }
};

class JITJavascriptNumber : JITRecyclableObject
{
private:
    double value;

};

template <>
struct DefaultComparer<JITJavascriptString*>
{
    inline static bool Equals(JITJavascriptString * x, JITJavascriptString * y)
    {
        return Js::JavascriptStringHelpers<JITJavascriptString>::Equals(x, y);
    }

    inline static uint GetHashCode(JITJavascriptString * pStr)
    {
        return JsUtil::CharacterBuffer<char16>::StaticGetHashCode(pStr->GetString(), pStr->GetLength());
    }
};
