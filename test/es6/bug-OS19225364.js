function* g() {
  try {
    while (true) yield;
  } finally {
    WScript.Echo("finally");
  }
}

function f() {
  var i = 0;
  var b = false;
  for (var iter of g()) {
    if (++i > 2)
      break;
  }
  WScript.Echo('done');
}

f();
WScript.Echo("-------------------------");
f();
