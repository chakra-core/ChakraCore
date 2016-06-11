//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
/// \mainpage Chakra Hosting Debugging API Reference
///
/// Chakra is Microsoft's JavaScript engine. It is an integral part of Internet Explorer but can
/// also be hosted independently by other applications. This reference describes the APIs available
/// to applications to debug JavaScript.

/// \file
/// \brief The Chakra hosting debugging API.
///
/// This file contains a flat C API layer. This is the API exported by ChakraCore.dll.

#ifdef _MSC_VER
#pragma once
#endif  // _MSC_VER

#ifndef _CHAKRADEBUG_H_
#define _CHAKRADEBUG_H_

#include "ChakraCommon.h"

    /// <summary>
    ///     Debug events reported from ChakraCore engine.
    /// </summary>
    typedef enum _JsDiagDebugEvent
    {
        /// <summary>
        ///     Indicates a new script being compiled, this includes script, eval, new function.
        /// </summary>
        JsDiagDebugEventSourceCompile = 0,
        /// <summary>
        ///     Indicates compile error for a script.
        /// </summary>
        JsDiagDebugEventCompileError = 1,
        /// <summary>
        ///     Indicates a break due to a breakpoint.
        /// </summary>
        JsDiagDebugEventBreakpoint = 2,
        /// <summary>
        ///     Indicates a break after completion of step action.
        /// </summary>
        JsDiagDebugEventStepComplete = 3,
        /// <summary>
        ///     Indicates a break due to debugger statement.
        /// </summary>
        JsDiagDebugEventDebuggerStatement = 4,
        /// <summary>
        ///     Indicates a break due to async break.
        /// </summary>
        JsDiagDebugEventAsyncBreak = 5,
        /// <summary>
        ///     Indicates a break due to a runtime script exception.
        /// </summary>
        JsDiagDebugEventRuntimeException = 6
    } JsDiagDebugEvent;

    /// <summary>
    ///     Break on Exception attributes.
    /// </summary>
    typedef enum _JsDiagBreakOnExceptionAttributes
    {
        /// <summary>
        ///     Don't break on any exception.
        /// </summary>
        JsDiagBreakOnExceptionAttributeNone = 0x0,
        /// <summary>
        ///     Break on uncaught exception.
        /// </summary>
        JsDiagBreakOnExceptionAttributeUncaught = 0x1,
        /// <summary>
        ///     Break on first chance exception.
        /// </summary>
        JsDiagBreakOnExceptionAttributeFirstChance = 0x2
    } JsDiagBreakOnExceptionAttributes;

    /// <summary>
    ///     Stepping types.
    /// </summary>
    typedef enum _JsDiagStepType
    {
        /// <summary>
        ///     Perform a step operation to next statement.
        /// </summary>
        JsDiagStepTypeStepIn = 0,
        /// <summary>
        ///     Perform a step out from the current function.
        /// </summary>
        JsDiagStepTypeStepOut = 1,
        /// <summary>
        ///     Perform a single step over after a debug break if the next statement is a function call, else behaves as a stepin.
        /// </summary>
        JsDiagStepTypeStepOver = 2
    } JsDiagStepType;

    /// <summary>
    ///     User implemented callback routine for debug events.
    /// </summary>
    /// <remarks>
    ///     Use <c>JsDiagStartDebugging</c> to register the callback.
    /// </remarks>
    /// <param name="debugEvent">The type of JsDiagDebugEvent event.</param>
    /// <param name="eventData">Additional data related to the debug event.</param>
    /// <param name="callbackState">The state passed to <c>JsDiagStartDebugging</c>.</param>
    typedef void (CHAKRA_CALLBACK * JsDiagDebugEventCallback)(_In_ JsDiagDebugEvent debugEvent, _In_ JsValueRef eventData, _In_opt_ void* callbackState);

    /// <summary>
    ///     Starts debugging in the given runtime.
    /// </summary>
    /// <param name="runtimeHandle">Runtime to put into debug mode.</param>
    /// <param name="debugEventCallback">Registers a callback to be called on every JsDiagDebugEvent.</param>
    /// <param name="callbackState">User provided state that will be passed back to the callback.</param>
    /// <returns>
    ///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
    /// </returns>
    /// <remarks>
    ///     The runtime should be active on the current thread and should not be in debug state.
    /// </remarks>
    CHAKRA_API
        JsDiagStartDebugging(
            _In_ JsRuntimeHandle runtimeHandle,
            _In_ JsDiagDebugEventCallback debugEventCallback,
            _In_opt_ void* callbackState);

    /// <summary>
    ///     Stops debugging in the given runtime.
    /// </summary>
    /// <param name="runtimeHandle">Runtime to stop debugging.</param>
    /// <param name="callbackState">User provided state that was passed in JsDiagStartDebugging.</param>
    /// <returns>
    ///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
    /// </returns>
    /// <remarks>
    ///     The runtime should be active on the current thread and in debug state.
    /// </remarks>
    CHAKRA_API
        JsDiagStopDebugging(
            _In_ JsRuntimeHandle runtimeHandle,
            _Out_ void** callbackState);

    /// <summary>
    ///     Request the runtime to break on next JavaScript statement.
    /// </summary>
    /// <param name="runtimeHandle">Runtime to request break.</param>
    /// <returns>
    ///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
    /// </returns>
    /// <remarks>
    ///     The runtime should be in debug state. This API can be called from another runtime.
    /// </remarks>
    CHAKRA_API
        JsDiagRequestAsyncBreak(
            _In_ JsRuntimeHandle runtimeHandle);

    /// <summary>
    ///     List all breakpoints in the current runtime.
    /// </summary>
    /// <param name="breakpoints">Array of breakpoints.</param>
    /// <remarks>
    ///     <para>
    ///     [{
    ///         "breakpointId" : 1,
    ///         "scriptId" : 1,
    ///         "line" : 0,
    ///         "column" : 62
    ///     }]
    ///     </para>
    /// </remarks>
    /// <returns>
    ///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
    /// </returns>
    /// <remarks>
    ///     The current runtime should be in debug state. This API can be called when runtime is at a break or running.
    /// </remarks>
    CHAKRA_API
        JsDiagGetBreakpoints(
            _Out_ JsValueRef *breakpoints);

    /// <summary>
    ///     Sets breakpoint in the specified script at give location.
    /// </summary>
    /// <param name="scriptId">Id of script from JsDiagGetScripts or JsDiagGetSource to put breakpoint.</param>
    /// <param name="lineNumber">0 based line number to put breakpoint.</param>
    /// <param name="columnNumber">0 based column number to put breakpoint.</param>
    /// <param name="breakpoint">Breakpoint object with id, line and column if success.</param>
    /// <remarks>
    ///     <para>
    ///     {
    ///         "breakpointId" : 1,
    ///         "line" : 2,
    ///         "column" : 4
    ///     }
    ///     </para>
    /// </remarks>
    /// <returns>
    ///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
    /// </returns>
    /// <remarks>
    ///     The current runtime should be in debug state. This API can be called when runtime is at a break or running.
    /// </remarks>
    CHAKRA_API
        JsDiagSetBreakpoint(
            _In_ unsigned int scriptId,
            _In_ unsigned int lineNumber,
            _In_ unsigned int columnNumber,
            _Out_ JsValueRef *breakpoint);

    /// <summary>
    ///     Remove a breakpoint.
    /// </summary>
    /// <param name="breakpointId">Breakpoint id returned from JsDiagSetBreakpoint.</param>
    /// <returns>
    ///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
    /// </returns>
    /// <remarks>
    ///     The current runtime should be in debug state. This API can be called when runtime is at a break or running.
    /// </remarks>
    CHAKRA_API
        JsDiagRemoveBreakpoint(
            _In_ unsigned int breakpointId);

    /// <summary>
    ///     Sets break on exception handling.
    /// </summary>
    /// <param name="runtimeHandle">Runtime to set break on exception attributes.</param>
    /// <param name="exceptionAttributes">Mask of JsDiagBreakOnExceptionAttributes to set.</param>
    /// <remarks>
    ///     <para>
    ///         If this API is not called the default value is set to JsDiagBreakOnExceptionAttributeUncaught in the runtime.
    ///     </para>
    /// </remarks>
    /// <returns>
    ///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
    /// </returns>
    /// <remarks>
    ///     The runtime should be in debug state. This API can be called from another runtime.
    /// </remarks>
    CHAKRA_API
        JsDiagSetBreakOnException(
            _In_ JsRuntimeHandle runtimeHandle,
            _In_ JsDiagBreakOnExceptionAttributes exceptionAttributes);

    /// <summary>
    ///     Gets break on exception setting.
    /// </summary>
    /// <param name="runtimeHandle">Runtime from which to get break on exception attributes, should be in debug mode.</param>
    /// <param name="exceptionAttributes">Mask of JsDiagBreakOnExceptionAttributes.</param>
    /// <returns>
    ///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
    /// </returns>
    /// <remarks>
    ///     The runtime should be in debug state. This API can be called from another runtime.
    /// </remarks>
    CHAKRA_API
        JsDiagGetBreakOnException(
            _In_ JsRuntimeHandle runtimeHandle,
            _Out_ JsDiagBreakOnExceptionAttributes* exceptionAttributes);

    /// <summary>
    ///     Sets the step type in the runtime after a debug break.
    /// </summary>
    /// <remarks>
    ///     Requires to be at a debug break.
    /// </remarks>
    /// <param name="resumeType">Type of JsDiagStepType.</param>
    /// <returns>
    ///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
    /// </returns>
    /// <remarks>
    ///     The current runtime should be in debug state. This API can only be called when runtime is at a break.
    /// </remarks>
    CHAKRA_API
        JsDiagSetStepType(
            _In_ JsDiagStepType stepType);

    /// <summary>
    ///     Gets list of scripts.
    /// </summary>
    /// <param name="scriptsArray">Array of script objects.</param>
    /// <remarks>
    ///     <para>
    ///     [{
    ///         "scriptId" : 2,
    ///         "fileName" : "c:\\Test\\Test.js",
    ///         "lineCount" : 4,
    ///         "sourceLength" : 111
    ///       }, {
    ///         "scriptId" : 3,
    ///         "parentScriptId" : 2,
    ///         "scriptType" : "eval code",
    ///         "lineCount" : 1,
    ///         "sourceLength" : 12
    ///     }]
    ///     </para>
    /// </remarks>
    /// <returns>
    ///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
    /// </returns>
    /// <remarks>
    ///     The current runtime should be in debug state. This API can be called when runtime is at a break or running.
    /// </remarks>
    CHAKRA_API
        JsDiagGetScripts(
            _Out_ JsValueRef *scriptsArray);

    /// <summary>
    ///     Gets source for a specific script identified by scriptId from JsDiagGetScripts.
    /// </summary>
    /// <param name="scriptId">Id of the script.</param>
    /// <param name="source">Source object.</param>
    /// <remarks>
    ///     <para>
    ///     {
    ///         "scriptId" : 1,
    ///         "fileName" : "c:\\Test\\Test.js",
    ///         "lineCount" : 12,
    ///         "sourceLength" : 15154,
    ///         "source" : "var x = 1;"
    ///     }
    ///     </para>
    /// </remarks>
    /// <returns>
    ///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
    /// </returns>
    /// <remarks>
    ///     The current runtime should be in debug state. This API can be called when runtime is at a break or running.
    /// </remarks>
    CHAKRA_API
        JsDiagGetSource(
            _In_ unsigned int scriptId,
            _Out_ JsValueRef *source);

    /// <summary>
    ///     Gets the source information for a function object.
    /// </summary>
    /// <param name="function">JavaScript function.</param>
    /// <param name="functionPosition">Function position - scriptId, start line, start column, line number of first statement, column number of first statement.</param>
    /// <remarks>
    ///     <para>
    ///     {
    ///         "scriptId" : 1,
    ///         "fileName" : "c:\\Test\\Test.js",
    ///         "line" : 1,
    ///         "column" : 2,
    ///         "firstStatementLine" : 6,
    ///         "firstStatementColumn" : 0
    ///     }
    ///     </para>
    /// </remarks>
    /// <returns>
    ///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
    /// </returns>
    /// <remarks>
    ///     This API can be called when runtime is at a break or running.
    /// </remarks>
    CHAKRA_API
        JsDiagGetFunctionPosition(
            _In_ JsValueRef function,
            _Out_ JsValueRef *functionPosition);

    /// <summary>
    ///     Gets the stack trace information.
    /// </summary>
    /// <param name="stackTrace">Stack trace information.</param>
    /// <remarks>
    ///     <para>
    ///     [{
    ///         "index" : 0,
    ///         "scriptId" : 2,
    ///         "line" : 3,
    ///         "column" : 0,
    ///         "sourceLength" : 9,
    ///         "sourceText" : "var x = 1",
    ///         "functionHandle" : 1
    ///     }]
    ///    </para>
    /// </remarks>
    /// <returns>
    ///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
    /// </returns>
    /// <remarks>
    ///     The current runtime should be in debug state. This API can only be called when runtime is at a break.
    /// </remarks>
    CHAKRA_API
        JsDiagGetStackTrace(
            _Out_ JsValueRef *stackTrace);

    /// <summary>
    ///     Gets the list of properties corresponding to the frame.
    /// </summary>
    /// <param name="stackFrameIndex">Index of stack frame from JsDiagGetStackTrace.</param>
    /// <param name="properties">Object of properties array (properties, scopes and globals).</param>
    /// <remarks>
    ///     <para>
    ///     propertyAttributes is a bit mask of
    ///         NONE = 0x1,
    ///         HAVE_CHILDRENS = 0x2,
    ///         READ_ONLY_VALUE = 0x4,
    ///     </para>
    /// </remarks>
    /// <remarks>
    ///     <para>
    ///     {
    ///         "thisObject": {
    ///             "name": "this",
    ///             "type" : "object",
    ///             "className" : "Object",
    ///             "display" : "{...}",
    ///             "propertyAttributes" : 1,
    ///             "handle" : 306
    ///         },
    ///         "exception" : {
    ///             "name" : "{exception}",
    ///             "type" : "object",
    ///             "display" : "'a' is undefined",
    ///             "className" : "Error",
    ///             "propertyAttributes" : 1,
    ///             "handle" : 307
    ///         }
    ///         "arguments" : {
    ///             "name" : "arguments",
    ///             "type" : "object",
    ///             "display" : "{...}",
    ///             "className" : "Object",
    ///             "propertyAttributes" : 1,
    ///             "handle" : 190
    ///         },
    ///         "returnValue" : {
    ///             "name" : "[Return value]",
    ///             "type" : "undefined",
    ///             "propertyAttributes" : 0,
    ///             "handle" : 192
    ///         },
    ///         "functionCallsReturn" : [{
    ///                 "name" : "[foo1 returned]",
    ///                 "type" : "number",
    ///                 "value" : 1,
    ///                 "propertyAttributes" : 2,
    ///                 "handle" : 191
    ///             }
    ///         ],
    ///         "locals" : [],
    ///         "scopes" : [{
    ///                 "index" : 0,
    ///                 "handle" : 193
    ///             }
    ///         ],
    ///         "globals" : {
    ///             "handle" : 194
    ///         }
    ///     }
    ///     </para>
    /// </remarks>
    /// <returns>
    ///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
    /// </returns>
    /// <remarks>
    ///     The current runtime should be in debug state. This API can only be called when runtime is at a break.
    /// </remarks>
    CHAKRA_API
        JsDiagGetStackProperties(
            _In_ unsigned int stackFrameIndex,
            _Out_ JsValueRef *properties);

    /// <summary>
    ///     Gets the list of children of a handle.
    /// </summary>
    /// <param name="objectHandle">Handle of object.</param>
    /// <param name="fromCount">0-based from count of properties, usually 0.</param>
    /// <param name="totalCount">Number of properties to return.</param>
    /// <param name="propertiesObject">Array of properties.</param>
    /// <remarks>Handle should be from objects returned from call to JsDiagGetStackProperties.</remarks>
    /// <remarks>For scenarios where object have large number of properties totalCount can be used to control how many properties are given.</remarks>
    /// <remarks>
    ///     <para>
    ///     {
    ///         "totalPropertiesOfObject": 10,
    ///         "properties" : [{
    ///                 "name" : "__proto__",
    ///                 "type" : "object",
    ///                 "display" : "{...}",
    ///                 "className" : "Object",
    ///                 "propertyAttributes" : 1,
    ///                 "handle" : 156
    ///             }
    ///         ],
    ///         "debuggerOnlyProperties" : [{
    ///                 "name" : "[Map]",
    ///                 "type" : "string",
    ///                 "value" : "size = 0",
    ///                 "propertyAttributes" : 2,
    ///                 "handle" : 157
    ///             }
    ///         ]
    ///     }
    ///     </para>
    /// </remarks>
    /// <returns>
    ///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
    /// </returns>
    /// <remarks>
    ///     The current runtime should be in debug state. This API can only be called when runtime is at a break.
    /// </remarks>
    CHAKRA_API
        JsDiagGetProperties(
            _In_ unsigned int objectHandle,
            _In_ unsigned int fromCount,
            _In_ unsigned int totalCount,
            _Out_ JsValueRef *propertiesObject);

    /// <summary>
    ///     Gets the object corresponding to handle.
    /// </summary>
    /// <param name="objectHandle">Handle of object.</param>
    /// <param name="handleObject">Object corresponding to the handle.</param>
    /// <remarks>
    ///     <para>
    ///     {
    ///         "scriptId" : 24,
    ///          "line" : 1,
    ///          "column" : 63,
    ///          "name" : "foo",
    ///          "type" : "function",
    ///          "handle" : 2
    ///     }
    ///    </para>
    /// </remarks>
    /// <returns>
    ///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
    /// </returns>
    /// <remarks>
    ///     The current runtime should be in debug state. This API can only be called when runtime is at a break.
    /// </remarks>
    CHAKRA_API
        JsDiagGetObjectFromHandle(
            _In_ unsigned int objectHandle,
            _Out_ JsValueRef *handleObject);

#ifdef _WIN32
    /// <summary>
    ///     Evaluates an expression on given frame.
    /// </summary>
    /// <param name="expression">Expression to evaluate.</param>
    /// <param name="stackFrameIndex">Index of stack frame on which to evaluate the expression.</param>
    /// <param name="evalResult">Result of evaluation.</param>
    /// <remarks>
    ///     <para>
    ///     evalResult when evaluating 'this' and return is JsNoError
    ///     {
    ///         "name" : "this",
    ///         "type" : "object",
    ///         "className" : "Object",
    ///         "display" : "{...}",
    ///         "propertyAttributes" : 1,
    ///         "handle" : 18
    ///     }
    ///
    ///     evalResult when evaluating a script which throws JavaScript error and return is JsErrorScriptException
    ///     {
    ///         "name" : "a.b.c",
    ///         "type" : "object",
    ///         "className" : "Error",
    ///         "display" : "'a' is undefined",
    ///         "propertyAttributes" : 1,
    ///         "handle" : 18
    ///     }
    ///     </para>
    /// </remarks>
    /// <returns>
    ///     The code <c>JsNoError</c> if the operation succeeded, evalResult will contain the result
    ///     The code <c>JsErrorScriptException</c> if evaluate generated a JavaScript exception, evalResult will contain the error details
    ///     Other error code for invalid parameters or API was not called at break
    /// </returns>
    /// <remarks>
    ///     The current runtime should be in debug state. This API can only be called when runtime is at a break.
    /// </remarks>
    CHAKRA_API
        JsDiagEvaluate(
            _In_ const wchar_t *expression,
            _In_ unsigned int stackFrameIndex,
            _Out_ JsValueRef *evalResult);
#endif // _WIN32

#endif // _CHAKRADEBUG_H_
