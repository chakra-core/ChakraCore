var x = { foo: 3, bar: null };
var y = x;

y.baz = "new prop";

var z = new Object();
z['foo'] = 3;
z[1] = "bob";
z['2'] = "bob2";

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    ttdTestReport("typeof (x)", typeof (x), "object");
    ttdTestReport("typeof (z)", typeof (z), "object");

    ttdTestReport("x === y", x === y, true);
    ttdTestReport("x !== z", x !== z, true);

    ttdTestReport("y.foo", y.foo, 3);
    ttdTestReport("z.foo", z.foo, 3);
    ttdTestReport("z[1]", z[1], "bob");
    ttdTestReport("z[2]", z[2], "bob2");

    ttdTestReport("x.foo", x.foo, 3);
    ttdTestReport("x.bar", x.bar, null);
    ttdTestReport("x.baz", x.baz, "new prop");

    ttdTestReport("x.notPresent", x.notPresent, undefined);
    ttdTestReport("z[0]", z[0], undefined);
    ttdTestReport("z[5]", z[5], undefined);

    ////
    z.foo = 0;
    y.foo = 10;
    y.foo2 = "ten";
    y[10] = "foo";
    y.bar = 3;
    ////

    ttdTestReport("post update -- z[0]", z[0], undefined);
    ttdTestReport("post update -- z.foo", z.foo, 0);
    ttdTestReport("post update -- x.foo", x.foo, 10);
    ttdTestReport("post update -- x.foo2", x.foo2, "ten");
    ttdTestReport("post update -- x[0]", x[0], undefined);
    ttdTestReport("post update -- x[10]", x[10], "foo");

    ttdTestReport("post update -- y.bar", y.bar, 3);
    ttdTestReport("post update -- x.bar", x.bar, 3);
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}