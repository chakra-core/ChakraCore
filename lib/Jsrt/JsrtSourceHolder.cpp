//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "JsrtPch.h"
#include "JsrtSourceHolder.h"

namespace Js
{
    template <typename TLoadCallback>
    class JsrtSourceHolderPolicy
    {
    };

#ifdef _WIN32  // JsSerializedScriptLoadSourceCallback is WIN32 only
    template <>
    class JsrtSourceHolderPolicy<JsSerializedScriptLoadSourceCallback>
    {
    public:
        typedef WCHAR TLoadCharType;

        // Helper function for converting a Unicode script to utf8.
        // If heapAlloc is true the returned buffer must be freed with HeapDelete.
        // Otherwise scriptContext must be provided and GCed object is
        // returned.
        static void ScriptToUtf8(_When_(heapAlloc, _In_opt_) _When_(!heapAlloc, _In_) Js::ScriptContext *scriptContext,
            _In_z_ const WCHAR *script, _Outptr_result_buffer_(*utf8Length) utf8char_t **utf8Script, _Out_ size_t *utf8Length,
            _Out_ size_t *scriptLength, _Out_opt_ size_t *utf8AllocLength, _In_ bool heapAlloc)
        {
            Assert(utf8Script != nullptr);
            Assert(utf8Length != nullptr);
            Assert(scriptLength != nullptr);

            *utf8Script = nullptr;
            *utf8Length = 0;
            *scriptLength = 0;

            if (utf8AllocLength != nullptr)
            {
                *utf8AllocLength = 0;
            }

            size_t length = wcslen(script);
            if (length > UINT_MAX)
            {
                Js::JavascriptError::ThrowOutOfMemoryError(nullptr);
            }

            // `length` should not be bigger than MAXLONG
            // UINT_MAX / 3 < MAXLONG
            size_t cbUtf8Buffer = ((UINT_MAX / 3) - 1 > length) ? (length + 1) * 3 : UINT_MAX;
            if (cbUtf8Buffer >= UINT_MAX)
            {
                Js::JavascriptError::ThrowOutOfMemoryError(nullptr);
            }

            if (!heapAlloc)
            {
                Assert(scriptContext != nullptr);
                *utf8Script = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), utf8char_t, cbUtf8Buffer);
            }
            else
            {
                *utf8Script = HeapNewArray(utf8char_t, cbUtf8Buffer);
            }

            *utf8Length = utf8::EncodeTrueUtf8IntoAndNullTerminate(*utf8Script, script, static_cast<charcount_t>(length));
            *scriptLength = length;

            if (utf8AllocLength != nullptr)
            {
                *utf8AllocLength = cbUtf8Buffer;
            }
        }

        static void FreeMappedSource(utf8char_t const * source, size_t allocLength)
        {
            HeapDeleteArray(allocLength, source);
        }
    };
#endif  // _WIN32

    template <typename TLoadCallback, typename TUnloadCallback>
    void JsrtSourceHolder<TLoadCallback, TUnloadCallback>::EnsureSource(MapRequestFor requestedFor, const WCHAR* reasonString)
    {
        if (this->mappedSource != nullptr)
        {
            return;
        }

        Assert(scriptLoadCallback != nullptr);
        Assert(this->mappedSource == nullptr);

        const typename JsrtSourceHolderPolicy<TLoadCallback>::TLoadCharType *source = nullptr;
        size_t sourceLength = 0;
        utf8char_t *utf8Source = nullptr;
        size_t utf8Length = 0;
        size_t utf8AllocLength = 0;

        if (!scriptLoadCallback(sourceContext, &source))
        {
            // Assume out of memory
            Js::JavascriptError::ThrowOutOfMemoryError(nullptr);
        }

        JsrtSourceHolderPolicy<TLoadCallback>::ScriptToUtf8(nullptr, source, &utf8Source, &utf8Length, &sourceLength, &utf8AllocLength, true);

        this->mappedSource = utf8Source;
        this->mappedSourceByteLength = utf8Length;
        this->mappedAllocLength = utf8AllocLength;

        this->scriptLoadCallback = nullptr;

#if ENABLE_DEBUG_CONFIG_OPTIONS
        AssertMsg(reasonString != nullptr, "Reason string for why we are mapping the source was not provided.");
        JS_ETW(EventWriteJSCRIPT_SOURCEMAPPING((uint32)wcslen(reasonString), reasonString, (ushort)requestedFor));
#endif
    }

    template <typename TLoadCallback, typename TUnloadCallback>
    void JsrtSourceHolder<TLoadCallback, TUnloadCallback>::Finalize(bool isShutdown)
    {
        if (scriptUnloadCallback == nullptr)
        {
            return;
        }

        scriptUnloadCallback(sourceContext);

        if (this->mappedSource != nullptr)
        {
            JsrtSourceHolderPolicy<TLoadCallback>::FreeMappedSource(
                this->mappedSource, this->mappedAllocLength);
            this->mappedSource = nullptr;
        }

        // Don't allow load or unload again after told to unload.
        scriptLoadCallback = nullptr;
        scriptUnloadCallback = nullptr;
        sourceContext = NULL;
    }

#ifndef NTBUILD // ChakraCore Only
    template <>
    class JsrtSourceHolderPolicy<JsSerializedLoadScriptCallback>
    {
    public:
        typedef JsValueRef TLoadCharType;

        static void ScriptToUtf8(_When_(heapAlloc, _In_opt_) _When_(!heapAlloc, _In_) Js::ScriptContext *scriptContext,
            _In_z_ const byte *script_, bool isUtf8, size_t length, _Outptr_result_buffer_(*utf8Length) utf8char_t **utf8Script, _Out_ size_t *utf8Length,
            _Out_ size_t *scriptLength, _Out_opt_ size_t *utf8AllocLength, _In_ bool heapAlloc)
        {
            if (isUtf8)
            {
                *utf8Script = (utf8char_t*)script_;
                *utf8Length = length;
                *scriptLength = length; // xplat-todo: incorrect for utf8

                if (utf8AllocLength)
                {
                    *utf8AllocLength = 0;
                }
            }
            else
            {
                const WCHAR *script = (const WCHAR*) script_;
                Assert(utf8Script != nullptr);
                Assert(utf8Length != nullptr);
                Assert(scriptLength != nullptr);

                *utf8Script = nullptr;
                *utf8Length = 0;
                *scriptLength = 0;

                if (utf8AllocLength != nullptr)
                {
                    *utf8AllocLength = 0;
                }

                size_t script_length = wcslen(script);
                if (script_length > UINT_MAX)
                {
                    Js::JavascriptError::ThrowOutOfMemoryError(nullptr);
                }

                // `script_length` should not be bigger than MAXLONG
                // UINT_MAX / 3 < MAXLONG
                size_t cbUtf8Buffer = ((UINT_MAX / 3) - 1 > script_length) ? (script_length + 1) * 3 : UINT_MAX;
                if (cbUtf8Buffer >= UINT_MAX)
                {
                    Js::JavascriptError::ThrowOutOfMemoryError(nullptr);
                }

                if (!heapAlloc)
                {
                    Assert(scriptContext != nullptr);
                    *utf8Script = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), utf8char_t, cbUtf8Buffer);
                }
                else
                {
                    *utf8Script = HeapNewArray(utf8char_t, cbUtf8Buffer);
                }

                *utf8Length = utf8::EncodeTrueUtf8IntoAndNullTerminate(*utf8Script,
                    script, static_cast<charcount_t>(script_length));
                *scriptLength = script_length;

                if (utf8AllocLength != nullptr)
                {
                    *utf8AllocLength = cbUtf8Buffer;
                }
            }
        }

        static void FreeMappedSource(utf8char_t const * source, size_t allocLength)
        {
            if (allocLength)
            {
                HeapDeleteArray(allocLength, source);
            }
        }
    };

    template <>
    void JsrtSourceHolder<JsSerializedLoadScriptCallback,
        JsSerializedScriptUnloadCallback>::EnsureSource(MapRequestFor requestedFor,
        const WCHAR* reasonString)
    {
        if (this->mappedSource != nullptr)
        {
            return;
        }

        Assert(scriptLoadCallback != nullptr);
        Assert(this->mappedSource == nullptr);

        JsValueRef scriptVal;
        JsParseScriptAttributes attributes = JsParseScriptAttributeNone;
        size_t sourceLength = 0;
        utf8char_t *utf8Source = nullptr;
        size_t utf8Length = 0;
        size_t utf8AllocLength = 0;

        Js::ScriptContext* scriptContext = JsrtContext::GetCurrent()->GetScriptContext();
        BEGIN_LEAVE_SCRIPT(scriptContext)
        if (!scriptLoadCallback(sourceContext, &scriptVal, &attributes))
        {
            // Assume out of memory
            Js::JavascriptError::ThrowOutOfMemoryError(nullptr);
        }
        END_LEAVE_SCRIPT(scriptContext);

        bool isExternalArray = Js::ExternalArrayBuffer::Is(scriptVal),
             isString = false;
        bool isUtf8   = !(attributes & JsParseScriptAttributeArrayBufferIsUtf16Encoded);

        if (!isExternalArray)
        {
            isString = Js::JavascriptString::Is(scriptVal);
            if (!isString)
            {
                Js::JavascriptError::ThrowOutOfMemoryError(nullptr);
                return;
            }
        }

        const byte* script = isExternalArray ?
            ((Js::ExternalArrayBuffer*)(scriptVal))->GetBuffer() :
            (const byte*)((Js::JavascriptString*)(scriptVal))->GetSz();
        const size_t cb = isExternalArray ?
            ((Js::ExternalArrayBuffer*)(scriptVal))->GetByteLength() :
            ((Js::JavascriptString*)(scriptVal))->GetLength();

        JsrtSourceHolderPolicy<JsSerializedLoadScriptCallback>::ScriptToUtf8(nullptr,
            script, isUtf8, cb, &utf8Source, &utf8Length, &sourceLength,
            &utf8AllocLength, true);

        this->mappedScriptValue = scriptVal;
        this->mappedSource = utf8Source;
        this->mappedSourceByteLength = utf8Length;
        this->mappedAllocLength = utf8AllocLength;

        this->scriptLoadCallback = nullptr;

#if ENABLE_DEBUG_CONFIG_OPTIONS
        AssertMsg(reasonString != nullptr, "Reason string for why we are mapping the source was not provided.");
        JS_ETW(EventWriteJSCRIPT_SOURCEMAPPING((uint32)wcslen(reasonString), reasonString, (ushort)requestedFor));
#endif
    }

    template <>
    void JsrtSourceHolder<JsSerializedLoadScriptCallback, JsSerializedScriptUnloadCallback>::Finalize(bool isShutdown)
    {
        if (this->mappedSource != nullptr)
        {
            JsrtSourceHolderPolicy<JsSerializedLoadScriptCallback>::FreeMappedSource(
                this->mappedSource, this->mappedAllocLength);
            this->mappedSource = nullptr;
        }
        this->mappedScriptValue = nullptr;
        this->mappedSerializedScriptValue = nullptr;

        // Don't allow load or unload again after told to unload.
        scriptLoadCallback = nullptr;
        scriptUnloadCallback = nullptr;
        sourceContext = NULL;
    }

template class JsrtSourceHolder<JsSerializedLoadScriptCallback, JsSerializedScriptUnloadCallback>;

#endif // NTBUILD

#ifdef _WIN32
template class JsrtSourceHolder<JsSerializedScriptLoadSourceCallback, JsSerializedScriptUnloadCallback>;
#endif // _WIN32
};
