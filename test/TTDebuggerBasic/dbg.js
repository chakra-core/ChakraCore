
var hello = "Hello";

function foo(thing)
{
    var msg = "No Message";
    
    if(thing !== "World")
    {
        msg = hello + ' ' + thing + '!!!';
    }
    else
    {
        msg = hello + ' ' + thing;
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

