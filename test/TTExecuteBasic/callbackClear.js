//
//In theory this test could be race-y -- in which case suppress the console printing but in general ch should be well behaved
//

telemetryLog("Start Global Code", true);

var foo1id = undefined;

function foo1()
{
    telemetryLog("Start Foo1", true);
    
    WScript.Echo("Hello World - CallBack 1");
    
    telemetryLog("End Foo1", true);
}

function foo2()
{
    telemetryLog("Start Foo2", true);
    
    WScript.Echo("Cancel Callback 1 from CallBack 2");
    WScript.ClearTimeout(foo1id);
    
    telemetryLog("End Foo2", true);
}

foo1id = WScript.SetTimeout(foo1, 500);
WScript.SetTimeout(foo2, 100);
WScript.Echo("Hello World - Global");

telemetryLog("End Global Code", true);

