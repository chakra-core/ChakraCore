//---------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

namespace Js
{
    namespace SCACore
    {
        class Serializer
        {
        public:
            Serializer(ScriptContext *scriptContext, HostStream *stream)
                : m_streamWriter(scriptContext, stream),
                m_transferableHolder(nullptr)
            {

            }

            HRESULT SetTransferableVars(Var *vars, size_t count);

            void WriteRawBytes(const void* source, size_t length);
            bool WriteValue(Var rootObject);
            bool DetachArrayBuffer();
            void *GetTransferableHolder();

            bool Release(byte** data, size_t *dataLength);

        private:
            StreamWriter m_streamWriter;
            TransferablesHolder* m_transferableHolder;
        };

        class Deserializer
        {
        public:
            Deserializer(void *data, size_t length, void *holder, ScriptContext *scriptContext)
                : m_streamReader(scriptContext, (byte*)data, length),
                m_transferableHolder((TransferablesHolder*)holder)
            {
            }

            bool ReadRawBytes(size_t length, void **data);
			bool ReadBytes(size_t length, void **data);
			Var ReadValue();

        private:
            StreamReader m_streamReader;
            TransferablesHolder* m_transferableHolder;
        };
    }

}

