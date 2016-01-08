
var hello = undefined;

function foo(thing)
{
    var msg = "No Message";
    
    if(hello.doExcited)
    {
        msg = 'Hello' + ' ' + thing + '!!!';
    }
    else
    {
        msg = 'Hello' + ' ' + thing;
    }
    
    return msg;
}

function doIt()
{
    var txt = "World";
    var msg = foo(txt);
    WScript.Echo(msg);
}

WScript.SetTimeout(doIt, 50);

