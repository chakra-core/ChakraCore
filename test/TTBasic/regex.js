var x = /World/;
var y = new RegExp("l", "g");

y.exec("Hello World");

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    ttdTestReport("y.lastIndex", y.lastIndex, 3);

    ////
    var m = "Hello World".match(x);
    y.exec("Hello World");
    ////

    ttdTestReport("m.index", m.index, 6);
    ttdTestReport("post update -- y.lastIndex", y.lastIndex, 4);
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}