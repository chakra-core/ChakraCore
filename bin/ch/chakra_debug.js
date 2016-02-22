// Copyright Microsoft. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and / or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

(function () {
    var isAtBreak = false;
    var suspendMessage = null;
    var frames = null;
    var nextScopeHandle = 0;

    function GetNextScopeHandle() {
        return --nextScopeHandle;
    }

    var Logger = (function () {
        var logTypes = {
            API: 1,
            Output: 2,
            Input: 3
        };

        var allEnabled = true;
        var enabledLogType = [];
        enabledLogType[logTypes.API] = allEnabled;
        enabledLogType[logTypes.Output] = allEnabled;
        enabledLogType[logTypes.Input] = allEnabled;

        var logFunc = function (logType, msg, obj) {
            if (enabledLogType[logType]) {
                var printStr = "";
                if (obj != undefined) {
                    printStr = ": ";
                    printStr += (typeof obj != 'string') ? JSON.stringify(obj) : obj;
                }

                chakraDebug.log(msg + printStr);
            }
        };

        return {
            LogAPI: function (msg, obj) {
                logFunc(logTypes.API, msg, obj);
            },
            LogOutput: function (msg, obj) {
                logFunc(logTypes.Output, msg, obj);
            },
            LogInput: function (msg, obj) {
                logFunc(logTypes.Input, msg, obj);
            }
        };
    })();

    var DebugManager = (function () {
        var eventSeq = 0;
        var responseSeq = 0;
        var breakScriptId = undefined;

        function GetNextResponseSeq() {
            return responseSeq++;
        }

        var retObj = {};

        retObj.GetNextEventSeq = function () {
            return eventSeq++;
        }

        retObj.MakeResponse = function (debugProtoObj, success) {
            return {
                "seq": GetNextResponseSeq(),
                "request_seq": debugProtoObj.seq,
                "type": "response",
                "command": debugProtoObj.command,
                "success": success,
                "refs": [],
                "running": !isAtBreak
            };
        }

        retObj.SetBreakScriptId = function (scriptId) {
            breakScriptId = scriptId;
        }

        retObj.GetBreakScriptId = function () {
            return breakScriptId;
        }

        retObj.ScriptManager = (function () {
            var scriptIdFileNameArray = [];
            return {
                GetFileNameFromId: function (scriptId) {
                    if (scriptId in scriptIdFileNameArray) {
                        var scriptName = scriptIdFileNameArray[scriptId];
                        Logger.LogAPI("GetFileNameFromId scriptId: " + scriptId + ", scriptName", scriptName);
                        return scriptName;
                    } else {
                        var scripts = chakraDebug.JsDiagGetScripts();
                        //scriptIdFileNameArray = scripts;
                        Logger.LogAPI("GetFileNameFromId JsDiagGetScripts", scripts);
                        scripts.forEach(function (element, index, array) {
                            scriptIdFileNameArray[element.scriptId] = element.fileName;
                        });

                        if (scriptId in scriptIdFileNameArray) {
                            var scriptName = scriptIdFileNameArray[scriptId];
                            Logger.LogAPI("GetFileNameFromId scriptId: " + scriptId + ", scriptName", scriptName);
                            return scriptName;
                        }
                    }
                    return "";
                }
            }
        })();

        retObj.BreakpointManager = (function () {
            var bpList = [];
            var pendingBpList = [];
            return {
                Add: function (id, type, target) {
                    if (id > 0) {
                        bpList[id] = {
                            type: type,
                            target: target
                        }
                    }
                },
                GetTypeAndTarget: function (id) {
                    return bpList[id];
                },
                AddPendingBP: function (reqObj) {
                    pendingBpList.push(reqObj);
                },
                SetBreakpoint: function (debugProtoObj, addToPendingList) {
                    /* {"command":"setbreakpoint","arguments":{"type":"scriptRegExp","target":"^(.*[\\/\\\\])?test\\.js$","line":2,"condition":0},"type":"request","seq":1} */
                    /* {"command":"setbreakpoint","arguments":{"type":"scriptId","target":"19","line":3,"condition":0},"type":"request","seq":1} */
                    /* {"command":"setbreakpoint","arguments":{"type":"script","target":"c:\\nodejs\\Test\\Test.js","line":6,"column":0},"type":"request","seq":32} */

                    var scriptId = -1;
                    var bpType = "scriptId";

                    if (debugProtoObj.arguments) {
                        if (debugProtoObj.arguments.type == "scriptRegExp" || debugProtoObj.arguments.type == "script") {

                            var scriptsArray = chakraDebug.JsDiagGetScripts();
                            Logger.LogAPI("JsDiagGetScripts", scriptsArray);

                            var targetRegex = new RegExp(debugProtoObj.arguments.target);

                            for (var i = 0; i < scriptsArray.length; ++i) {
                                if (debugProtoObj.arguments.type == "scriptRegExp") {
                                    if (scriptsArray[i].fileName.match(targetRegex)) {
                                        scriptId = scriptsArray[i].scriptId;
                                        bpType = "scriptRegExp";
                                        break;
                                    }
                                }
                                else if (debugProtoObj.arguments.type == "script" && (debugProtoObj.arguments.target == null || debugProtoObj.arguments.target == undefined)) {
                                    scriptId = DebugManager.GetBreakScriptId();
                                    break;
                                }
                                else if (debugProtoObj.arguments.type == "script" && debugProtoObj.arguments.target) {
                                    if (scriptsArray[i].fileName.toLowerCase() == debugProtoObj.arguments.target.toLowerCase()) {
                                        scriptId = scriptsArray[i].scriptId;
                                        bpType = "scriptName";
                                        break;
                                    }
                                }
                            }
                        } else if (debugProtoObj.arguments.type == "scriptId") {
                            scriptId = parseInt(debugProtoObj.arguments.target);
                        }
                    }

                    var response = DebugManager.MakeResponse(debugProtoObj, false);

                    if (typeof scriptId == "number" && scriptId != -1) {
                        var column = 0;
                        if (debugProtoObj.arguments && debugProtoObj.arguments.column && (typeof debugProtoObj.arguments.column == "number")) {
                            column = debugProtoObj.arguments.column;
                        }
                        Logger.LogAPI("Calling JsDiagSetBreakpoint(" + scriptId + "," + debugProtoObj.arguments.line + "," + column + ")");
                        var bpObject = chakraDebug.JsDiagSetBreakpoint(scriptId, debugProtoObj.arguments.line, column);
                        var bpId = bpObject.breakpointId;
                        Logger.LogAPI("bpId", bpId);

                        if (bpId > 0) {
                            response["body"] = debugProtoObj.arguments;
                            response["body"].actual_locations = [];
                            response["body"].breakpoint = bpId;
                            response["success"] = true;

                            DebugManager.BreakpointManager.Add(bpId, bpType, debugProtoObj.arguments.target);
                        }
                    } else if (addToPendingList == true) {
                        this.AddPendingBP(debugProtoObj);
                    }

                    return response;
                },
                ProcessPendingBP: function () {
                    pendingBpList.forEach(function (debugProtoObj, index, array) {
                        DebugManager.BreakpointManager.SetBreakpoint(debugProtoObj, false);
                    });
                }
            }
        })();

        return retObj;
    })();

    function ConvertEventDataToDebugProtocol(debugEvent, msgJSON) {
        var debugProtocol = undefined;

        // Needs to be in sync with JsDiagDebugEvent
        switch (debugEvent) {
            case 0:
                /* JsDiagDebugEventSourceCompilation */
                var compileMsg = {
                    "seq": DebugManager.GetNextEventSeq(),
                    "type": "event",
                    "event": "afterCompile",
                    "success": true,
                    "body": {
                        "script": {
                            "type": "script",
                            "name": DebugManager.ScriptManager.GetFileNameFromId(msgJSON.scriptId),
                            "id": msgJSON.scriptId,
                            "lineOffset": 0,
                            "columnOffset": 0,
                            "lineCount": msgJSON.lineCount,
                            "sourceStart": "",
                            "sourceLength": msgJSON.sourceLength,
                            "scriptType": 2,
                            "compilationType": 0,
                            "text": DebugManager.ScriptManager.GetFileNameFromId(msgJSON.scriptId) + " (lines: " + msgJSON.lineCount + ")"
                        }
                    },
                    "running": !isAtBreak
                }

                debugProtocol = JSON.stringify(compileMsg);

                DebugManager.BreakpointManager.ProcessPendingBP();

                break;
            case 1: /* JsDiagDebugEventCompileError */
                break;
            case 2: /* JsDiagDebugEventBreak */
            case 3: /* JsDiagDebugEventStepComplete */
            case 4: /* JsDiagDebugEventDebuggerStatement */
                var breakMsg = {
                    "seq": DebugManager.GetNextEventSeq(),
                    "type": "event",
                    "event": "break",
                    "body": {
                        "sourceLine": msgJSON.line,
                        "sourceColumn": msgJSON.column,
                        "sourceLineText": msgJSON.sourceText,
                        "script": {
                            "id": msgJSON.scriptId,
                            "name": DebugManager.ScriptManager.GetFileNameFromId(msgJSON.scriptId)
                        }
                    }
                }
                debugProtocol = JSON.stringify(breakMsg);
                DebugManager.SetBreakScriptId(msgJSON.scriptId);
                isAtBreak = true;
                break;
            
            
            case 5:
                /* JsDiagDebugEventAsyncBreak */
                if (suspendMessage != null) {
                    isAtBreak = true;
                    var delayedResponse = JSON.stringify(DebugManager.MakeResponse(suspendMessage, true));
                    suspendMessage = null;
                    chakraDebug.SendDelayedRespose(delayedResponse);
                }
                break;
            case 6:
                /* JsDiagDebugEventRuntimeException */
                var obj = msgJSON["exception"];

                AddChildrens(obj);

                var breakMsg = {
                    "seq": DebugManager.GetNextEventSeq(),
                    "type": "event",
                    "event": "exception",
                    "body": {
                        "uncaught": msgJSON.uncaught,
                        "exception": msgJSON["exception"],
                        "sourceLine": msgJSON.line,
                        "sourceColumn": msgJSON.column,
                        "sourceLineText": msgJSON.sourceText,
                        "script": {
                            "id": msgJSON.scriptId,
                            "name": DebugManager.ScriptManager.GetFileNameFromId(msgJSON.scriptId)
                        }
                    }
                }
                debugProtocol = JSON.stringify(breakMsg);
                DebugManager.SetBreakScriptId(msgJSON.scriptId);
                isAtBreak = true;
                break;
            default:
                throw new Error('Invalid debugEvent: ' + debugEvent);
                break;
        }

        return debugProtocol;
    }

    function AddChildrens(obj) {
        if ("display" in obj) {
            if (!("value" in obj)) {
                obj["value"] = obj["display"];
            }
            if (!("text" in obj)) {
                obj["text"] = obj["display"];
            }
            if (!("className" in obj)) {
                obj["className"] = obj["type"];
            }
        }

        var PROPERTY_ATTRIBUTE_HAVE_CHILDRENS = 0x1;
        var PROPERTY_ATTRIBUTE_READ_ONLY_VALUE = 0x2;

        if (("propertyAttributes" in obj) && ((obj["propertyAttributes"] & PROPERTY_ATTRIBUTE_HAVE_CHILDRENS) == PROPERTY_ATTRIBUTE_HAVE_CHILDRENS)) {
            var objectHandle = obj["handle"];
            var childProperties = chakraDebug.JsDiagGetProperties([objectHandle]);
            Logger.LogAPI("JsDiagGetProperties childProperties", childProperties);

            if (objectHandle in childProperties) {
                var propertiesArray = [];
                if ("properties" in childProperties[objectHandle]) {
                    var properties = childProperties[objectHandle]["properties"];
                    for (var propsLen = 0; propsLen < properties.length; ++propsLen) {
                        propertiesArray.push({
                            "name": properties[propsLen]["name"],
                            "propertyType": 0,
                            "ref": properties[propsLen]["handle"]
                        });
                    }
                }
                if ("debuggerOnlyProperties" in childProperties[objectHandle]) {
                    var debuggerOnlyProperties = childProperties[objectHandle]["debuggerOnlyProperties"];
                    for (var propsLen = 0; propsLen < debuggerOnlyProperties.length; ++propsLen) {
                        propertiesArray.push({
                            "name": debuggerOnlyProperties[propsLen]["name"],
                            "propertyType": 0,
                            "ref": debuggerOnlyProperties[propsLen]["handle"]
                        });
                    }
                }
                obj["properties"] = propertiesArray;
            }
        }
    }

    var DebugProtocolHandler = {
        "scripts": function (debugProtoObj) {
            /* {"command":"scripts","type":"request","seq":1} */
            /* {"command":"scripts","arguments":{"types":7,"includeSource":true,"ids":[70]},"type":"request","seq":22} */
            /* {"command":"scripts","arguments":{"types":7,"filter":"module.js"},"type":"request","seq":81} */

            var chakraScriptsArray = chakraDebug.JsDiagGetScripts();
            Logger.LogAPI("JsDiagGetScripts", chakraScriptsArray);

            var ids = null;
            var filter = null;
            var includeSource = false;
            if (debugProtoObj.arguments) {
                if (debugProtoObj.arguments.ids) {
                    ids = debugProtoObj.arguments.ids;
                }
                if (debugProtoObj.arguments.includeSource) {
                    includeSource = true;
                }
                if (debugProtoObj.arguments.filter) {
                    filter = debugProtoObj.arguments.filter;
                }
            }

            var body = [];
            chakraScriptsArray.forEach(function (element, index, array) {
                var found = true;

                if (ids != null) {
                    found = false;
                    for (var i = 0; i < ids.length; ++i) {
                        if (ids[i] == element.scriptId) {
                            found = true;
                            break;
                        }
                    }
                }

                if (filter != null) {
                    found = false;
                    if (filter == element.fileName) {
                        found = true;
                    }
                }

                if (found == true) {
                    var scriptObj = {
                        "handle": element.handle,
                        "type": "script",
                        "name": element.fileName,
                        "id": element.scriptId,
                        "lineOffset": 0,
                        "columnOffset": 0,
                        "lineCount": element.lineCount,
                        "sourceStart": "",
                        "sourceLength": element.sourceLength,
                        "scriptType": 2,
                        "compilationType": 0,
                        "text": element.fileName + " (lines: " + element.lineCount + ")"
                    };
                    if (includeSource == true) {
                        var chakraSourceObj = chakraDebug.JsDiagGetSource(element.scriptId);
                        Logger.LogAPI("JsDiagGetSource", chakraSourceObj);
                        scriptObj["source"] = chakraSourceObj.source;
                        scriptObj["lineCount"] = chakraSourceObj.lineCount;
                        scriptObj["sourceLength"] = chakraSourceObj.sourceLength;
                    }
                    body.push(scriptObj);
                }
            });

            var response = DebugManager.MakeResponse(debugProtoObj, true);
            response["body"] = body;
            return response;
        },
        "source": function (debugProtoObj) {
            /* {"command":"source","fromLine":2,"toLine":6,"type":"request","seq":1} */
            chakraSourceObj = chakraDebug.JsDiagGetSource(DebugManager.GetBreakScriptId());
            Logger.LogAPI("JsDiagGetSource", chakraSourceObj);
            var response = DebugManager.MakeResponse(debugProtoObj, true);

            response["body"] = {
                "source": chakraSourceObj.source,
                "fromLine": 0,
                "toLine": chakraSourceObj.lineCount,
                "fromPosition": 0,
                "toPosition": chakraSourceObj.sourceLength,
                "totalLines": chakraSourceObj.lineCount
            };

            return response;
        },
        "continue": function (debugProtoObj) {
            /* {"command":"continue","type":"request","seq":1} */
            /* {"command":"continue","arguments":{"stepaction":"in","stepcount":1},"type":"request","seq":1} */
            var success = true;
            if (debugProtoObj.arguments && debugProtoObj.arguments.stepaction) {
                var jsDiagResumeType = 0;
                if (debugProtoObj.arguments.stepaction == "in") {
                    /* JsDiagResumeTypeStepIn */
                    jsDiagResumeType = 0;
                } else if (debugProtoObj.arguments.stepaction == "out") {
                    /* JsDiagResumeTypeStepOut */
                    jsDiagResumeType = 1;
                } else if (debugProtoObj.arguments.stepaction == "next") {
                    /* JsDiagResumeTypeStepOver */
                    jsDiagResumeType = 2;
                } else {
                    throw new Error('Unhandled stepaction: ' + debugProtoObj.arguments.stepaction);
                }

                if (!chakraDebug.JsDiagResume(jsDiagResumeType)) {
                    success = false;
                }
            }
            return DebugManager.MakeResponse(debugProtoObj, success);
        },
        "setbreakpoint": function (debugProtoObj) {
            return DebugManager.BreakpointManager.SetBreakpoint(debugProtoObj, true);
        },
        "backtrace": function (debugProtoObj) {
            // {"command":"backtrace","arguments":{"fromFrame":0,"toFrame":1},"type":"request","seq":7}
            var response = DebugManager.MakeResponse(debugProtoObj, true);
            if (frames == null || frames.length == 0) {
                var stackTrace = chakraDebug.JsDiagGetStacktrace();
                Logger.LogAPI("JsDiagGetStacktrace", stackTrace);

                frames = [];
                stackTrace.forEach(function (element, index, array) {
                    var thisObj = chakraDebug.JsDiagEvaluate("this", element.index);
                    Logger.LogAPI("JsDiagEvaluate", thisObj);

                    frames.push({
                        "type": "frame",
                        "index": element.index,
                        "handle": element.handle,
                        "constructCall": false,
                        "atReturn": false,
                        "debuggerFrame": false,
                        "position": 0,
                        "line": element.line,
                        "column": element.column,
                        "sourceLineText": element.sourceText,
                        "func": {
                            "ref": element.functionHandle
                        },
                        "script": {
                            "ref": element.scriptHandle
                        },
                        "receiver": {
                            "ref": ("handle" in thisObj) ? thisObj["handle"] : element.functionHandle
                        },
                        "arguments": [],
                        "locals": [],
                        "scopes": [],
                        "text": ""
                    });
                });
            }

            if (debugProtoObj.arguments && debugProtoObj.arguments.fromFrame != undefined) {
                response["body"] = {
                    "fromFrame": debugProtoObj.arguments.fromFrame,
                    "toFrame": debugProtoObj.arguments.toFrame,
                    "totalFrames": frames.length,
                    "frames": []
                };
                response["refs"] = [];
                for (var i = debugProtoObj.arguments.fromFrame; i < debugProtoObj.arguments.toFrame; ++i) {
                    response["body"]["frames"].push(frames[i]);
                }
            } else {
                response["body"] = {
                    "fromFrame": 0,
                    "toFrame": frames.length,
                    "totalFrames": frames.length,
                    "frames": []
                };
                response["body"]["frames"] = frames;
                response["refs"] = [];
            }

            return response;
        },
        "lookup": function (debugProtoObj) {
            // {"command":"lookup","arguments":{"handles":[8,0]},"type":"request","seq":2}
            var handlesResult = chakraDebug.JsDiagLookupHandles(debugProtoObj.arguments.handles);
            Logger.LogAPI("JsDiagLookupHandles", handlesResult);
            var response = DebugManager.MakeResponse(debugProtoObj, true);
            response["body"] = {};
            for (var id in handlesResult) {
                AddChildrens(handlesResult[id]);

                if ("fileName" in handlesResult[id] && !("name" in handlesResult[id])) {
                    handlesResult[id]["name"] = handlesResult[id]["fileName"];
                }

                if ("scriptId" in handlesResult[id] && !("id" in handlesResult[id])) {
                    handlesResult[id]["id"] = handlesResult[id]["scriptId"];
                }

                response["body"][id] = handlesResult[id];
            }
            response["refs"] = [];

            return response;
        },
        "evaluate": function (debugProtoObj) {
            var response = DebugManager.MakeResponse(debugProtoObj, true);
            var evalResult = undefined;
            if (isAtBreak && debugProtoObj.arguments.frame != undefined) {
                // {"command":"evaluate","arguments":{"expression":"x","disable_break":true,"maxStringLength":10000,"frame":0},"type":"request","seq":35}
                // {"seq":37,"request_seq":35,"type":"response","command":"evaluate","success":true,"body":{"handle":13,"type":"number","value":1,"text":"1"},"refs":[],"running":false}
                evalResult = chakraDebug.JsDiagEvaluate(debugProtoObj.arguments.expression, debugProtoObj.arguments.frame);
                Logger.LogAPI("JsDiagEvaluate", evalResult);

                AddChildrens(evalResult);

                response["body"] = evalResult;
            } else {
                // {"command":"evaluate","arguments":{"expression":"process.pid","global":true},"type":"request","seq":1}
                evalResult = chakraDebug.JsDiagEvaluateScript(debugProtoObj.arguments.expression);
                Logger.LogAPI("JsDiagEvaluateScript", evalResult);
                response["body"] = {
                    "value": evalResult,
                    "text": new String(evalResult)
                };
            }

            return response;
        },
        "threads": function (debugProtoObj) {
            var response = DebugManager.MakeResponse(debugProtoObj, true);
            response["body"] = {
                "totalThreads": 1,
                "threads": [{
                    "current": true,
                    "id": 1
                }
                ]
            };
            response["refs"] = [];
            return response;
        },
        "setexceptionbreak": function (debugProtoObj) {
            var enabled = false;
            if (debugProtoObj.arguments && debugProtoObj.arguments.enabled) {
                enabled = debugProtoObj.arguments.enabled;
            }

            var breakOnExceptionType = 0; // JsDiagBreakOnExceptionTypeNone

            if (enabled && debugProtoObj.arguments && debugProtoObj.arguments.type) {
                if (debugProtoObj.arguments.type == "all") {
                    breakOnExceptionType = 2; // JsDiagBreakOnExceptionTypeAll
                } else if (debugProtoObj.arguments.type == "uncaught") {
                    breakOnExceptionType = 1; // JsDiagBreakOnExceptionTypeUncaught
                }
            }

            Logger.LogAPI("breakOnExceptionType", breakOnExceptionType);

            var success = chakraDebug.JsDiagSetBreakOnException(breakOnExceptionType);

            var response = DebugManager.MakeResponse(debugProtoObj, success);

            response["body"] = {
                "type": debugProtoObj.arguments.type,
                "enabled": enabled
            };
            response["refs"] = [];
            return response;
        },
        "scopes": function (debugProtoObj) {
            var response = DebugManager.MakeResponse(debugProtoObj, false);
            if (frames != null && frames.length > 0) {
                var frameIndex = -1;
                for (var i = 0; i < frames.length; ++i) {
                    if (frames[i].index == debugProtoObj.arguments.frameNumber) {
                        frameIndex = i;
                        break;
                    }
                }
                if (frameIndex != -1) {
                    var props = chakraDebug.JsDiagGetStackProperties(frames[frameIndex].index);
                    Logger.LogAPI("JsDiagGetStackProperties", props);
                    var scopes = [];
                    var refs = [];

                    function AddToScope(scopeType, propertiesArray) {
                        var scopeRef = GetNextScopeHandle();
                        scopes.push({
                            "type": scopeType,
                            "index": frames[frameIndex].index,
                            "frameIndex": debugProtoObj.arguments.frame_index,
                            "object": {
                                "ref": scopeRef
                            }
                        });
                        var properties = [];
                        for (var propsLen = 0; propsLen < propertiesArray.length; ++propsLen) {
                            properties.push({
                                "name": propertiesArray[propsLen]["name"],
                                "propertyType": 0,
                                "ref": propertiesArray[propsLen]["handle"]
                            });
                        }
                        refs.push({
                            "handle": scopeRef,
                            "type": "object",
                            "className": "Object",
                            "constructorFunction": {
                                "ref": 100000
                            },
                            "protoObject": {
                                "ref": 100000
                            },
                            "prototypeObject": {
                                "ref": 100000
                            },
                            "properties": properties
                        });
                    }

                    if ("returnValue" in props) {
                        props["locals"].push(props["returnValue"]);
                    }

                    if ("functionCallsReturn" in props) {
                        props["functionCallsReturn"].forEach(function (element, index, array) {
                            props["locals"].push(element);
                        });
                    }

                    //var scopesMap = { "locals": 1, "globals": 0, "scopes": 3 };
                    if (props["locals"] && props["locals"].length > 0) {
                        AddToScope(1, props["locals"]);
                    }

                    if (props["scopes"] && props["scopes"].length > 0) {
                        var closureHandles = [];
                        for (var closuresLen = 0; closuresLen < props["scopes"].length; ++closuresLen) {
                            closureHandles.push(props["scopes"][closuresLen].handle);
                        }

                        var closureProps = chakraDebug.JsDiagGetProperties(closureHandles);
                        Logger.LogAPI("JsDiagGetProperties scope", closureProps);

                        var closureProperties = [];
                        for (var closuresLen = 0; closuresLen < props["scopes"].length; ++closuresLen) {
                            if (props["scopes"][closuresLen].handle in closureProps) {
                                for (var i = 0; i < closureProps[props["scopes"][closuresLen].handle]["properties"].length; ++i) {
                                    closureProperties.push(closureProps[props["scopes"][closuresLen].handle]["properties"][i]);
                                }
                                for (var i = 0; i < closureProps[props["scopes"][closuresLen].handle]["debuggerOnlyProperties"].length; ++i) {
                                    closureProperties.push(closureProps[props["scopes"][closuresLen].handle]["debuggerOnlyProperties"][i]);
                                }
                            }
                        }

                        if (closureProperties.length > 0) {
                            AddToScope(3, closureProperties);
                        }
                    }

                    if (props["globals"] && props["globals"].handle) {
                        var globalsProps = chakraDebug.JsDiagGetProperties([props["globals"].handle]);
                        Logger.LogAPI("JsDiagGetProperties globals", globalsProps);

                        if (props["globals"].handle in globalsProps) {
                            var globalProperties = [];
                            for (var i = 0; i < globalsProps[props["globals"].handle]["properties"].length; ++i) {
                                globalProperties.push(globalsProps[props["globals"].handle]["properties"][i]);
                            }
                            for (var i = 0; i < globalsProps[props["globals"].handle]["debuggerOnlyProperties"].length; ++i) {
                                globalProperties.push(globalsProps[props["globals"].handle]["debuggerOnlyProperties"][i]);
                            }
                        }

                        if (globalProperties.length > 0) {
                            AddToScope(0, globalProperties);
                        }
                    }

                    response["success"] = true;
                    response["refs"] = refs;
                    response["body"] = {
                        "fromScope": 0,
                        "toScope": 1,
                        "totalScopes": 1,
                        "scopes": scopes
                    };
                }
            }
            return response;
        },
        "listbreakpoints": function (debugProtoObj) {
            // {"command":"listbreakpoints","type":"request","seq":25}
            var breakpoints = chakraDebug.JsDiagGetBreakpoints();
            Logger.LogAPI("JsDiagGetBreakpoints", breakpoints);

            var breakOnExceptionType = chakraDebug.JsDiagGetBreakOnException();
            Logger.LogAPI("JsDiagGetBreakOnException", breakOnExceptionType);

            var response = DebugManager.MakeResponse(debugProtoObj, true);
            response["body"] = {
                "breakpoints": [],
                "breakOnExceptions": breakOnExceptionType == 2,
                "breakOnUncaughtExceptions": breakOnExceptionType == 1
            }

            for (var i = 0; i < breakpoints.length; ++i) {
                var bpObj = {
                    "number": breakpoints[i].breakpointId,
                    "line": breakpoints[i].line,
                    "column": breakpoints[i].column,
                    "groupId": null,
                    "hit_count": 0,
                    "active": true,
                    "condition": null,
                    "ignoreCount": 0,
                    "actual_locations": [{
                        "line": breakpoints[i].line,
                        "column": breakpoints[i].column,
                        "script_id": breakpoints[i].scriptId
                    }
                    ],
                    "type": "scriptId",
                    "script_id": breakpoints[i].scriptId,
                    "script_name": DebugManager.ScriptManager.GetFileNameFromId(breakpoints[i].scriptId)
                };

                var bpTypeObj = DebugManager.BreakpointManager.GetTypeAndTarget(breakpoints[i].breakpointId);

                if (bpTypeObj) {
                    bpObj["type"] = bpTypeObj.type;
                    if (bpTypeObj.type == "scriptRegExp") {
                        bpObj["script_regexp"] = bpTypeObj.target;
                        // Need to fill in something?
                    }
                }

                response["body"]["breakpoints"].push(bpObj);
            }
            return response;
        },
        "clearbreakpoint": function (debugProtoObj) {
            // {"command":"clearbreakpoint","arguments":{"breakpoint":2},"type":"request","seq":39}
            var response = DebugManager.MakeResponse(debugProtoObj, true);
            chakraDebug.JsDiagRemoveBreakpoint(debugProtoObj.arguments.breakpoint);
            return response;
        },
        "suspend": function (debugProtoObj) {
            // {"command":"suspend","type":"request","seq":26}
            suspendMessage = debugProtoObj;
        }
    };

    function DebugProtocolToChakra(debugProtoJSON) {
        var debugProtoObj = JSON.parse(debugProtoJSON);
        var returnJSON = undefined;

        switch (debugProtoObj.command) {
            case "scripts":
            case "source":
            case "setbreakpoint":
            case "backtrace":
            case "lookup":
            case "listbreakpoints":
            case "clearbreakpoint":
            case "suspend":
            case "evaluate":
            case "scopes":
            case "threads":
            case "setexceptionbreak":
                returnJSON = DebugProtocolHandler[debugProtoObj.command](debugProtoObj);
                break;
            case "continue":
                isAtBreak = false;
                frames = null;
                returnJSON = DebugProtocolHandler[debugProtoObj.command](debugProtoObj);
                break;
                // Not handled, return failure response
            case "break":
            case "changebreakpoint":
            case "changelive":
            case "clearbreakpointgroup":
            case "disconnect":
            case "flags":
            case "frame":
            case "gc":
            case "references":
            case "restartframe":
            case "scope":
            case "setvariablevalue":
            case "v8flag":
            case "version":
                returnJSON = DebugManager.MakeResponse(debugProtoObj, false);
                break;
            default:
                throw new Error('Unhandled command: ' + debugProtoObj.command);
                break;
        }

        return returnJSON;
    }

    function GetSourceContextOfFunction(compiledWrapper, line, column) {
        Logger.LogAPI("GetSourceContextOfFunction(" + compiledWrapper + "," + line + "," + column + ")");
        var funcInfo = chakraDebug.JsDiagGetFunctionPosition(compiledWrapper);
        Logger.LogAPI("JsDiagGetFunctionPosition", funcInfo);

        if (funcInfo.scriptId >= 0) {
            var bpLine = funcInfo.stmtStartLine + line;
            var bpColumn = funcInfo.stmtStartColumn + column;
            var bpObject = chakraDebug.JsDiagSetBreakpoint(funcInfo.scriptId, bpLine, bpColumn);
            var bpId = bpObject.breakpointId;
            Logger.LogAPI("JsDiagSetBreakpoint", bpId);

            DebugManager.BreakpointManager.Add(bpId, "scriptId", funcInfo.id);

            return bpId;
        }
        return -1;
    }

    function DebugEventToString(debugEvent) {
        switch (debugEvent) {
            case 0: return "JsDiagDebugEventSourceCompile";
            case 1: return "JsDiagDebugEventCompileError";
            case 2: return "JsDiagDebugEventBreak";
            case 3: return "JsDiagDebugEventStepComplete";
            case 4: return "JsDiagDebugEventDebuggerStatement";
            case 5: return "JsDiagDebugEventAsyncBreak";
            case 6: return "JsDiagDebugEventRuntimeException";
            default: return "Unhandled JsDiagDebugEvent: " + debugEvent;
        }
    }

    chakraDebug = {
        'ProcessDebugProtocolJSON': function (debugProtoJSON) {
            Logger.LogInput("ProcessDebugProtocolJSON debugProtoJSON", debugProtoJSON);
            try {
                var debugProtoJSONReturn = JSON.stringify(DebugProtocolToChakra(debugProtoJSON));
                Logger.LogInput("ProcessDebugProtocolJSON debugProtoJSONReturn", debugProtoJSONReturn);
                return debugProtoJSONReturn;
            } catch (ex) {
                Logger.LogInput("ProcessDebugProtocolJSON exception", ex.message);
            }
        },
        'ProcessJsrtEventData': function (debugEvent, eventData) {
            Logger.LogInput("ProcessJsrtEventData debugEvent: " + DebugEventToString(debugEvent) + ", eventData: " + JSON.stringify(eventData));
            try {
                var debugProtoJSONReturn = ConvertEventDataToDebugProtocol(debugEvent, eventData);
                Logger.LogOutput("ProcessJsrtEventData debugEvent: " + DebugEventToString(debugEvent) + ", debugProtoJSONReturn: " + debugProtoJSONReturn);
                return debugProtoJSONReturn;
            } catch (ex) {
                Logger.LogInput("ProcessJsrtEventData exception", ex.message);
            }
        },
        'DebugObject': {
            'Debug': {
                'setListener': function (fn) { },
                'setBreakPoint': function (compiledWrapper, line, column) {
                    GetSourceContextOfFunction(compiledWrapper, line, column);
                }
            }
        }
    }

    Object.defineProperty(chakraDebug, 'shouldContinue', {
        get: function () {
            Logger.LogAPI("shouldContinue: ", !isAtBreak);
            return !isAtBreak;
        }
    });

    this.chakraDebug = chakraDebug;

    return this.chakraDebug;
});
