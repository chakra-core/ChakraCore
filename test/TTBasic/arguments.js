
var x = foo(1, 2, 3);
var y = fooDeleted(1, 2, 3);

function foo(a, b, c)
{
    var res = {};
    var args = arguments;
    
    res.length = function() { return args.length; };
    res.named = function() { return b; };
    res.position = function() { return args[1]; };
   
    return res;
}

function fooDeleted(a, b, c)
{
    delete arguments[1];

    var res = {};
    var args = arguments;
    
    res.length = function() { return args.length; };
    res.named = function() { return b; }; 
    res.position = function() { return args[1]; };
   
    return res;
}

WScript.SetTimeout(testFunction, 20);

function testFunction()
{
    ttdTestReport("xlength", x.length(), 3);
    ttdTestReport("xnamed", x.named(), 2);
    ttdTestReport("xposition", x.position(), 2);
    
    ttdTestReport("ylength", y.length(), 3);
    ttdTestReport("ynamed", y.named(), 2);
    ttdTestReport("yposition", y.position(), undefined);
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}


