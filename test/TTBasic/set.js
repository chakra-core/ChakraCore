var x = new Set();
var y = x;
y.baz = 5;

var z = new Set();
z.add(5);

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    ttdTestReport("x === y", x === y, true);
    ttdTestReport("x.baz", x.baz, 5);
    ttdTestReport("z.size", z.size, 1);
    ttdTestReport("z.has(5)", z.has(5), true);

    ////
    x.add(3);
    z.delete(3);
    z.delete(5);
    ////

    ttdTestReport("post update 1 -- y.has(3)", y.has(3), true);
    ttdTestReport("post update 1 -- z.size", z.size, 0);
    ttdTestReport("post update 1 -- z.has(5)", z.has(5), false);

    ////
    y.add(3);
    y.add(5);
    ////

    ttdTestReport("post update 2 -- x.has(5)", x.has(5), true);
    ttdTestReport("post update 2 -- x.size", x.size, 2);
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}