var gi = 3;
var bigArray = new Array(50);
bigArray.fill(42);
function foo() {
return arguments[gi];
}
function bar() {
bigArray.every(function (x) {
foo();
});
}
for (var i = 0; i < 3; ++i) {
bar();
}
WScript.Echo('pass');
