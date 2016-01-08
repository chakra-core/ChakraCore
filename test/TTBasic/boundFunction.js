this.x = 9;
var module = {
    x: 81,
    getX: function (y) { return this.x + y; }
};

var getX = module.getX;
var boundGetX = getX.bind(module, 3);

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    ttdTestReport("getX(1)", getX(1), 10);
    ttdTestReport("module.getX(1)", module.getX(1), 82);
    ttdTestReport("boundGetX()", boundGetX(), 84);
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}