
var globalMsg = 'Hello'

function foo(thing)
{
    var ctr = 0;
    return function () { return globalMsg + ' ' + thing + ' #' + ctr++; };
}
