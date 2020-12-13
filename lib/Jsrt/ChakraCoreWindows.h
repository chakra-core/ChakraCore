//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma once
#endif // _MSC_VER

#ifndef _CHAKRACOREWINDOWS_H_
#define _CHAKRACOREWINDOWS_H_

#include <rpc.h>

/// <summary>
///     Globally enables out-of-process JIT.
/// </summary>
/// <remarks>
///     Should be called before JS code is executed.
///     Code in all runtimes will run in interpreter until JsConnectJITProcess is called.
/// </remarks>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsEnableOOPJIT();

/// <summary>
///     Globally enables out-of-process JIT and connects to a Chakra JIT process that was initialized
///     by calling JsInitializeJITServer
/// </summary>
/// <remarks>
///     Out-of-process JIT should be enabled before JS code is executed.
/// </remarks>
/// <param name="processHandle">Handle to the JIT process</param>
/// <param name="serverSecurityDescriptor">Optional pointer to an RPC SECURITY_DESCRIPTOR structure</param>
/// <param name="connectionId">Same UUID that was passed to JsInitializeJITServer</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
CHAKRA_API
JsConnectJITProcess(_In_ HANDLE processHandle, _In_opt_ void* serverSecurityDescriptor, _In_ UUID connectionId);

#endif // _CHAKRACOREWINDOWS_H_
