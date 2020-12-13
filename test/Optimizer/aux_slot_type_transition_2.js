// -maxinterpretcount:1 -maxsimplejitruncount:1 -bgjit-
function test() {
  var f = function () {};
  f.p1 = 1;
  (function () {
    Object.defineProperty(f, 'length', { writable: true });
    f.length = undefined;
    f.p2 = 2;
  }());
}

test();
test();
test();

print('Pass');
