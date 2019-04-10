'use strict';

function _interopDefault (ex) { return (ex && (typeof ex === 'object') && 'default' in ex) ? ex['default'] : ex; }

var debug = _interopDefault(require('debug'));
var heimdall = _interopDefault(require('heimdalljs'));

var asyncGenerator = function () {
  function AwaitValue(value) {
    this.value = value;
  }

  function AsyncGenerator(gen) {
    var front, back;

    function send(key, arg) {
      return new Promise(function (resolve, reject) {
        var request = {
          key: key,
          arg: arg,
          resolve: resolve,
          reject: reject,
          next: null
        };

        if (back) {
          back = back.next = request;
        } else {
          front = back = request;
          resume(key, arg);
        }
      });
    }

    function resume(key, arg) {
      try {
        var result = gen[key](arg);
        var value = result.value;

        if (value instanceof AwaitValue) {
          Promise.resolve(value.value).then(function (arg) {
            resume("next", arg);
          }, function (arg) {
            resume("throw", arg);
          });
        } else {
          settle(result.done ? "return" : "normal", result.value);
        }
      } catch (err) {
        settle("throw", err);
      }
    }

    function settle(type, value) {
      switch (type) {
        case "return":
          front.resolve({
            value: value,
            done: true
          });
          break;

        case "throw":
          front.reject(value);
          break;

        default:
          front.resolve({
            value: value,
            done: false
          });
          break;
      }

      front = front.next;

      if (front) {
        resume(front.key, front.arg);
      } else {
        back = null;
      }
    }

    this._invoke = send;

    if (typeof gen.return !== "function") {
      this.return = undefined;
    }
  }

  if (typeof Symbol === "function" && Symbol.asyncIterator) {
    AsyncGenerator.prototype[Symbol.asyncIterator] = function () {
      return this;
    };
  }

  AsyncGenerator.prototype.next = function (arg) {
    return this._invoke("next", arg);
  };

  AsyncGenerator.prototype.throw = function (arg) {
    return this._invoke("throw", arg);
  };

  AsyncGenerator.prototype.return = function (arg) {
    return this._invoke("return", arg);
  };

  return {
    wrap: function (fn) {
      return function () {
        return new AsyncGenerator(fn.apply(this, arguments));
      };
    },
    await: function (value) {
      return new AwaitValue(value);
    }
  };
}();

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

var MATCHER = function MATCHER(n) {
  return true;
};

var Prefixer = function () {
  function Prefixer() {
    classCallCheck(this, Prefixer);

    var logConfig = heimdall.configFor('logging');

    this.matcher = logConfig.matcher || MATCHER;
    this.depth = typeof logConfig.depth === 'number' ? logConfig.depth : 3;
  }

  // TODO: possibly memoize this using a WeakMap
  //  currently we compute prefix on every call to `log`


  createClass(Prefixer, [{
    key: 'prefix',
    value: function prefix() {
      var parts = [];
      var node = heimdall.current;

      while (node) {
        if (node.isRoot || parts.length >= this.depth) {
          break;
        }

        if (this.matcher(node.id)) {
          parts.push(node.id.name + '#' + node._id);
        }

        node = node.parent;
      }

      return parts.length > 0 ? '[' + parts.reverse().join(' -> ') + '] ' : '';
    }
  }]);
  return Prefixer;
}();

var ERROR = 0;
var WARN = 1;
var INFO = 2;
var DEBUG = 3;
var TRACE = 4;
var Logger = function () {
  function Logger(namespace, level) {
    classCallCheck(this, Logger);

    this.level = level;

    this._print = debug(namespace);
    this._prefixer = new Prefixer();
  }

  createClass(Logger, [{
    key: '_message',
    value: function _message(level, msg) {
      if (level <= this.level) {
        for (var _len = arguments.length, args = Array(_len > 2 ? _len - 2 : 0), _key = 2; _key < _len; _key++) {
          args[_key - 2] = arguments[_key];
        }

        this._print.apply(this, ['' + this._prefixer.prefix() + msg].concat(args));
      }
    }
  }, {
    key: 'trace',
    value: function trace() {
      for (var _len2 = arguments.length, args = Array(_len2), _key2 = 0; _key2 < _len2; _key2++) {
        args[_key2] = arguments[_key2];
      }

      return this._message.apply(this, [TRACE].concat(args));
    }
  }, {
    key: 'debug',
    value: function debug() {
      for (var _len3 = arguments.length, args = Array(_len3), _key3 = 0; _key3 < _len3; _key3++) {
        args[_key3] = arguments[_key3];
      }

      return this._message.apply(this, [DEBUG].concat(args));
    }
  }, {
    key: 'info',
    value: function info() {
      for (var _len4 = arguments.length, args = Array(_len4), _key4 = 0; _key4 < _len4; _key4++) {
        args[_key4] = arguments[_key4];
      }

      return this._message.apply(this, [INFO].concat(args));
    }
  }, {
    key: 'warn',
    value: function warn() {
      for (var _len5 = arguments.length, args = Array(_len5), _key5 = 0; _key5 < _len5; _key5++) {
        args[_key5] = arguments[_key5];
      }

      return this._message.apply(this, [WARN].concat(args));
    }
  }, {
    key: 'error',
    value: function error() {
      for (var _len6 = arguments.length, args = Array(_len6), _key6 = 0; _key6 < _len6; _key6++) {
        args[_key6] = arguments[_key6];
      }

      return this._message.apply(this, [ERROR].concat(args));
    }
  }]);
  return Logger;
}();

var NULL_LOGGER = {
  trace: function trace() {},
  debug: function debug() {},
  info: function info() {},
  warn: function warn() {},
  error: function error() {}
};

function computeDebugLevel() {
  var level = void 0;

  if (!process.env.DEBUG_LEVEL) {
    level = INFO;
  } else {
    switch (process.env.DEBUG_LEVEL.toUpperCase()) {
      case 'ERROR':
        level = ERROR;break;
      case 'WARN':
        level = WARN;break;
      case 'INFO':
        level = INFO;break;
      case 'DEBUG':
        level = DEBUG;break;
      case 'TRACE':
        level = TRACE;break;
      default:
        level = parseInt(process.env.DEBUG_LEVEL, 10);
    }
  }

  logGenerator.debugLevel = level;
}

function logGenerator(namespace) {
  if (debug.enabled(namespace)) {
    if (logGenerator.debugLevel === undefined) {
      computeDebugLevel();
    }

    return new Logger(namespace, logGenerator.debugLevel);
  } else {
    return NULL_LOGGER;
  }
}

logGenerator.debugLevel = undefined;

module.exports = logGenerator;