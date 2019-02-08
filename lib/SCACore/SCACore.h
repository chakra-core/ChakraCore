//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js
{
    namespace SCACore
    {
        class Serializer
        {
        public:
            Serializer(ScriptContext *scriptContext, HostStream *stream)
                : m_streamWriter(scriptContext, stream)
            {
            }

            HRESULT SetTransferableVars(Var *vars, size_t count);

            void WriteRawBytes(const void* source, size_t length);
            bool WriteValue(Var rootObject);
            bool DetachArrayBuffer();

            bool Release(byte** data, size_t *dataLength);

        private:
            StreamWriter m_streamWriter;
            Var* m_transferableVars = nullptr;
            size_t m_cTransferableVars = 0;
        };

        class Deserializer
        {
        public:
            Deserializer(void *data, size_t length, ScriptContext *scriptContext, HostReadStream *stream)
                : m_streamReader(scriptContext, (byte*)data, length, stream)
            {
            }

            HRESULT SetTransferableVars(Var *vars, size_t count);

            bool ReadRawBytes(size_t length, void **data);
            bool ReadBytes(size_t length, void **data);
            Var ReadValue();

        private:
            StreamReader m_streamReader;
            Var* m_transferableVars = nullptr;
            size_t m_cTransferableVars = 0;
        };
    }

}

