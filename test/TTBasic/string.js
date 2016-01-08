var x = 'Hello';
var y = 'World';
var empty = '';

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    ttdTestReport("x", x, "Hello");
    ttdTestReport("y", y, "World");

    ttdTestReport("empty.length", empty.length, 0);
    ttdTestReport("x.length", x.length, 5);
    ttdTestReport("x + \' \' + y", x + ' ' + y, "Hello World");
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}