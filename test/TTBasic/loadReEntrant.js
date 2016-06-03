
WScript.LoadScriptFile('../../test/TTBasic/loadTarget.js');

var msgFunction = foo('World');

var msg0 = msgFunction();

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog(`msg0: ${msg0}`, true); //Hello World #0
        
    telemetryLog(`msgFunction() -- 1: ${msgFunction()}`, true); //Hello World #1
    telemetryLog(`msgFunction() -- 2: ${msgFunction()}`, true); //Hello World #2
}