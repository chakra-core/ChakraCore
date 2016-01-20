var x = { foo: 3, bar: null };
x.foo2 = 6;

var y = { foo: 5, bar: 'bar', baz: null };
delete y.bar;
y.bar2 = 'bar2'

var z = { foo: 10, bar: 'bar' };
delete z.bar;
z.baz = 'baz'
z.bar = 'bar'

var xo = [];
for(var xname in x)
{
    xo.push(xname);
}

var yo = [];
for(var yname in y)
{
    yo.push(yname);
}

var zo = [];
for(var zname in z)
{
    zo.push(zname);
}

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    var idx = -1;
    
    idx = 0;
    for(var xname in x)
    {
        ttdTestReport("xname", xname, xo[idx]);
        idx++;    
    }
    
    idx = 0;
    for(var yname in y)
    {
        ttdTestReport("yname", yname, yo[idx]);
        idx++;    
    }
    
    idx = 0;
    for(var zname in z)
    {
        ttdTestReport("zname", zname, zo[idx]);
        idx++;    
    }
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}