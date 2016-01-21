var x = { foo: 3, bar: null };

Object.defineProperty(x, "b", {
    get: function () { return this.foo + 1; },
    set: function (x) { this.foo = x / 2; }
});

Object.defineProperty(x, "onlyone", {
    get: function () { return this.bar; }
});

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    ttdTestReport("typeof (x)", typeof (x), "object");

    ttdTestReport("x.foo", x.foo, 3);
    ttdTestReport("x.b", x.b, 4);

    ttdTestReport("x.onlyone", x.onlyone, null);

    ////
    x.b = 12;
    ////

    ttdTestReport("x.foo", x.foo, 6);
    ttdTestReport("x.b", x.b, 7);
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}