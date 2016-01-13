
var evalCode1;
eval("evalCode1 = function() { return evalCode1 + ' ' + captured; }");

var evalCode2 = undefined;
var evalCode3 = undefined;

var captured = "ok";

function setCode2()
{
    var notCaptured = 5;
    var captured = "bob in setCode2";
    eval("evalCode2 = function() { return evalCode2 + ' ' + captured; }");
    
    eval.call(this, "evalCode3 = function() { return evalCode3 + ' ' + captured; }")
}

setCode2();
WScript.SetTimeout(testFunction, 50);

function testFunction()
{
    ttdTestReport("evalCode1", evalCode1(), "function () { return evalCode1 + ' ' + captured; } ok");
    ttdTestReport("evalCode2", evalCode2(), "function () { return evalCode2 + ' ' + captured; } bob in setCode2");
    
    ttdTestReport("evalCode3", evalCode3(), "function () { return evalCode3 + ' ' + captured; } ok");
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}

