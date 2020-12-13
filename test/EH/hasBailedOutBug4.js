var shouldBailout = false;
var caught = false;

function test0() {
  function func0() {
    if (shouldBailout) {
      throw new Error('oops');
    }
  }

  function func1() { func0() }
  function func2() { func1() }
  function func3() { shouldBailout ? obj0 : null }

  var obj0 = { method0: func1 };
  var obj1 = { method0: func2 };

  try {
    try {} finally { func3(); }
  } catch {
    caught = true;
  }

  func2();
}

// generate profile
test0();
test0();

// run code with bailouts enabled
shouldBailout = true;
try {
  test0();
} catch {}
if (!caught) {
  print('Passed');
}
