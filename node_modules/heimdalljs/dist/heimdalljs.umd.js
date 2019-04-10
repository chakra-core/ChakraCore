(function (global, factory) {
  typeof exports === 'object' && typeof module !== 'undefined' ? module.exports = factory() :
  typeof define === 'function' && define.amd ? define(factory) :
  (global.heimdall = factory());
}(this, (function () {

function indexOf(callbacks, callback) {
  for (var i=0, l=callbacks.length; i<l; i++) {
    if (callbacks[i] === callback) { return i; }
  }

  return -1;
}

function callbacksFor(object) {
  var callbacks = object._promiseCallbacks;

  if (!callbacks) {
    callbacks = object._promiseCallbacks = {};
  }

  return callbacks;
}

/**
  @class RSVP.EventTarget
*/
var EventTarget = {

  /**
    `RSVP.EventTarget.mixin` extends an object with EventTarget methods. For
    Example:

    ```javascript
    var object = {};

    RSVP.EventTarget.mixin(object);

    object.on('finished', function(event) {
      // handle event
    });

    object.trigger('finished', { detail: value });
    ```

    `EventTarget.mixin` also works with prototypes:

    ```javascript
    var Person = function() {};
    RSVP.EventTarget.mixin(Person.prototype);

    var yehuda = new Person();
    var tom = new Person();

    yehuda.on('poke', function(event) {
      console.log('Yehuda says OW');
    });

    tom.on('poke', function(event) {
      console.log('Tom says OW');
    });

    yehuda.trigger('poke');
    tom.trigger('poke');
    ```

    @method mixin
    @for RSVP.EventTarget
    @private
    @param {Object} object object to extend with EventTarget methods
  */
  'mixin': function(object) {
    object['on']      = this['on'];
    object['off']     = this['off'];
    object['trigger'] = this['trigger'];
    object._promiseCallbacks = undefined;
    return object;
  },

  /**
    Registers a callback to be executed when `eventName` is triggered

    ```javascript
    object.on('event', function(eventInfo){
      // handle the event
    });

    object.trigger('event');
    ```

    @method on
    @for RSVP.EventTarget
    @private
    @param {String} eventName name of the event to listen for
    @param {Function} callback function to be called when the event is triggered.
  */
  'on': function(eventName, callback) {
    if (typeof callback !== 'function') {
      throw new TypeError('Callback must be a function');
    }

    var allCallbacks = callbacksFor(this), callbacks;

    callbacks = allCallbacks[eventName];

    if (!callbacks) {
      callbacks = allCallbacks[eventName] = [];
    }

    if (indexOf(callbacks, callback) === -1) {
      callbacks.push(callback);
    }
  },

  /**
    You can use `off` to stop firing a particular callback for an event:

    ```javascript
    function doStuff() { // do stuff! }
    object.on('stuff', doStuff);

    object.trigger('stuff'); // doStuff will be called

    // Unregister ONLY the doStuff callback
    object.off('stuff', doStuff);
    object.trigger('stuff'); // doStuff will NOT be called
    ```

    If you don't pass a `callback` argument to `off`, ALL callbacks for the
    event will not be executed when the event fires. For example:

    ```javascript
    var callback1 = function(){};
    var callback2 = function(){};

    object.on('stuff', callback1);
    object.on('stuff', callback2);

    object.trigger('stuff'); // callback1 and callback2 will be executed.

    object.off('stuff');
    object.trigger('stuff'); // callback1 and callback2 will not be executed!
    ```

    @method off
    @for RSVP.EventTarget
    @private
    @param {String} eventName event to stop listening to
    @param {Function} callback optional argument. If given, only the function
    given will be removed from the event's callback queue. If no `callback`
    argument is given, all callbacks will be removed from the event's callback
    queue.
  */
  'off': function(eventName, callback) {
    var allCallbacks = callbacksFor(this), callbacks, index;

    if (!callback) {
      allCallbacks[eventName] = [];
      return;
    }

    callbacks = allCallbacks[eventName];

    index = indexOf(callbacks, callback);

    if (index !== -1) { callbacks.splice(index, 1); }
  },

  /**
    Use `trigger` to fire custom events. For example:

    ```javascript
    object.on('foo', function(){
      console.log('foo event happened!');
    });
    object.trigger('foo');
    // 'foo event happened!' logged to the console
    ```

    You can also pass a value as a second argument to `trigger` that will be
    passed as an argument to all event listeners for the event:

    ```javascript
    object.on('foo', function(value){
      console.log(value.name);
    });

    object.trigger('foo', { name: 'bar' });
    // 'bar' logged to the console
    ```

    @method trigger
    @for RSVP.EventTarget
    @private
    @param {String} eventName name of the event to be triggered
    @param {*} options optional value to be passed to any event handlers for
    the given `eventName`
  */
  'trigger': function(eventName, options, label) {
    var allCallbacks = callbacksFor(this), callbacks, callback;

    if (callbacks = allCallbacks[eventName]) {
      // Don't cache the callbacks.length since it may grow
      for (var i=0; i<callbacks.length; i++) {
        callback = callbacks[i];

        callback(options, label);
      }
    }
  }
};

var config = {
  instrument: false
};

EventTarget['mixin'](config);

function configure(name, value) {
  if (name === 'onerror') {
    // handle for legacy users that expect the actual
    // error to be passed to their function added via
    // `RSVP.configure('onerror', someFunctionHere);`
    config['on']('error', value);
    return;
  }

  if (arguments.length === 2) {
    config[name] = value;
  } else {
    return config[name];
  }
}

function objectOrFunction(x) {
  return typeof x === 'function' || (typeof x === 'object' && x !== null);
}

function isFunction(x) {
  return typeof x === 'function';
}

function isMaybeThenable(x) {
  return typeof x === 'object' && x !== null;
}

var _isArray;
if (!Array.isArray) {
  _isArray = function (x) {
    return Object.prototype.toString.call(x) === '[object Array]';
  };
} else {
  _isArray = Array.isArray;
}

var isArray = _isArray;

// Date.now is not available in browsers < IE9
// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/now#Compatibility
var now = Date.now || function() { return new Date().getTime(); };

function F() { }

var o_create = (Object.create || function (o) {
  if (arguments.length > 1) {
    throw new Error('Second argument not supported');
  }
  if (typeof o !== 'object') {
    throw new TypeError('Argument must be an object');
  }
  F.prototype = o;
  return new F();
});

var queue = [];

function scheduleFlush() {
  setTimeout(function() {
    var entry;
    for (var i = 0; i < queue.length; i++) {
      entry = queue[i];

      var payload = entry.payload;

      payload.guid = payload.key + payload.id;
      payload.childGuid = payload.key + payload.childId;
      if (payload.error) {
        payload.stack = payload.error.stack;
      }

      config['trigger'](entry.name, entry.payload);
    }
    queue.length = 0;
  }, 50);
}

function instrument(eventName, promise, child) {
  if (1 === queue.push({
    name: eventName,
    payload: {
      key: promise._guidKey,
      id:  promise._id,
      eventName: eventName,
      detail: promise._result,
      childId: child && child._id,
      label: promise._label,
      timeStamp: now(),
      error: config["instrument-with-stack"] ? new Error(promise._label) : null
    }})) {
      scheduleFlush();
    }
  }

/**
  `RSVP.Promise.resolve` returns a promise that will become resolved with the
  passed `value`. It is shorthand for the following:

  ```javascript
  var promise = new RSVP.Promise(function(resolve, reject){
    resolve(1);
  });

  promise.then(function(value){
    // value === 1
  });
  ```

  Instead of writing the above, your code now simply becomes the following:

  ```javascript
  var promise = RSVP.Promise.resolve(1);

  promise.then(function(value){
    // value === 1
  });
  ```

  @method resolve
  @static
  @param {*} object value that the returned promise will be resolved with
  @param {String} label optional string for identifying the returned promise.
  Useful for tooling.
  @return {Promise} a promise that will become fulfilled with the given
  `value`
*/
function resolve$1(object, label) {
  /*jshint validthis:true */
  var Constructor = this;

  if (object && typeof object === 'object' && object.constructor === Constructor) {
    return object;
  }

  var promise = new Constructor(noop, label);
  resolve(promise, object);
  return promise;
}

function  withOwnPromise() {
  return new TypeError('A promises callback cannot return that same promise.');
}

function noop() {}

var PENDING   = void 0;
var FULFILLED = 1;
var REJECTED  = 2;

var GET_THEN_ERROR = new ErrorObject();

function getThen(promise) {
  try {
    return promise.then;
  } catch(error) {
    GET_THEN_ERROR.error = error;
    return GET_THEN_ERROR;
  }
}

function tryThen(then$$1, value, fulfillmentHandler, rejectionHandler) {
  try {
    then$$1.call(value, fulfillmentHandler, rejectionHandler);
  } catch(e) {
    return e;
  }
}

function handleForeignThenable(promise, thenable, then$$1) {
  config.async(function(promise) {
    var sealed = false;
    var error = tryThen(then$$1, thenable, function(value) {
      if (sealed) { return; }
      sealed = true;
      if (thenable !== value) {
        resolve(promise, value, undefined);
      } else {
        fulfill(promise, value);
      }
    }, function(reason) {
      if (sealed) { return; }
      sealed = true;

      reject(promise, reason);
    }, 'Settle: ' + (promise._label || ' unknown promise'));

    if (!sealed && error) {
      sealed = true;
      reject(promise, error);
    }
  }, promise);
}

function handleOwnThenable(promise, thenable) {
  if (thenable._state === FULFILLED) {
    fulfill(promise, thenable._result);
  } else if (thenable._state === REJECTED) {
    thenable._onError = null;
    reject(promise, thenable._result);
  } else {
    subscribe(thenable, undefined, function(value) {
      if (thenable !== value) {
        resolve(promise, value, undefined);
      } else {
        fulfill(promise, value);
      }
    }, function(reason) {
      reject(promise, reason);
    });
  }
}

function handleMaybeThenable(promise, maybeThenable, then$$1) {
  if (maybeThenable.constructor === promise.constructor &&
      then$$1 === then &&
      constructor.resolve === resolve$1) {
    handleOwnThenable(promise, maybeThenable);
  } else {
    if (then$$1 === GET_THEN_ERROR) {
      reject(promise, GET_THEN_ERROR.error);
    } else if (then$$1 === undefined) {
      fulfill(promise, maybeThenable);
    } else if (isFunction(then$$1)) {
      handleForeignThenable(promise, maybeThenable, then$$1);
    } else {
      fulfill(promise, maybeThenable);
    }
  }
}

function resolve(promise, value) {
  if (promise === value) {
    fulfill(promise, value);
  } else if (objectOrFunction(value)) {
    handleMaybeThenable(promise, value, getThen(value));
  } else {
    fulfill(promise, value);
  }
}

function publishRejection(promise) {
  if (promise._onError) {
    promise._onError(promise._result);
  }

  publish(promise);
}

function fulfill(promise, value) {
  if (promise._state !== PENDING) { return; }

  promise._result = value;
  promise._state = FULFILLED;

  if (promise._subscribers.length === 0) {
    if (config.instrument) {
      instrument('fulfilled', promise);
    }
  } else {
    config.async(publish, promise);
  }
}

function reject(promise, reason) {
  if (promise._state !== PENDING) { return; }
  promise._state = REJECTED;
  promise._result = reason;
  config.async(publishRejection, promise);
}

function subscribe(parent, child, onFulfillment, onRejection) {
  var subscribers = parent._subscribers;
  var length = subscribers.length;

  parent._onError = null;

  subscribers[length] = child;
  subscribers[length + FULFILLED] = onFulfillment;
  subscribers[length + REJECTED]  = onRejection;

  if (length === 0 && parent._state) {
    config.async(publish, parent);
  }
}

function publish(promise) {
  var subscribers = promise._subscribers;
  var settled = promise._state;

  if (config.instrument) {
    instrument(settled === FULFILLED ? 'fulfilled' : 'rejected', promise);
  }

  if (subscribers.length === 0) { return; }

  var child, callback, detail = promise._result;

  for (var i = 0; i < subscribers.length; i += 3) {
    child = subscribers[i];
    callback = subscribers[i + settled];

    if (child) {
      invokeCallback(settled, child, callback, detail);
    } else {
      callback(detail);
    }
  }

  promise._subscribers.length = 0;
}

function ErrorObject() {
  this.error = null;
}

var TRY_CATCH_ERROR = new ErrorObject();

function tryCatch(callback, detail) {
  try {
    return callback(detail);
  } catch(e) {
    TRY_CATCH_ERROR.error = e;
    return TRY_CATCH_ERROR;
  }
}

function invokeCallback(settled, promise, callback, detail) {
  var hasCallback = isFunction(callback),
      value, error, succeeded, failed;

  if (hasCallback) {
    value = tryCatch(callback, detail);

    if (value === TRY_CATCH_ERROR) {
      failed = true;
      error = value.error;
      value = null;
    } else {
      succeeded = true;
    }

    if (promise === value) {
      reject(promise, withOwnPromise());
      return;
    }

  } else {
    value = detail;
    succeeded = true;
  }

  if (promise._state !== PENDING) {
    // noop
  } else if (hasCallback && succeeded) {
    resolve(promise, value);
  } else if (failed) {
    reject(promise, error);
  } else if (settled === FULFILLED) {
    fulfill(promise, value);
  } else if (settled === REJECTED) {
    reject(promise, value);
  }
}

function initializePromise(promise, resolver) {
  var resolved = false;
  try {
    resolver(function resolvePromise(value){
      if (resolved) { return; }
      resolved = true;
      resolve(promise, value);
    }, function rejectPromise(reason) {
      if (resolved) { return; }
      resolved = true;
      reject(promise, reason);
    });
  } catch(e) {
    reject(promise, e);
  }
}

function then(onFulfillment, onRejection, label) {
  var parent = this;
  var state = parent._state;

  if (state === FULFILLED && !onFulfillment || state === REJECTED && !onRejection) {
    config.instrument && instrument('chained', parent, parent);
    return parent;
  }

  parent._onError = null;

  var child = new parent.constructor(noop, label);
  var result = parent._result;

  config.instrument && instrument('chained', parent, child);

  if (state) {
    var callback = arguments[state - 1];
    config.async(function(){
      invokeCallback(state, child, callback, result);
    });
  } else {
    subscribe(parent, child, onFulfillment, onRejection);
  }

  return child;
}

function makeSettledResult(state, position, value) {
  if (state === FULFILLED) {
    return {
      state: 'fulfilled',
      value: value
    };
  } else {
     return {
      state: 'rejected',
      reason: value
    };
  }
}

function Enumerator(Constructor, input, abortOnReject, label) {
  this._instanceConstructor = Constructor;
  this.promise = new Constructor(noop, label);
  this._abortOnReject = abortOnReject;

  if (this._validateInput(input)) {
    this._input     = input;
    this.length     = input.length;
    this._remaining = input.length;

    this._init();

    if (this.length === 0) {
      fulfill(this.promise, this._result);
    } else {
      this.length = this.length || 0;
      this._enumerate();
      if (this._remaining === 0) {
        fulfill(this.promise, this._result);
      }
    }
  } else {
    reject(this.promise, this._validationError());
  }
}

Enumerator.prototype._validateInput = function(input) {
  return isArray(input);
};

Enumerator.prototype._validationError = function() {
  return new Error('Array Methods must be provided an Array');
};

Enumerator.prototype._init = function() {
  this._result = new Array(this.length);
};

Enumerator.prototype._enumerate = function() {
  var length     = this.length;
  var promise    = this.promise;
  var input      = this._input;

  for (var i = 0; promise._state === PENDING && i < length; i++) {
    this._eachEntry(input[i], i);
  }
};

Enumerator.prototype._settleMaybeThenable = function(entry, i) {
  var c = this._instanceConstructor;
  var resolve$$1 = c.resolve;

  if (resolve$$1 === resolve$1) {
    var then$$1 = getThen(entry);

    if (then$$1 === then &&
        entry._state !== PENDING) {
      entry._onError = null;
      this._settledAt(entry._state, i, entry._result);
    } else if (typeof then$$1 !== 'function') {
      this._remaining--;
      this._result[i] = this._makeResult(FULFILLED, i, entry);
    } else if (c === Promise$1) {
      var promise = new c(noop);
      handleMaybeThenable(promise, entry, then$$1);
      this._willSettleAt(promise, i);
    } else {
      this._willSettleAt(new c(function(resolve$$1) { resolve$$1(entry); }), i);
    }
  } else {
    this._willSettleAt(resolve$$1(entry), i);
  }
};

Enumerator.prototype._eachEntry = function(entry, i) {
  if (isMaybeThenable(entry)) {
    this._settleMaybeThenable(entry, i);
  } else {
    this._remaining--;
    this._result[i] = this._makeResult(FULFILLED, i, entry);
  }
};

Enumerator.prototype._settledAt = function(state, i, value) {
  var promise = this.promise;

  if (promise._state === PENDING) {
    this._remaining--;

    if (this._abortOnReject && state === REJECTED) {
      reject(promise, value);
    } else {
      this._result[i] = this._makeResult(state, i, value);
    }
  }

  if (this._remaining === 0) {
    fulfill(promise, this._result);
  }
};

Enumerator.prototype._makeResult = function(state, i, value) {
  return value;
};

Enumerator.prototype._willSettleAt = function(promise, i) {
  var enumerator = this;

  subscribe(promise, undefined, function(value) {
    enumerator._settledAt(FULFILLED, i, value);
  }, function(reason) {
    enumerator._settledAt(REJECTED, i, reason);
  });
};

/**
  `RSVP.Promise.all` accepts an array of promises, and returns a new promise which
  is fulfilled with an array of fulfillment values for the passed promises, or
  rejected with the reason of the first passed promise to be rejected. It casts all
  elements of the passed iterable to promises as it runs this algorithm.

  Example:

  ```javascript
  var promise1 = RSVP.resolve(1);
  var promise2 = RSVP.resolve(2);
  var promise3 = RSVP.resolve(3);
  var promises = [ promise1, promise2, promise3 ];

  RSVP.Promise.all(promises).then(function(array){
    // The array here would be [ 1, 2, 3 ];
  });
  ```

  If any of the `promises` given to `RSVP.all` are rejected, the first promise
  that is rejected will be given as an argument to the returned promises's
  rejection handler. For example:

  Example:

  ```javascript
  var promise1 = RSVP.resolve(1);
  var promise2 = RSVP.reject(new Error("2"));
  var promise3 = RSVP.reject(new Error("3"));
  var promises = [ promise1, promise2, promise3 ];

  RSVP.Promise.all(promises).then(function(array){
    // Code here never runs because there are rejected promises!
  }, function(error) {
    // error.message === "2"
  });
  ```

  @method all
  @static
  @param {Array} entries array of promises
  @param {String} label optional string for labeling the promise.
  Useful for tooling.
  @return {Promise} promise that is fulfilled when all `promises` have been
  fulfilled, or rejected if any of them become rejected.
  @static
*/
function all(entries, label) {
  return new Enumerator(this, entries, true /* abort on reject */, label).promise;
}

/**
  `RSVP.Promise.race` returns a new promise which is settled in the same way as the
  first passed promise to settle.

  Example:

  ```javascript
  var promise1 = new RSVP.Promise(function(resolve, reject){
    setTimeout(function(){
      resolve('promise 1');
    }, 200);
  });

  var promise2 = new RSVP.Promise(function(resolve, reject){
    setTimeout(function(){
      resolve('promise 2');
    }, 100);
  });

  RSVP.Promise.race([promise1, promise2]).then(function(result){
    // result === 'promise 2' because it was resolved before promise1
    // was resolved.
  });
  ```

  `RSVP.Promise.race` is deterministic in that only the state of the first
  settled promise matters. For example, even if other promises given to the
  `promises` array argument are resolved, but the first settled promise has
  become rejected before the other promises became fulfilled, the returned
  promise will become rejected:

  ```javascript
  var promise1 = new RSVP.Promise(function(resolve, reject){
    setTimeout(function(){
      resolve('promise 1');
    }, 200);
  });

  var promise2 = new RSVP.Promise(function(resolve, reject){
    setTimeout(function(){
      reject(new Error('promise 2'));
    }, 100);
  });

  RSVP.Promise.race([promise1, promise2]).then(function(result){
    // Code here never runs
  }, function(reason){
    // reason.message === 'promise 2' because promise 2 became rejected before
    // promise 1 became fulfilled
  });
  ```

  An example real-world use case is implementing timeouts:

  ```javascript
  RSVP.Promise.race([ajax('foo.json'), timeout(5000)])
  ```

  @method race
  @static
  @param {Array} entries array of promises to observe
  @param {String} label optional string for describing the promise returned.
  Useful for tooling.
  @return {Promise} a promise which settles in the same way as the first passed
  promise to settle.
*/
function race(entries, label) {
  /*jshint validthis:true */
  var Constructor = this;

  var promise = new Constructor(noop, label);

  if (!isArray(entries)) {
    reject(promise, new TypeError('You must pass an array to race.'));
    return promise;
  }

  var length = entries.length;

  function onFulfillment(value) {
    resolve(promise, value);
  }

  function onRejection(reason) {
    reject(promise, reason);
  }

  for (var i = 0; promise._state === PENDING && i < length; i++) {
    subscribe(Constructor.resolve(entries[i]), undefined, onFulfillment, onRejection);
  }

  return promise;
}

/**
  `RSVP.Promise.reject` returns a promise rejected with the passed `reason`.
  It is shorthand for the following:

  ```javascript
  var promise = new RSVP.Promise(function(resolve, reject){
    reject(new Error('WHOOPS'));
  });

  promise.then(function(value){
    // Code here doesn't run because the promise is rejected!
  }, function(reason){
    // reason.message === 'WHOOPS'
  });
  ```

  Instead of writing the above, your code now simply becomes the following:

  ```javascript
  var promise = RSVP.Promise.reject(new Error('WHOOPS'));

  promise.then(function(value){
    // Code here doesn't run because the promise is rejected!
  }, function(reason){
    // reason.message === 'WHOOPS'
  });
  ```

  @method reject
  @static
  @param {*} reason value that the returned promise will be rejected with.
  @param {String} label optional string for identifying the returned promise.
  Useful for tooling.
  @return {Promise} a promise rejected with the given `reason`.
*/
function reject$1(reason, label) {
  /*jshint validthis:true */
  var Constructor = this;
  var promise = new Constructor(noop, label);
  reject(promise, reason);
  return promise;
}

var guidKey = 'rsvp_' + now() + '-';
var counter = 0;

function needsResolver() {
  throw new TypeError('You must pass a resolver function as the first argument to the promise constructor');
}

function needsNew() {
  throw new TypeError("Failed to construct 'Promise': Please use the 'new' operator, this object constructor cannot be called as a function.");
}

/**
  Promise objects represent the eventual result of an asynchronous operation. The
  primary way of interacting with a promise is through its `then` method, which
  registers callbacks to receive either a promise’s eventual value or the reason
  why the promise cannot be fulfilled.

  Terminology
  -----------

  - `promise` is an object or function with a `then` method whose behavior conforms to this specification.
  - `thenable` is an object or function that defines a `then` method.
  - `value` is any legal JavaScript value (including undefined, a thenable, or a promise).
  - `exception` is a value that is thrown using the throw statement.
  - `reason` is a value that indicates why a promise was rejected.
  - `settled` the final resting state of a promise, fulfilled or rejected.

  A promise can be in one of three states: pending, fulfilled, or rejected.

  Promises that are fulfilled have a fulfillment value and are in the fulfilled
  state.  Promises that are rejected have a rejection reason and are in the
  rejected state.  A fulfillment value is never a thenable.

  Promises can also be said to *resolve* a value.  If this value is also a
  promise, then the original promise's settled state will match the value's
  settled state.  So a promise that *resolves* a promise that rejects will
  itself reject, and a promise that *resolves* a promise that fulfills will
  itself fulfill.


  Basic Usage:
  ------------

  ```js
  var promise = new Promise(function(resolve, reject) {
    // on success
    resolve(value);

    // on failure
    reject(reason);
  });

  promise.then(function(value) {
    // on fulfillment
  }, function(reason) {
    // on rejection
  });
  ```

  Advanced Usage:
  ---------------

  Promises shine when abstracting away asynchronous interactions such as
  `XMLHttpRequest`s.

  ```js
  function getJSON(url) {
    return new Promise(function(resolve, reject){
      var xhr = new XMLHttpRequest();

      xhr.open('GET', url);
      xhr.onreadystatechange = handler;
      xhr.responseType = 'json';
      xhr.setRequestHeader('Accept', 'application/json');
      xhr.send();

      function handler() {
        if (this.readyState === this.DONE) {
          if (this.status === 200) {
            resolve(this.response);
          } else {
            reject(new Error('getJSON: `' + url + '` failed with status: [' + this.status + ']'));
          }
        }
      };
    });
  }

  getJSON('/posts.json').then(function(json) {
    // on fulfillment
  }, function(reason) {
    // on rejection
  });
  ```

  Unlike callbacks, promises are great composable primitives.

  ```js
  Promise.all([
    getJSON('/posts'),
    getJSON('/comments')
  ]).then(function(values){
    values[0] // => postsJSON
    values[1] // => commentsJSON

    return values;
  });
  ```

  @class RSVP.Promise
  @param {function} resolver
  @param {String} label optional string for labeling the promise.
  Useful for tooling.
  @constructor
*/
function Promise$1(resolver, label) {
  this._id = counter++;
  this._label = label;
  this._state = undefined;
  this._result = undefined;
  this._subscribers = [];

  config.instrument && instrument('created', this);

  if (noop !== resolver) {
    typeof resolver !== 'function' && needsResolver();
    this instanceof Promise$1 ? initializePromise(this, resolver) : needsNew();
  }
}

Promise$1.cast = resolve$1; // deprecated
Promise$1.all = all;
Promise$1.race = race;
Promise$1.resolve = resolve$1;
Promise$1.reject = reject$1;

Promise$1.prototype = {
  constructor: Promise$1,

  _guidKey: guidKey,

  _onError: function (reason) {
    var promise = this;
    config.after(function() {
      if (promise._onError) {
        config['trigger']('error', reason, promise._label);
      }
    });
  },

/**
  The primary way of interacting with a promise is through its `then` method,
  which registers callbacks to receive either a promise's eventual value or the
  reason why the promise cannot be fulfilled.

  ```js
  findUser().then(function(user){
    // user is available
  }, function(reason){
    // user is unavailable, and you are given the reason why
  });
  ```

  Chaining
  --------

  The return value of `then` is itself a promise.  This second, 'downstream'
  promise is resolved with the return value of the first promise's fulfillment
  or rejection handler, or rejected if the handler throws an exception.

  ```js
  findUser().then(function (user) {
    return user.name;
  }, function (reason) {
    return 'default name';
  }).then(function (userName) {
    // If `findUser` fulfilled, `userName` will be the user's name, otherwise it
    // will be `'default name'`
  });

  findUser().then(function (user) {
    throw new Error('Found user, but still unhappy');
  }, function (reason) {
    throw new Error('`findUser` rejected and we're unhappy');
  }).then(function (value) {
    // never reached
  }, function (reason) {
    // if `findUser` fulfilled, `reason` will be 'Found user, but still unhappy'.
    // If `findUser` rejected, `reason` will be '`findUser` rejected and we're unhappy'.
  });
  ```
  If the downstream promise does not specify a rejection handler, rejection reasons will be propagated further downstream.

  ```js
  findUser().then(function (user) {
    throw new PedagogicalException('Upstream error');
  }).then(function (value) {
    // never reached
  }).then(function (value) {
    // never reached
  }, function (reason) {
    // The `PedgagocialException` is propagated all the way down to here
  });
  ```

  Assimilation
  ------------

  Sometimes the value you want to propagate to a downstream promise can only be
  retrieved asynchronously. This can be achieved by returning a promise in the
  fulfillment or rejection handler. The downstream promise will then be pending
  until the returned promise is settled. This is called *assimilation*.

  ```js
  findUser().then(function (user) {
    return findCommentsByAuthor(user);
  }).then(function (comments) {
    // The user's comments are now available
  });
  ```

  If the assimliated promise rejects, then the downstream promise will also reject.

  ```js
  findUser().then(function (user) {
    return findCommentsByAuthor(user);
  }).then(function (comments) {
    // If `findCommentsByAuthor` fulfills, we'll have the value here
  }, function (reason) {
    // If `findCommentsByAuthor` rejects, we'll have the reason here
  });
  ```

  Simple Example
  --------------

  Synchronous Example

  ```javascript
  var result;

  try {
    result = findResult();
    // success
  } catch(reason) {
    // failure
  }
  ```

  Errback Example

  ```js
  findResult(function(result, err){
    if (err) {
      // failure
    } else {
      // success
    }
  });
  ```

  Promise Example;

  ```javascript
  findResult().then(function(result){
    // success
  }, function(reason){
    // failure
  });
  ```

  Advanced Example
  --------------

  Synchronous Example

  ```javascript
  var author, books;

  try {
    author = findAuthor();
    books  = findBooksByAuthor(author);
    // success
  } catch(reason) {
    // failure
  }
  ```

  Errback Example

  ```js

  function foundBooks(books) {

  }

  function failure(reason) {

  }

  findAuthor(function(author, err){
    if (err) {
      failure(err);
      // failure
    } else {
      try {
        findBoooksByAuthor(author, function(books, err) {
          if (err) {
            failure(err);
          } else {
            try {
              foundBooks(books);
            } catch(reason) {
              failure(reason);
            }
          }
        });
      } catch(error) {
        failure(err);
      }
      // success
    }
  });
  ```

  Promise Example;

  ```javascript
  findAuthor().
    then(findBooksByAuthor).
    then(function(books){
      // found books
  }).catch(function(reason){
    // something went wrong
  });
  ```

  @method then
  @param {Function} onFulfillment
  @param {Function} onRejection
  @param {String} label optional string for labeling the promise.
  Useful for tooling.
  @return {Promise}
*/
  then: then,

/**
  `catch` is simply sugar for `then(undefined, onRejection)` which makes it the same
  as the catch block of a try/catch statement.

  ```js
  function findAuthor(){
    throw new Error('couldn't find that author');
  }

  // synchronous
  try {
    findAuthor();
  } catch(reason) {
    // something went wrong
  }

  // async with promises
  findAuthor().catch(function(reason){
    // something went wrong
  });
  ```

  @method catch
  @param {Function} onRejection
  @param {String} label optional string for labeling the promise.
  Useful for tooling.
  @return {Promise}
*/
  'catch': function(onRejection, label) {
    return this.then(undefined, onRejection, label);
  },

/**
  `finally` will be invoked regardless of the promise's fate just as native
  try/catch/finally behaves

  Synchronous example:

  ```js
  findAuthor() {
    if (Math.random() > 0.5) {
      throw new Error();
    }
    return new Author();
  }

  try {
    return findAuthor(); // succeed or fail
  } catch(error) {
    return findOtherAuther();
  } finally {
    // always runs
    // doesn't affect the return value
  }
  ```

  Asynchronous example:

  ```js
  findAuthor().catch(function(reason){
    return findOtherAuther();
  }).finally(function(){
    // author was either found, or not
  });
  ```

  @method finally
  @param {Function} callback
  @param {String} label optional string for labeling the promise.
  Useful for tooling.
  @return {Promise}
*/
  'finally': function(callback, label) {
    var promise = this;
    var constructor = promise.constructor;

    return promise.then(function(value) {
      return constructor.resolve(callback()).then(function() {
        return value;
      });
    }, function(reason) {
      return constructor.resolve(callback()).then(function() {
        return constructor.reject(reason);
      });
    }, label);
  }
};

function AllSettled(Constructor, entries, label) {
  this._superConstructor(Constructor, entries, false /* don't abort on reject */, label);
}

AllSettled.prototype = o_create(Enumerator.prototype);
AllSettled.prototype._superConstructor = Enumerator;
AllSettled.prototype._makeResult = makeSettledResult;
AllSettled.prototype._validationError = function() {
  return new Error('allSettled must be called with an array');
};

function PromiseHash(Constructor, object, label) {
  this._superConstructor(Constructor, object, true, label);
}

PromiseHash.prototype = o_create(Enumerator.prototype);
PromiseHash.prototype._superConstructor = Enumerator;
PromiseHash.prototype._init = function() {
  this._result = {};
};

PromiseHash.prototype._validateInput = function(input) {
  return input && typeof input === 'object';
};

PromiseHash.prototype._validationError = function() {
  return new Error('Promise.hash must be called with an object');
};

PromiseHash.prototype._enumerate = function() {
  var enumerator = this;
  var promise    = enumerator.promise;
  var input      = enumerator._input;
  var results    = [];

  for (var key in input) {
    if (promise._state === PENDING && Object.prototype.hasOwnProperty.call(input, key)) {
      results.push({
        position: key,
        entry: input[key]
      });
    }
  }

  var length = results.length;
  enumerator._remaining = length;
  var result;

  for (var i = 0; promise._state === PENDING && i < length; i++) {
    result = results[i];
    enumerator._eachEntry(result.entry, result.position);
  }
};

function HashSettled(Constructor, object, label) {
  this._superConstructor(Constructor, object, false, label);
}

HashSettled.prototype = o_create(PromiseHash.prototype);
HashSettled.prototype._superConstructor = Enumerator;
HashSettled.prototype._makeResult = makeSettledResult;

HashSettled.prototype._validationError = function() {
  return new Error('hashSettled must be called with an object');
};

/**
  `RSVP.rethrow` will rethrow an error on the next turn of the JavaScript event
  loop in order to aid debugging.

  Promises A+ specifies that any exceptions that occur with a promise must be
  caught by the promises implementation and bubbled to the last handler. For
  this reason, it is recommended that you always specify a second rejection
  handler function to `then`. However, `RSVP.rethrow` will throw the exception
  outside of the promise, so it bubbles up to your console if in the browser,
  or domain/cause uncaught exception in Node. `rethrow` will also throw the
  error again so the error can be handled by the promise per the spec.

  ```javascript
  function throws(){
    throw new Error('Whoops!');
  }

  var promise = new RSVP.Promise(function(resolve, reject){
    throws();
  });

  promise.catch(RSVP.rethrow).then(function(){
    // Code here doesn't run because the promise became rejected due to an
    // error!
  }, function (err){
    // handle the error here
  });
  ```

  The 'Whoops' error will be thrown on the next turn of the event loop
  and you can watch for it in your console. You can also handle it using a
  rejection handler given to `.then` or `.catch` on the returned promise.

  @method rethrow
  @static
  @for RSVP
  @param {Error} reason reason the promise became rejected.
  @throws Error
  @static
*/

var len = 0;
var vertxNext;
function asap(callback, arg) {
  queue$1[len] = callback;
  queue$1[len + 1] = arg;
  len += 2;
  if (len === 2) {
    // If len is 1, that means that we need to schedule an async flush.
    // If additional callbacks are queued before the queue is flushed, they
    // will be processed by this flush that we are scheduling.
    scheduleFlush$1();
  }
}

var browserWindow = (typeof window !== 'undefined') ? window : undefined;
var browserGlobal = browserWindow || {};
var BrowserMutationObserver = browserGlobal.MutationObserver || browserGlobal.WebKitMutationObserver;
var isNode = typeof self === 'undefined' &&
  typeof process !== 'undefined' && {}.toString.call(process) === '[object process]';

// test for web worker but not in IE10
var isWorker = typeof Uint8ClampedArray !== 'undefined' &&
  typeof importScripts !== 'undefined' &&
  typeof MessageChannel !== 'undefined';

// node
function useNextTick() {
  var nextTick = process.nextTick;
  // node version 0.10.x displays a deprecation warning when nextTick is used recursively
  // setImmediate should be used instead instead
  var version = process.versions.node.match(/^(?:(\d+)\.)?(?:(\d+)\.)?(\*|\d+)$/);
  if (Array.isArray(version) && version[1] === '0' && version[2] === '10') {
    nextTick = setImmediate;
  }
  return function() {
    nextTick(flush);
  };
}

// vertx
function useVertxTimer() {
  return function() {
    vertxNext(flush);
  };
}

function useMutationObserver() {
  var iterations = 0;
  var observer = new BrowserMutationObserver(flush);
  var node = document.createTextNode('');
  observer.observe(node, { characterData: true });

  return function() {
    node.data = (iterations = ++iterations % 2);
  };
}

// web worker
function useMessageChannel() {
  var channel = new MessageChannel();
  channel.port1.onmessage = flush;
  return function () {
    channel.port2.postMessage(0);
  };
}

function useSetTimeout() {
  return function() {
    setTimeout(flush, 1);
  };
}

var queue$1 = new Array(1000);
function flush() {
  for (var i = 0; i < len; i+=2) {
    var callback = queue$1[i];
    var arg = queue$1[i+1];

    callback(arg);

    queue$1[i] = undefined;
    queue$1[i+1] = undefined;
  }

  len = 0;
}

function attemptVertex() {
  try {
    var r = require;
    var vertx = r('vertx');
    vertxNext = vertx.runOnLoop || vertx.runOnContext;
    return useVertxTimer();
  } catch(e) {
    return useSetTimeout();
  }
}

var scheduleFlush$1;
// Decide what async method to use to triggering processing of queued callbacks:
if (isNode) {
  scheduleFlush$1 = useNextTick();
} else if (BrowserMutationObserver) {
  scheduleFlush$1 = useMutationObserver();
} else if (isWorker) {
  scheduleFlush$1 = useMessageChannel();
} else if (browserWindow === undefined && typeof require === 'function') {
  scheduleFlush$1 = attemptVertex();
} else {
  scheduleFlush$1 = useSetTimeout();
}

// defaults
config.async = asap;
config.after = function(cb) {
  setTimeout(cb, 0);
};
function on() {
  config['on'].apply(config, arguments);
}

// Set up instrumentation through `window.__PROMISE_INTRUMENTATION__`
if (typeof window !== 'undefined' && typeof window['__PROMISE_INSTRUMENTATION__'] === 'object') {
  var callbacks = window['__PROMISE_INSTRUMENTATION__'];
  configure('instrument', true);
  for (var eventName in callbacks) {
    if (callbacks.hasOwnProperty(eventName)) {
      on(eventName, callbacks[eventName]);
    }
  }
}

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) {
  return typeof obj;
} : function (obj) {
  return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj;
};











var classCallCheck = function (instance, Constructor) {
  if (!(instance instanceof Constructor)) {
    throw new TypeError("Cannot call a class as a function");
  }
};

var createClass = function () {
  function defineProperties(target, props) {
    for (var i = 0; i < props.length; i++) {
      var descriptor = props[i];
      descriptor.enumerable = descriptor.enumerable || false;
      descriptor.configurable = true;
      if ("value" in descriptor) descriptor.writable = true;
      Object.defineProperty(target, descriptor.key, descriptor);
    }
  }

  return function (Constructor, protoProps, staticProps) {
    if (protoProps) defineProperties(Constructor.prototype, protoProps);
    if (staticProps) defineProperties(Constructor, staticProps);
    return Constructor;
  };
}();

var Cookie = function () {
  function Cookie(node, heimdall) {
    classCallCheck(this, Cookie);

    this._node = node;
    this._restoreNode = node.parent;
    this._heimdall = heimdall;
    this._stopped = false;
  }

  createClass(Cookie, [{
    key: "stop",
    value: function stop() {
      var monitor = void 0;

      if (this._heimdall.current !== this._node) {
        return;
      } else if (this.stopped === true) {
        return;
      }

      this._stopped = true;
      this._heimdall._recordTime();
      this._heimdall._session.current = this._restoreNode;
    }
  }, {
    key: "resume",
    value: function resume() {
      if (this._stopped === false) {
        return;
      }

      this._stopped = false;
      this._restoreNode = this._heimdall.current;
      this._heimdall._session.current = this._node;
    }
  }, {
    key: "stats",
    get: function get() {
      return this._node.stats.own;
    }
  }]);
  return Cookie;
}();

var HeimdallNode = function () {
  function HeimdallNode(heimdall, id, data) {
    classCallCheck(this, HeimdallNode);

    this._heimdall = heimdall;

    this._id = heimdall.generateNextId();
    this.id = id;

    if (!(_typeof(this.id) === 'object' && this.id !== null && typeof this.id.name === 'string')) {
      throw new TypeError('HeimdallNode#id.name must be a string');
    }

    this.stats = {
      own: data,
      time: { self: 0 }
    };

    this._children = [];

    this.parent = null;
  }

  createClass(HeimdallNode, [{
    key: 'visitPreOrder',
    value: function visitPreOrder(cb) {
      cb(this);

      for (var i = 0; i < this._children.length; i++) {
        this._children[i].visitPreOrder(cb);
      }
    }
  }, {
    key: 'visitPostOrder',
    value: function visitPostOrder(cb) {
      for (var i = 0; i < this._children.length; i++) {
        this._children[i].visitPostOrder(cb);
      }

      cb(this);
    }
  }, {
    key: 'forEachChild',
    value: function forEachChild(cb) {
      for (var i = 0; i < this._children.length; ++i) {
        cb(this._children[i]);
      }
    }
  }, {
    key: 'remove',
    value: function remove() {
      if (!this.parent) {
        throw new Error('Cannot remove the root heimdalljs node.');
      }
      if (this._heimdall.current === this) {
        throw new Error('Cannot remove an active heimdalljs node.');
      }

      return this.parent.removeChild(this);
    }
  }, {
    key: 'toJSON',
    value: function toJSON() {
      return {
        _id: this._id,
        id: this.id,
        stats: this.stats,
        children: this._children.map(function (child) {
          return child._id;
        })
      };
    }
  }, {
    key: 'toJSONSubgraph',
    value: function toJSONSubgraph() {
      var nodes = [];

      this.visitPreOrder(function (node) {
        return nodes.push(node.toJSON());
      });

      return nodes;
    }
  }, {
    key: 'addChild',
    value: function addChild(node) {
      if (node.parent) {
        throw new TypeError('Node ' + node._id + ' already has a parent.  Cannot add to ' + this._id);
      }

      this._children.push(node);

      node.parent = this;
    }
  }, {
    key: 'removeChild',
    value: function removeChild(child) {
      var index = this._children.indexOf(child);

      if (index < 0) {
        throw new Error('Child(' + child._id + ') not found in Parent(' + this._id + ').  Something is very wrong.');
      }
      this._children.splice(index, 1);

      child.parent = null;

      return child;
    }
  }, {
    key: 'isRoot',
    get: function get() {
      return this.parent === null;
    }
  }]);
  return HeimdallNode;
}();

// provides easily interceptable indirection.
var Dict = function () {
  function Dict() {
    classCallCheck(this, Dict);

    this._storage = {};
  }

  createClass(Dict, [{
    key: 'has',
    value: function has(key) {
      return key in this._storage;
    }
  }, {
    key: 'get',
    value: function get(key) {
      return this._storage[key];
    }
  }, {
    key: 'set',
    value: function set(key, value) {
      return this._storage[key] = value;
    }
  }, {
    key: 'delete',
    value: function _delete(key) {
      delete this._storage[key];
    }
  }]);
  return Dict;
}();

var HeimdallSession = function () {
  function HeimdallSession() {
    classCallCheck(this, HeimdallSession);

    this.reset();
  }

  createClass(HeimdallSession, [{
    key: 'reset',
    value: function reset() {
      this._nextId = 0;
      this.current = undefined;
      this.root = null;
      this.previousTimeNS = 0;
      this.monitorSchemas = new Dict();
      this.configs = new Dict();
    }
  }, {
    key: 'generateNextId',
    value: function generateNextId() {
      return this._nextId++;
    }
  }]);
  return HeimdallSession;
}();

var timeNS = void 0;

// adapted from
// https://gist.github.com/paulirish/5438650
var now$1 = void 0;
if ((typeof performance === 'undefined' ? 'undefined' : _typeof(performance)) === 'object' && typeof performance.now === 'function') {
  now$1 = function now$1() {
    return performance.now.call(performance);
  };
} else {
  now$1 = Date.now || function () {
    return new Date().getTime();
  };
}

var dateOffset = now$1();

function timeFromDate() {
  var timeMS = now$1() - dateOffset;

  return Math.floor(timeMS * 1e6);
}

function timeFromHRTime() {
  var hrtime = process.hrtime();
  return hrtime[0] * 1e9 + hrtime[1];
}

if ((typeof process === 'undefined' ? 'undefined' : _typeof(process)) === 'object' && typeof process.hrtime === 'function') {
  timeNS = timeFromHRTime;
} else {
  timeNS = timeFromDate;
}

var timeNS$1 = timeNS;

var Heimdall = function () {
  function Heimdall(session) {
    classCallCheck(this, Heimdall);

    if (arguments.length < 1) {
      session = new HeimdallSession();
    }

    this._session = session;
    this._reset(false);
  }

  createClass(Heimdall, [{
    key: '_reset',
    value: function _reset(resetSession) {
      if (resetSession !== false) {
        this._session.reset();
      }

      if (!this.root) {
        // The first heimdall to start will create the session and root.  Subsequent
        // heimdall instances continue to use the existing graph
        this.start('heimdall');
        this._session.root = this._session.current;
      }
    }
  }, {
    key: 'start',
    value: function start(name, Schema) {
      var id = void 0;
      var data = void 0;

      if (typeof name === 'string') {
        id = { name: name };
      } else {
        id = name;
      }

      if (typeof Schema === 'function') {
        data = new Schema();
      } else {
        data = {};
      }

      this._recordTime();

      var node = new HeimdallNode(this, id, data);
      if (this.current) {
        this.current.addChild(node);
      }

      this._session.current = node;

      return new Cookie(node, this);
    }
  }, {
    key: '_recordTime',
    value: function _recordTime() {
      var time = timeNS$1();

      // always true except for root
      if (this.current) {
        var delta = time - this._session.previousTimeNS;
        this.current.stats.time.self += delta;
      }
      this._session.previousTimeNS = time;
    }
  }, {
    key: 'node',
    value: function node(name, Schema, callback, context) {
      if (arguments.length < 3) {
        callback = Schema;
        Schema = undefined;
      }

      var cookie = this.start(name, Schema);

      // NOTE: only works in very specific scenarios, specifically promises must
      // not escape their parents lifetime. In theory, promises could be augmented
      // to support those more advanced scenarios.
      return new Promise$1(function (resolve$$1) {
        return resolve$$1(callback.call(context, cookie._node.stats.own));
      }).finally(function () {
        return cookie.stop();
      });
    }
  }, {
    key: 'hasMonitor',
    value: function hasMonitor(name) {
      return this._session.monitorSchemas.has(name);
    }
  }, {
    key: 'registerMonitor',
    value: function registerMonitor(name, Schema) {
      if (name === 'own' || name === 'time') {
        throw new Error('Cannot register monitor at namespace "' + name + '".  "own" and "time" are reserved');
      }
      if (this.hasMonitor(name)) {
        throw new Error('A monitor for "' + name + '" is already registered"');
      }

      this._session.monitorSchemas.set(name, Schema);
    }
  }, {
    key: 'statsFor',
    value: function statsFor(name) {
      var stats = this.current.stats;
      var Schema = void 0;

      if (!stats[name]) {
        Schema = this._session.monitorSchemas.get(name);
        if (!Schema) {
          throw new Error('No monitor registered for "' + name + '"');
        }
        stats[name] = new Schema();
      }

      return stats[name];
    }
  }, {
    key: 'configFor',
    value: function configFor(name) {
      var config = this._session.configs.get(name);

      if (!config) {
        config = this._session.configs.set(name, {});
      }

      return config;
    }
  }, {
    key: 'toJSON',
    value: function toJSON() {
      return { nodes: this.root.toJSONSubgraph() };
    }
  }, {
    key: 'visitPreOrder',
    value: function visitPreOrder(cb) {
      return this.root.visitPreOrder(cb);
    }
  }, {
    key: 'visitPostOrder',
    value: function visitPostOrder(cb) {
      return this.root.visitPostOrder(cb);
    }
  }, {
    key: 'generateNextId',
    value: function generateNextId() {
      return this._session.generateNextId();
    }
  }, {
    key: 'current',
    get: function get() {
      return this._session.current;
    }
  }, {
    key: 'root',
    get: function get() {
      return this._session.root;
    }
  }, {
    key: 'stack',
    get: function get() {
      var stack = [];
      var top = this.current;

      while (top !== undefined && top !== this.root) {
        stack.unshift(top);
        top = top.parent;
      }

      return stack.map(function (node) {
        return node.id.name;
      });
    }
  }]);
  return Heimdall;
}();

function setupSession(global) {

  // The name of the property encodes the session/node compatibilty version
  if (!global._heimdall_session_2) {
    global._heimdall_session_2 = new HeimdallSession();
  }
}

setupSession(self);

// browser equivalent of heimdall.js
self.Heimdall = Heimdall;

var index = new Heimdall(self._heimdall_session_2);

return index;

})));
