var x = 'Hello';

var xs = Symbol("Hello");
var ys = xs;

var zs = Symbol("Hello");

var obj = {};
obj[x] = 1;
obj[xs] = 2;
obj[zs] = 3;

var symObj = Object(zs);

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    ttdTestReport("typeof zs", typeof(zs), 'symbol');
    ttdTestReport("typeof symObj", typeof(symObj), 'object');
    
    ttdTestReport("xs == ys", xs == ys, true);
    ttdTestReport("xs == zs", xs == zs, false);

    ttdTestReport("obj[x]", obj[x], 1);
    ttdTestReport("obj.Hello", obj.Hello, 1);

    ttdTestReport("obj[xs]", obj[xs], 2);
    ttdTestReport("obj[ys]", obj[ys], 2);

    ttdTestReport("obj[zs]", obj[zs], 3);
    ttdTestReport("obj[symObj]", obj[symObj], 3);
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}