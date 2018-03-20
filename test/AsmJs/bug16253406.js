function AsmModule(stdlib) {
  'use asm';
  var fr = stdlib.fround;
  function f3(x) {
    x = fr();
  }
}
console.log("pass");
