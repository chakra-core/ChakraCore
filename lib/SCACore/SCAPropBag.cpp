//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "SCACorePch.h"


#define IfFailGoto(expr, label) \
    do                          \
    {                           \
        hr = (expr);            \
        if (FAILED (hr))        \
        {                       \
            goto label;         \
        }                       \
    } while (FALSE)             \

#define IfFailGo(expr) IfFailGoto(expr, Error)

namespace Js
{
    SCAPropBag::SCAPropBag(ScriptContext* scriptContext)
        : ScriptContextHolder(scriptContext), m_refCount(1)
    {
        Recycler* recycler = GetScriptContext()->GetRecycler();
        m_properties.Root(RecyclerNew(recycler, PropertyDictionary, recycler), recycler);        
    }

    SCAPropBag::~SCAPropBag()
    {
        Recycler* recycler = GetScriptContext()->GetRecycler();
        m_properties.Unroot(recycler);
    }

    void SCAPropBag::CreateInstance(ScriptContext* scriptContext, SCAPropBag** ppInstance)
    {
        *ppInstance = HeapNew(SCAPropBag, scriptContext);
    }

    STDMETHODIMP_(ULONG) SCAPropBag::AddRef()
    {
        Assert(m_refCount > 0);
        return InterlockedIncrement(&m_refCount);
    }

    STDMETHODIMP_(ULONG) SCAPropBag::Release()
    {
        Assert(m_refCount > 0);
        ULONG ref = InterlockedDecrement(&m_refCount);
        if (ref == 0)
        {
            HeapDelete(this);
        }
        return ref;
    }

    STDMETHODIMP SCAPropBag::QueryInterface(REFIID riid, void** ppv)
    {
        if (ppv == nullptr)
        {
            return E_POINTER;
        }

        if (IsEqualGUID(riid, IID_IUnknown))
        {
            *ppv = static_cast<IUnknown*>(this);
            AddRef();
            return S_OK;
        }

        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT SCAPropBag::Add(LPCWSTR name, Var value)
    {
        HRESULT hr = S_OK;
        ScriptContext* scriptContext = GetScriptContext();

        BEGIN_TRANSLATE_EXCEPTION_AND_ERROROBJECT_TO_HRESULT
        {
            BEGIN_JS_RUNTIME_CALL_EX(scriptContext, false)
            {
                charcount_t len;
                IfFailGo(SizeTToUInt(wcslen(name), &len));
                IfFailGo(InternalAdd(name, len, value));
Error:
                ; // Fall through
            }
            END_JS_RUNTIME_CALL(scriptContext);
        }
        END_TRANSLATE_EXCEPTION_AND_ERROROBJECT_TO_HRESULT(hr);

        return hr;
    }

    HRESULT SCAPropBag::Get(LPCWSTR name, Var* pValue)
    {
        HRESULT hr = S_OK;
        ScriptContext* scriptContext = GetScriptContext();

        BEGIN_JS_RUNTIME_CALL_EX(scriptContext, false)
        {
            int len;
            IfFailGo(SizeTToInt(wcslen(name), &len));

            if (!m_properties->TryGetValue(InternalString(name, len), pValue))
            {
                hr = E_FAIL;
            }
        }
        END_JS_RUNTIME_CALL(scriptContext);
Error:
        return hr;
    }

    HRESULT SCAPropBag::InternalAdd(LPCWSTR name, charcount_t len, Var value)
    {
        char16* buf = nullptr;
        HRESULT hr = S_OK;
        ScriptContext* scriptContext = GetScriptContext();
        Recycler* recycler = scriptContext->GetRecycler();
        charcount_t fullLen;
        charcount_t byteLen;
        
        // fullLen == len + 1
        IfFailGo(UIntAdd(len, 1, &fullLen));
        
        // byte length (excluding null terminator)
        IfFailGo(UIntMult(len, sizeof(char16), &byteLen));

        // Make a copy of name
        buf = RecyclerNewArrayLeaf(recycler, char16, fullLen);
        js_memcpy_s(buf, byteLen, name, byteLen);
        buf[len] = _u('\0');

        // Add to the property bag
        IfFailGo(InternalAddNoCopy(buf, len, value));

Error:
        return hr;
    }

    HRESULT SCAPropBag::InternalAddNoCopy(LPCWSTR name, charcount_t len, Var value)
    {
        HRESULT hr = S_OK;

        int intLen;
        IfFailGo(UIntToInt(len, &intLen));
        m_properties->Item(InternalString(name, intLen), value);

Error:
        return hr;
    }

    bool SCAPropBag::PropBagEnumerator::MoveNext()
    {
        while (++m_curIndex < m_properties->Count())
        {
            if (m_properties->GetValueAt(m_curIndex))
            {
                return true;
            }
        }
        return false;
    }
}
