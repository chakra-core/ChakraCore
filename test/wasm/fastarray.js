function checkTrap(fn) {
  try {
    fn();
    console.log("Should have trapped");
  } catch (e) {
    if (!(e instanceof WebAssembly.RuntimeError)) {
      console.log(e);
    }
  }
}

function runScenario() {
  const module = new WebAssembly.Module(readbuffer("binaries/fastarray.wasm"));
  const {exports: {load, store, mem}} = new WebAssembly.Instance(module);
  function test() {
    checkTrap(() => load(0));
    checkTrap(() => load(0xFF));
    checkTrap(() => load(0xFFFFFFFF));
    checkTrap(() => store(0));
    checkTrap(() => store(0xFF));
    checkTrap(() => store(0xFFFFFFFF));
  }

  test();
  mem.grow(5);
  test();
  mem.grow(2);
  test();
}

runScenario();

console.log("PASSED");
