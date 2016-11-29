//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
/// \mainpage Chakra Hosting API Reference
///
/// Chakra is Microsoft's JavaScript engine. It is an integral part of Internet Explorer but can
/// also be hosted independently by other applications. This reference describes the APIs available
/// to applications to host Chakra.
///

/// \file
/// \brief The Chakra Core hosting API.
///
/// This file contains a flat C API layer. This is the API exported by ChakraCore.dll.

#ifdef _MSC_VER
#pragma once
#endif // _MSC_VER

#ifndef _CHAKRACORE_H_
#define _CHAKRACORE_H_

#include "ChakraCommon.h"

#include "ChakraDebug.h"

typedef void* JsModuleRecord;

typedef enum JsParseModuleSourceFlags
{
    JsParseModuleSourceFlags_DataIsUTF16LE = 0x00000000,
    JsParseModuleSourceFlags_DataIsUTF8 = 0x00000001
} JsParseModuleSourceFlags;

typedef enum JsModuleHostInfoKind
{
    JsModuleHostInfo_Exception = 0x01,
    JsModuleHostInfo_HostDefined = 0x02,
    JsModuleHostInfo_NotifyModuleReadyCallback = 0x3,
    JsModuleHostInfo_FetchImportedModuleCallback = 0x4
} JsModuleHostInfoKind;

/// <summary>
///     User implemented callback to fetch additional imported modules.
/// </summary>
/// <remarks>
/// Notify the host to fetch the dependent module. This is the "import" part before HostResolveImportedModule in ES6 spec.
/// This notifies the host that the referencing module has the specified module dependency, and the host need to retrieve the module back.
/// </remarks>
/// <param name="referencingModule">The referencing module that is requesting the dependency modules.</param>
/// <param name="specifier">The specifier coming from the module source code.</param>
/// <param name="dependentModuleRecord">The ModuleRecord of the dependent module. If the module was requested before from other source, return the
///                           existing ModuleRecord, otherwise return a newly created ModuleRecord.</param>
/// <returns>
///     true if the operation succeeded, false otherwise.
/// </returns>
typedef JsErrorCode(CHAKRA_CALLBACK * FetchImportedModuleCallBack)(_In_ JsModuleRecord referencingModule, _In_ JsValueRef specifier, _Outptr_result_maybenull_ JsModuleRecord* dependentModuleRecord);

/// <summary>
///     User implemented callback to get notification when the module is ready.
/// </summary>
/// <remarks>
/// Notify the host after ModuleDeclarationInstantiation step (15.2.1.1.6.4) is finished. If there was error in the process, exceptionVar
/// holds the exception. Otherwise the referencingModule is ready and the host should schedule execution afterwards.
/// </remarks>
/// <param name="referencingModule</param>The referencing module that have finished running ModuleDeclarationInstantiation step.
/// <param name="exceptionVar">If nullptr, the module is successfully initialized and host should queue the execution job
///                           otherwise it's the exception object.</param>
/// <returns>
///     true if the operation succeeded, false otherwise.
/// </returns>
typedef JsErrorCode(CHAKRA_CALLBACK * NotifyModuleReadyCallback)(_In_opt_ JsModuleRecord referencingModule, _In_opt_ JsValueRef exceptionVar);

/// <summary>
///     Initialize a ModuleRecord from host
/// </summary>
/// <remarks>
///     Bootstrap the module loading process by creating a new module record.
/// </remarks>
/// <param name="referencingModule">The referencingModule as in HostResolveImportedModule (15.2.1.17). nullptr if this is the top level module.</param>
/// <param name="normalizedSpecifier">The host normalized specifier. This is the key to a unique ModuleRecord.</param>
/// <param name="moduleRecord">The new ModuleRecord created. The host should not try to call this API twice with the same normalizedSpecifier.
///                           chakra will return an existing ModuleRecord if the specifier was passed in before.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsInitializeModuleRecord(
    _In_opt_ JsModuleRecord referencingModule,
    _In_ JsValueRef normalizedSpecifier,
    _Outptr_result_maybenull_ JsModuleRecord* moduleRecord);

/// <summary>
///     Parse the module source
/// </summary>
/// <remarks>
/// This is basically ParseModule operation in ES6 spec. It is slightly different in that the ModuleRecord was initialized earlier, and passed in as an argument.
/// </remarks>
/// <param name="requestModule">The ModuleRecord that holds the parse tree of the source code.</param>
/// <param name="sourceContext">A cookie identifying the script that can be used by debuggable script contexts.</param>
/// <param name="script">The source script to be parsed, but not executed in this code.</param>
/// <param name="scriptLength">The source length of sourceText. The input might contain embedded null.</param>
/// <param name="sourceFlag">The type of the source code passed in. It could be UNICODE or utf8 at this time.</param>
/// <param name="exceptionValueRef">The error object if there is parse error.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsParseModuleSource(
    _In_ JsModuleRecord requestModule,
    _In_ JsSourceContext sourceContext,
    _In_ BYTE* script,
    _In_ unsigned int scriptLength,
    _In_ JsParseModuleSourceFlags sourceFlag,
    _Outptr_result_maybenull_ JsValueRef* exceptionValueRef);

/// <summary>
///     Execute module code.
/// </summary>
/// <remarks>
///     This method implements 15.2.1.1.6.5, "ModuleEvaluation" concrete method.
///     When this methid is called, the chakra engine should have notified the host that the module and all its dependent are ready to be executed.
///     One moduleRecord will be executed only once. Additional execution call on the same moduleRecord will fail.
/// </remarks>
/// <param name="requestModule">The module to be executed.</param>
/// <param name="result">The return value of the module.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsModuleEvaluation(
    _In_ JsModuleRecord requestModule,
    _Outptr_result_maybenull_ JsValueRef* result);

/// <summary>
///     Set the host info for the specified module.
/// </summary>
/// <param name="requestModule">The request module.</param>
/// <param name="moduleHostInfo">The type of host info to be set.</param>
/// <param name="hostInfo">The host info to be set.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsSetModuleHostInfo(
    _In_ JsModuleRecord requestModule,
    _In_ JsModuleHostInfoKind moduleHostInfo,
    _In_ void* hostInfo);

/// <summary>
///     Retrieve the host info for the specified module.
/// </summary>
/// <param name="requestModule">The request module.</param>
/// <param name="moduleHostInfo">The type of host info to get.</param>
/// <param name="hostInfo">The host info to be retrieved.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsGetModuleHostInfo(
    _In_  JsModuleRecord requestModule,
    _In_ JsModuleHostInfoKind moduleHostInfo,
    _Outptr_result_maybenull_ void** hostInfo);

#ifndef NTBUILD
/// <summary>
///     Called by the runtime to load the source code of the serialized script.
/// </summary>
/// <param name="sourceContext">The context passed to Js[Parse|Run]SerializedScriptCallback</param>
/// <param name="script">The script returned.</param>
/// <returns>
///     true if the operation succeeded, false otherwise.
/// </returns>
typedef bool (CHAKRA_CALLBACK * JsSerializedLoadScriptCallback)
    (JsSourceContext sourceContext, _Out_ JsValueRef *value,
    _Out_ JsParseScriptAttributes *parseAttributes);

/// <summary>
///     Create JavascriptString variable from C string
/// </summary>
/// <remarks>
///     <para>
///         C string is expected to be ASCII
///     </para>
/// </remarks>
/// <param name="content">Pointer to string memory.</param>
/// <param name="length">Number of bytes within the string</param>
/// <param name="value">JsValueRef representing the JavascriptString</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsCreateString(
        _In_ const char *content,
        _In_ size_t length,
        _Out_ JsValueRef *value);

/// <summary>
///     Create JavascriptString variable from Utf8 string
/// </summary>
/// <remarks>
///     <para>
///         Input string can be either ASCII or Utf8
///     </para>
/// </remarks>
/// <param name="content">Pointer to string memory.</param>
/// <param name="length">Number of bytes within the string</param>
/// <param name="value">JsValueRef representing the JavascriptString</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsCreateStringUtf8(
        _In_ const uint8_t *content,
        _In_ size_t length,
        _Out_ JsValueRef *value);

/// <summary>
///     Create JavascriptString variable from Utf16 string
/// </summary>
/// <remarks>
///     <para>
///         Expects Utf16 string
///     </para>
/// </remarks>
/// <param name="content">Pointer to string memory.</param>
/// <param name="length">Number of characters within the string</param>
/// <param name="value">JsValueRef representing the JavascriptString</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsCreateStringUtf16(
        _In_ const uint16_t *content,
        _In_ size_t length,
        _Out_ JsValueRef *value);

/// <summary>
///     Write JavascriptString value into C string buffer
/// </summary>
/// <remarks>
///     <para>
///         When size of the `buffer` is unknown,
///         `buffer` argument can be nullptr.
///         In that case, `written` argument will return the length needed.
///     </para>
///     <para>
///         when start is out of range or < 0, returns JsErrorInvalidArgument
///         and `written` will be equal to 0.
///         If calculated length is 0 (It can be due to string length or `start`
///         and length combination), then `written` will be equal to 0 and call
///         returns JsNoError
///     </para>
/// </remarks>
/// <param name="value">JavascriptString value</param>
/// <param name="start">start offset of buffer</param>
/// <param name="length">length to be written</param>
/// <param name="buffer">Pointer to buffer</param>
/// <param name="written">Total number of characters written</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsCopyString(
        _In_ JsValueRef value,
        _In_ int start,
        _In_ int length,
        _Out_opt_ char* buffer,
        _Out_opt_ size_t* written);

/// <summary>
///     Write JavascriptString value into Utf8 string buffer
/// </summary>
/// <remarks>
///     <para>
///         When size of the `buffer` is unknown,
///         `buffer` argument can be nullptr.
///         In that case, `written` argument will return the length needed.
///     </para>
/// </remarks>
/// <param name="value">JavascriptString value</param>
/// <param name="buffer">Pointer to buffer</param>
/// <param name="bufferSize">Buffer size</param>
/// <param name="written">Total number of characters written</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsCopyStringUtf8(
        _In_ JsValueRef value,
        _Out_opt_ uint8_t* buffer,
        _In_ size_t bufferSize,
        _Out_opt_ size_t* written);

/// <summary>
///     Write string value into Utf16 string buffer
/// </summary>
/// <remarks>
///     <para>
///         When size of the `buffer` is unknown,
///         `buffer` argument can be nullptr.
///         In that case, `written` argument will return the length needed.
///     </para>
///     <para>
///         when start is out of range or < 0, returns JsErrorInvalidArgument
///         and `written` will be equal to 0.
///         If calculated length is 0 (It can be due to string length or `start`
///         and length combination), then `written` will be equal to 0 and call
///         returns JsNoError
///     </para>
/// </remarks>
/// <param name="value">JavascriptString value</param>
/// <param name="start">start offset of buffer</param>
/// <param name="length">length to be written</param>
/// <param name="buffer">Pointer to buffer</param>
/// <param name="written">Total number of characters written</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsCopyStringUtf16(
        _In_ JsValueRef value,
        _In_ int start,
        _In_ int length,
        _Out_opt_ uint16_t* buffer,
        _Out_opt_ size_t* written);

/// <summary>
///     Parses a script and returns a function representing the script.
/// </summary>
/// <remarks>
///     <para>
///        Requires an active script context.
///     </para>
///     <para>
///         Script source can be either JavascriptString or JavascriptExternalArrayBuffer.
///         In case it is an ExternalArrayBuffer, and the encoding of the buffer is Utf16,
///         JsParseScriptAttributeArrayBufferIsUtf16Encoded is expected on parseAttributes.
///     </para>
///     <para>
///         Use JavascriptExternalArrayBuffer with Utf8/ASCII script source
///         for better performance and smaller memory footprint.
///     </para>
/// </remarks>
/// <param name="script">The script to run.</param>
/// <param name="sourceContext">
///     A cookie identifying the script that can be used by debuggable script contexts.
/// </param>
/// <param name="sourceUrl">The location the script came from.</param>
/// <param name="parseAttributes">Attribute mask for parsing the script</param>
/// <param name="result">The result of the compiled script.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsParse(
        _In_ JsValueRef script,
        _In_ JsSourceContext sourceContext,
        _In_ JsValueRef sourceUrl,
        _In_ JsParseScriptAttributes parseAttributes,
        _Out_ JsValueRef *result);

/// <summary>
///     Executes a script.
/// </summary>
/// <remarks>
///     <para>
///        Requires an active script context.
///     </para>
///     <para>
///         Script source can be either JavascriptString or JavascriptExternalArrayBuffer.
///         In case it is an ExternalArrayBuffer, and the encoding of the buffer is Utf16,
///         JsParseScriptAttributeArrayBufferIsUtf16Encoded is expected on parseAttributes.
///     </para>
///     <para>
///         Use JavascriptExternalArrayBuffer with Utf8/ASCII script source
///         for better performance and smaller memory footprint.
///     </para>
/// </remarks>
/// <param name="script">The script to run.</param>
/// <param name="sourceContext">
///     A cookie identifying the script that can be used by debuggable script contexts.
/// </param>
/// <param name="sourceUrl">The location the script came from</param>
/// <param name="parseAttributes">Attribute mask for parsing the script</param>
/// <param name="result">The result of the script, if any. This parameter can be null.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsRun(
        _In_ JsValueRef script,
        _In_ JsSourceContext sourceContext,
        _In_ JsValueRef sourceUrl,
        _In_ JsParseScriptAttributes parseAttributes,
        _Out_ JsValueRef *result);

/// <summary>
///     Creates the property ID associated with the name.
/// </summary>
/// <remarks>
///     <para>
///         Property IDs are specific to a context and cannot be used across contexts.
///     </para>
///     <para>
///         Requires an active script context.
///     </para>
/// </remarks>
/// <param name="name">
///     The name of the property ID to get or create. The name may consist of only digits.
///     The string is expected to be ASCII / utf8 encoded.
/// </param>
/// <param name="length">length of the name in bytes</param>
/// <param name="propertyId">The property ID in this runtime for the given name.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsCreatePropertyIdUtf8(
        _In_z_ const char *name,
        _In_ size_t length,
        _Out_ JsPropertyIdRef *propertyId);

/// <summary>
///     Copies the name associated with the property ID into a buffer.
/// </summary>
/// <remarks>
///     <para>
///         Requires an active script context.
///     </para>
///     <para>
///         When size of the `buffer` is unknown,
///         `buffer` argument can be nullptr.
///         `length` argument will return the size needed.
///     </para>
/// </remarks>
/// <param name="propertyId">The property ID to get the name of.</param>
/// <param name="buffer">The buffer holding the name associated with the property ID, encoded as utf8</param>
/// <param name="bufferSize">Size of the buffer.</param>
/// <param name="written">Total number of characters written or to be written</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsCopyPropertyIdUtf8(
        _In_ JsPropertyIdRef propertyId,
        _Out_ uint8_t* buffer,
        _In_ size_t bufferSize,
        _Out_ size_t* length);

/// <summary>
///     Serializes a parsed script to a buffer than can be reused.
/// </summary>
/// <remarks>
///     <para>
///     <c>JsSerializeScript</c> parses a script and then stores the parsed form of the script in a
///     runtime-independent format. The serialized script then can be deserialized in any
///     runtime without requiring the script to be re-parsed.
///     </para>
///     <para>
///     Requires an active script context.
///     </para>
///     <para>
///         Script source can be either JavascriptString or JavascriptExternalArrayBuffer.
///         In case it is an ExternalArrayBuffer, and the encoding of the buffer is Utf16,
///         JsParseScriptAttributeArrayBufferIsUtf16Encoded is expected on parseAttributes.
///     </para>
///     <para>
///         Use JavascriptExternalArrayBuffer with Utf8/ASCII script source
///         for better performance and smaller memory footprint.
///     </para>
/// </remarks>
/// <param name="script">The script to serialize</param>
/// <param name="buffer">The buffer to put the serialized script into. Can be null.</param>
/// <param name="bufferSize">
///     On entry, the size of the buffer, in bytes; on exit, the size of the buffer, in bytes,
///     required to hold the serialized script.
/// </param>
/// <param name="parseAttributes">Encoding for the script.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsSerialize(
        _In_ JsValueRef script,
        _Out_ BYTE *buffer,
        _Inout_ unsigned int *bufferSize,
        _In_ JsParseScriptAttributes parseAttributes);

/// <summary>
///     Parses a serialized script and returns a function representing the script.
///     Provides the ability to lazy load the script source only if/when it is needed.
/// </summary>
/// <remarks>
///     <para>
///     Requires an active script context.
///     </para>
/// </remarks>
/// <param name="buffer">The serialized script.</param>
/// <param name="scriptLoadCallback">
///     Callback called when the source code of the script needs to be loaded.
///     This is an optional parameter, set to null if not needed.
/// </param>
/// <param name="sourceContext">
///     A cookie identifying the script that can be used by debuggable script contexts.
///     This context will passed into scriptLoadCallback.
/// </param>
/// <param name="sourceUrl">The location the script came from.</param>
/// <param name="result">A function representing the script code.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsParseSerialized(
        _In_ BYTE *buffer,
        _In_ JsSerializedLoadScriptCallback scriptLoadCallback,
        _In_ JsSourceContext sourceContext,
        _In_ JsValueRef sourceUrl,
        _Out_ JsValueRef *result);

/// <summary>
///     Runs a serialized script.
///     Provides the ability to lazy load the script source only if/when it is needed.
/// </summary>
/// <remarks>
///     <para>
///     Requires an active script context.
///     </para>
///     <para>
///     The runtime will hold on to the buffer until all instances of any functions created from
///     the buffer are garbage collected.
///     </para>
/// </remarks>
/// <param name="buffer">The serialized script.</param>
/// <param name="scriptLoadCallback">Callback called when the source code of the script needs to be loaded.</param>
/// <param name="sourceContext">
///     A cookie identifying the script that can be used by debuggable script contexts.
///     This context will passed into scriptLoadCallback.
/// </param>
/// <param name="sourceUrl">The location the script came from.</param>
/// <param name="result">
///     The result of running the script, if any. This parameter can be null.
/// </param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsRunSerialized(
        _In_ BYTE *buffer,
        _In_ JsSerializedLoadScriptCallback scriptLoadCallback,
        _In_ JsSourceContext sourceContext,
        _In_ JsValueRef sourceUrl,
        _Out_ JsValueRef *result);
#endif // NTBUILD
#endif // _CHAKRACORE_H_
