var x = new Date();
var y = x;

var z = new Date(2012, 1);

y.foo = 3;

var w = Date.now();

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    ttdTestReport("x === y", x === y, true);
    ttdTestReport("w !== z", w !== z.valueOf(), true);

    ttdTestReport("y.foo", y.foo, 3);
    ttdTestReport("x.foo", x.foo, 3);

    ttdTestReport("w - z > 0", w - z.valueOf() > 0, true);
    ttdTestReport("x - y", x.valueOf() - y.valueOf(), 0);
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}