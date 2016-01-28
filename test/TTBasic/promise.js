
var p1 = new Promise((resolve, reject) => resolve(5));  
var p2 = p1.then((val) => { return val + 1; }); // 5  

var p3 = new Promise((resolve, reject) => resolve(10));

var v1 = undefined;
var v2 = undefined;
var v3 = undefined;

function doIt()
{
    p1.then((val) => v1 = val);
    p2.then((val) => v2 = val);
    p3.then((val) => v3 = val);
    
    WScript.SetTimeout(testFunction, 50);
}

WScript.SetTimeout(doIt, 50);

function testFunction()
{            
    ttdTestReport("v1", v1, 5);
    ttdTestReport("v2", v2, 6);
    ttdTestReport("v3", v3, 10);
            
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}


