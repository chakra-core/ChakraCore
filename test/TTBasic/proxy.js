var handler1 = {
    get: function(target, key)
    {
        return key in target ? target[key] : 'Not Found';
    }
};

var handler2 = {
    get: function(target, key)
    {
        return "[[" + key + "]]";;
    }
};

var p = new Proxy({}, handler1);
p.a = 1;

var revocable = Proxy.revocable({}, handler2);
var proxy = revocable.proxy;

var revocableDone = Proxy.revocable({}, handler2);
var proxyDone = revocableDone.proxy;

revocableDone.revoke();

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    var threw = false;
        
    ttdTestReport("p.a", p.a, 1);
    ttdTestReport("p.b", p.b, 'Not Found');

    try
    {
        proxyDone.foo;
    }
    catch(e)
    {
        threw = true;
    }
    ttdTestReport("proxyDone.foo", threw, true);

    ttdTestReport("proxy.foo", proxy.foo, '[[foo]]');

    revocable.revoke();
    try
    {
        proxy.foo;
    }
    catch(e)
    {
        threw = true;
    }
    ttdTestReport("proxy.foo (after revoke)", threw, true);
    
    if(this.ttdTestsFailed)
    {
        ttdTestWrite("Failures!");
    }
    else
    {
        ttdTestWrite("All tests passed!");   
    }
}