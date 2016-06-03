
telemetryLog("Start Global Code", true);

function foo()
{
    telemetryLog("Start Foo", true);
    
    WScript.Echo("Hello World - CallBack");
    
    telemetryLog("End Foo", true);
}

WScript.SetTimeout(foo, 250);
WScript.Echo("Hello World - Global");

telemetryLog("End Global Code", true);

