//
//In theory this test could be race-y -- in which case suppress the console printing but in general ch should be well behaved
//

ttdTestWrite("Start Global Code");

var foo1id = undefined;

function foo1()
{
    ttdTestWrite("Start Foo1");
    
    WScript.Echo("Hello World - CallBack 1");
    
    ttdTestWrite("End Foo1");
}

function foo2()
{
    ttdTestWrite("Start Foo2");
    
    WScript.Echo("Cancel Callback 1 from CallBack 2");
    WScript.ClearTimeout(foo1id);
    
    ttdTestWrite("End Foo2");
}

foo1id = WScript.SetTimeout(foo1, 500);
WScript.SetTimeout(foo2, 100);
WScript.Echo("Hello World - Global");

ttdTestWrite("End Global Code");

