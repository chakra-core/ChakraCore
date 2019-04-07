'use strict';
/**
 * Only accept codes that are numbers, otherwise discard them
 * @param {*} code
 * @returns {number}
 * @private
 */

function _typeof(obj) { if (typeof Symbol === "function" && typeof Symbol.iterator === "symbol") { _typeof = function _typeof(obj) { return typeof obj; }; } else { _typeof = function _typeof(obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; }; } return _typeof(obj); }

function _toConsumableArray(arr) { return _arrayWithoutHoles(arr) || _iterableToArray(arr) || _nonIterableSpread(); }

function _nonIterableSpread() { throw new TypeError("Invalid attempt to spread non-iterable instance"); }

function _iterableToArray(iter) { if (Symbol.iterator in Object(iter) || Object.prototype.toString.call(iter) === "[object Arguments]") return Array.from(iter); }

function _arrayWithoutHoles(arr) { if (Array.isArray(arr)) { for (var i = 0, arr2 = new Array(arr.length); i < arr.length; i++) { arr2[i] = arr[i]; } return arr2; } }

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function _defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } }

function _createClass(Constructor, protoProps, staticProps) { if (protoProps) _defineProperties(Constructor.prototype, protoProps); if (staticProps) _defineProperties(Constructor, staticProps); return Constructor; }

function _possibleConstructorReturn(self, call) { if (call && (_typeof(call) === "object" || typeof call === "function")) { return call; } return _assertThisInitialized(self); }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function"); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, writable: true, configurable: true } }); if (superClass) _setPrototypeOf(subClass, superClass); }

function _assertThisInitialized(self) { if (self === void 0) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return self; }

function _wrapNativeSuper(Class) { var _cache = typeof Map === "function" ? new Map() : undefined; _wrapNativeSuper = function _wrapNativeSuper(Class) { if (Class === null || !_isNativeFunction(Class)) return Class; if (typeof Class !== "function") { throw new TypeError("Super expression must either be null or a function"); } if (typeof _cache !== "undefined") { if (_cache.has(Class)) return _cache.get(Class); _cache.set(Class, Wrapper); } function Wrapper() { return _construct(Class, arguments, _getPrototypeOf(this).constructor); } Wrapper.prototype = Object.create(Class.prototype, { constructor: { value: Wrapper, enumerable: false, writable: true, configurable: true } }); return _setPrototypeOf(Wrapper, Class); }; return _wrapNativeSuper(Class); }

function isNativeReflectConstruct() { if (typeof Reflect === "undefined" || !Reflect.construct) return false; if (Reflect.construct.sham) return false; if (typeof Proxy === "function") return true; try { Date.prototype.toString.call(Reflect.construct(Date, [], function () {})); return true; } catch (e) { return false; } }

function _construct(Parent, args, Class) { if (isNativeReflectConstruct()) { _construct = Reflect.construct; } else { _construct = function _construct(Parent, args, Class) { var a = [null]; a.push.apply(a, args); var Constructor = Function.bind.apply(Parent, a); var instance = new Constructor(); if (Class) _setPrototypeOf(instance, Class.prototype); return instance; }; } return _construct.apply(null, arguments); }

function _isNativeFunction(fn) { return Function.toString.call(fn).indexOf("[native code]") !== -1; }

function _setPrototypeOf(o, p) { _setPrototypeOf = Object.setPrototypeOf || function _setPrototypeOf(o, p) { o.__proto__ = p; return o; }; return _setPrototypeOf(o, p); }

function _getPrototypeOf(o) { _getPrototypeOf = Object.setPrototypeOf ? Object.getPrototypeOf : function _getPrototypeOf(o) { return o.__proto__ || Object.getPrototypeOf(o); }; return _getPrototypeOf(o); }

function parseCode(code) {
  var number = Number(code);
  if (isNaN(number)) return null;
  return number;
}
/**
 * Fetch the code from the value
 * @param {Object|Error} value
 * @returns {boolean}
 * @private
 */


function fetchCode(value) {
  return value && (parseCode(value.exitCode) || parseCode(value.errno) || parseCode(value.code));
}
/**
 * Prevent [a weird error on node version 4](https://github.com/bevry/errlop/issues/1) and below.
 * @param {*} value
 * @returns {boolean}
 * @private
 */


function isValid(value) {
  /* eslint no-use-before-define:0 */
  return value instanceof Error || Errlop.isErrlop(value);
}
/**
 * Create an instance of an error, using a message, as well as an optional parent.
 * If the parent is provided, then the `fullStack` property will include its stack too
 * @class Errlop
 * @constructor
 * @param {Errlop|Error|Object|string} input
 * @param {Errlop|Error} [parent]
 * @public
 */


var Errlop =
/*#__PURE__*/
function (_Error) {
  _inherits(Errlop, _Error);

  function Errlop(input, parent) {
    var _this;

    _classCallCheck(this, Errlop);

    if (!input) throw new Error('Attempted to create an Errlop without a input'); // Instantiate with the above

    _this = _possibleConstructorReturn(this, _getPrototypeOf(Errlop).call(this, input.message || input));
    /**
     * Duck typing as node 4 and intanceof does not work for error extensions
     * @type {Errlop}
     * @public
     */

    _this.klass = Errlop;
    /**
     * The parent error if it was provided.
     * If a parent was provided, then use that, otherwise use the input's parent, if it exists.
     * @type {Error?}
     * @public
     */

    _this.parent = parent || input.parent;
    /**
     * An array of all the ancestors. From parent, to grand parent, and so on.
     * @type {Array<Error>}
     * @public
     */

    _this.ancestors = [];
    var ancestor = _this.parent;

    while (ancestor) {
      _this.ancestors.push(ancestor);

      ancestor = ancestor.parent;
    } // this code must support node 0.8, as well as prevent a weird bug in node v4: https://travis-ci.org/bevry/editions/jobs/408828147


    var exitCode = fetchCode(input);
    if (exitCode == null) exitCode = fetchCode(_assertThisInitialized(_assertThisInitialized(_this)));

    for (var index = 0; index < _this.ancestors.length && exitCode == null; ++index) {
      var error = _this.ancestors[index];
      if (isValid(error)) exitCode = fetchCode(error);
    }
    /**
     * A numeric code to use for the exit status if desired by the consumer.
     * It cycles through [input, this, ...ancestors] until it finds the first [exitCode, errno, code] that is valid.
     * @type {Number?}
     * @public
     */


    _this.exitCode = exitCode;
    /**
     * The stack for our instance alone, without any parents.
     * If the input contained a stack, then use that.
     * @type {string}
     * @public
     */

    _this.orphanStack = (input.stack || _this.stack).toString();
    /**
     * The stack which now contains the accumalated stacks of its ancestors.
     * This is used instead of an alias like `fullStack` or the like, to ensure existing code that uses `err.stack` doesn't need to be changed to remain functional.
     * @type {string}
     * @public
     */

    _this.stack = [_this.orphanStack].concat(_toConsumableArray(_this.ancestors)).reduce(function (accumulator, error) {
      return "".concat(accumulator, "\n\u21B3 ").concat(error.orphanStack || error.stack || error);
    });
    return _this;
  }
  /**
   * Syntatic sugar for Errlop class creation.
   * Enables `Errlop.create(...args)` to achieve `new Errlop(...args)`
   * @param {...*} args
   * @returns {Errlop}
   * @static
   * @public
   */


  _createClass(Errlop, null, [{
    key: "create",
    value: function create() {
      for (var _len = arguments.length, args = new Array(_len), _key = 0; _key < _len; _key++) {
        args[_key] = arguments[_key];
      }

      return _construct(this, args);
    }
    /**
     * Check whether or not the value is an Errlop instance
     * @param {*} value
     * @returns {boolean}
     * @static
     * @public
     */

  }, {
    key: "isErrlop",
    value: function isErrlop(value) {
      return value && (value instanceof this || value.klass === this);
    }
    /**
     * Ensure that the value is an Errlop instance
     * @param {*} value
     * @returns {Errlop}
     * @static
     * @public
     */

  }, {
    key: "ensure",
    value: function ensure(value) {
      return this.isErrlop(value) ? value : this.create(value);
    }
  }]);

  return Errlop;
}(_wrapNativeSuper(Error));

module.exports = Errlop;