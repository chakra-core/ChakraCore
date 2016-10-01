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

    static bool JITJavascriptString::Equals(Js::Var aLeft, Js::Var aRight)
    {
        return Js::JavascriptStringHelpers<JITJavascriptString>::Equals(aLeft, aRight);
    }

    static bool JITJavascriptString::Is(Js::Var var)
    {
        JITRecyclableObject * jitObj = reinterpret_cast<JITRecyclableObject*>(var);
        return jitObj->GetTypeId() == Js::TypeIds_String;
    }

    static JITJavascriptString * JITJavascriptString::FromVar(Js::Var var)
    {
        JITJavascriptString * jitString = reinterpret_cast<JITJavascriptString*>(var);
#if DBG
        Assert(Is(var));
        Assert(jitString->GetTypeId() == Js::TypeIds_String);
        if (!JITManager::GetJITManager()->IsOOPJITEnabled())
        {
            Js::JavascriptString * fullString = Js::JavascriptString::FromVar(var);
            // ensure layouts are same
            Assert(fullString->GetLength() == jitString->GetLength());
            Assert(wmemcmp(fullString->GetString(), jitString->GetString(), jitString->GetLength()) == 0);
        }
#endif
        return jitString;
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
