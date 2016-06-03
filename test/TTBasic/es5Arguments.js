function foo(a){
  "use strict";
  a = 42;
  
  var res = {};
  var args = arguments;
    
  res.a = function() { return a; };
  res.arg = function() { return args[0]; };

  return res;
}

var x = foo(17);

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog(`a: ${x.a()}`, true); //42
    telemetryLog(`arg: ${x.arg()}`, true); //17
}