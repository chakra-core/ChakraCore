var xi = [2, 1, 0];
var xd = [2.5, 1.5, 0.5];

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    ttdTestReport("Array.isArray(xi)", Array.isArray(xi), true);
    ttdTestReport("xi.length", xi.length, 3);
    ttdTestReport("xi[1]", xi[1], 1);

    ////
    xi[1] = 5;
    xi.push(10);
    ////

    ttdTestReport("post update -- xi[1]", xi[1], 5);
    ttdTestReport("post update -- xi.length", xi.length, 4);
    ttdTestReport("post update -- xi[3]", xi[3], 10);
    ttdTestReport("post update -- xi[5]", xi[5], undefined);

    ttdTestReport("Array.isArray(xd)", Array.isArray(xd), true);
    ttdTestReport("xd.length", xd.length, 3);
    ttdTestReport("xd[1]", xd[1], 1.5);

    ////
    xd[1] = 5.0;
    xd.push(10.0);
    xd[10] = 10.0;
    ////

    ttdTestReport("post update -- xd[1]", xd[1], 5.0);
    ttdTestReport("post update -- xd.length", xd.length, 11);
    ttdTestReport("post update -- xd[3]", xd[3], 10.0);
    ttdTestReport("post update -- xd[5]", xd[5], undefined);
    ttdTestReport("post update -- xd[10]", xd[10], 10.0);
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}