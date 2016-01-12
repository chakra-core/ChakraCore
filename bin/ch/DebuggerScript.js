var controllerObj = {
    Logger: (function () {
        var logLevel = 0;
        return {
            Log: function (msg, loggingLevel) {
                if ((typeof loggingLevel === "undefined") || (typeof loggingLevel === "number" && loggingLevel <= logLevel)) {
                    WScript.Echo(msg);
                }
            },
            LogError: function (msg) {
                WScript.Echo(msg);
            },
            SetLogLevel: function (newLogLevel) {
                logLevel = newLogLevel;
            },
            GetLogLevel: function () {
                return logLevel;
            }
        };
    })(),
    ProcessDebugEvent: function (event, eventJSON) {
        switch (event) {
            case "SourceCompilation": {
                this.Logger.Log("SourceCompilation: " + eventJSON, 0);
                this.GetAndProcessResponse();
                break;
            }
            case "CompileError":
                this.Logger.Log("SourceCompilation: " + eventJSON, 0);
                break;
            case "Break": {
                this.Logger.Log("Break: " + eventJSON, 0);
                this.GetAndProcessResponse();
                break;
            }
            default: {
                this.Logger.LogError("ProcessDebugEvent UnHandled event: " + event);
                break;
            }
        }
    },
    ProcessUserInput: function (userInput) {
        try {
            this.Logger.Log(userInput, 1);
            if (!eval(userInput)) {
                this.GetAndProcessResponse();
            }
        } catch (ex) {
            this.Logger.LogError(ex);
        }
    },
    GetAndProcessResponse: function () {
        var userInput = WScript.GetUserInput();
        this.ProcessUserInput(userInput);
    }
}


function ProcessDebugEvent(event, eventJSON) {
    controllerObj.ProcessDebugEvent(event, eventJSON);
}

// Commands/Functions which can be executed from debugger prompt
function bp(sourceId, line, column) {
    if (typeof sourceId === "number" && typeof line === "number" && typeof column === "number") {
        var bpId = WScript.InsertBreakpoint(sourceId, line, column);
        return true;
    }
    return false;
}

function step() {
    WScript.ResumeFromBreakpoint(0);
    return true;
}

function stepover() {
    WScript.ResumeFromBreakpoint(1);
    return true;
}

function stepout() {
    WScript.ResumeFromBreakpoint(2);
    return true;
}

function cont() {
    return true;
}

function help() {
    controllerObj.Logger.Log("Commands ", 0);
    controllerObj.Logger.Log("\tbp(sourceId, line, column)", 0);
    controllerObj.Logger.Log("\tcont()", 0);
    controllerObj.Logger.Log("\tstep()", 0);
    controllerObj.Logger.Log("\tstepover()", 0);
    controllerObj.Logger.Log("\tstepout()", 0);
    controllerObj.Logger.Log("\tloglevel(level)", 0);
    return false;
}

function loglevel(newLevel) {
    controllerObj.Logger.SetLogLevel(newLevel);
    return false;
}