//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
namespace Js
{
    //
    // Implements ISCAPropBag.
    //
    class SCAPropBag sealed :
        public ScriptContextHolder,
        public IUnknown
    {
        typedef JsUtil::BaseDictionary<InternalString, Var, RecyclerNonLeafAllocator, PowerOf2SizePolicy,
            DefaultComparer, JsUtil::DictionaryEntry> PropertyDictionary;

    private:
        RecyclerRootPtr<PropertyDictionary> m_properties;
        ULONG m_refCount;

        SCAPropBag(ScriptContext* scriptContext);
        HRESULT InternalAdd(LPCWSTR name, charcount_t len, Var value);

    public:
        ~SCAPropBag();
        static void CreateInstance(ScriptContext* scriptContext, SCAPropBag** ppInstance);

        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();
        STDMETHODIMP QueryInterface(REFIID riid, void** ppv);

        HRESULT Add(LPCWSTR name, Var value);
        HRESULT Get(LPCWSTR name, Var* pValue);

        HRESULT InternalAddNoCopy(LPCWSTR name, charcount_t len, Var value);

        //
        // PropBag property enumerator for WriteObjectProperties.
        //
        class PropBagEnumerator
        {
        private:
            PropertyDictionary* m_properties;
            int m_curIndex;

        public:
            PropBagEnumerator(SCAPropBag* propBag)
                : m_properties(propBag->m_properties), m_curIndex(-1)
            {
            }

            bool MoveNext();

            const char16* GetNameString() const
            {
                return m_properties->GetKeyAt(m_curIndex).GetBuffer();
            }

            charcount_t GetNameLength() const
            {
                return m_properties->GetKeyAt(m_curIndex).GetLength();
            }

            Var GetValue() const
            {
                return m_properties->GetValueAt(m_curIndex);
            }
        };
    };
}
