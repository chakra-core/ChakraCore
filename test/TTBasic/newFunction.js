

var functionCode1 = new Function("arg", "return arg;");
var functionCode2 = undefined;

var captured = "ok";

function setCode2()
{
    var captured = "bob in setCode2";
    
    functionCode2 = new Function("arg", "return arg + ' ' + captured;");
}

setCode2();
WScript.SetTimeout(testFunction, 50);

function testFunction()
{
    ttdTestReport("functionCode1", functionCode1("Hello"), "Hello");
    ttdTestReport("functionCode2", functionCode2("Hello"), "Hello ok");
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}

