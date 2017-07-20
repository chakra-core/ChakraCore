"use strict";
var $ = {  global: this,  createRealm(options) {    options = options || {};    options.globals = options.globals || {};    var realm = WScript.LoadScript(this.source, 'samethread');    realm.$.source = this.source;    realm.$.destroy = function () {      if (options.destroy) {        options.destroy();      }    };    for(var glob in options.globals) {      realm.$.global[glob] = options.globals[glob];    }    return realm.$;  },  evalScript(code) {    try {      WScript.LoadScript(code);      return { type: 'normal', value: undefined };    } catch (e) {      return { type: 'throw', value: e };    }  },  getGlobal(name) {    return this.global[name];  },  setGlobal(name, value) {    this.global[name] = value;  },  destroy() { /* noop */ },  source: "var $ = {  global: this,  createRealm(options) {    options = options || {};    options.globals = options.globals || {};    var realm = WScript.LoadScript(this.source, 'samethread');    realm.$.source = this.source;    realm.$.destroy = function () {      if (options.destroy) {        options.destroy();      }    };    for(var glob in options.globals) {      realm.$.global[glob] = options.globals[glob];    }    return realm.$;  },  evalScript(code) {    try {      WScript.LoadScript(code);      return { type: 'normal', value: undefined };    } catch (e) {      return { type: 'throw', value: e };    }  },  getGlobal(name) {    return this.global[name];  },  setGlobal(name, value) {    this.global[name] = value;  },  destroy() { /* noop */ },  source: \"\"};"};function Test262Error(message) {
    if (message) this.message = message;
}

Test262Error.prototype.name = "Test262Error";

Test262Error.prototype.toString = function () {
    return "Test262Error: " + this.message;
};

function $ERROR(err) {
  if(typeof err === "object" && err !== null && "name" in err) {
    print('test262/error ' + err.name + ': ' + err.message);
  } else {
    print('test262/error Test262Error: ' + err);
  }
}

function $DONE(err) {
  if (err) {
    $ERROR(err);
  }
  print('pass');
  $.destroy();
}

function $LOG(str) {
  print(str);
}


function assert(mustBeTrue, message) {
  if (mustBeTrue === true) {
    return;
  }

  if (message === undefined) {
    message = 'Expected true but got ' + String(mustBeTrue);
  }
  $ERROR(message);
}

assert._isSameValue = function (a, b) {
  if (a === b) {
    // Handle +/-0 vs. -/+0
    return a !== 0 || 1 / a === 1 / b;
  }

  // Handle NaN vs. NaN
  return a !== a && b !== b;
};

assert.sameValue = function (actual, expected, message) {
  if (assert._isSameValue(actual, expected)) {
    return;
  }

  if (message === undefined) {
    message = '';
  } else {
    message += ' ';
  }

  message += 'Expected SameValue(«' + String(actual) + '», «' + String(expected) + '») to be true';

  $ERROR(message);
};

assert.notSameValue = function (actual, unexpected, message) {
  if (!assert._isSameValue(actual, unexpected)) {
    return;
  }

  if (message === undefined) {
    message = '';
  } else {
    message += ' ';
  }

  message += 'Expected SameValue(«' + String(actual) + '», «' + String(unexpected) + '») to be false';

  $ERROR(message);
};

assert.throws = function (expectedErrorConstructor, func, message) {
  if (typeof func !== "function") {
    $ERROR('assert.throws requires two arguments: the error constructor ' +
      'and a function to run');
    return;
  }
  if (message === undefined) {
    message = '';
  } else {
    message += ' ';
  }

  try {
    func();
  } catch (thrown) {
    if (typeof thrown !== 'object' || thrown === null) {
      message += 'Thrown value was not an object!';
      $ERROR(message);
    } else if (thrown.constructor !== expectedErrorConstructor) {
      message += 'Expected a ' + expectedErrorConstructor.name + ' but got a ' + thrown.constructor.name;
      $ERROR(message);
    }
    return;
  }

  message += 'Expected a ' + expectedErrorConstructor.name + ' to be thrown but no exception was thrown at all';
  $ERROR(message);
};

assert.throws.early = function(err, code) {
  let wrappedCode = `function wrapperFn() { ${code} }`;
  let ieval = eval;

  assert.throws(err, () => { Function(wrappedCode); }, `Function: ${code}`);
};


// Create workers and start them all spinning.  We set atomic slots to make
// them go into a wait, thus controlling the waiting order.  Then we wake them
// one by one and observe the wakeup order.

for ( var i=0 ; i < 3 ; i++ ) {
$262.agent.start(
`
$262.agent.receiveBroadcast(function (sab) {
  var ia = new Int32Array(sab);
  while (Atomics.load(ia, ${i+1}) == 0);
  $262.agent.report(${i} + Atomics.wait(ia, 0, 0));
  $262.agent.leaving();
})
`);
}

var ia = new Int32Array(new SharedArrayBuffer(Int32Array.BYTES_PER_ELEMENT*4));
$262.agent.broadcast(ia.buffer);

// Make them sleep in order 0 1 2 on ia[0]
for ( var i=0 ; i < 3 ; i++ ) {
  Atomics.store(ia, i+1, 1);
  $262.agent.sleep(500);
}

// Wake them up one at a time and check the order is 0 1 2
for ( var i=0 ; i < 3 ; i++ ) {
  assert.sameValue(Atomics.wake(ia, 0, 1), 1);
  assert.sameValue(getReport(), i + "ok");
}

function getReport() {
    var r;
    while ((r = $262.agent.getReport()) == null)
        $262.agent.sleep(100);
    return r;
}



;$DONE();
;$.destroy();