var x = [3, null, {}];
var y = x;
y.baz = 5;

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    ttdTestReport("Array.isArray(x)", Array.isArray(x), true);
    ttdTestReport("x.length", x.length, 3);

    ttdTestReport("x === y", x === y, true);
    ttdTestReport("x.baz", x.baz, 5);
    ttdTestReport("x[0]", x[0], 3);
    ttdTestReport("y[1]", y[1], null);
    ttdTestReport("x[5]", x[5], undefined);

    ////
    x[1] = "non-null";
    x[5] = { bar: 3 };
    x.push(10);
    ////

    ttdTestReport("post update -- y[1]", y[1], "non-null");
    ttdTestReport("post update -- x[5] !== null", x[5] !== null, true);
    ttdTestReport("post update -- x[5].bar", x[5].bar, 3);
    ttdTestReport("post update -- y[6]", y[6], 10);
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}