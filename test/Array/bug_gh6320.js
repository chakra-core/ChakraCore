function f(array) {
  Array.prototype.push.call(array, 1);
  ' ' + array;
}

f([0]);
f([0]);
f(2.3023e-320);

print('PASS');
