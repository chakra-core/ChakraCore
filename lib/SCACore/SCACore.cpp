//---------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#include "SCACorePch.h"

namespace Js
{
    namespace SCACore
    {
        HRESULT Serializer::SetTransferableVars(Var *vars, size_t count)
        {
            if (m_transferableHolder != nullptr)
            {
                Assert(false);
                return E_FAIL;
            }
            else if (count > 0)
            {
                for (uint i = 0; i < count; i++)
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

                m_transferableHolder = HeapNewNoThrow(TransferablesHolder, vars, count);
                if (m_transferableHolder == nullptr)
                {
                    return E_OUTOFMEMORY;
                }
                m_transferableHolder->AddRef();
            }
            return S_OK;
        }

        bool Serializer::WriteValue(Var rootObject)
        {
            ScriptContext *scriptContext = m_streamWriter.GetScriptContext();
            BEGIN_JS_RUNTIME_CALL(scriptContext)
            {
                Var *transferableVars = m_transferableHolder ? m_transferableHolder->GetVars() : nullptr;
                size_t transferableCount = m_transferableHolder ? m_transferableHolder->GetTranferableCount() : 0;
                auto sharedContentList = m_transferableHolder ? m_transferableHolder->GetSharedContentsList() : nullptr;

                Js::SCASerializationEngine::Serialize(rootObject, &m_streamWriter, transferableVars, transferableCount, sharedContentList);
            }
            END_JS_RUNTIME_CALL(scriptContext)
            return true;
        }

        void *Serializer::GetTransferableHolder()
        {
            return m_transferableHolder;
        }

        bool Serializer::DetachArrayBuffer()
        {
            if (m_transferableHolder)
            {
                ScriptContext *scriptContext = m_streamWriter.GetScriptContext();
                BEGIN_JS_RUNTIME_CALL(scriptContext)
                {
                    m_transferableHolder->DetachAll();
                }
                END_JS_RUNTIME_CALL(scriptContext)
            }
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
                returnedValue = Js::SCADeserializationEngine::Deserialize(&m_streamReader, m_transferableHolder);
                if (m_transferableHolder)
                {
                    ULONG ref = m_transferableHolder->Release();
                    Assert(ref == 0);
                }
            }
            END_JS_RUNTIME_CALL(scriptContext)
            return returnedValue;
        }
    }

}


