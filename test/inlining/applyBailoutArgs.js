var hasArgs = true;

function method0() {}

function f1(a = (hasArgs = false)) {}
  
function f2() {
  this.method0.apply(this, arguments);
}

function test0() {
  var obj1 = {};
  obj1.method0 = f1;
  obj1.method1 = f2;
  obj1.method1.call(undefined);
  obj1.method1(1);
}

test0();
test0();
test0();

if (hasArgs) {
  WScript.Echo('Passed');
} else {
  WScript.Echo('Arguments not passed to inlinee on bailout');
}
