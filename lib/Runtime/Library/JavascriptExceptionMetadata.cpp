//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#include "JavascriptExceptionMetadata.h"

namespace Js {
    SimplePropertyDescriptor const ExceptionMetadataPropertyDescriptors[6] =
    {
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::exception), PropertyWritable),
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::source), PropertyWritable),
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::line), PropertyWritable),
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::column), PropertyWritable),
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::length), PropertyWritable),
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::url), PropertyWritable)
    };


    SimpleTypeHandler<6> JavascriptExceptionMetadata::ExceptionMetadataTypeHandler(NO_WRITE_BARRIER_TAG(ExceptionMetadataPropertyDescriptors));

    Var JavascriptExceptionMetadata::CreateMetadataVar(ScriptContext * scriptContext) {
        DynamicType* exceptionMetadataType = DynamicType::New(scriptContext, TypeIds_Object,
            scriptContext->GetLibrary()->GetNull(), nullptr, &JavascriptExceptionMetadata::ExceptionMetadataTypeHandler, true, true);
        return DynamicObject::New(scriptContext->GetRecycler(), exceptionMetadataType);
    }

    void JavascriptExceptionMetadata::PopulateMetadataFromCompileException(Var metadata, Var exception, ScriptContext * scriptContext) {
        Js::Var var;

        var = Js::JavascriptOperators::ToNumber(
            Js::JavascriptOperators::OP_GetProperty(exception, Js::PropertyIds::line, scriptContext),
            scriptContext);
        Js::JavascriptOperators::OP_SetProperty(metadata, Js::PropertyIds::line, var, scriptContext);

        var = Js::JavascriptOperators::ToNumber(
            Js::JavascriptOperators::OP_GetProperty(exception, Js::PropertyIds::column, scriptContext),
            scriptContext);
        Js::JavascriptOperators::OP_SetProperty(metadata, Js::PropertyIds::column, var, scriptContext);

        var = Js::JavascriptOperators::ToNumber(
            Js::JavascriptOperators::OP_GetProperty(exception, Js::PropertyIds::length, scriptContext),
            scriptContext);
        Js::JavascriptOperators::OP_SetProperty(metadata, Js::PropertyIds::length, var, scriptContext);

        var = Js::JavascriptConversion::ToString(
            Js::JavascriptOperators::OP_GetProperty(exception, Js::PropertyIds::source, scriptContext),
            scriptContext);
        Js::JavascriptOperators::OP_SetProperty(metadata, Js::PropertyIds::source, var, scriptContext);

        var = Js::JavascriptConversion::ToString(
            Js::JavascriptOperators::OP_GetProperty(exception, Js::PropertyIds::url, scriptContext),
            scriptContext);
        Js::JavascriptOperators::OP_SetProperty(metadata, Js::PropertyIds::url, var, scriptContext);
    }

    bool JavascriptExceptionMetadata::PopulateMetadataFromException(Var metadata, JavascriptExceptionObject * recordedException, ScriptContext * scriptContext) {
        uint32 offset = recordedException->GetByteCodeOffset();
        FunctionBody * functionBody = recordedException->GetFunctionBody();
        ULONG line;
        LONG column;
        if (functionBody->GetUtf8SourceInfo()->GetIsLibraryCode() ||
            !functionBody->GetLineCharOffset(offset, &line, &column)) {
            line = 0;
            column = 0;
        }

        Js::Utf8SourceInfo* sourceInfo = functionBody->GetUtf8SourceInfo();
        sourceInfo->EnsureLineOffsetCache();
        LineOffsetCache *cache = sourceInfo->GetLineOffsetCache();

        if (line >= cache->GetLineCount())
        {
            return false;
        }

        uint32 nextLine;
        if (UInt32Math::Add(line, 1, &nextLine))
        {
            // Overflowed
            return false;
        }

        Js::JavascriptOperators::OP_SetProperty(metadata, Js::PropertyIds::line,
            Js::JavascriptNumber::New(line, scriptContext), scriptContext);

        Js::JavascriptOperators::OP_SetProperty(metadata, Js::PropertyIds::column,
            Js::JavascriptNumber::New(column, scriptContext), scriptContext);

        Js::JavascriptOperators::OP_SetProperty(metadata, Js::PropertyIds::length,
            Js::JavascriptNumber::New(0, scriptContext), scriptContext);

        Js::JavascriptOperators::OP_SetProperty(metadata, Js::PropertyIds::url,
            Js::JavascriptString::NewCopySz(functionBody->GetSourceContextInfo()->url, scriptContext), scriptContext);

        LPCUTF8 functionSource = sourceInfo->GetSource(_u("Jsrt::JsExperimentalGetAndClearExceptionWithMetadata"));

        charcount_t startByteOffset = 0;
        charcount_t endByteOffset = 0;
        charcount_t startCharOffset = 0;
        charcount_t endCharOffset = 0;

        
        startCharOffset = cache->GetCharacterOffsetForLine(line, &startByteOffset);

        if (nextLine >= cache->GetLineCount())
        {
            endByteOffset = functionBody->LengthInBytes();
            endCharOffset = functionBody->LengthInChars();
        }
        else
        {
            endCharOffset = cache->GetCharacterOffsetForLine(nextLine, &endByteOffset);

            // The offsets above point to the start of the following line,
            // while we need to find the end of the current line.
            // To do so, just step back over the preceeding newline character(s)
            if (functionSource[endByteOffset - 1] == _u('\n'))
            {
                endCharOffset--;
                endByteOffset--;
                // This may have been \r\n
                if (endByteOffset > 0 && functionSource[endByteOffset - 1] == _u('\r'))
                {
                    endCharOffset--;
                    endByteOffset--;
                }
            }
            else
            {
                utf8::DecodeOptions options = utf8::doAllowThreeByteSurrogates;
                LPCUTF8 potentialNewlineStart = functionSource + endByteOffset - 3;
                char16 decodedCharacter = utf8::Decode(potentialNewlineStart, functionSource + endByteOffset, options);
                if (decodedCharacter == 0x2028 || decodedCharacter == 0x2029)
                {
                    endCharOffset--;
                    endByteOffset -= 3;
                }
                else if (functionSource[endByteOffset - 1] == _u('\r'))
                {
                    endCharOffset--;
                    endByteOffset--;
                }
                else
                {
                    AssertMsg(FALSE, "Line ending logic out of sync between Js::JavascriptExceptionMetadata and Js::LineOffsetCache::GetCharacterOffsetForLine");
                    return false;
                }
            }
        }

        LPCUTF8 functionStart = functionSource + startByteOffset;

        Js::BufferStringBuilder builder(endCharOffset - startCharOffset, scriptContext);
        utf8::DecodeOptions options = sourceInfo->IsCesu8() ? utf8::doAllowThreeByteSurrogates : utf8::doDefault;
        utf8::DecodeUnitsInto(builder.DangerousGetWritableBuffer(), functionStart, functionSource + endByteOffset, options);

        Js::JavascriptOperators::OP_SetProperty(metadata, Js::PropertyIds::source,
            builder.ToString(), scriptContext);

        return true;
    }
}