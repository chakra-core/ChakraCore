//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class ISourceHolder;
namespace Js
{
    template <typename TLoadCallback, typename TUnloadCallback>
    class JsrtSourceHolder sealed : public ISourceHolder
    {
    private:
        enum MapRequestFor { Source = 1, Length = 2 };

        FieldNoBarrier(TLoadCallback) scriptLoadCallback;
        FieldNoBarrier(TUnloadCallback) scriptUnloadCallback;
        Field(JsSourceContext) sourceContext;

#ifndef NTBUILD
        Field(JsValueRef) mappedScriptValue;
        Field(JsValueRef) mappedSerializedScriptValue;
#endif
        Field(utf8char_t const *) mappedSource;
        Field(size_t) mappedSourceByteLength;
        Field(size_t) mappedAllocLength;

        // Wrapper methods with Asserts to ensure that we aren't trying to access unmapped source
        utf8char_t const * GetMappedSource()
        {
            AssertMsg(mappedSource != nullptr, "Our mapped source is nullptr, isSourceMapped (Assert above) should be false.");
            AssertMsg(scriptUnloadCallback != nullptr, "scriptUnloadCallback is null, this means that this object has been finalized.");
            return mappedSource;
        };

        size_t GetMappedSourceLength()
        {
            AssertMsg(mappedSource != nullptr, "Our mapped source is nullptr, isSourceMapped (Assert above) should be false.");
            AssertMsg(scriptUnloadCallback != nullptr, "scriptUnloadCallback is null, this means that this object has been finalized.");
            return mappedSourceByteLength;
        };

        void EnsureSource(MapRequestFor requestedFor, const WCHAR* reasonString);

    public:
        JsrtSourceHolder(_In_ TLoadCallback scriptLoadCallback,
            _In_ TUnloadCallback scriptUnloadCallback,
            _In_ JsSourceContext sourceContext,
            JsValueRef serializedScriptValue = nullptr) :
            scriptLoadCallback(scriptLoadCallback),
            scriptUnloadCallback(scriptUnloadCallback),
            sourceContext(sourceContext),
#ifndef NTBUILD
            mappedScriptValue(nullptr),
            mappedSerializedScriptValue(serializedScriptValue),
#endif
            mappedSourceByteLength(0),
            mappedSource(nullptr)
        {
            AssertMsg(scriptLoadCallback != nullptr, "script load callback given is null.");
            AssertMsg(scriptUnloadCallback != nullptr, "script unload callback given is null.");
        };

        virtual bool IsEmpty() override
        {
            return false;
        }

        // Following two methods do not attempt any source mapping
        LPCUTF8 GetSourceUnchecked()
        {
            return this->GetMappedSource();
        }

        // Following two methods are calls to EnsureSource before attempting to get the source
        virtual LPCUTF8 GetSource(const WCHAR* reasonString) override
        {
            this->EnsureSource(MapRequestFor::Source, reasonString);
            return this->GetMappedSource();
        }

        virtual size_t GetByteLength(const WCHAR* reasonString) override
        {
            this->EnsureSource(MapRequestFor::Length, reasonString);
            return this->GetMappedSourceLength();
        }

        virtual void Finalize(bool isShutdown) override;

        virtual void Dispose(bool isShutdown) override
        {
        }

        virtual void Mark(Recycler * recycler) override
        {
        }

        virtual bool Equals(ISourceHolder* other) override
        {
            return this == other ||
                (this->GetByteLength(_u("Equal Comparison")) == other->GetByteLength(_u("Equal Comparison"))
                    && (this->GetSource(_u("Equal Comparison")) == other->GetSource(_u("Equal Comparison"))
                        || memcmp(this->GetSource(_u("Equal Comparison")), other->GetSource(_u("Equal Comparison")), this->GetByteLength(_u("Equal Comparison"))) == 0));
        }

        virtual ISourceHolder* Clone(ScriptContext *scriptContext) override
        {
            return RecyclerNewFinalized(scriptContext->GetRecycler(), JsrtSourceHolder, this->scriptLoadCallback, this->scriptUnloadCallback, this->sourceContext);
        }

        virtual int GetHashCode() override
        {
            LPCUTF8 source = GetSource(_u("Hash Code Calculation"));
            size_t byteLength = GetByteLength(_u("Hash Code Calculation"));
            Assert(byteLength < MAXUINT32);
            return JsUtil::CharacterBuffer<utf8char_t>::StaticGetHashCode(source, (charcount_t)byteLength);
        }


        virtual bool IsDeferrable() override
        {
            return !PHASE_OFF1(Js::DeferSourceLoadPhase);
        }
    };
}
