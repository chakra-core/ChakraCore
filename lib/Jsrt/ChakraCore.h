//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
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

#if !defined(NTBUILD) && !defined(_CHAKRACOREBUILD)
#define _CHAKRACOREBUILD
#endif

#include "ChakraCommon.h"
#include "ChakraDebug.h"

// Begin ChakraCore only APIs
#ifdef _CHAKRACOREBUILD

/// <summary>
///     A reference to an ES module.
/// </summary>
/// <remarks>
///     A module record represents an ES module.
/// </remarks>
typedef void* JsModuleRecord;

/// <summary>
///     A reference to an object owned by the SharedArrayBuffer.
/// </summary>
/// <remarks>
///     This represents SharedContents which is heap allocated object, it can be passed through
///     different runtimes to share the underlying buffer.
/// </remarks>
typedef void *JsSharedArrayBufferContentHandle;

/// <summary>
///     A reference to a SCA Serializer.
/// </summary>
/// <remarks>
///     This represents the internal state of a Serializer
/// </remarks>
typedef void *JsVarSerializerHandle;

/// <summary>
///     A reference to a SCA Deserializer.
/// </summary>
/// <remarks>
///     This represents the internal state of a Deserializer
/// </remarks>
typedef void *JsVarDeserializerHandle;


/// <summary>
///     Flags for parsing a module.
/// </summary>
typedef enum JsParseModuleSourceFlags
{
    /// <summary>
    ///     Module source is UTF16.
    /// </summary>
    JsParseModuleSourceFlags_DataIsUTF16LE = 0x00000000,
    /// <summary>
    ///     Module source is UTF8.
    /// </summary>
    JsParseModuleSourceFlags_DataIsUTF8 = 0x00000001
} JsParseModuleSourceFlags;

/// <summary>
///     The types of host info that can be set on a module record with JsSetModuleHostInfo.
/// </summary>
/// <remarks>
///     For more information see JsSetModuleHostInfo.
/// </remarks>
typedef enum JsModuleHostInfoKind
{
    /// <summary>
    ///     An exception object - e.g. if the module file cannot be found.
    /// </summary>
    JsModuleHostInfo_Exception = 0x01,
    /// <summary>
    ///     Host defined info.
    /// </summary>
    JsModuleHostInfo_HostDefined = 0x02,
    /// <summary>
    ///     Callback for receiving notification when module is ready.
    /// </summary>
    JsModuleHostInfo_NotifyModuleReadyCallback = 0x3,
    /// <summary>
    ///     Callback for receiving notification to fetch a dependent module.
    /// </summary>
    JsModuleHostInfo_FetchImportedModuleCallback = 0x4,
    /// <summary>
    ///     Callback for receiving notification for calls to ```import()```
    /// </summary>
    JsModuleHostInfo_FetchImportedModuleFromScriptCallback = 0x5,
    /// <summary>
    ///     URL for use in error stack traces and debugging.
    /// </summary>
    JsModuleHostInfo_Url = 0x6,
    /// <summary>
    ///     Callback to allow host to initialize import.meta object properties.
    /// </summary>
    JsModuleHostInfo_InitializeImportMetaCallback = 0x7,
    /// <summary>
    ///     Callback to report module completion or exception thrown when evaluating a module.
    /// </summary>
    JsModuleHostInfo_ReportModuleCompletionCallback = 0x8
} JsModuleHostInfoKind;

/// <summary>
///     The possible states for a Promise object.
/// </summary>
typedef enum _JsPromiseState
{
    JsPromiseStatePending = 0x0,
    JsPromiseStateFulfilled = 0x1,
    JsPromiseStateRejected = 0x2
} JsPromiseState;

/// <summary>
///     User implemented callback to fetch additional imported modules in ES modules.
/// </summary>
/// <remarks>
///     The callback is invoked on the current runtime execution thread, therefore execution is blocked until 
///     the callback completes. Notify the host to fetch the dependent module. This is the "import" part 
///     before HostResolveImportedModule in ES6 spec. This notifies the host that the referencing module has
///     the specified module dependency, and the host needs to retrieve the module back.
///
///     Callback should:
///     1. Check if the requested module has been requested before - if yes return the existing
///         module record
///     2. If no create and initialize a new module record with JsInitializeModuleRecord to
///         return and schedule a call to JsParseModuleSource for the new record.
/// </remarks>
/// <param name="referencingModule">The referencing module that is requesting the dependent module.</param>
/// <param name="specifier">The specifier coming from the module source code.</param>
/// <param name="dependentModuleRecord">The ModuleRecord of the dependent module. If the module was requested 
///                                     before from other source, return the existing ModuleRecord, otherwise
///                                     return a newly created ModuleRecord.</param>
/// <returns>
///     Returns a <c>JsNoError</c> if the operation succeeded an error code otherwise.
/// </returns>
typedef JsErrorCode(CHAKRA_CALLBACK * FetchImportedModuleCallBack)(_In_ JsModuleRecord referencingModule, _In_ JsValueRef specifier, _Outptr_result_maybenull_ JsModuleRecord* dependentModuleRecord);

/// <summary>
///     User implemented callback to fetch imported modules dynamically in scripts.
/// </summary>
/// <remarks>
///     The callback is invoked on the current runtime execution thread, therefore execution is blocked until
///     the callback completes. Notify the host to fetch the dependent module. This is used for the dynamic
///     import() syntax.
///
///     Callback should:
///     1. Check if the requested module has been requested before - if yes return the existing module record
///     2. If no create and initialize a new module record with JsInitializeModuleRecord to return and
///         schedule a call to JsParseModuleSource for the new record.
/// </remarks>
/// <param name="dwReferencingSourceContext">The referencing script context that calls import()</param>
/// <param name="specifier">The specifier provided to the import() call.</param>
/// <param name="dependentModuleRecord">The ModuleRecord of the dependent module. If the module was requested
///                                     before from other source, return the existing ModuleRecord, otherwise
///                                     return a newly created ModuleRecord.</param>
/// <returns>
///     Returns <c>JsNoError</c> if the operation succeeded or an error code otherwise.
/// </returns>
typedef JsErrorCode(CHAKRA_CALLBACK * FetchImportedModuleFromScriptCallBack)(_In_ JsSourceContext dwReferencingSourceContext, _In_ JsValueRef specifier, _Outptr_result_maybenull_ JsModuleRecord* dependentModuleRecord);

/// <summary>
///     User implemented callback to get notification when the module is ready.
/// </summary>
/// <remarks>
///     The callback is invoked on the current runtime execution thread, therefore execution is blocked until the
///     callback completes. This callback should schedule a call to JsEvaluateModule to run the module that has been loaded.
/// </remarks>
/// <param name="referencingModule">The referencing module that has finished running ModuleDeclarationInstantiation step.</param>
/// <param name="exceptionVar">If nullptr, the module is successfully initialized and host should queue the execution job
///                            otherwise it's the exception object.</param>
/// <returns>
///     Returns a JsErrorCode - note, the return value is ignored.
/// </returns>
typedef JsErrorCode(CHAKRA_CALLBACK * NotifyModuleReadyCallback)(_In_opt_ JsModuleRecord referencingModule, _In_opt_ JsValueRef exceptionVar);

/// <summary>
///     User implemented callback to fill in module properties for the import.meta object.
/// </summary>
/// <remarks>
///     This callback allows the host to fill module details for the referencing module in the import.meta object
///     loaded by script.
///     The callback is invoked on the current runtime execution thread, therefore execution is blocked until the
///     callback completes.
/// </remarks>
/// <param name="referencingModule">The referencing module that is loading an import.meta object.</param>
/// <param name="importMetaVar">The object which will be returned to script for the referencing module.</param>
/// <returns>
///     Returns a JsErrorCode - note, the return value is ignored.
/// </returns>
typedef JsErrorCode(CHAKRA_CALLBACK * InitializeImportMetaCallback)(_In_opt_ JsModuleRecord referencingModule, _In_opt_ JsValueRef importMetaVar);

/// <summary>
///     User implemented callback to report completion of module execution.
/// </summary>
/// <remarks>
///     This callback is used to report the completion of module execution and to report any runtime exceptions.
///     Note it is not used for dynamicly imported modules import() as the reuslt from those are handled with a
///     promise.
///     If this callback is not set and a module produces an exception:
///     a) a purely synchronous module tree with an exception will set the exception on the runtime
///        (this is not done if this callback is set)
///     b) an exception in an asynchronous module tree will not be reported directly.
///
///     However in all cases the exception will be set on the JsModuleRecord.
/// </remarks>
/// <param name="module">The root module that has completed either with an exception or normally.</param>
/// <param name="exception">The exception object which was thrown or nullptr if the module had a normal completion.</param>
/// <returns>
///     Returns a JsErrorCode: JsNoError if successful.
/// </returns>
typedef JsErrorCode(CHAKRA_CALLBACK * ReportModuleCompletionCallback)(_In_ JsModuleRecord module, _In_opt_ JsValueRef exception);

/// <summary>
///     A structure containing information about a native function callback.
/// </summary>
typedef struct JsNativeFunctionInfo
{
    JsValueRef thisArg;
    JsValueRef newTargetArg;
    bool isConstructCall;
}JsNativeFunctionInfo;

/// <summary>
///     A function callback.
/// </summary>
/// <param name="callee">
///     A function object that represents the function being invoked.
/// </param>
/// <param name="arguments">The arguments to the call.</param>
/// <param name="argumentCount">The number of arguments.</param>
/// <param name="info">Additional information about this function call.</param>
/// <param name="callbackState">
///     The state passed to <c>JsCreateFunction</c>.
/// </param>
/// <returns>The result of the call, if any.</returns>
typedef _Ret_maybenull_ JsValueRef(CHAKRA_CALLBACK * JsEnhancedNativeFunction)(_In_ JsValueRef callee, _In_ JsValueRef *arguments, _In_ unsigned short argumentCount, _In_ JsNativeFunctionInfo *info, _In_opt_ void *callbackState);

/// <summary>
///     A Promise Rejection Tracker callback.
/// </summary>
/// <remarks>
///     The host can specify a promise rejection tracker callback in <c>JsSetHostPromiseRejectionTracker</c>.
///     If a promise is rejected with no reactions or a reaction is added to a promise that was rejected
///     before it had reactions by default nothing is done.
///     A Promise Rejection Tracker callback may be set - which will then be called when this occurs.
///     Note - per draft ECMASpec 2018 25.4.1.9 this function should not set or return an exception
///     Note also the promise and reason parameters may be garbage collected after this function is called
///     if you wish to make further use of them you will need to use JsAddRef to preserve them
///     However if you use JsAddRef you must also call JsRelease and not hold unto them after 
///     a handled notification (both per spec and to avoid memory leaks)
/// </remarks>
/// <param name="promise">The promise object, represented as a JsValueRef.</param>
/// <param name="reason">The value/cause of the rejection, represented as a JsValueRef.</param>
/// <param name="handled">Boolean - false for promiseRejected: i.e. if the promise has just been rejected with no handler, 
///                         true for promiseHandled: i.e. if it was rejected before without a handler and is now being handled.</param>
/// <param name="callbackState">The state passed to <c>JsSetHostPromiseRejectionTracker</c>.</param>
typedef void (CHAKRA_CALLBACK *JsHostPromiseRejectionTrackerCallback)(_In_ JsValueRef promise, _In_ JsValueRef reason, _In_ bool handled, _In_opt_ void *callbackState);

/// <summary>
///     A structure containing information about interceptors.
/// </summary>
typedef struct _JsGetterSetterInterceptor {
    JsValueRef getTrap;
    JsValueRef setTrap;
    JsValueRef deletePropertyTrap;
    JsValueRef enumerateTrap;
    JsValueRef ownKeysTrap;
    JsValueRef hasTrap;
    JsValueRef getOwnPropertyDescriptorTrap;
    JsValueRef definePropertyTrap;
    JsValueRef initializerTrap;
} JsGetterSetterInterceptor;

/// <summary>
///     A callback for tracing references back from Chakra to DOM wrappers.
/// </summary>
typedef void (CHAKRA_CALLBACK *JsDOMWrapperTracingCallback)(_In_opt_ void *data);

/// <summary>
///     A callback for checking whether tracing from Chakra to DOM wrappers has completed.
/// </summary>
typedef bool (CHAKRA_CALLBACK *JsDOMWrapperTracingDoneCallback)(_In_opt_ void *data);

/// <summary>
///     A callback for entering final pause for tracing DOM wrappers.
/// </summary>
typedef void(CHAKRA_CALLBACK *JsDOMWrapperTracingEnterFinalPauseCallback)(_In_opt_ void *data);

/// <summary>
///     A trace callback.
/// </summary>
/// <param name="data">
///     The external data that was passed in when creating the object being traced.
/// </param>
typedef void (CHAKRA_CALLBACK *JsTraceCallback)(_In_opt_ void *data);

/// <summary>
///     Creates a new enhanced JavaScript function.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="nativeFunction">The method to call when the function is invoked.</param>
/// <param name="metadata">If this is not <c>JS_INVALID_REFERENCE</c>, it is converted to a string and used as the name of the function.</param>
/// <param name="callbackState">
///     User provided state that will be passed back to the callback.
/// </param>
/// <param name="function">The new function object.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsCreateEnhancedFunction(
    _In_ JsEnhancedNativeFunction nativeFunction,
    _In_opt_ JsValueRef metadata,
    _In_opt_ void *callbackState,
    _Out_ JsValueRef *function);

/// <summary>
///     Initialize a ModuleRecord from host
/// </summary>
/// <remarks>
///     Bootstrap the module loading process by creating a new module record.
/// </remarks>
/// <param name="referencingModule">Unused parameter - exists for backwards compatability, supply nullptr</param>
/// <param name="normalizedSpecifier">The normalized specifier or url for the module - used in script errors, optional.</param>
/// <param name="moduleRecord">The new module record.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsInitializeModuleRecord(
    _In_opt_ JsModuleRecord referencingModule,
    _In_opt_ JsValueRef normalizedSpecifier,
    _Outptr_result_maybenull_ JsModuleRecord* moduleRecord);

/// <summary>
///     Parse the source for an ES module
/// </summary>
/// <remarks>
///     This is basically ParseModule operation in ES6 spec. It is slightly different in that:
///     a) The ModuleRecord was initialized earlier, and passed in as an argument.
///     b) This includes a check to see if the module being Parsed is the last module in the
/// dependency tree. If it is it automatically triggers Module Instantiation.
/// </remarks>
/// <param name="requestModule">The ModuleRecord being parsed.</param>
/// <param name="sourceContext">A cookie identifying the script that can be used by debuggable script contexts.</param>
/// <param name="script">The source script to be parsed, but not executed in this code.</param>
/// <param name="scriptLength">The length of sourceText in bytes. As the input might contain a embedded null.</param>
/// <param name="sourceFlag">The type of the source code passed in. It could be utf16 or utf8 at this time.</param>
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
///     This method should be called after the engine notifies the host that the module is ready.
///     This method only needs to be called on root modules - it will execute all of the dependent modules.
///
///     One moduleRecord will be executed only once. Additional execution call on the same moduleRecord will fail.
/// </remarks>
/// <param name="requestModule">The ModuleRecord being executed.</param>
/// <param name="result">The return value of the module.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsModuleEvaluation(
    _In_ JsModuleRecord requestModule,
    _Outptr_result_maybenull_ JsValueRef* result);

/// <summary>
///     Set host info for the specified module.
/// </summary>
/// <remarks>
///     This is used for four things:
///     1. Setting up the callbacks for module loading - note these are actually
///         set on the module's Context not the module itself so only have to be set
///         for the first root module in any given context.
///         Alternatively you can set these on the currentContext by supplying a nullptr
///         as the requestModule
///     2. Setting host defined info on a module record - can be anything that
///         you wish to associate with your modules.
///     3. Setting a URL for a module to be used for stack traces/debugging -
///         note this must be set before calling JsParseModuleSource on the module
///         or it will be ignored.
///     4. Setting an exception on the module object - only relevant prior to it being Parsed.
/// </remarks>
/// <param name="requestModule">The request module, optional for setting callbacks, required for other uses.</param>
/// <param name="moduleHostInfo">The type of host info to be set.</param>
/// <param name="hostInfo">The host info to be set.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsSetModuleHostInfo(
    _In_opt_ JsModuleRecord requestModule,
    _In_ JsModuleHostInfoKind moduleHostInfo,
    _In_ void* hostInfo);

/// <summary>
///     Retrieve the host info for the specified module.
/// </summary>
/// <remarks>
///     This can used to retrieve info previously set with JsSetModuleHostInfo.
/// </remarks>
/// <param name="requestModule">The request module.</param>
/// <param name="moduleHostInfo">The type of host info to be retrieved.</param>
/// <param name="hostInfo">The retrieved host info for the module.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsGetModuleHostInfo(
    _In_  JsModuleRecord requestModule,
    _In_ JsModuleHostInfoKind moduleHostInfo,
    _Outptr_result_maybenull_ void** hostInfo);

/// <summary>
///     Returns metadata relating to the exception that caused the runtime of the current context
///     to be in the exception state and resets the exception state for that runtime. The metadata
///     includes a reference to the exception itself.
/// </summary>
/// <remarks>
///     <para>
///     If the runtime of the current context is not in an exception state, this API will return
///     <c>JsErrorInvalidArgument</c>. If the runtime is disabled, this will return an exception
///     indicating that the script was terminated, but it will not clear the exception (the
///     exception will be cleared if the runtime is re-enabled using
///     <c>JsEnableRuntimeExecution</c>).
///     </para>
///     <para>
///     The metadata value is a javascript object with the following properties: <c>exception</c>, the
///     thrown exception object; <c>line</c>, the 0 indexed line number where the exception was thrown;
///     <c>column</c>, the 0 indexed column number where the exception was thrown; <c>length</c>, the
///     source-length of the cause of the exception; <c>source</c>, a string containing the line of
///     source code where the exception was thrown; and <c>url</c>, a string containing the name of
///     the script file containing the code that threw the exception.
///     </para>
///     <para>
///     Requires an active script context.
///     </para>
/// </remarks>
/// <param name="metadata">The exception metadata for the runtime of the current context.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsGetAndClearExceptionWithMetadata(
    _Out_ JsValueRef *metadata);

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
///     Create JavascriptString variable from ASCII or Utf8 string
/// </summary>
/// <remarks>
///     <para>
///        Requires an active script context.
///     </para>
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
    JsCreateString(
        _In_ const char *content,
        _In_ size_t length,
        _Out_ JsValueRef *value);

/// <summary>
///     Create JavascriptString variable from Utf16 string
/// </summary>
/// <remarks>
///     <para>
///         Requires an active script context.
///     </para>
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
///     Write JavascriptString value into C string buffer (Utf8)
/// </summary>
/// <remarks>
///     <para>
///         When size of the `buffer` is unknown,
///         `buffer` argument can be nullptr.
///         In that case, `length` argument will return the length needed.
///     </para>
/// </remarks>
/// <param name="value">JavascriptString value</param>
/// <param name="buffer">Pointer to buffer</param>
/// <param name="bufferSize">Buffer size</param>
/// <param name="length">Total number of characters needed or written</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsCopyString(
        _In_ JsValueRef value,
        _Out_opt_ char* buffer,
        _In_ size_t bufferSize,
        _Out_opt_ size_t* length);

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
///         when start is out of range or &lt; 0, returns JsErrorInvalidArgument
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
///         Requires an active script context.
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
///         Requires an active script context.
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
///     The name of the property ID to get or create. The string is expected to be ASCII / utf8 encoded.
///     The name can be any JavaScript property identifier, including all digits.
/// </param>
/// <param name="length">length of the name in bytes</param>
/// <param name="propertyId">The property ID in this runtime for the given name.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsCreatePropertyId(
        _In_z_ const char *name,
        _In_ size_t length,
        _Out_ JsPropertyIdRef *propertyId);

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
/// <param name="propertyString">The property string in this runtime for the given name.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsCreatePropertyString(
    _In_z_ const char *name,
    _In_ size_t length,
    _Out_ JsValueRef* propertyString);

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
    JsCopyPropertyId(
        _In_ JsPropertyIdRef propertyId,
        _Out_ char* buffer,
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
/// <param name="buffer">ArrayBuffer</param>
/// <param name="parseAttributes">Encoding for the script.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsSerialize(
        _In_ JsValueRef script,
        _Out_ JsValueRef *buffer,
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
/// <param name="buffer">The serialized script as an ArrayBuffer (preferably ExternalArrayBuffer).</param>
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
        _In_ JsValueRef buffer,
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
///     The runtime will detach the data from the buffer and hold on to it until all
///     instances of any functions created from the buffer are garbage collected.
///     </para>
/// </remarks>
/// <param name="buffer">The serialized script as an ArrayBuffer (preferably ExternalArrayBuffer).</param>
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
        _In_ JsValueRef buffer,
        _In_ JsSerializedLoadScriptCallback scriptLoadCallback,
        _In_ JsSourceContext sourceContext,
        _In_ JsValueRef sourceUrl,
        _Out_ JsValueRef *result);

/// <summary>
///     Gets the state of a given Promise object.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="promise">The Promise object.</param>
/// <param name="state">The current state of the Promise.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsGetPromiseState(
        _In_ JsValueRef promise,
        _Out_ JsPromiseState *state);

/// <summary>
///     Gets the result of a given Promise object.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="promise">The Promise object.</param>
/// <param name="result">The result of the Promise.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsGetPromiseResult(
        _In_ JsValueRef promise,
        _Out_ JsValueRef *result);

/// <summary>
///     Creates a new JavaScript Promise object.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="promise">The new Promise object.</param>
/// <param name="resolveFunction">The function called to resolve the created Promise object.</param>
/// <param name="rejectFunction">The function called to reject the created Promise object.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsCreatePromise(
        _Out_ JsValueRef *promise,
        _Out_ JsValueRef *resolveFunction,
        _Out_ JsValueRef *rejectFunction);

/// <summary>
///     A weak reference to a JavaScript value.
/// </summary>
/// <remarks>
///     A value with only weak references is available for garbage-collection. A strong reference
///     to the value (<c>JsValueRef</c>) may be obtained from a weak reference if the value happens
///     to still be available.
/// </remarks>
typedef JsRef JsWeakRef;

/// <summary>
///     Creates a weak reference to a value.
/// </summary>
/// <param name="value">The value to be referenced.</param>
/// <param name="weakRef">Weak reference to the value.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsCreateWeakReference(
        _In_ JsValueRef value,
        _Out_ JsWeakRef* weakRef);

/// <summary>
///     Gets a strong reference to the value referred to by a weak reference.
/// </summary>
/// <param name="weakRef">A weak reference.</param>
/// <param name="value">Reference to the value, or <c>JS_INVALID_REFERENCE</c> if the value is
///     no longer available.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsGetWeakReferenceValue(
        _In_ JsWeakRef weakRef,
        _Out_ JsValueRef* value);

/// <summary>
///     Creates a Javascript SharedArrayBuffer object with shared content get from JsGetSharedArrayBufferContent.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="sharedContents">
///     The storage object of a SharedArrayBuffer which can be shared between multiple thread.
/// </param>
/// <param name="result">The new SharedArrayBuffer object.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsCreateSharedArrayBufferWithSharedContent(
    _In_ JsSharedArrayBufferContentHandle sharedContents,
    _Out_ JsValueRef *result);

/// <summary>
///     Get the storage object from a SharedArrayBuffer.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="sharedArrayBuffer">The SharedArrayBuffer object.</param>
/// <param name="sharedContents">
///     The storage object of a SharedArrayBuffer which can be shared between multiple thread.
///     User should call JsReleaseSharedArrayBufferContentHandle after finished using it.
/// </param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsGetSharedArrayBufferContent(
    _In_ JsValueRef sharedArrayBuffer,
    _Out_ JsSharedArrayBufferContentHandle *sharedContents);

/// <summary>
///     Decrease the reference count on a SharedArrayBuffer storage object.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="sharedContents">
///     The storage object of a SharedArrayBuffer which can be shared between multiple thread.
/// </param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsReleaseSharedArrayBufferContentHandle(
    _In_ JsSharedArrayBufferContentHandle sharedContents);

/// <summary>
///     Determines whether an object has a non-inherited property.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="object">The object that may contain the property.</param>
/// <param name="propertyId">The ID of the property.</param>
/// <param name="hasOwnProperty">Whether the object has the non-inherited property.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsHasOwnProperty(
        _In_ JsValueRef object,
        _In_ JsPropertyIdRef propertyId,
        _Out_ bool *hasOwnProperty);

/// <summary>
///     Determines whether an object has a non-inherited property.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="object">The object that may contain the item.</param>
/// <param name="index">The index to find.</param>
/// <param name="hasOwnProperty">Whether the object has the non-inherited
/// property.</param> <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code
///     otherwise.
/// </returns>
CHAKRA_API
    JsHasOwnItem(
        _In_ JsValueRef object,
        _In_ uint32_t index,
        _Out_ bool* hasOwnItem);

    /// <summary>
///     Write JS string value into char string buffer without a null terminator
/// </summary>
/// <remarks>
///     <para>
///         When size of the `buffer` is unknown,
///         `buffer` argument can be nullptr.
///         In that case, `written` argument will return the length needed.
///     </para>
///     <para>
///         When start is out of range or &lt; 0, returns JsErrorInvalidArgument
///         and `written` will be equal to 0.
///         If calculated length is 0 (It can be due to string length or `start`
///         and length combination), then `written` will be equal to 0 and call
///         returns JsNoError
///     </para>
///     <para>
///         The JS string `value` will be converted one utf16 code point at a time,
///         and if it has code points that do not fit in one byte, those values
///         will be truncated.
///     </para>
/// </remarks>
/// <param name="value">JavascriptString value</param>
/// <param name="start">Start offset of buffer</param>
/// <param name="length">Number of characters to be written</param>
/// <param name="buffer">Pointer to buffer</param>
/// <param name="written">Total number of characters written</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsCopyStringOneByte(
    _In_ JsValueRef value,
    _In_ int start,
    _In_ int length,
    _Out_opt_ char* buffer,
    _Out_opt_ size_t* written);

/// <summary>
///     Obtains frequently used properties of a data view.
/// </summary>
/// <param name="dataView">The data view instance.</param>
/// <param name="arrayBuffer">The ArrayBuffer backstore of the view.</param>
/// <param name="byteOffset">The offset in bytes from the start of arrayBuffer referenced by the array.</param>
/// <param name="byteLength">The number of bytes in the array.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsGetDataViewInfo(
        _In_ JsValueRef dataView,
        _Out_opt_ JsValueRef *arrayBuffer,
        _Out_opt_ unsigned int *byteOffset,
        _Out_opt_ unsigned int *byteLength);

/// <summary>
///     Determine if one JavaScript value is less than another JavaScript value.
/// </summary>
/// <remarks>
///     <para>
///     This function is equivalent to the <c>&lt;</c> operator in Javascript.
///     </para>
///     <para>
///     Requires an active script context.
///     </para>
/// </remarks>
/// <param name="object1">The first object to compare.</param>
/// <param name="object2">The second object to compare.</param>
/// <param name="result">Whether object1 is less than object2.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsLessThan(
        _In_ JsValueRef object1,
        _In_ JsValueRef object2,
        _Out_ bool *result);

/// <summary>
///     Determine if one JavaScript value is less than or equal to another JavaScript value.
/// </summary>
/// <remarks>
///     <para>
///     This function is equivalent to the <c>&lt;=</c> operator in Javascript.
///     </para>
///     <para>
///     Requires an active script context.
///     </para>
/// </remarks>
/// <param name="object1">The first object to compare.</param>
/// <param name="object2">The second object to compare.</param>
/// <param name="result">Whether object1 is less than or equal to object2.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsLessThanOrEqual(
        _In_ JsValueRef object1,
        _In_ JsValueRef object2,
        _Out_ bool *result);

/// <summary>
///     Creates a new object (with prototype) that stores some external data.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="data">External data that the object will represent. May be null.</param>
/// <param name="finalizeCallback">
///     A callback for when the object is finalized. May be null.
/// </param>
/// <param name="prototype">Prototype object or nullptr.</param>
/// <param name="object">The new object.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsCreateExternalObjectWithPrototype(
        _In_opt_ void *data,
        _In_opt_ JsFinalizeCallback finalizeCallback,
        _In_opt_ JsValueRef prototype,
        _Out_ JsValueRef *object);

/// <summary>
///     Creates a new object (with prototype) that stores some data.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="data">External data that the object will represent. May be null.</param>
/// <param name="traceCallback">
///     A callback for when the object is traced. May be null.
/// <param name="finalizeCallback">
///     A callback for when the object is finalized. May be null.
/// </param>
/// <param name="prototype">Prototype object or nullptr.</param>
/// <param name="object">The new object.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsCreateTracedExternalObject(
    _In_opt_ void *data,
    _In_opt_ size_t inlineSlotSize,
    _In_opt_ JsTraceCallback traceCallback,
    _In_opt_ JsFinalizeCallback finalizeCallback,
    _In_opt_ JsValueRef prototype,
    _Out_ JsValueRef *object);

/// <summary>
///     Creates a new object (with prototype) that stores some external data and also supports interceptors.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="data">External data that the object will represent. May be null.</param>
/// <param name="finalizeCallback">
///     A callback for when the object is finalized. May be null.
/// </param>
/// <param name="prototype">Prototype object or nullptr.</param>
/// <param name="object">The new object.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsCreateCustomExternalObject(
    _In_opt_ void *data,
    _In_opt_ size_t inlineSlotSize,
    _In_opt_ JsTraceCallback traceCallback,
    _In_opt_ JsFinalizeCallback finalizeCallback,
    _Inout_opt_ JsGetterSetterInterceptor ** getterSetterInterceptor,
    _In_opt_ JsValueRef prototype,
    _Out_ JsValueRef * object);

/// <summary>
///     Returns a reference to the Array.Prototype.forEach function. The function is created if it's not already present.
/// </summary>
/// <param name="result">A reference to the Array.Prototype.forEach function.</param>
CHAKRA_API
JsGetArrayForEachFunction(_Out_ JsValueRef * result);

/// <summary>
///     Returns a reference to the Array.Prototype.keys function. The function is created if it's not already present.
/// </summary>
/// <param name="result">A reference to the Array.Prototype.keys function.</param>
CHAKRA_API
JsGetArrayKeysFunction(_Out_ JsValueRef * result);

/// <summary>
///     Returns a reference to the Array.Prototype.values function. The function is created if it's not already present.
/// </summary>
/// <param name="result">A reference to the Array.Prototype.values function.</param>
CHAKRA_API
JsGetArrayValuesFunction(_Out_ JsValueRef * result);

/// <summary>
///     Returns a reference to the Array.Prototype.entries function. The function is created if it's not already present.
/// </summary>
/// <param name="result">A reference to the Array.Prototype.entries function.</param>
CHAKRA_API
JsGetArrayEntriesFunction(_Out_ JsValueRef * result);

/// <summary>
///     Returns the property id of the Symbol.iterator property.
/// </summary>
/// <param name="propertyId">The property id of the Symbol.iterator property.</param>
CHAKRA_API
JsGetPropertyIdSymbolIterator(_Out_ JsPropertyIdRef * propertyId);

/// <summary>
///     Returns a reference to the Javascript error prototype object.
/// </summary>
/// <param name="result">A reference to the Javascript error prototype object.</param>
CHAKRA_API
JsGetErrorPrototype(_Out_ JsValueRef * result);

/// <summary>
///     Returns a reference to the Javascript iterator prototype object.
/// </summary>
/// <param name="result">A reference to the Javascript iterator prototype object.</param>
CHAKRA_API
JsGetIteratorPrototype(_Out_ JsValueRef * result);

/// <summary>
///      Returns a value that indicates whether an object is callable.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="object">The object to test.</param>
/// <param name="isConstructor">If the object is callable, <c>true</c>, <c>false</c> otherwise.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsIsCallable(
    _In_ JsValueRef object,
    _Out_ bool *isCallable);

/// <summary>
///      Returns a value that indicates whether an object is a constructor.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="object">The object to test.</param>
/// <param name="isConstructor">If the object is a constructor, <c>true</c>, <c>false</c> otherwise.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsIsConstructor(
    _In_ JsValueRef object,
    _Out_ bool *isConstructor);

/// <summary>
///     Clones an object
/// </summary>
/// <param name="source">The original object.</param>
/// <param name="clonedObject">
///     Pointer to the cloned object.
/// </param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsCloneObject(
    _In_ JsValueRef source,
    _Out_ JsValueRef* clonedObject);

/// <summary>
///     Determines whether an object has a private property.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="object">The object that may contain the property.</param>
/// <param name="key">The key (JavascriptString or JavascriptSymbol) to the property.</param>
/// <param name="hasProperty">Whether the object (or a prototype) has the property.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsPrivateHasProperty(
    _In_ JsValueRef object,
    _In_ JsValueRef key,
    _Out_ bool *hasProperty);

/// <summary>
///     Gets an object's private property
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="object">The object that contains the property.</param>
/// <param name="key">The key (JavascriptString or JavascriptSymbol) to the property.</param>
/// <param name="value">The value of the property.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsPrivateGetProperty(
    _In_ JsValueRef object,
    _In_ JsValueRef key,
    _Out_ JsValueRef *value);

/// <summary>
///     Puts an object's private property.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="object">The object that contains the property.</param>
/// <param name="key">The key (JavascriptString or JavascriptSymbol) to the property.</param>
/// <param name="value">The new value of the property.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsPrivateSetProperty(
    _In_ JsValueRef object,
    _In_ JsValueRef key,
    _In_ JsValueRef value);

/// <summary>
///     Deletes an object's private property.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="object">The object that contains the property.</param>
/// <param name="key">The key (JavascriptString or JavascriptSymbol) to the property.</param>
/// <param name="result">Whether the property was deleted.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsPrivateDeleteProperty(
    _In_ JsValueRef object,
    _In_ JsValueRef key,
    _Out_ JsValueRef *result);

/// <summary>
///     Gets an object's property.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="object">The object that contains the property.</param>
/// <param name="key">The key (JavascriptString or JavascriptSymbol) to the property.</param>
/// <param name="value">The value of the property.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsObjectGetProperty(
        _In_ JsValueRef object,
        _In_ JsValueRef key,
        _Out_ JsValueRef *value);

/// <summary>
///     Puts an object's property.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="object">The object that contains the property.</param>
/// <param name="key">The key (JavascriptString or JavascriptSymbol) to the property.</param>
/// <param name="value">The new value of the property.</param>
/// <param name="useStrictRules">The property set should follow strict mode rules.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsObjectSetProperty(
        _In_ JsValueRef object,
        _In_ JsValueRef key,
        _In_ JsValueRef value,
        _In_ bool useStrictRules);

/// <summary>
///     Determines whether an object has a property.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="object">The object that may contain the property.</param>
/// <param name="key">The key (JavascriptString or JavascriptSymbol) to the property.</param>
/// <param name="hasProperty">Whether the object (or a prototype) has the property.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsObjectHasProperty(
        _In_ JsValueRef object,
        _In_ JsValueRef key,
        _Out_ bool *hasProperty);

/// <summary>
///     Defines a new object's own property from a property descriptor.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="object">The object that has the property.</param>
/// <param name="key">The key (JavascriptString or JavascriptSymbol) to the property.</param>
/// <param name="propertyDescriptor">The property descriptor.</param>
/// <param name="result">Whether the property was defined.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsObjectDefineProperty(
        _In_ JsValueRef object,
        _In_ JsValueRef key,
        _In_ JsValueRef propertyDescriptor,
        _Out_ bool *result);

/// <summary>
///     Defines a new object's own property from a property descriptor.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="object">The object that has the property.</param>
/// <param name="key">The key (JavascriptString or JavascriptSymbol) to the property.</param>
/// <param name="propertyDescriptor">The property descriptor.</param>
/// <param name="result">Whether the property was defined.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsObjectDefinePropertyFull(
    _In_ JsValueRef object,
    _In_ JsValueRef key,
    _In_opt_ JsValueRef value,
    _In_opt_ JsValueRef getter,
    _In_opt_ JsValueRef setter,
    _In_ bool writable,
    _In_ bool enumerable,
    _In_ bool configurable,
    _Out_ bool *result);

/// <summary>
///     Deletes an object's property.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="object">The object that contains the property.</param>
/// <param name="key">The key (JavascriptString or JavascriptSymbol) to the property.</param>
/// <param name="useStrictRules">The property set should follow strict mode rules.</param>
/// <param name="result">Whether the property was deleted.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsObjectDeleteProperty(
        _In_ JsValueRef object,
        _In_ JsValueRef key,
        _In_ bool useStrictRules,
        _Out_ JsValueRef *result);

/// <summary>
///     Gets a property descriptor for an object's own property.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="object">The object that has the property.</param>
/// <param name="key">The key (JavascriptString or JavascriptSymbol) to the property.</param>
/// <param name="propertyDescriptor">The property descriptor.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsObjectGetOwnPropertyDescriptor(
        _In_ JsValueRef object,
        _In_ JsValueRef key,
        _Out_ JsValueRef *propertyDescriptor);

/// <summary>
///     Determines whether an object has a non-inherited property.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="object">The object that may contain the property.</param>
/// <param name="key">The key (JavascriptString or JavascriptSymbol) to the property.</param>
/// <param name="hasOwnProperty">Whether the object has the non-inherited property.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsObjectHasOwnProperty(
        _In_ JsValueRef object,
        _In_ JsValueRef key,
        _Out_ bool *hasOwnProperty);

/// <summary>
///     Sets whether any action should be taken when a promise is rejected with no reactions
///     or a reaction is added to a promise that was rejected before it had reactions.
///     By default in either of these cases nothing occurs.
///     This function allows you to specify if something should occur and provide a callback
///     to implement whatever should occur.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="promiseRejectionTrackerCallback">The callback function being set.</param>
/// <param name="callbackState">
///     User provided state that will be passed back to the callback.
/// </param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsSetHostPromiseRejectionTracker(
        _In_ JsHostPromiseRejectionTrackerCallback promiseRejectionTrackerCallback, 
        _In_opt_ void *callbackState);

/// <summary>
///     Retrieve the namespace object for a module.
/// </summary>
/// <remarks>
///     Requires an active script context and that the module has already been evaluated.
/// </remarks>
/// <param name="requestModule">The JsModuleRecord for which the namespace is being requested.</param>
/// <param name="moduleNamespace">A JsValueRef - the requested namespace object.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsGetModuleNamespace(
        _In_ JsModuleRecord requestModule,
        _Outptr_result_maybenull_ JsValueRef *moduleNamespace);

/// <summary>
///     Determines if a provided object is a JavscriptProxy Object and
///     provides references to a Proxy's target and handler.
/// </summary>
/// <remarks>
///     Requires an active script context.
///     If object is not a Proxy object the target and handler parameters are not touched.
///     If nullptr is supplied for target or handler the function returns after
///     setting the isProxy value.
///     If the object is a revoked Proxy target and handler are set to JS_INVALID_REFERENCE.
///     If it is a Proxy object that has not been revoked target and handler are set to the
///     the object's target and handler.
/// </remarks>
/// <param name="object">The object that may be a Proxy.</param>
/// <param name="isProxy">Pointer to a Boolean - is the object a proxy?</param>
/// <param name="target">Pointer to a JsValueRef - the object's target.</param>
/// <param name="handler">Pointer to a JsValueRef - the object's handler.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsGetProxyProperties(
        _In_ JsValueRef object,
        _Out_ bool* isProxy,
        _Out_opt_ JsValueRef* target,
        _Out_opt_ JsValueRef* handler);

/// <summary>
///     Parses a script and stores the generated parser state cache into a buffer which can be reused.
/// </summary>
/// <remarks>
///     <para>
///     <c>JsSerializeParserState</c> parses a script and then stores a cache of the parser state
///     in a runtime-independent format. The parser state may be deserialized in any runtime along
///     with the same script to skip the initial parse phase.
///     </para>
///     <para>
///         Requires an active script context.
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
/// <param name="scriptVal">The script to parse.</param>
/// <param name="bufferVal">The buffer to put the serialized parser state cache into.</param>
/// <param name="parseAttributes">Encoding for the script.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsSerializeParserState(
        _In_ JsValueRef scriptVal,
        _Out_ JsValueRef *bufferVal,
        _In_ JsParseScriptAttributes parseAttributes);

/// <summary>
///     Deserializes the cache of initial parser state and (along with the same
///     script source) executes the script and returns the result.
/// </summary>
/// <remarks>
///     <para>
///         Requires an active script context.
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
/// <param name="parserState">
///     A buffer containing a cache of the parser state generated by <c>JsSerializeParserState</c>.
/// </param>
/// <param name="result">The result of the script, if any. This parameter can be null.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsRunScriptWithParserState(
        _In_ JsValueRef script,
        _In_ JsSourceContext sourceContext,
        _In_ JsValueRef sourceUrl,
        _In_ JsParseScriptAttributes parseAttributes,
        _In_ JsValueRef parserState,
        _Out_ JsValueRef * result);

/// <summary>
///     Deserializes the cache of initial parser state and (along with the same
///     script source) returns a function representing that script.
/// </summary>
/// <remarks>
///     <para>
///         Requires an active script context.
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
/// <param name="script">The script to deserialize.</param>
/// <param name="sourceContext">
///     A cookie identifying the script that can be used by debuggable script contexts.
/// </param>
/// <param name="sourceUrl">The location the script came from</param>
/// <param name="parseAttributes">Attribute mask for parsing the script</param>
/// <param name="parserState">
///     A buffer containing a cache of the parser state generated by <c>JsSerializeParserState</c>.
/// </param>
/// <param name="result">A function representing the script. This parameter can be null.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
    JsDeserializeParserState(
        _In_ JsValueRef script,
        _In_ JsSourceContext sourceContext,
        _In_ JsValueRef sourceUrl,
        _In_ JsParseScriptAttributes parseAttributes,
        _In_ JsValueRef parserState,
        _Out_ JsValueRef* result);

typedef void (CHAKRA_CALLBACK *JsBeforeSweepCallback)(_In_opt_ void *callbackState);

CHAKRA_API
    JsSetRuntimeBeforeSweepCallback(
        _In_ JsRuntimeHandle runtimeHandle,
        _In_opt_ void *callbackState,
        _In_ JsBeforeSweepCallback beforeSweepCallback);

CHAKRA_API
JsSetRuntimeDomWrapperTracingCallbacks(
    _In_ JsRuntimeHandle runtimeHandle,
    _In_ JsRef wrapperTracingState,
    _In_ JsDOMWrapperTracingCallback wrapperTracingCallback,
    _In_ JsDOMWrapperTracingDoneCallback wrapperTracingDoneCallback,
    _In_ JsDOMWrapperTracingEnterFinalPauseCallback enterFinalPauseCallback);

CHAKRA_API
JsTraceExternalReference(
        _In_ JsRuntimeHandle runtimeHandle,
        _In_ JsValueRef value
    );

CHAKRA_API
JsAllocRawData(
    _In_ JsRuntimeHandle runtimeHandle,
    _In_ size_t sizeInBytes,
    _In_ bool zeroed,
    _Out_ JsRef * buffer
);

/// <summary>
///     A callback function to ask host to re-allocated buffer to the new size when the current buffer is full
/// </summary>
/// <param name="state">Pointer representing state of the serializer</param>
/// <param name="oldBuffer">An old memory buffer, which may be null, to be reallocated</param>
/// <param name="allocatedSize">Request host to allocate buffer of this size</param>
/// <param name="arrayBuffer">Actual size of the new buffer</param>
/// <returns>
///     New buffer will be returned upon success, null otherwise.
/// </returns>
typedef byte * (CHAKRA_CALLBACK *ReallocateBufferMemoryFunc)(void* state, byte *oldBuffer, size_t newSize, size_t *allocatedSize);

/// <summary>
///     A callback to ask host write current Host object to the serialization buffer.
/// </summary>
/// <returns>
///     A Boolean true is returned upon success, false otherwise.
/// </returns>
typedef bool (CHAKRA_CALLBACK *WriteHostObjectFunc)(void* state, void* hostObject);

/// <summary>
///     Initialize Serialization of the object.
/// </summary>
/// <param name="reallocateBufferMemory">A callback function to ask host to re-allocated buffer to the new size when the current buffer is full</param>
/// <param name="writeHostObject">A callback function to interact with host during serialization</param>
/// <param name="writeHostObject">A state object to pass to callback functions</param>
/// <param name="serializerHandle">A handle which provides various functionalities to serialize objects</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsVarSerializer(
    _In_ ReallocateBufferMemoryFunc reallocateBufferMemory,
    _In_ WriteHostObjectFunc writeHostObject,
    _In_opt_ void * callbackState,
    _Out_ JsVarSerializerHandle *serializerHandle);

/// <summary>
///     Write raw bytes to the buffer.
/// </summary>
/// <param name="source">Source byte buffer</param>
/// <param name="length">Length of bytes to write from source raw byte buffer</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsVarSerializerWriteRawBytes(
    _In_ JsVarSerializerHandle serializerHandle,
    _In_ const void* source,
    _In_ size_t length);

/// <summary>
///     A method to serialize given Javascript object to the serialization buffer
/// </summary>
/// <param name="rootObject">A Javascript object to be serialized</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsVarSerializerWriteValue(
    _In_ JsVarSerializerHandle serializerHandle,
    _In_ JsValueRef rootObject);

/// <summary>
///     A method to pass on the current serialized buffer (this buffer was allocated using ReallocateBufferMemory) to host.
/// </summary>
/// <param name="data">A buffer which holds current serialized data</param>
/// <param name="dataLength">Length of the buffer</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsVarSerializerReleaseData(
    _In_ JsVarSerializerHandle serializerHandle,
    _Out_ byte** data,
    _Out_ size_t *dataLength);

/// <summary>
///     Detach all array buffers which were passed using SetTransferableVars.
/// </summary>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsVarSerializerDetachArrayBuffer(_In_ JsVarSerializerHandle serializerHandle);

/// <summary>
///     Host provides all the objects which has transferable semantics (Such as ArrayBuffers).
/// </summary>
/// <param name="transferableVars">An array of transferable objects</param>
/// <param name="transferableVarsCount">Length of transferableVars array </param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsVarSerializerSetTransferableVars(
    _In_ JsVarSerializerHandle serializerHandle,
    _In_opt_ JsValueRef *transferableVars,
    _In_ size_t transferableVarsCount);

/// <summary>
///     Free current object (which was created upon JsVarSerializer) when the serialization is done. SerializerHandleBase object should not be used further after FreeSelf call.
/// </summary>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsVarSerializerFree(_In_ JsVarSerializerHandle serializerHandle);

/// <summary>
///     A callback to ask host to read the current data from the serialization buffer as a Host object.
/// </summary>
/// <returns>
///     A valid host object is returned upon success, an exception is thrown otherwise.
/// </returns>
typedef JsValueRef(*ReadHostObjectFunc)(void* state);

/// <summary>
///     A callback to ask host to retrieve SharedArrayBuffer object from given ID.
/// </summary>
/// <param name="id">An ID, which was provided by SerializerCallbackBase::GetSharedArrayBufferId method</param>
/// <returns>
///     A valid SharedArrayBuffer is returned upon success, an exception is thrown otherwise.
/// </returns>
typedef JsValueRef(*GetSharedArrayBufferFromIdFunc)(void* state, uint32_t id);

/// <summary>
///     Initiate Deserialization of the memory buffer to a Javascript object.
/// </summary>
/// <param name="data">A memory buffer which holds the serialized data</param>
/// <param name="size">Length of the passed data in bytes</param>
/// <param name="ReadHostObjectFunc">A callback to ask host to read the current data from the serialization buffer as a Host object.</param>
/// <param name="GetSharedArrayBufferFromIdFunc">A callback to ask host to retrieve SharedArrayBuffer object from given ID.</param>
/// <param name="callbackState">A callback object to interact with host during deserialization</param>
/// <param name="deserializerHandle">A handle which provides various functionalities to deserailize a buffer to an object</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsVarDeserializer(
    _In_ void *data,
    _In_ size_t size,
    _In_ ReadHostObjectFunc readHostObject,
    _In_ GetSharedArrayBufferFromIdFunc getSharedArrayBufferFromId,
    _In_opt_ void* callbackState,
    _Out_ JsVarDeserializerHandle *deserializerHandle);

/// <summary>
///     A method to read bytes from the serialized buffer. Caller should not allocate the data buffer.
/// </summary>
/// <param name="length">Advance current buffer's position by length</param>
/// <param name="data">The data will be pointing to the raw serialized buffer</param>
/// <returns>
///     A Boolean value true is returned upon success, false otherwise.
/// </returns>
CHAKRA_API
JsVarDeserializerReadRawBytes(_In_ JsVarDeserializerHandle deserializerHandle, _In_ size_t length, _Out_ void **data);

/// <summary>
///     A method to read bytes from the serialized buffer. Caller must allocate data buffer by length.
/// </summary>
/// <param name="length">Length of data buffer</param>
/// <param name="data">data buffer to be populated from the serialized buffer till the given length</param>
/// <returns>
///     A Boolean value true is returned upon success, false otherwise.
/// </returns>
CHAKRA_API
JsVarDeserializerReadBytes(_In_ JsVarDeserializerHandle deserializerHandle, _In_ size_t length, _Out_ void **data);

/// <summary>
///     Deserialized current buffer and pass the root object.
/// </summary>
/// <returns>
///     A valid Javascript object is returned upon success, an exception is thrown otherwise.
/// </returns>
CHAKRA_API
JsVarDeserializerReadValue(_In_ JsVarDeserializerHandle deserializerHandle, _Out_ JsValueRef* value);

/// <summary>
///     Host provides all the objects which has transferable semantics (Such as ArrayBuffers).
/// </summary>
/// <param name="transferableVars">An array of transferable objects</param>
/// <param name="transferableVarsCount">Length of transferableVars array </param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsVarDeserializerSetTransferableVars(_In_ JsVarDeserializerHandle deserializerHandle, _In_opt_ JsValueRef *transferableVars, _In_ size_t transferableVarsCount);

/// <summary>
///     Free current object (which was created upon JsVarSerializer) when the serialization is done. JsVarSerializerHandle object should not be used further after FreeSelf call.
/// </summary>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsVarDeserializerFree(_In_ JsVarDeserializerHandle deserializerHandle);

/// <summary>
///     Extract extra info stored from an ArrayBuffer object
/// </summary>
/// <param name="arrayBuffer">An ArrayBuffer from which the extrainfor needed to extracted</param>
/// <param name="extraInfo">The host information (some flags such as object externalized, detached) stored in the object</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsGetArrayBufferExtraInfo(
    _In_ JsValueRef arrayBuffer,
    _Out_ char *extraInfo);

/// <summary>
///     Set Extra info (host data) to an ArrayBuffer object.
/// </summary>
/// <param name="arrayBuffer">An ArrayBuffer on which the host information will be stored</param>
/// <param name="extraInfo">The host data</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsSetArrayBufferExtraInfo(
    _In_ JsValueRef arrayBuffer,
    _In_ char extraInfo);

/// <summary>
///     Neuter current ArrayBuffer
/// </summary>
/// <param name="arrayBuffer">An ArrayBuffer which will be neutered </param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsDetachArrayBuffer(
    _In_ JsValueRef arrayBuffer);

typedef void(__cdecl *ArrayBufferFreeFn)(void*);

/// <summary>
///     Returns the function which free the underlying buffer of ArrayBuffer
/// </summary>
/// <param name="arrayBuffer">An ArrayBuffer for which Free function to be returned </param>
/// <param name="freeFn">Free function will be returned</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code
///     otherwise.
/// </returns>
CHAKRA_API
JsGetArrayBufferFreeFunction(
    _In_ JsValueRef arrayBuffer,
    _Out_ ArrayBufferFreeFn* freeFn);

/// <summary>
///     Take ownership of current ArrayBuffer
/// </summary>
/// <param name="arrayBuffer">An ArrayBuffer to take ownership of</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsExternalizeArrayBuffer(
    _In_ JsValueRef arrayBuffer);

/// <summary>
///     Get host embedded data from the current object
/// </summary>
/// <param name="instance">Js object from which an embedder data to be fetched</param>
/// <param name="embedderData">An embedder data to be returned, it will be nullptr if not found</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code
///     otherwise.
/// </returns>
CHAKRA_API
JsGetEmbedderData(_In_ JsValueRef instance, _Out_ JsValueRef* embedderData);

/// <summary>
///     Set host embedded data on the current object
/// </summary>
/// <param name="instance">Js object from which an embedder data to be fetched</param>
/// <param name="embedderData">An embedder data to be set on the passed object</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code
///     otherwise.
/// </returns>
CHAKRA_API
JsSetEmbedderData(_In_ JsValueRef instance, _In_ JsValueRef embedderData);

#ifdef _WIN32
#include "ChakraCoreWindows.h"
#endif // _WIN32


#endif // _CHAKRACOREBUILD
#endif // _CHAKRACORE_H_
