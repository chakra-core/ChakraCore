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

#endif // _CHAKRACORE_H_
