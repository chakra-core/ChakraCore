// -bgjit- -loopiterpretcount:1
let returnCalled;
let iter;

function test() {
  returnCalled = false;
  let i = 0;
  for (let x of iter) {
    if (i++ > 1)
      break;
  }
  print(`Return called: ${returnCalled}`);
}

iter = {
  [Symbol.iterator]() { return this },
  next() { return { done: false } },
  return() { returnCalled = true; return { done: true }; },
};

test();
print('-----');
test();
