var x = true;
var y = false;

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    ttdTestReport("x", x, true);
    ttdTestReport("y", y, false);

    ttdTestReport("x === y", x === y, false);

    ttdTestReport("x && y", x && y, false);
    ttdTestReport("x || y", x || y, true);
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}