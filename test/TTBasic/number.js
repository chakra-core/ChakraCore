var x = 3;
var y = 5;

var xd = 4.6;
var yd = 9.2;

var myInf = Infinity;

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    ttdTestReport("x", x, 3);
    ttdTestReport("y", y, 5);
    ttdTestReport("xd", xd, 4.6);
    ttdTestReport("yd", yd, 9.2);

    ttdTestReport("x + y", x + y, 8);
    ttdTestReport("x - y", x - y, -2);
    ttdTestReport("x * y", x * y, 15);
    ttdTestReport("x / y", x / y, 0.6);

    ttdTestReport("isFinite(xd)", isFinite(xd), true);
    ttdTestReport("isFinite(myInf)", isFinite(myInf), false);
    ttdTestReport("isFinite(Infinity)", isFinite(Infinity), false);

    ttdTestReport("Math.abs(-2)", Math.abs(-2), 2);
    ttdTestReport("Math.floor(1.5)", Math.floor(1.5), 1.0);
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}