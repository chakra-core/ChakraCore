var count = 0;
var myEval = eval;

function foo() {
  count += 1;
  myEval("var v" + count);
}

for (i = 0, v0 = 0; i < 10; i += count, v0 += 1) {
  foo(v0);
}

print('PASSED');
