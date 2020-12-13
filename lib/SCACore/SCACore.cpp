//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "SCACorePch.h"

namespace Js
{
    namespace SCACore
    {
        HRESULT ValidateTransferableVars(Var *vars, size_t count)
        {
            for (size_t i = 0; i < count; i++)
            {
                Js::TypeId typeId = Js::JavascriptOperators::GetTypeId(vars[i]);
                if (typeId != TypeIds_ArrayBuffer)
                {

                    AssertMsg(false, "These should have been filtered out by the host.");
                    return E_SCA_TRANSFERABLE_UNSUPPORTED;
                }

                if (Js::JavascriptOperators::IsObjectDetached(vars[i]))
                {
                    return E_SCA_TRANSFERABLE_NEUTERED;
                }
            }
            return S_OK;
        }


        HRESULT Serializer::SetTransferableVars(Var *vars, size_t count)
        {
            if (m_transferableVars != nullptr)
            {
                Assert(false);
                return E_FAIL;
            }
            else if (count > 0)
            {
                HRESULT hr = ValidateTransferableVars(vars, count);
                if (hr != S_OK)
                {
                    return hr;
                }
                m_transferableVars = vars;
                m_cTransferableVars = count;
            }
            return S_OK;
        }

        bool Serializer::WriteValue(Var rootObject)
        {
            ScriptContext *scriptContext = m_streamWriter.GetScriptContext();
            BEGIN_JS_RUNTIME_CALL(scriptContext)
            {
                Js::SCASerializationEngine::Serialize(rootObject, &m_streamWriter, m_transferableVars, m_cTransferableVars, nullptr /*TBD*/);
            }
            END_JS_RUNTIME_CALL(scriptContext)
                return true;
        }

        bool Serializer::DetachArrayBuffer()
        {
            Assert(false);
            return true;
        }

        void Serializer::WriteRawBytes(const void* source, size_t length)
        {
            ScriptContext *scriptContext = m_streamWriter.GetScriptContext();
            BEGIN_JS_RUNTIME_CALL(scriptContext)
            {
                m_streamWriter.Write(source, length);
            }
            END_JS_RUNTIME_CALL(scriptContext)
        }

        bool Serializer::Release(byte** data, size_t *dataLength)
        {
            *data = m_streamWriter.GetBuffer();
            *dataLength = m_streamWriter.GetLength();
            return true;
        }

        bool Deserializer::ReadRawBytes(size_t length, void **data)
        {
            m_streamReader.ReadRawBytes(data, length);
            return true;
        }

        bool Deserializer::ReadBytes(size_t length, void **data)
        {
            m_streamReader.Read(*data, length);
            return true;
        }

        Var Deserializer::ReadValue()
        {
            Var returnedValue = nullptr;
            ScriptContext *scriptContext = m_streamReader.GetScriptContext();
            BEGIN_JS_RUNTIME_CALL(scriptContext)
            {
                returnedValue = Js::SCADeserializationEngine::Deserialize(&m_streamReader, m_transferableVars, m_cTransferableVars);
            }
            END_JS_RUNTIME_CALL(scriptContext)
                return returnedValue;
        }

        HRESULT Deserializer::SetTransferableVars(Var *vars, size_t count)
        {
            if (m_transferableVars != nullptr)
            {
                Assert(false);
                return E_FAIL;
            }
            else if (count > 0)
            {
                HRESULT hr = ValidateTransferableVars(vars, count);
                if (hr != S_OK)
                {
                    return hr;
                }
                m_transferableVars = vars;
                m_cTransferableVars = count;
            }
            return S_OK;
        }

    }

}


