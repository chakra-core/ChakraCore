var xb = new Boolean(true);
var yb = xb;
yb.foob = 3;

var zb = new Boolean(true);

var xn = new Number(5);
var yn = xn;
yn.foon = 3;

var zn = new Number(5);

var xs = new String("bob");
var ys = xs;
ys.foos = 3;

var zs = new String("bob");

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    ttdTestReport("typeof (xb)", typeof (xb), "object");
    ttdTestReport("xb === yb", xb === yb, true)
    ttdTestReport("xb !== zb", xb !== zb, true);
    ttdTestReport("xb == true", xb == true, true);
    ttdTestReport("xb === true", xb === true, false);
    ttdTestReport("xb.foob", xb.foob, 3);

    ttdTestReport("typeof (xn)", typeof (xn), "object");
    ttdTestReport("xn === yn", xn === yn, true);
    ttdTestReport("xn !== zn", xn !== zn, true);
    ttdTestReport("xn == 5", xn == 5, true);
    ttdTestReport("xn === 5", xn === 5, false);
    ttdTestReport("xn.foon", xn.foon, 3);

    ttdTestReport("typeof (xs)", typeof (xs), "object");
    ttdTestReport("xs === ys", xs === ys, true);
    ttdTestReport("xs !== zs", xs !== zs, true);
    ttdTestReport("xs == \'bob\'", xs == "bob", true);
    ttdTestReport("xs === \'bob\'", xs === "bob", false);
    ttdTestReport("xs.foos", xs.foos, 3);

    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}