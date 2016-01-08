function outer(val)
{
    var iic = val + 1;

    function inner() { return iic++; }

    return inner;
}
var fouter = outer(3);
var gouter = outer(5);

function ctr(val)
{
    var iic = val;

    this.inc = function () { return iic++; }
    this.dec = function () { return iic--; }
}
var fctr = new ctr(3);
var fctr2 = fctr;
var gctr = new ctr(5);

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    ////
    fouter();
    ////

    ttdTestReport("fouter()", fouter(), 5);
    ttdTestReport("gouter()", gouter(), 6);

    ////
    fctr.inc();
    ////

    ttdTestReport("fctr.inc()", fctr.inc(), 4);
    ttdTestReport("gctr.inc()", gctr.inc(), 5);

    ////
    fctr2.dec();
    fctr2.dec();
    ////

    ttdTestReport("post decrement -- fctr.inc()", fctr.inc(), 3);
    ttdTestReport("post decrement -- gctr.inc()", gctr.inc(), 6);
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}