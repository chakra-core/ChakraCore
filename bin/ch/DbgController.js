//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var TRACE_COMMANDS = 0x001;
var TRACE_DIAG_OUTPUT = 0x002;
var TRACE_INTERNAL_FUNCTIONS = 0x004;
var TRACE_DEBUG_EVENTS = 0x008;
var TRACE_ALL = TRACE_COMMANDS | TRACE_DIAG_OUTPUT | TRACE_INTERNAL_FUNCTIONS | TRACE_DEBUG_EVENTS;


// Have all JsDiag* functions installed on it by Debugger.cpp
var hostDebugObject = {};

var controllerObj = (function () {
    var _commandList = [];
    var _commandCompletions = [];
    var _wasResumed = false;
    var _currentStackFrameIndex = 0;
    var _trace = 0x0000;
    var _eventLog = [];
    var _baseline = undefined;
    var _exceptionCommands = undefined;

    _trace = TRACE_ALL;

    function internalPrint(str) {
        WScript.Echo(str);
    }

    function isTracing(traceFlag) {
        return _trace & traceFlag;
    }

    function internalTrace(traceFlag, ...varArgs) {
        if (isTracing(traceFlag)) {
            var str = "";
            varArgs.map(function (element) {
                str += (typeof element != "string") ? JSON.stringify(element, undefined, "  ") : element;
            });
            internalPrint(str);
        }
    }

    function printError(str) {
        internalPrint("Error: " + str);
    }

    function callHostFunction(fn) {
        var result = fn.apply(undefined, [].slice.call(arguments, 1));
        var obj = {};
        obj[fn.name] = result;
        internalTrace(TRACE_DIAG_OUTPUT, obj);
        return result;
    }

    function recordEvent(json) {
        _eventLog.push(json);
    }

    // remove path from file name and just have the filename with extension
    function filterFileName(fileName) {
        try {
            var index = fileName.lastIndexOf("\\");
            if (index >= 0) {
                return fileName.substring(index + 1);
            }
        } catch (ex) { }
        return "";
    }

    var bpManager = (function () {
        var _bpMap = [];
        var _locBpId = -1;

        function getBpFromName(name) {
            for (var i in _bpMap) {
                if (_bpMap[i].name === name) {
                    return _bpMap[i];
                }
            }
            printError("Breakpoint named '" + name + "' was not found");
        }

        function internalSetBp(name, scriptId, line, column, execStr) {
            var bpObject = callHostFunction(hostDebugObject.JsDiagSetBreakpoint, scriptId, line, column);
            return {
                id: bpObject.breakpointId,
                scriptId : scriptId,
                name: name,
                line: bpObject.line,
                column: bpObject.column,
                execStr: execStr
            };
        }

        function addBpObject(bpObj)
        {
            internalTrace(TRACE_INTERNAL_FUNCTIONS, "addBpObject: ", bpObj);
            _bpMap[bpObj.id] = bpObj;
        }

        return {
            setBreakpoint: function (name, scriptId, line, column, execStr) {
                var bpObj = internalSetBp(name, scriptId, line, column, execStr);
                addBpObject(bpObj);
            },
            setLocationBreakpoint: function (name, scriptId, line, column, execStr) {
                var bpObj = {
                    id: _locBpId--,
                    scriptId: scriptId,
                    name: name,
                    line: line,
                    column: column,
                    execStr: execStr
                }
                addBpObject(bpObj);
            },
            enableBreakpoint: function (name) {
                var bpObj = getBpFromName(name);
                delete _bpMap[bpObj.id];
                internalTrace(TRACE_INTERNAL_FUNCTIONS, "enableBreakpoint: ", name, " bpObj: ", bpObj);
                bpObj = internalSetBp(bpObj.name, bpObj.scriptId, bpObj.line, bpObj.column, bpObj.execStr);
                addBpObject(bpObj);
            },
            deleteBreakpoint: function (name) {
                var bpObj = getBpFromName(name);
                internalTrace(TRACE_INTERNAL_FUNCTIONS, "deleteBreakpoint: ", name, " bpObj: ", bpObj);
                callHostFunction(hostDebugObject.JsDiagRemoveBreakpoint, bpObj.id);
            },
            getExecStr: function (id) {
                for (var i in _bpMap) {
                    if (_bpMap[i].id === id) {
                        return _bpMap[i].execStr;
                    }
                }
                printError("Breakpoint '" + id + "' was not found");
            },
            setExecStr: function (id, newExecStr) {
                for (var i in _bpMap) {
                    if (_bpMap[i].id === id) {
                        _bpMap[i].execStr = newExecStr;
                    }
                }
            }
        }
    })();

    function addSourceFile(text, srcId) {
        try {
            // Split the text into lines.  Note this doesn't take into account block comments, but that's probably okay.
            var lines = text.split(/\n/);

            // /**bp        <-- a breakpoint
            // /**loc       <-- a named source location used for enabling bp at later stage
            // /**exception <-- set exception handling, catch all or only uncaught exception

            var bpStartToken = "/**";
            var bpStartStrings = ["bp", "loc", "exception"];
            var bpEnd = "**/";

            // Iterate through each source line, setting any breakpoints.
            for (var i = 0; i < lines.length; ++i) {
                var line = lines[i];
                for (var startString in bpStartStrings) {
                    var bpStart = bpStartToken + bpStartStrings[startString];
                    var isLocationBreakpoint = (bpStart.indexOf("loc") != -1);
                    var isExceptionBreakpoint = (bpStart.indexOf("exception") != -1);
                    var startIdx = -1;
                    while ((startIdx = line.indexOf(bpStart, startIdx + 1)) != -1) {
                        var endIdx;
                        var currLine = i;
                        var bpLine = i;
                        var currBpLineString = "";

                        // Gather up any lines within the breakpoint comment.
                        do {
                            var currentStartIdx = 0;
                            if (currLine == i) {
                                currentStartIdx = startIdx;
                            }
                            currBpLineString += lines[currLine++];
                            endIdx = currBpLineString.indexOf(bpEnd, currentStartIdx);
                        } while (endIdx == -1 && currLine < lines.length && lines[currLine].indexOf(bpStartToken) == -1);

                        // Move the line cursor forward, allowing the current line to be re-checked
                        i = currLine - 1;

                        // Do some checking
                        if (endIdx == -1) {
                            printError("Unterminated breakpoint expression");
                            return;
                        }

                        var bpStrStartIdx = startIdx + bpStart.length;
                        var bpStr = currBpLineString.substring(bpStrStartIdx, endIdx);

                        var bpFullStr = currBpLineString.substring(startIdx, endIdx);

                        // Quick check to make sure the breakpoint is not within a
                        // quoted string (such as an eval).  If it is within an eval, the
                        // eval will cause a separate call to have its breakpoints parsed.
                        // This check can be defeated, but it should cover the useful scenarios.
                        var quoteCount = 0;
                        var escapeCount = 0;
                        for (var j = 0; j < bpStrStartIdx; ++j) {
                            switch (currBpLineString[j]) {
                                case '\\':
                                    escapeCount++;
                                    continue;
                                case '"':
                                case '\'':
                                    if (escapeCount % 2 == 0)
                                        quoteCount++;
                                    /* fall through */
                                default:
                                    escapeCount = 0;
                            }
                        }
                        if (quoteCount % 2 == 1) {
                            // The breakpoint was in a quoted string.
                            continue;
                        }

                        // Only support strings like:
                        //  /**bp**/
                        //  /**bp(name)**/
                        //  /**bp(columnoffset)**/         takes an integer
                        //  /**bp:locals();stack()**/
                        //  /**bp(name):locals();stack()**/
                        //  /**loc(name)**/
                        //  /**loc(name):locals();stack()**/
                        //

                        // Parse the breakpoint name (if it exists)
                        var bpName = undefined;
                        var bpColumnOffset = 0;
                        var bpExecStr = undefined;
                        var parseIdx = 0;
                        if (bpStr[parseIdx] == '(') {
                            // The name and offset is overloaded with the same parameter.
                            // if that is int (determined by parseInt), then it is column offset otherwise left as name.
                            bpName = bpStr.match(/\(([\w,]+?)\)/)[1];
                            parseIdx = bpName.length + 2;
                            bpColumnOffset = parseInt(bpName);
                            if (isNaN(bpColumnOffset)) {
                                bpColumnOffset = 0;
                            } else {
                                bpName = undefined;
                                if (bpColumnOffset > line.length) {
                                    bpColumnOffset = line.length - 1;
                                } else if (bpColumnOffset < 0) {
                                    bpColumnOffset = 0;
                                }
                            }
                        } else if (isLocationBreakpoint) {
                            printError("'loc' sites require a label, for example /**loc(myFunc)**/");
                            return;
                        }

                        // Process the exception label:
                        //   exception(none)
                        //   exception(uncaught)
                        //   exception(all)
                        if (isExceptionBreakpoint) {
                            var exceptionType = -1;
                            if (bpName !== undefined) {
                                if (bpName == "none") {
                                    exceptionType = 0; // JsDiagBreakOnExceptionTypeNone
                                } else if (bpName == "uncaught") {
                                    exceptionType = 1; // JsDiagBreakOnExceptionTypeUncaught
                                } else if (bpName == "all") {
                                    exceptionType = 2; // JsDiagBreakOnExceptionTypeAll
                                }
                            } else {
                                // throw "No exception type specified";
                            }
                            callHostFunction(hostDebugObject.JsDiagSetBreakOnException, exceptionType);
                        }

                        // Parse the breakpoint execution string
                        if (bpStr[parseIdx] == ':') {
                            bpExecStr = bpStr.substring(parseIdx + 1);
                        } else if (parseIdx != bpStr.length) {
                            printError("Invalid breakpoint string: " + bpStr);
                            return;
                        }

                        // Insert the breakpoint at the beginning of the line.
                        if (isExceptionBreakpoint) {
                            if (_exceptionCommands != undefined) {
                                printError("More than one 'exception' annotation found");
                                return;
                            }
                            _exceptionCommands = bpExecStr;
                        } else {
                            if (!isLocationBreakpoint) {
                                bpManager.setBreakpoint(bpName, srcId, bpLine, bpColumnOffset, bpExecStr);
                            } else {
                                bpManager.setLocationBreakpoint(bpName, srcId, bpLine, bpColumnOffset, bpExecStr);
                            }
                        }
                    }
                }
            }
        } catch (ex) {
            printError(ex);
        }
    }

    function handleBreakpoint(id) {
        internalTrace(TRACE_INTERNAL_FUNCTIONS, "handleBreakpoint id: ", id)
        _wasResumed = false;
        if (id != -1) {
            try {
                var execStr = "";
                if (id === "exception") {
                    execStr = _exceptionCommands;
                    if (execStr && execStr.toString().search("removeExpr()") != -1) {
                        _exceptionCommands = undefined;
                    }
                } else {
                    // Retrieve this breakpoint's execution string
                    execStr = bpManager.getExecStr(id);
                    if (execStr.toString().search("removeExpr()") != -1) {
                        // A hack to remove entire expression, so that it will not run again.
                        bpManager.setExecStr(id, null);
                    }
                }
                internalTrace(TRACE_INTERNAL_FUNCTIONS, "handleBreakpoint execStr: ", execStr)
                if (execStr != null) {
                    eval(execStr);
                }
            } catch (ex) {
                printError(ex);
            }
        }

        internalTrace(TRACE_INTERNAL_FUNCTIONS, "_commandList length: ", _commandList.length, " _wasResumed: ", _wasResumed);

        // Continue processing the command list.
        while (_commandList.length > 0 && !_wasResumed) {
            var cmd = _commandList.shift();
            internalTrace(TRACE_INTERNAL_FUNCTIONS, "cmd: ", cmd);
            var completion = cmd.fn.apply(this, cmd.args);
            if (typeof completion === "function") {
                _commandCompletions.push(completion);
            }
        }
        while (_commandCompletions.length > 0) {
            var completion = _commandCompletions.shift();
            completion();
        }

        if (!_wasResumed) {
            _currentStackFrameIndex = 0;
            _wasResumed = true;
        }
    }

    function GetChild(obj, level) {
        function GetChildrens(obj, level) {
            for (var i = 0; i < obj.length; ++i) {
                GetChild(obj[i], level);
            }
        }
        if ("handle" in obj) {
            if (level >= 0) {
                var childProps = callHostFunction(hostDebugObject.JsDiagGetProperties, obj["handle"], 1000, 1000);
                if (childProps && (obj["handle"] in childProps)) {
                    var properties = childProps[obj["handle"]]["properties"];
                    GetChildrens(properties, level - 1);

                    var debuggerOnlyProperties = childProps[obj["handle"]]["debuggerOnlyProperties"];
                    GetChildrens(debuggerOnlyProperties, level - 1);

                    Array.prototype.push.apply(properties, debuggerOnlyProperties);
                    if (properties.length > 0) {
                        obj["childrens"] = properties;
                    }
                }
            }
            delete obj["handle"];
        }
        if ("propertyAttributes" in obj) {
            delete obj["propertyAttributes"];
        }
    }

    return {
        pushCommand: function pushCommand(fn, args) {
            _commandList.push({
                fn: fn,
                args: args
            });
        },
        debuggerCommands: {
            log: function (str) {
                internalPrint("LOG: " + str);
            },
            logJson: function (str) {
                internalPrint(str);
            },
            resume: function (kind) {
                if (_wasResumed) {
                    printError("Breakpoint resumed twice");
                } else {
                    var stepType = -1;
                    if (kind == "step_into") {
                        stepType = 0;
                    } else if (kind == "step_out") {
                        stepType = 1;
                    } else if (kind == "step_over") {
                        stepType = 2;
                    } else if (kind == "step_document") {
                        stepType = 0;
                    }
                    if (stepType != -1) {
                        callHostFunction(hostDebugObject.JsDiagSetStepType, stepType);
                    } else if (kind != "continue") {
                        throw new Error("Unhandled step type - " + kind);
                    }
                    _wasResumed = true;
                    _currentStackFrameIndex = 0;
                }
            },
            locals: function (expandLevel, flags) {
                if (expandLevel == undefined || expandLevel < 0) {
                    expandLevel = 0;
                }
                var localsObject = callHostFunction(hostDebugObject.JsDiagGetStackProperties, _currentStackFrameIndex);

                if (expandLevel >= 0) {
                    if ("exception" in localsObject) {
                        GetChild(localsObject["exception"], expandLevel - 1);
                    }
                    if ("arguments" in localsObject) {
                        GetChild(localsObject["arguments"], expandLevel - 1);
                    }
                    if ("returnValue" in localsObject) {
                        GetChild(localsObject["returnValue"], expandLevel - 1);
                    }
                    if ("functionCallsReturn" in localsObject) {
                        var functionCallsReturn = localsObject["functionCallsReturn"];
                        for (var i = 0; i < functionCallsReturn.length; ++i) {
                            GetChild(functionCallsReturn[i], expandLevel - 1);
                        }
                    }
                    if ("locals" in localsObject) {
                        var locals = localsObject["locals"];
                        for (var i = 0; i < locals.length; ++i) {
                            GetChild(locals[i], expandLevel - 1);
                        }
                    }
                    if ("scopes" in localsObject) {
                        var scopes = localsObject["scopes"];
                        for (var i = 0; i < scopes.length; ++i) {
                            GetChild(scopes[i], expandLevel - 1);
                        }
                    }
                    if ("globals" in localsObject) {
                        var globals = localsObject["globals"];
                        GetChild(globals, expandLevel - 1);
                    }

                    

                    recordEvent(localsObject);
                }
            },
            stack: function (flags) {
                if (flags === undefined) {
                    flags = 0;
                }

                var stackTrace = callHostFunction(hostDebugObject.JsDiagGetStackTrace);

                for (var i = 0; i < stackTrace.length; ++i) {
                    var stack = stackTrace[i];
                    stack.fileName = filterFileName(stack.fileName);

                    var lookupObject = callHostFunction(hostDebugObject.JsDiagGetObjectFromHandle, stackTrace[i].functionHandle);
                    if ("fileName" in lookupObject) {
                        lookupObject["fileName"] = filterFileName(lookupObject["fileName"]);
                    }
                    if ("handle" in lookupObject) {
                        delete lookupObject["handle"];
                    }

                    stackTrace[i]["functionHandle"] = lookupObject;

                    lookupObject = callHostFunction(hostDebugObject.JsDiagGetObjectFromHandle, stackTrace[i].scriptHandle);
                    if ("fileName" in lookupObject) {
                        lookupObject["fileName"] = filterFileName(lookupObject["fileName"]);
                    }
                    if ("handle" in lookupObject) {
                        delete lookupObject["handle"];
                    }


                    stackTrace[i]["scriptHandle"] = lookupObject;
                    delete stack["handle"];
                }

                var lookupHandles = [];
                for (var i = 0; i < stackTrace.length; ++i) {
                    lookupHandles.push(stackTrace[i].functionHandle);
                    lookupHandles.push(stackTrace[i].scriptHandle);
                }

                recordEvent({
                    'callStack': stackTrace
                });
            },
            evaluate: function (expression, expandLevel, flags) {
                if (expression != undefined) {
                    var evalResult = callHostFunction(hostDebugObject.JsDiagEvaluate, _currentStackFrameIndex, expression);
                    GetChild(evalResult, expandLevel - 1);
                    recordEvent({
                        'evaluate': evalResult
                    });
                }
            },
            enableBp: function (name) {
                bpManager.enableBreakpoint(name);
            },
            disableBp: function (name) {
                bpManager.deleteBreakpoint(name);
            },
            deleteBp: function (name) {
                bpManager.deleteBreakpoint(name);
            },
            setFrame: function (depth) {
                var stackTrace = callHostFunction(hostDebugObject.JsDiagGetStackTrace);
                for (var i = 0; i < stackTrace.length; ++i) {
                    if (stackTrace[i].index == depth) {
                        _currentStackFrameIndex = depth;
                        break;
                    }
                }
            },
            dumpBreak: function () {
                var breakpoints = callHostFunction(hostDebugObject.JsDiagGetBreakpoints);
                recordEvent({
                    'breakpoints': breakpoints
                });
            },
            dumpSourceList: function () {
                var sources = callHostFunction(hostDebugObject.JsDiagGetScripts);
                recordEvent({
                    'sources': sources
                });
            },
            trace: function (traceFlag) {
                _trace |= traceFlag;
            }
        },
        setBaseline: function (baseline) {
            try {
                _baseline = JSON.parse(baseline);
            } catch (ex) {
                printError("Invalid JSON passed to setBaseline: " + ex);
                internalPrint(baseline);
            }
        },
        getOutputJson: function () {
            return JSON.stringify(_eventLog, undefined, "  ");
        },
        handleDebugEvent: function (debugEvent, eventData) {
            function debugEventToString(debugEvent) {
                switch (debugEvent) {
                    case 0: return "JsDiagDebugEventSourceCompile";
                    case 1: return "JsDiagDebugEventCompileError";
                    case 2: return "JsDiagDebugEventBreak";
                    case 3: return "JsDiagDebugEventStepComplete";
                    case 4: return "JsDiagDebugEventDebuggerStatement";
                    case 5: return "JsDiagDebugEventAsyncBreak";
                    case 6: return "JsDiagDebugEventRuntimeException";
                    default: return "UnKnown";
                }
            }
            internalTrace(TRACE_DEBUG_EVENTS, { 'debugEvent': debugEventToString(debugEvent), 'eventData': eventData });
            switch (debugEvent) {
                case 0:
                    /*JsDiagDebugEventSourceCompile*/
                    var source = callHostFunction(hostDebugObject.JsDiagGetSource, eventData.scriptId);
                    addSourceFile(source.source, source.scriptId);
                    break;
                case 1:
                    /*JsDiagDebugEventCompileError*/
                    var stackTrace = callHostFunction(hostDebugObject.JsDiagGetScripts);
                    break;
                case 2:
                    /*JsDiagDebugEventBreak*/
                case 3:
                    /*JsDiagDebugEventStepComplete*/
                case 4:
                    /*JsDiagDebugEventDebuggerStatement*/
                    handleBreakpoint(("breakpointId" in eventData) ? eventData.breakpointId : -1);
                    break;
                case 5:
                    /*JsDiagDebugEventAsyncBreak*/
                    break;
                case 6:
                    /*JsDiagDebugEventRuntimeException*/
                    handleBreakpoint("exception");
                    break;
                default:
                    throw "Unhandled JsDiagDebugEvent value " + debugEvent;
                    break;
            }
        },
        handleSourceRunDown: function (sources) {
            for (var len = 0; len < sources.length; ++len) {
                var source = callHostFunction(hostDebugObject.JsDiagGetSource, sources[len].scriptId);
                addSourceFile(source.source, source.scriptId);
            };
        },
    }
})();


// Commands for use from the breakpoint execution string in test files
function log(str) {
    // Prints given string as 'LOG: <given string>' on console
    controllerObj.pushCommand(controllerObj.debuggerCommands.log, arguments);
}
function logJson(str) {
    // Prints given string on console
    controllerObj.pushCommand(controllerObj.debuggerCommands.logJson, arguments);
}
function resume(kind) {
    // Resume exceution after a break, kinds - step_into, step_out, step_over
    controllerObj.pushCommand(controllerObj.debuggerCommands.resume, arguments);
}
function locals(expandLevel, flags) {
    // Dumps locals tree, expand till expandLevel with given flags
    controllerObj.pushCommand(controllerObj.debuggerCommands.locals, arguments);
}
function stack() {
    controllerObj.pushCommand(controllerObj.debuggerCommands.stack, arguments);
}
function removeExpr(bpId) {
    // A workaround to remove the current expression
}
function evaluate(expression, expandLevel, flags) {
    controllerObj.pushCommand(controllerObj.debuggerCommands.evaluate, arguments);
}
function enableBp(name) {
    controllerObj.pushCommand(controllerObj.debuggerCommands.enableBp, arguments);
}
function disableBp(name) {
    controllerObj.pushCommand(controllerObj.debuggerCommands.disableBp, arguments);
}
function deleteBp(name) {
    controllerObj.pushCommand(controllerObj.debuggerCommands.deleteBp, arguments);
}
function setFrame(name) {
    controllerObj.pushCommand(controllerObj.debuggerCommands.setFrame, arguments);
}
function dumpBreak() {
    controllerObj.pushCommand(controllerObj.debuggerCommands.dumpBreak, arguments);
}
function dumpSourceList() {
    controllerObj.pushCommand(controllerObj.debuggerCommands.dumpSourceList, arguments);
}
// Start internal tracing. E.g.: /**bp:trace(TRACE_COMMANDS)**/
function trace() {
    controllerObj.pushCommand(controllerObj.debuggerCommands.trace, arguments);
}

// Non Supported - TO BE REMOVED
function setExceptionResume() { }
function setnext() { }
function evaluateAsync() { }
function trackProjectionCall() { }
function mbp() { }
function deleteMbp() { }

// APIs exposed to Debugger.cpp
function GetOutputJson() {
    return controllerObj.getOutputJson.apply(controllerObj, arguments);
}
function SetBaseline() {
    return controllerObj.setBaseline.apply(controllerObj, arguments);
}

// Called from Debugger.cpp to handle JsDiagDebugEvent
function HandleDebugEvent() {
    return controllerObj.handleDebugEvent.apply(controllerObj, arguments);
}

// Called from Debugger.cpp when debugger is attached using WScript.Attach
function HandleSourceRunDown() {
    return controllerObj.handleSourceRunDown.apply(controllerObj, arguments);
}
