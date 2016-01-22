
WScript.LoadScriptFile('C:\\Chakra\\ScratchJS\\loadTarget.js');

var msgFunction = foo('World');

var msg0 = msgFunction();

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    ttdTestReport("msg0", msg0, 'Hello World #0');
        
    ttdTestReport("msgFunction() -- 1", msgFunction(), 'Hello World #1');
    ttdTestReport("msgFunction() -- 2", msgFunction(), 'Hello World #2');
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}