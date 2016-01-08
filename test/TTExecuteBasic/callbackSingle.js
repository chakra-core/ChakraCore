
ttdTestWrite("Start Global Code");

function foo()
{
    ttdTestWrite("Start Foo");
    
    WScript.Echo("Hello World - CallBack");
    
    ttdTestWrite("End Foo");
}

WScript.SetTimeout(foo, 250);
WScript.Echo("Hello World - Global");

ttdTestWrite("End Global Code");

