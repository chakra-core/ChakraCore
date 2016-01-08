
ttdTestWrite("Start Global Code");

function foo1()
{
    ttdTestWrite("Start Foo1");
    
    WScript.Echo("Hello World - CallBack 1");
    WScript.SetTimeout(foo2, 200);
    
    ttdTestWrite("End Foo1");
}

function foo2()
{
    ttdTestWrite("Start Foo2");
    
    WScript.Echo("Hello World - CallBack 2");
    WScript.SetTimeout(foo3, 200);

    ttdTestWrite("End Foo2");
}

function foo3()
{
    ttdTestWrite("Start Foo3");
    
    WScript.Echo("Hello World - CallBack 3");
    
    ttdTestWrite("End Foo3");
}

WScript.SetTimeout(foo1, 200);
WScript.Echo("Hello World - Global");

ttdTestWrite("End Global Code");

