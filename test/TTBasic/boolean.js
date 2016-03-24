var x = true;
var y = false;

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog(`x: ${x}`, true); //true
    telemetryLog(`y: ${y}`, true); //false

    telemetryLog(`x === y: ${x === y}`, true); //false

    telemetryLog(`x && y: ${x && y}`, true); //false
    telemetryLog(`x || y: ${x || y}`, true); //true
}