function opt(o, zz) {
  function foo () {
    String.prototype.replace.call(o, zz, {});
  }
  foo ();
}

for (let xi = 0; xi < 500; xi++) {
  opt({}, "kkkk");
}

WScript.Echo("Passed");