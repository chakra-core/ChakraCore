function f()
{
    return "called f";
}
f.foo = 3;
            
var g = f;

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    ttdTestReport("f !== null", f !== null, true);
    ttdTestReport("f === g", f === g, true);
    ttdTestReport("g.foo", g.foo, 3);

    ttdTestReport("f()", f(), "called f");
    ttdTestReport("g()", g(), "called f");
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}