(function() {
/*!
 * @overview  Ember - JavaScript Application Framework
 * @copyright Copyright 2011-2018 Tilde Inc. and contributors
 *            Portions Copyright 2006-2011 Strobe Inc.
 *            Portions Copyright 2008-2011 Apple Inc. All rights reserved.
 * @license   Licensed under MIT license
 *            See https://raw.github.com/emberjs/ember.js/master/LICENSE
 * @version   3.9.1
 */

/*globals process */
var enifed, requireModule, Ember;

// Used in @ember/-internals/environment/lib/global.js
mainContext = this; // eslint-disable-line no-undef

(function() {
  function missingModule(name, referrerName) {
    if (referrerName) {
      throw new Error('Could not find module ' + name + ' required by: ' + referrerName);
    } else {
      throw new Error('Could not find module ' + name);
    }
  }

  function internalRequire(_name, referrerName) {
    var name = _name;
    var mod = registry[name];

    if (!mod) {
      name = name + '/index';
      mod = registry[name];
    }

    var exports = seen[name];

    if (exports !== undefined) {
      return exports;
    }

    exports = seen[name] = {};

    if (!mod) {
      missingModule(_name, referrerName);
    }

    var deps = mod.deps;
    var callback = mod.callback;
    var reified = new Array(deps.length);

    for (var i = 0; i < deps.length; i++) {
      if (deps[i] === 'exports') {
        reified[i] = exports;
      } else if (deps[i] === 'require') {
        reified[i] = requireModule;
      } else {
        reified[i] = internalRequire(deps[i], name);
      }
    }

    callback.apply(this, reified);

    return exports;
  }

  var isNode =
    typeof window === 'undefined' &&
    typeof process !== 'undefined' &&
    {}.toString.call(process) === '[object process]';

  if (!isNode) {
    Ember = this.Ember = this.Ember || {};
  }

  if (typeof Ember === 'undefined') {
    Ember = {};
  }

  if (typeof Ember.__loader === 'undefined') {
    var registry = Object.create(null);
    var seen = Object.create(null);

    enifed = function(name, deps, callback) {
      var value = {};

      if (!callback) {
        value.deps = [];
        value.callback = deps;
      } else {
        value.deps = deps;
        value.callback = callback;
      }

      registry[name] = value;
    };

    requireModule = function(name) {
      return internalRequire(name, null);
    };

    // setup `require` module
    requireModule['default'] = requireModule;

    requireModule.has = function registryHas(moduleName) {
      return Boolean(registry[moduleName]) || Boolean(registry[moduleName + '/index']);
    };

    requireModule._eak_seen = registry;

    Ember.__loader = {
      define: enifed,
      require: requireModule,
      registry: registry,
    };
  } else {
    enifed = Ember.__loader.define;
    requireModule = Ember.__loader.require;
  }
})();

enifed("@ember/-internals/browser-environment", ["exports"], function (_exports) {
  "use strict";

  _exports.hasDOM = _exports.isFirefox = _exports.isChrome = _exports.userAgent = _exports.history = _exports.location = _exports.window = void 0;
  // check if window exists and actually is the global
  var hasDom = typeof self === 'object' && self !== null && self.Object === Object && typeof Window !== 'undefined' && self.constructor === Window && typeof document === 'object' && document !== null && self.document === document && typeof location === 'object' && location !== null && self.location === location && typeof history === 'object' && history !== null && self.history === history && typeof navigator === 'object' && navigator !== null && self.navigator === navigator && typeof navigator.userAgent === 'string';
  _exports.hasDOM = hasDom;
  const window = hasDom ? self : null;
  _exports.window = window;
  const location$1 = hasDom ? self.location : null;
  _exports.location = location$1;
  const history$1 = hasDom ? self.history : null;
  _exports.history = history$1;
  const userAgent = hasDom ? self.navigator.userAgent : 'Lynx (textmode)';
  _exports.userAgent = userAgent;
  const isChrome = hasDom ? Boolean(window.chrome) && !window.opera : false;
  _exports.isChrome = isChrome;
  const isFirefox = hasDom ? typeof InstallTrigger !== 'undefined' : false;
  _exports.isFirefox = isFirefox;
});
enifed("@ember/-internals/environment", ["exports"], function (_exports) {
  "use strict";

  _exports.getLookup = getLookup;
  _exports.setLookup = setLookup;
  _exports.getENV = getENV;
  _exports.ENV = _exports.context = _exports.global = void 0;

  // from lodash to catch fake globals
  function checkGlobal(value) {
    return value && value.Object === Object ? value : undefined;
  } // element ids can ruin global miss checks


  function checkElementIdShadowing(value) {
    return value && value.nodeType === undefined ? value : undefined;
  } // export real global


  var global$1 = checkGlobal(checkElementIdShadowing(typeof global === 'object' && global)) || checkGlobal(typeof self === 'object' && self) || checkGlobal(typeof window === 'object' && window) || typeof mainContext !== 'undefined' && mainContext || // set before strict mode in Ember loader/wrapper
  new Function('return this')(); // eval outside of strict mode
  // legacy imports/exports/lookup stuff (should we keep this??)

  _exports.global = global$1;

  const context = function (global, Ember) {
    return Ember === undefined ? {
      imports: global,
      exports: global,
      lookup: global
    } : {
      // import jQuery
      imports: Ember.imports || global,
      // export Ember
      exports: Ember.exports || global,
      // search for Namespaces
      lookup: Ember.lookup || global
    };
  }(global$1, global$1.Ember);

  _exports.context = context;

  function getLookup() {
    return context.lookup;
  }

  function setLookup(value) {
    context.lookup = value;
  }
  /**
    The hash of environment variables used to control various configuration
    settings. To specify your own or override default settings, add the
    desired properties to a global hash named `EmberENV` (or `ENV` for
    backwards compatibility with earlier versions of Ember). The `EmberENV`
    hash must be created before loading Ember.
  
    @class EmberENV
    @type Object
    @public
  */


  const ENV = {
    ENABLE_OPTIONAL_FEATURES: false,

    /**
      Determines whether Ember should add to `Array`, `Function`, and `String`
      native object prototypes, a few extra methods in order to provide a more
      friendly API.
         We generally recommend leaving this option set to true however, if you need
      to turn it off, you can add the configuration property
      `EXTEND_PROTOTYPES` to `EmberENV` and set it to `false`.
         Note, when disabled (the default configuration for Ember Addons), you will
      instead have to access all methods and functions from the Ember
      namespace.
         @property EXTEND_PROTOTYPES
      @type Boolean
      @default true
      @for EmberENV
      @public
    */
    EXTEND_PROTOTYPES: {
      Array: true,
      Function: true,
      String: true
    },

    /**
      The `LOG_STACKTRACE_ON_DEPRECATION` property, when true, tells Ember to log
      a full stack trace during deprecation warnings.
         @property LOG_STACKTRACE_ON_DEPRECATION
      @type Boolean
      @default true
      @for EmberENV
      @public
    */
    LOG_STACKTRACE_ON_DEPRECATION: true,

    /**
      The `LOG_VERSION` property, when true, tells Ember to log versions of all
      dependent libraries in use.
         @property LOG_VERSION
      @type Boolean
      @default true
      @for EmberENV
      @public
    */
    LOG_VERSION: true,
    RAISE_ON_DEPRECATION: false,
    STRUCTURED_PROFILE: false,

    /**
      Whether to insert a `<div class="ember-view" />` wrapper around the
      application template. See RFC #280.
         This is not intended to be set directly, as the implementation may change in
      the future. Use `@ember/optional-features` instead.
         @property _APPLICATION_TEMPLATE_WRAPPER
      @for EmberENV
      @type Boolean
      @default true
      @private
    */
    _APPLICATION_TEMPLATE_WRAPPER: true,

    /**
      Whether to use Glimmer Component semantics (as opposed to the classic "Curly"
      components semantics) for template-only components. See RFC #278.
         This is not intended to be set directly, as the implementation may change in
      the future. Use `@ember/optional-features` instead.
         @property _TEMPLATE_ONLY_GLIMMER_COMPONENTS
      @for EmberENV
      @type Boolean
      @default false
      @private
    */
    _TEMPLATE_ONLY_GLIMMER_COMPONENTS: false,

    /**
      Whether the app is using jQuery. See RFC #294.
         This is not intended to be set directly, as the implementation may change in
      the future. Use `@ember/optional-features` instead.
         @property _JQUERY_INTEGRATION
      @for EmberENV
      @type Boolean
      @default true
      @private
    */
    _JQUERY_INTEGRATION: true,

    /**
      Controls the maximum number of scheduled rerenders without "settling". In general,
      applications should not need to modify this environment variable, but please
      open an issue so that we can determine if a better default value is needed.
         @property _RERENDER_LOOP_LIMIT
      @for EmberENV
      @type number
      @default 1000
      @private
     */
    _RERENDER_LOOP_LIMIT: 1000,
    EMBER_LOAD_HOOKS: {},
    FEATURES: {}
  };
  _exports.ENV = ENV;

  (EmberENV => {
    if (typeof EmberENV !== 'object' || EmberENV === null) return;

    for (let flag in EmberENV) {
      if (!EmberENV.hasOwnProperty(flag) || flag === 'EXTEND_PROTOTYPES' || flag === 'EMBER_LOAD_HOOKS') continue;
      let defaultValue = ENV[flag];

      if (defaultValue === true) {
        ENV[flag] = EmberENV[flag] !== false;
      } else if (defaultValue === false) {
        ENV[flag] = EmberENV[flag] === true;
      }
    }

    let {
      EXTEND_PROTOTYPES
    } = EmberENV;

    if (EXTEND_PROTOTYPES !== undefined) {
      if (typeof EXTEND_PROTOTYPES === 'object' && EXTEND_PROTOTYPES !== null) {
        ENV.EXTEND_PROTOTYPES.String = EXTEND_PROTOTYPES.String !== false;
        ENV.EXTEND_PROTOTYPES.Function = EXTEND_PROTOTYPES.Function !== false;
        ENV.EXTEND_PROTOTYPES.Array = EXTEND_PROTOTYPES.Array !== false;
      } else {
        let isEnabled = EXTEND_PROTOTYPES !== false;
        ENV.EXTEND_PROTOTYPES.String = isEnabled;
        ENV.EXTEND_PROTOTYPES.Function = isEnabled;
        ENV.EXTEND_PROTOTYPES.Array = isEnabled;
      }
    } // TODO this does not seem to be used by anything,
    //      can we remove it? do we need to deprecate it?


    let {
      EMBER_LOAD_HOOKS
    } = EmberENV;

    if (typeof EMBER_LOAD_HOOKS === 'object' && EMBER_LOAD_HOOKS !== null) {
      for (let hookName in EMBER_LOAD_HOOKS) {
        if (!EMBER_LOAD_HOOKS.hasOwnProperty(hookName)) continue;
        let hooks = EMBER_LOAD_HOOKS[hookName];

        if (Array.isArray(hooks)) {
          ENV.EMBER_LOAD_HOOKS[hookName] = hooks.filter(hook => typeof hook === 'function');
        }
      }
    }

    let {
      FEATURES
    } = EmberENV;

    if (typeof FEATURES === 'object' && FEATURES !== null) {
      for (let feature in FEATURES) {
        if (!FEATURES.hasOwnProperty(feature)) continue;
        ENV.FEATURES[feature] = FEATURES[feature] === true;
      }
    }
  })(global$1.EmberENV || global$1.ENV);

  function getENV() {
    return ENV;
  }
});
enifed("@ember/-internals/utils", ["exports", "@ember/polyfills"], function (_exports, _polyfills) {
  "use strict";

  _exports.symbol = symbol;
  _exports.isInternalSymbol = isInternalSymbol;
  _exports.dictionary = makeDictionary;
  _exports.uuid = uuid;
  _exports.generateGuid = generateGuid;
  _exports.guidFor = guidFor;
  _exports.intern = intern;
  _exports.wrap = wrap;
  _exports.getObservers = getObservers;
  _exports.getListeners = getListeners;
  _exports.setObservers = setObservers;
  _exports.setListeners = setListeners;
  _exports.inspect = inspect;
  _exports.lookupDescriptor = lookupDescriptor;
  _exports.canInvoke = canInvoke;
  _exports.tryInvoke = tryInvoke;
  _exports.makeArray = makeArray;
  _exports.getName = getName;
  _exports.setName = setName;
  _exports.toString = toString;
  _exports.isProxy = isProxy;
  _exports.setProxy = setProxy;
  _exports.Cache = _exports.HAS_NATIVE_PROXY = _exports.HAS_NATIVE_SYMBOL = _exports.ROOT = _exports.checkHasSuper = _exports.GUID_KEY = _exports.getOwnPropertyDescriptors = _exports.NAME_KEY = void 0;

  /**
    Strongly hint runtimes to intern the provided string.
  
    When do I need to use this function?
  
    For the most part, never. Pre-mature optimization is bad, and often the
    runtime does exactly what you need it to, and more often the trade-off isn't
    worth it.
  
    Why?
  
    Runtimes store strings in at least 2 different representations:
    Ropes and Symbols (interned strings). The Rope provides a memory efficient
    data-structure for strings created from concatenation or some other string
    manipulation like splitting.
  
    Unfortunately checking equality of different ropes can be quite costly as
    runtimes must resort to clever string comparison algorithms. These
    algorithms typically cost in proportion to the length of the string.
    Luckily, this is where the Symbols (interned strings) shine. As Symbols are
    unique by their string content, equality checks can be done by pointer
    comparison.
  
    How do I know if my string is a rope or symbol?
  
    Typically (warning general sweeping statement, but truthy in runtimes at
    present) static strings created as part of the JS source are interned.
    Strings often used for comparisons can be interned at runtime if some
    criteria are met.  One of these criteria can be the size of the entire rope.
    For example, in chrome 38 a rope longer then 12 characters will not
    intern, nor will segments of that rope.
  
    Some numbers: http://jsperf.com/eval-vs-keys/8
  
    Known Trick™
  
    @private
    @return {String} interned version of the provided string
  */
  function intern(str) {
    let obj = {};
    obj[str] = 1;

    for (let key in obj) {
      if (key === str) {
        return key;
      }
    }

    return str;
  }
  /**
    Returns whether Type(value) is Object.
  
    Useful for checking whether a value is a valid WeakMap key.
  
    Refs: https://tc39.github.io/ecma262/#sec-typeof-operator-runtime-semantics-evaluation
          https://tc39.github.io/ecma262/#sec-weakmap.prototype.set
  
    @private
    @function isObject
  */


  function isObject(value) {
    return value !== null && (typeof value === 'object' || typeof value === 'function');
  }
  /**
   @module @ember/object
  */

  /**
   Previously we used `Ember.$.uuid`, however `$.uuid` has been removed from
   jQuery master. We'll just bootstrap our own uuid now.
  
   @private
   @return {Number} the uuid
   */


  let _uuid = 0;
  /**
   Generates a universally unique identifier. This method
   is used internally by Ember for assisting with
   the generation of GUID's and other unique identifiers.
  
   @public
   @return {Number} [description]
   */

  function uuid() {
    return ++_uuid;
  }
  /**
   Prefix used for guids through out Ember.
   @private
   @property GUID_PREFIX
   @for Ember
   @type String
   @final
   */


  const GUID_PREFIX = 'ember'; // Used for guid generation...

  const OBJECT_GUIDS = new WeakMap();
  const NON_OBJECT_GUIDS = new Map();
  /**
    A unique key used to assign guids and other private metadata to objects.
    If you inspect an object in your browser debugger you will often see these.
    They can be safely ignored.
  
    On browsers that support it, these properties are added with enumeration
    disabled so they won't show up when you iterate over your properties.
  
    @private
    @property GUID_KEY
    @for Ember
    @type String
    @final
  */

  const GUID_KEY = intern(`__ember${Date.now()}`);
  /**
    Generates a new guid, optionally saving the guid to the object that you
    pass in. You will rarely need to use this method. Instead you should
    call `guidFor(obj)`, which return an existing guid if available.
  
    @private
    @method generateGuid
    @static
    @for @ember/object/internals
    @param {Object} [obj] Object the guid will be used for. If passed in, the guid will
      be saved on the object and reused whenever you pass the same object
      again.
  
      If no object is passed, just generate a new guid.
    @param {String} [prefix] Prefix to place in front of the guid. Useful when you want to
      separate the guid into separate namespaces.
    @return {String} the guid
  */

  _exports.GUID_KEY = GUID_KEY;

  function generateGuid(obj, prefix = GUID_PREFIX) {
    let guid = prefix + uuid();

    if (isObject(obj)) {
      OBJECT_GUIDS.set(obj, guid);
    }

    return guid;
  }
  /**
    Returns a unique id for the object. If the object does not yet have a guid,
    one will be assigned to it. You can call this on any object,
    `EmberObject`-based or not.
  
    You can also use this method on DOM Element objects.
  
    @public
    @static
    @method guidFor
    @for @ember/object/internals
    @param {Object} obj any object, string, number, Element, or primitive
    @return {String} the unique guid for this instance.
  */


  function guidFor(value) {
    let guid;

    if (isObject(value)) {
      guid = OBJECT_GUIDS.get(value);

      if (guid === undefined) {
        guid = GUID_PREFIX + uuid();
        OBJECT_GUIDS.set(value, guid);
      }
    } else {
      guid = NON_OBJECT_GUIDS.get(value);

      if (guid === undefined) {
        let type = typeof value;

        if (type === 'string') {
          guid = 'st' + uuid();
        } else if (type === 'number') {
          guid = 'nu' + uuid();
        } else if (type === 'symbol') {
          guid = 'sy' + uuid();
        } else {
          guid = '(' + value + ')';
        }

        NON_OBJECT_GUIDS.set(value, guid);
      }
    }

    return guid;
  }

  const GENERATED_SYMBOLS = [];

  function isInternalSymbol(possibleSymbol) {
    return GENERATED_SYMBOLS.indexOf(possibleSymbol) !== -1;
  }

  function symbol(debugName) {
    // TODO: Investigate using platform symbols, but we do not
    // want to require non-enumerability for this API, which
    // would introduce a large cost.
    let id = GUID_KEY + Math.floor(Math.random() * Date.now());
    let symbol = intern(`__${debugName}${id}__`);
    GENERATED_SYMBOLS.push(symbol);
    return symbol;
  } // the delete is meant to hint at runtimes that this object should remain in
  // dictionary mode. This is clearly a runtime specific hack, but currently it
  // appears worthwhile in some usecases. Please note, these deletes do increase
  // the cost of creation dramatically over a plain Object.create. And as this
  // only makes sense for long-lived dictionaries that aren't instantiated often.


  function makeDictionary(parent) {
    let dict = Object.create(parent);
    dict['_dict'] = null;
    delete dict['_dict'];
    return dict;
  }

  let getOwnPropertyDescriptors;

  if (Object.getOwnPropertyDescriptors !== undefined) {
    getOwnPropertyDescriptors = Object.getOwnPropertyDescriptors;
  } else {
    getOwnPropertyDescriptors = function (obj) {
      let descriptors = {};
      Object.keys(obj).forEach(key => {
        descriptors[key] = Object.getOwnPropertyDescriptor(obj, key);
      });
      return descriptors;
    };
  }

  var getOwnPropertyDescriptors$1 = getOwnPropertyDescriptors;
  _exports.getOwnPropertyDescriptors = getOwnPropertyDescriptors$1;
  const HAS_SUPER_PATTERN = /\.(_super|call\(this|apply\(this)/;
  const fnToString = Function.prototype.toString;

  const checkHasSuper = (() => {
    let sourceAvailable = fnToString.call(function () {
      return this;
    }).indexOf('return this') > -1;

    if (sourceAvailable) {
      return function checkHasSuper(func) {
        return HAS_SUPER_PATTERN.test(fnToString.call(func));
      };
    }

    return function checkHasSuper() {
      return true;
    };
  })();

  _exports.checkHasSuper = checkHasSuper;
  const HAS_SUPER_MAP = new WeakMap();
  const ROOT = Object.freeze(function () {});
  _exports.ROOT = ROOT;
  HAS_SUPER_MAP.set(ROOT, false);

  function hasSuper(func) {
    let hasSuper = HAS_SUPER_MAP.get(func);

    if (hasSuper === undefined) {
      hasSuper = checkHasSuper(func);
      HAS_SUPER_MAP.set(func, hasSuper);
    }

    return hasSuper;
  }

  const OBSERVERS_MAP = new WeakMap();

  function setObservers(func, observers) {
    if (observers) {
      OBSERVERS_MAP.set(func, observers);
    }
  }

  function getObservers(func) {
    return OBSERVERS_MAP.get(func);
  }

  const LISTENERS_MAP = new WeakMap();

  function setListeners(func, listeners) {
    if (listeners) {
      LISTENERS_MAP.set(func, listeners);
    }
  }

  function getListeners(func) {
    return LISTENERS_MAP.get(func);
  }

  const IS_WRAPPED_FUNCTION_SET = new _polyfills._WeakSet();
  /**
    Wraps the passed function so that `this._super` will point to the superFunc
    when the function is invoked. This is the primitive we use to implement
    calls to super.
  
    @private
    @method wrap
    @for Ember
    @param {Function} func The function to call
    @param {Function} superFunc The super function.
    @return {Function} wrapped function.
  */

  function wrap(func, superFunc) {
    if (!hasSuper(func)) {
      return func;
    } // ensure an unwrapped super that calls _super is wrapped with a terminal _super


    if (!IS_WRAPPED_FUNCTION_SET.has(superFunc) && hasSuper(superFunc)) {
      return _wrap(func, _wrap(superFunc, ROOT));
    }

    return _wrap(func, superFunc);
  }

  function _wrap(func, superFunc) {
    function superWrapper() {
      let orig = this._super;
      this._super = superFunc;
      let ret = func.apply(this, arguments);
      this._super = orig;
      return ret;
    }

    IS_WRAPPED_FUNCTION_SET.add(superWrapper);
    setObservers(superWrapper, getObservers(func));
    setListeners(superWrapper, getListeners(func));
    return superWrapper;
  }

  const {
    toString: objectToString
  } = Object.prototype;
  const {
    toString: functionToString
  } = Function.prototype;
  const {
    isArray
  } = Array;
  const {
    keys: objectKeys
  } = Object;
  const {
    stringify
  } = JSON;
  const LIST_LIMIT = 100;
  const DEPTH_LIMIT = 4;
  const SAFE_KEY = /^[\w$]+$/;
  /**
   @module @ember/debug
  */

  /**
    Convenience method to inspect an object. This method will attempt to
    convert the object into a useful string description.
  
    It is a pretty simple implementation. If you want something more robust,
    use something like JSDump: https://github.com/NV/jsDump
  
    @method inspect
    @static
    @param {Object} obj The object you want to inspect.
    @return {String} A description of the object
    @since 1.4.0
    @private
  */

  function inspect(obj) {
    // detect Node util.inspect call inspect(depth: number, opts: object)
    if (typeof obj === 'number' && arguments.length === 2) {
      return this;
    }

    return inspectValue(obj, 0);
  }

  function inspectValue(value, depth, seen) {
    let valueIsArray = false;

    switch (typeof value) {
      case 'undefined':
        return 'undefined';

      case 'object':
        if (value === null) return 'null';

        if (isArray(value)) {
          valueIsArray = true;
          break;
        } // is toString Object.prototype.toString or undefined then traverse


        if (value.toString === objectToString || value.toString === undefined) {
          break;
        } // custom toString


        return value.toString();

      case 'function':
        return value.toString === functionToString ? value.name ? `[Function:${value.name}]` : `[Function]` : value.toString();

      case 'string':
        return stringify(value);

      case 'symbol':
      case 'boolean':
      case 'number':
      default:
        return value.toString();
    }

    if (seen === undefined) {
      seen = new _polyfills._WeakSet();
    } else {
      if (seen.has(value)) return `[Circular]`;
    }

    seen.add(value);
    return valueIsArray ? inspectArray(value, depth + 1, seen) : inspectObject(value, depth + 1, seen);
  }

  function inspectKey(key) {
    return SAFE_KEY.test(key) ? key : stringify(key);
  }

  function inspectObject(obj, depth, seen) {
    if (depth > DEPTH_LIMIT) {
      return '[Object]';
    }

    let s = '{';
    let keys = objectKeys(obj);

    for (let i = 0; i < keys.length; i++) {
      s += i === 0 ? ' ' : ', ';

      if (i >= LIST_LIMIT) {
        s += `... ${keys.length - LIST_LIMIT} more keys`;
        break;
      }

      let key = keys[i];
      s += inspectKey(key) + ': ' + inspectValue(obj[key], depth, seen);
    }

    s += ' }';
    return s;
  }

  function inspectArray(arr, depth, seen) {
    if (depth > DEPTH_LIMIT) {
      return '[Array]';
    }

    let s = '[';

    for (let i = 0; i < arr.length; i++) {
      s += i === 0 ? ' ' : ', ';

      if (i >= LIST_LIMIT) {
        s += `... ${arr.length - LIST_LIMIT} more items`;
        break;
      }

      s += inspectValue(arr[i], depth, seen);
    }

    s += ' ]';
    return s;
  }

  function lookupDescriptor(obj, keyName) {
    let current = obj;

    do {
      let descriptor = Object.getOwnPropertyDescriptor(current, keyName);

      if (descriptor !== undefined) {
        return descriptor;
      }

      current = Object.getPrototypeOf(current);
    } while (current !== null);

    return null;
  }
  /**
    Checks to see if the `methodName` exists on the `obj`.
  
    ```javascript
    let foo = { bar: function() { return 'bar'; }, baz: null };
  
    Ember.canInvoke(foo, 'bar'); // true
    Ember.canInvoke(foo, 'baz'); // false
    Ember.canInvoke(foo, 'bat'); // false
    ```
  
    @method canInvoke
    @for Ember
    @param {Object} obj The object to check for the method
    @param {String} methodName The method name to check for
    @return {Boolean}
    @private
  */


  function canInvoke(obj, methodName) {
    return obj !== null && obj !== undefined && typeof obj[methodName] === 'function';
  }
  /**
    @module @ember/utils
  */

  /**
    Checks to see if the `methodName` exists on the `obj`,
    and if it does, invokes it with the arguments passed.
  
    ```javascript
    import { tryInvoke } from '@ember/utils';
  
    let d = new Date('03/15/2013');
  
    tryInvoke(d, 'getTime');              // 1363320000000
    tryInvoke(d, 'setFullYear', [2014]);  // 1394856000000
    tryInvoke(d, 'noSuchMethod', [2014]); // undefined
    ```
  
    @method tryInvoke
    @for @ember/utils
    @static
    @param {Object} obj The object to check for the method
    @param {String} methodName The method name to check for
    @param {Array} [args] The arguments to pass to the method
    @return {*} the return value of the invoked method or undefined if it cannot be invoked
    @public
  */


  function tryInvoke(obj, methodName, args) {
    if (canInvoke(obj, methodName)) {
      let method = obj[methodName];
      return method.apply(obj, args);
    }
  }

  const {
    isArray: isArray$1
  } = Array;

  function makeArray(obj) {
    if (obj === null || obj === undefined) {
      return [];
    }

    return isArray$1(obj) ? obj : [obj];
  }

  const NAMES = new WeakMap();

  function setName(obj, name) {
    if (isObject(obj)) NAMES.set(obj, name);
  }

  function getName(obj) {
    return NAMES.get(obj);
  }

  const objectToString$1 = Object.prototype.toString;

  function isNone(obj) {
    return obj === null || obj === undefined;
  }
  /*
   A `toString` util function that supports objects without a `toString`
   method, e.g. an object created with `Object.create(null)`.
  */


  function toString(obj) {
    if (typeof obj === 'string') {
      return obj;
    }

    if (null === obj) return 'null';
    if (undefined === obj) return 'undefined';

    if (Array.isArray(obj)) {
      // Reimplement Array.prototype.join according to spec (22.1.3.13)
      // Changing ToString(element) with this safe version of ToString.
      let r = '';

      for (let k = 0; k < obj.length; k++) {
        if (k > 0) {
          r += ',';
        }

        if (!isNone(obj[k])) {
          r += toString(obj[k]);
        }
      }

      return r;
    }

    if (typeof obj.toString === 'function') {
      return obj.toString();
    }

    return objectToString$1.call(obj);
  }

  const HAS_NATIVE_SYMBOL = function () {
    if (typeof Symbol !== 'function') {
      return false;
    } // use `Object`'s `.toString` directly to prevent us from detecting
    // polyfills as native


    return Object.prototype.toString.call(Symbol()) === '[object Symbol]';
  }();

  _exports.HAS_NATIVE_SYMBOL = HAS_NATIVE_SYMBOL;
  const HAS_NATIVE_PROXY = typeof Proxy === 'function';
  _exports.HAS_NATIVE_PROXY = HAS_NATIVE_PROXY;
  const PROXIES = new _polyfills._WeakSet();

  function isProxy(object) {
    if (isObject(object)) {
      return PROXIES.has(object);
    }

    return false;
  }

  function setProxy(object) {
    if (isObject(object)) {
      PROXIES.add(object);
    }
  }

  class Cache {
    constructor(limit, func, store) {
      this.limit = limit;
      this.func = func;
      this.store = store;
      this.size = 0;
      this.misses = 0;
      this.hits = 0;
      this.store = store || new Map();
    }

    get(key) {
      if (this.store.has(key)) {
        this.hits++;
        return this.store.get(key);
      } else {
        this.misses++;
        return this.set(key, this.func(key));
      }
    }

    set(key, value) {
      if (this.limit > this.size) {
        this.size++;
        this.store.set(key, value);
      }

      return value;
    }

    purge() {
      this.store.clear();
      this.size = 0;
      this.hits = 0;
      this.misses = 0;
    }

  }
  /*
   This package will be eagerly parsed and should have no dependencies on external
   packages.
  
   It is intended to be used to share utility methods that will be needed
   by every Ember application (and is **not** a dumping ground of useful utilities).
  
   Utility methods that are needed in < 80% of cases should be placed
   elsewhere (so they can be lazily evaluated / parsed).
  */


  _exports.Cache = Cache;
  const NAME_KEY = symbol('NAME_KEY');
  _exports.NAME_KEY = NAME_KEY;
});
enifed("@ember/canary-features/index", ["exports", "@ember/-internals/environment", "@ember/polyfills"], function (_exports, _environment, _polyfills) {
  "use strict";

  _exports.isEnabled = isEnabled;
  _exports.EMBER_ROUTING_BUILD_ROUTEINFO_METADATA = _exports.EMBER_GLIMMER_ARRAY_HELPER = _exports.GLIMMER_MODIFIER_MANAGER = _exports.EMBER_GLIMMER_ANGLE_BRACKET_INVOCATION = _exports.GLIMMER_CUSTOM_COMPONENT_MANAGER = _exports.EMBER_METAL_TRACKED_PROPERTIES = _exports.EMBER_MODULE_UNIFICATION = _exports.EMBER_ENGINES_MOUNT_PARAMS = _exports.EMBER_ROUTING_ROUTER_SERVICE = _exports.EMBER_GLIMMER_NAMED_ARGUMENTS = _exports.EMBER_IMPROVED_INSTRUMENTATION = _exports.EMBER_LIBRARIES_ISREGISTERED = _exports.FEATURES = _exports.DEFAULT_FEATURES = void 0;

  /**
    Set `EmberENV.FEATURES` in your application's `config/environment.js` file
    to enable canary features in your application.
  
    See the [feature flag guide](https://guides.emberjs.com/release/configuring-ember/feature-flags/)
    for more details.
  
    @module @ember/canary-features
    @public
  */
  const DEFAULT_FEATURES = {
    EMBER_LIBRARIES_ISREGISTERED: false,
    EMBER_IMPROVED_INSTRUMENTATION: false,
    EMBER_GLIMMER_NAMED_ARGUMENTS: true,
    EMBER_ROUTING_ROUTER_SERVICE: true,
    EMBER_ENGINES_MOUNT_PARAMS: true,
    EMBER_MODULE_UNIFICATION: false,
    GLIMMER_CUSTOM_COMPONENT_MANAGER: true,
    GLIMMER_MODIFIER_MANAGER: true,
    EMBER_METAL_TRACKED_PROPERTIES: false,
    EMBER_GLIMMER_ANGLE_BRACKET_INVOCATION: true,
    EMBER_GLIMMER_ARRAY_HELPER: true,
    EMBER_ROUTING_BUILD_ROUTEINFO_METADATA: false
  };
  /**
    The hash of enabled Canary features. Add to this, any canary features
    before creating your application.
  
    @class FEATURES
    @static
    @since 1.1.0
    @public
  */

  _exports.DEFAULT_FEATURES = DEFAULT_FEATURES;
  const FEATURES = (0, _polyfills.assign)(DEFAULT_FEATURES, _environment.ENV.FEATURES);
  /**
    Determine whether the specified `feature` is enabled. Used by Ember's
    build tools to exclude experimental features from beta/stable builds.
  
    You can define the following configuration options:
  
    * `EmberENV.ENABLE_OPTIONAL_FEATURES` - enable any features that have not been explicitly
      enabled/disabled.
  
    @method isEnabled
    @param {String} feature The feature to check
    @return {Boolean}
    @since 1.1.0
    @public
  */

  _exports.FEATURES = FEATURES;

  function isEnabled(feature) {
    let featureValue = FEATURES[feature];

    if (featureValue === true || featureValue === false) {
      return featureValue;
    } else if (_environment.ENV.ENABLE_OPTIONAL_FEATURES) {
      return true;
    } else {
      return false;
    }
  }

  function featureValue(value) {
    if (_environment.ENV.ENABLE_OPTIONAL_FEATURES && value === null) {
      return true;
    }

    return value;
  }

  const EMBER_LIBRARIES_ISREGISTERED = featureValue(FEATURES.EMBER_LIBRARIES_ISREGISTERED);
  _exports.EMBER_LIBRARIES_ISREGISTERED = EMBER_LIBRARIES_ISREGISTERED;
  const EMBER_IMPROVED_INSTRUMENTATION = featureValue(FEATURES.EMBER_IMPROVED_INSTRUMENTATION);
  _exports.EMBER_IMPROVED_INSTRUMENTATION = EMBER_IMPROVED_INSTRUMENTATION;
  const EMBER_GLIMMER_NAMED_ARGUMENTS = featureValue(FEATURES.EMBER_GLIMMER_NAMED_ARGUMENTS);
  _exports.EMBER_GLIMMER_NAMED_ARGUMENTS = EMBER_GLIMMER_NAMED_ARGUMENTS;
  const EMBER_ROUTING_ROUTER_SERVICE = featureValue(FEATURES.EMBER_ROUTING_ROUTER_SERVICE);
  _exports.EMBER_ROUTING_ROUTER_SERVICE = EMBER_ROUTING_ROUTER_SERVICE;
  const EMBER_ENGINES_MOUNT_PARAMS = featureValue(FEATURES.EMBER_ENGINES_MOUNT_PARAMS);
  _exports.EMBER_ENGINES_MOUNT_PARAMS = EMBER_ENGINES_MOUNT_PARAMS;
  const EMBER_MODULE_UNIFICATION = featureValue(FEATURES.EMBER_MODULE_UNIFICATION);
  _exports.EMBER_MODULE_UNIFICATION = EMBER_MODULE_UNIFICATION;
  const EMBER_METAL_TRACKED_PROPERTIES = featureValue(FEATURES.EMBER_METAL_TRACKED_PROPERTIES);
  _exports.EMBER_METAL_TRACKED_PROPERTIES = EMBER_METAL_TRACKED_PROPERTIES;
  const GLIMMER_CUSTOM_COMPONENT_MANAGER = featureValue(FEATURES.GLIMMER_CUSTOM_COMPONENT_MANAGER);
  _exports.GLIMMER_CUSTOM_COMPONENT_MANAGER = GLIMMER_CUSTOM_COMPONENT_MANAGER;
  const EMBER_GLIMMER_ANGLE_BRACKET_INVOCATION = featureValue(FEATURES.EMBER_GLIMMER_ANGLE_BRACKET_INVOCATION);
  _exports.EMBER_GLIMMER_ANGLE_BRACKET_INVOCATION = EMBER_GLIMMER_ANGLE_BRACKET_INVOCATION;
  const GLIMMER_MODIFIER_MANAGER = featureValue(FEATURES.GLIMMER_MODIFIER_MANAGER);
  _exports.GLIMMER_MODIFIER_MANAGER = GLIMMER_MODIFIER_MANAGER;
  const EMBER_GLIMMER_ARRAY_HELPER = featureValue(FEATURES.EMBER_GLIMMER_ARRAY_HELPER);
  _exports.EMBER_GLIMMER_ARRAY_HELPER = EMBER_GLIMMER_ARRAY_HELPER;
  const EMBER_ROUTING_BUILD_ROUTEINFO_METADATA = featureValue(FEATURES.EMBER_ROUTING_BUILD_ROUTEINFO_METADATA);
  _exports.EMBER_ROUTING_BUILD_ROUTEINFO_METADATA = EMBER_ROUTING_BUILD_ROUTEINFO_METADATA;
});
enifed("@ember/debug/index", ["exports", "@ember/-internals/browser-environment", "@ember/error", "@ember/debug/lib/deprecate", "@ember/debug/lib/testing", "@ember/debug/lib/warn"], function (_exports, _browserEnvironment, _error, _deprecate2, _testing, _warn2) {
  "use strict";

  Object.defineProperty(_exports, "registerDeprecationHandler", {
    enumerable: true,
    get: function () {
      return _deprecate2.registerHandler;
    }
  });
  Object.defineProperty(_exports, "isTesting", {
    enumerable: true,
    get: function () {
      return _testing.isTesting;
    }
  });
  Object.defineProperty(_exports, "setTesting", {
    enumerable: true,
    get: function () {
      return _testing.setTesting;
    }
  });
  Object.defineProperty(_exports, "registerWarnHandler", {
    enumerable: true,
    get: function () {
      return _warn2.registerHandler;
    }
  });
  _exports._warnIfUsingStrippedFeatureFlags = _exports.getDebugFunction = _exports.setDebugFunction = _exports.deprecateFunc = _exports.runInDebug = _exports.debugFreeze = _exports.debugSeal = _exports.deprecate = _exports.debug = _exports.warn = _exports.info = _exports.assert = void 0;

  // These are the default production build versions:
  const noop = () => {};

  let assert = noop;
  _exports.assert = assert;
  let info = noop;
  _exports.info = info;
  let warn = noop;
  _exports.warn = warn;
  let debug = noop;
  _exports.debug = debug;
  let deprecate = noop;
  _exports.deprecate = deprecate;
  let debugSeal = noop;
  _exports.debugSeal = debugSeal;
  let debugFreeze = noop;
  _exports.debugFreeze = debugFreeze;
  let runInDebug = noop;
  _exports.runInDebug = runInDebug;
  let setDebugFunction = noop;
  _exports.setDebugFunction = setDebugFunction;
  let getDebugFunction = noop;
  _exports.getDebugFunction = getDebugFunction;

  let deprecateFunc = function () {
    return arguments[arguments.length - 1];
  };

  _exports.deprecateFunc = deprecateFunc;

  if (true
  /* DEBUG */
  ) {
      _exports.setDebugFunction = setDebugFunction = function (type, callback) {
        switch (type) {
          case 'assert':
            return _exports.assert = assert = callback;

          case 'info':
            return _exports.info = info = callback;

          case 'warn':
            return _exports.warn = warn = callback;

          case 'debug':
            return _exports.debug = debug = callback;

          case 'deprecate':
            return _exports.deprecate = deprecate = callback;

          case 'debugSeal':
            return _exports.debugSeal = debugSeal = callback;

          case 'debugFreeze':
            return _exports.debugFreeze = debugFreeze = callback;

          case 'runInDebug':
            return _exports.runInDebug = runInDebug = callback;

          case 'deprecateFunc':
            return _exports.deprecateFunc = deprecateFunc = callback;
        }
      };

      _exports.getDebugFunction = getDebugFunction = function (type) {
        switch (type) {
          case 'assert':
            return assert;

          case 'info':
            return info;

          case 'warn':
            return warn;

          case 'debug':
            return debug;

          case 'deprecate':
            return deprecate;

          case 'debugSeal':
            return debugSeal;

          case 'debugFreeze':
            return debugFreeze;

          case 'runInDebug':
            return runInDebug;

          case 'deprecateFunc':
            return deprecateFunc;
        }
      };
    }
  /**
  @module @ember/debug
  */


  if (true
  /* DEBUG */
  ) {
      /**
        Verify that a certain expectation is met, or throw a exception otherwise.
           This is useful for communicating assumptions in the code to other human
        readers as well as catching bugs that accidentally violates these
        expectations.
           Assertions are removed from production builds, so they can be freely added
        for documentation and debugging purposes without worries of incuring any
        performance penalty. However, because of that, they should not be used for
        checks that could reasonably fail during normal usage. Furthermore, care
        should be taken to avoid accidentally relying on side-effects produced from
        evaluating the condition itself, since the code will not run in production.
           ```javascript
        import { assert } from '@ember/debug';
           // Test for truthiness
        assert('Must pass a string', typeof str === 'string');
           // Fail unconditionally
        assert('This code path should never be run');
        ```
           @method assert
        @static
        @for @ember/debug
        @param {String} description Describes the expectation. This will become the
          text of the Error thrown if the assertion fails.
        @param {Boolean} condition Must be truthy for the assertion to pass. If
          falsy, an exception will be thrown.
        @public
        @since 1.0.0
      */
      setDebugFunction('assert', function assert(desc, test) {
        if (!test) {
          throw new _error.default(`Assertion Failed: ${desc}`);
        }
      });
      /**
        Display a debug notice.
           Calls to this function are removed from production builds, so they can be
        freely added for documentation and debugging purposes without worries of
        incuring any performance penalty.
           ```javascript
        import { debug } from '@ember/debug';
           debug('I\'m a debug notice!');
        ```
           @method debug
        @for @ember/debug
        @static
        @param {String} message A debug message to display.
        @public
      */

      setDebugFunction('debug', function debug(message) {
        /* eslint-disable no-console */
        if (console.debug) {
          console.debug(`DEBUG: ${message}`);
        } else {
          console.log(`DEBUG: ${message}`);
        }
        /* eslint-ensable no-console */

      });
      /**
        Display an info notice.
           Calls to this function are removed from production builds, so they can be
        freely added for documentation and debugging purposes without worries of
        incuring any performance penalty.
           @method info
        @private
      */

      setDebugFunction('info', function info() {
        console.info(...arguments);
        /* eslint-disable-line no-console */
      });
      /**
       @module @ember/application
       @public
      */

      /**
        Alias an old, deprecated method with its new counterpart.
           Display a deprecation warning with the provided message and a stack trace
        (Chrome and Firefox only) when the assigned method is called.
           Calls to this function are removed from production builds, so they can be
        freely added for documentation and debugging purposes without worries of
        incuring any performance penalty.
           ```javascript
        import { deprecateFunc } from '@ember/application/deprecations';
           Ember.oldMethod = deprecateFunc('Please use the new, updated method', options, Ember.newMethod);
        ```
           @method deprecateFunc
        @static
        @for @ember/application/deprecations
        @param {String} message A description of the deprecation.
        @param {Object} [options] The options object for `deprecate`.
        @param {Function} func The new function called to replace its deprecated counterpart.
        @return {Function} A new function that wraps the original function with a deprecation warning
        @private
      */

      setDebugFunction('deprecateFunc', function deprecateFunc(...args) {
        if (args.length === 3) {
          let [message, options, func] = args;
          return function (...args) {
            deprecate(message, false, options);
            return func.apply(this, args);
          };
        } else {
          let [message, func] = args;
          return function () {
            deprecate(message);
            return func.apply(this, arguments);
          };
        }
      });
      /**
       @module @ember/debug
       @public
      */

      /**
        Run a function meant for debugging.
           Calls to this function are removed from production builds, so they can be
        freely added for documentation and debugging purposes without worries of
        incuring any performance penalty.
           ```javascript
        import Component from '@ember/component';
        import { runInDebug } from '@ember/debug';
           runInDebug(() => {
          Component.reopen({
            didInsertElement() {
              console.log("I'm happy");
            }
          });
        });
        ```
           @method runInDebug
        @for @ember/debug
        @static
        @param {Function} func The function to be executed.
        @since 1.5.0
        @public
      */

      setDebugFunction('runInDebug', function runInDebug(func) {
        func();
      });
      setDebugFunction('debugSeal', function debugSeal(obj) {
        Object.seal(obj);
      });
      setDebugFunction('debugFreeze', function debugFreeze(obj) {
        Object.freeze(obj);
      });
      setDebugFunction('deprecate', _deprecate2.default);
      setDebugFunction('warn', _warn2.default);
    }

  let _warnIfUsingStrippedFeatureFlags;

  _exports._warnIfUsingStrippedFeatureFlags = _warnIfUsingStrippedFeatureFlags;

  if (true
  /* DEBUG */
  && !(0, _testing.isTesting)()) {
    if (typeof window !== 'undefined' && (_browserEnvironment.isFirefox || _browserEnvironment.isChrome) && window.addEventListener) {
      window.addEventListener('load', () => {
        if (document.documentElement && document.documentElement.dataset && !document.documentElement.dataset.emberExtension) {
          let downloadURL;

          if (_browserEnvironment.isChrome) {
            downloadURL = 'https://chrome.google.com/webstore/detail/ember-inspector/bmdblncegkenkacieihfhpjfppoconhi';
          } else if (_browserEnvironment.isFirefox) {
            downloadURL = 'https://addons.mozilla.org/en-US/firefox/addon/ember-inspector/';
          }

          debug(`For more advanced debugging, install the Ember Inspector from ${downloadURL}`);
        }
      }, false);
    }
  }
});
enifed("@ember/debug/lib/deprecate", ["exports", "@ember/-internals/environment", "@ember/debug/index", "@ember/debug/lib/handlers"], function (_exports, _environment, _index, _handlers) {
  "use strict";

  _exports.missingOptionsUntilDeprecation = _exports.missingOptionsIdDeprecation = _exports.missingOptionsDeprecation = _exports.registerHandler = _exports.default = void 0;

  /**
   @module @ember/debug
   @public
  */

  /**
    Allows for runtime registration of handler functions that override the default deprecation behavior.
    Deprecations are invoked by calls to [@ember/application/deprecations/deprecate](https://emberjs.com/api/ember/release/classes/@ember%2Fapplication%2Fdeprecations/methods/deprecate?anchor=deprecate).
    The following example demonstrates its usage by registering a handler that throws an error if the
    message contains the word "should", otherwise defers to the default handler.
  
    ```javascript
    import { registerDeprecationHandler } from '@ember/debug';
  
    registerDeprecationHandler((message, options, next) => {
      if (message.indexOf('should') !== -1) {
        throw new Error(`Deprecation message with should: ${message}`);
      } else {
        // defer to whatever handler was registered before this one
        next(message, options);
      }
    });
    ```
  
    The handler function takes the following arguments:
  
    <ul>
      <li> <code>message</code> - The message received from the deprecation call.</li>
      <li> <code>options</code> - An object passed in with the deprecation call containing additional information including:</li>
        <ul>
          <li> <code>id</code> - An id of the deprecation in the form of <code>package-name.specific-deprecation</code>.</li>
          <li> <code>until</code> - The Ember version number the feature and deprecation will be removed in.</li>
        </ul>
      <li> <code>next</code> - A function that calls into the previously registered handler.</li>
    </ul>
  
    @public
    @static
    @method registerDeprecationHandler
    @for @ember/debug
    @param handler {Function} A function to handle deprecation calls.
    @since 2.1.0
  */
  let registerHandler = () => {};

  _exports.registerHandler = registerHandler;
  let missingOptionsDeprecation;
  _exports.missingOptionsDeprecation = missingOptionsDeprecation;
  let missingOptionsIdDeprecation;
  _exports.missingOptionsIdDeprecation = missingOptionsIdDeprecation;
  let missingOptionsUntilDeprecation;
  _exports.missingOptionsUntilDeprecation = missingOptionsUntilDeprecation;

  let deprecate = () => {};

  if (true
  /* DEBUG */
  ) {
      _exports.registerHandler = registerHandler = function registerHandler(handler) {
        (0, _handlers.registerHandler)('deprecate', handler);
      };

      let formatMessage = function formatMessage(_message, options) {
        let message = _message;

        if (options && options.id) {
          message = message + ` [deprecation id: ${options.id}]`;
        }

        if (options && options.url) {
          message += ` See ${options.url} for more details.`;
        }

        return message;
      };

      registerHandler(function logDeprecationToConsole(message, options) {
        let updatedMessage = formatMessage(message, options);
        console.warn(`DEPRECATION: ${updatedMessage}`); // eslint-disable-line no-console
      });
      let captureErrorForStack;

      if (new Error().stack) {
        captureErrorForStack = () => new Error();
      } else {
        captureErrorForStack = () => {
          try {
            __fail__.fail();
          } catch (e) {
            return e;
          }
        };
      }

      registerHandler(function logDeprecationStackTrace(message, options, next) {
        if (_environment.ENV.LOG_STACKTRACE_ON_DEPRECATION) {
          let stackStr = '';
          let error = captureErrorForStack();
          let stack;

          if (error.stack) {
            if (error['arguments']) {
              // Chrome
              stack = error.stack.replace(/^\s+at\s+/gm, '').replace(/^([^\(]+?)([\n$])/gm, '{anonymous}($1)$2').replace(/^Object.<anonymous>\s*\(([^\)]+)\)/gm, '{anonymous}($1)').split('\n');
              stack.shift();
            } else {
              // Firefox
              stack = error.stack.replace(/(?:\n@:0)?\s+$/m, '').replace(/^\(/gm, '{anonymous}(').split('\n');
            }

            stackStr = `\n    ${stack.slice(2).join('\n    ')}`;
          }

          let updatedMessage = formatMessage(message, options);
          console.warn(`DEPRECATION: ${updatedMessage}${stackStr}`); // eslint-disable-line no-console
        } else {
          next(message, options);
        }
      });
      registerHandler(function raiseOnDeprecation(message, options, next) {
        if (_environment.ENV.RAISE_ON_DEPRECATION) {
          let updatedMessage = formatMessage(message);
          throw new Error(updatedMessage);
        } else {
          next(message, options);
        }
      });
      _exports.missingOptionsDeprecation = missingOptionsDeprecation = 'When calling `deprecate` you ' + 'must provide an `options` hash as the third parameter.  ' + '`options` should include `id` and `until` properties.';
      _exports.missingOptionsIdDeprecation = missingOptionsIdDeprecation = 'When calling `deprecate` you must provide `id` in options.';
      _exports.missingOptionsUntilDeprecation = missingOptionsUntilDeprecation = 'When calling `deprecate` you must provide `until` in options.';
      /**
       @module @ember/application
       @public
       */

      /**
        Display a deprecation warning with the provided message and a stack trace
        (Chrome and Firefox only).
           * In a production build, this method is defined as an empty function (NOP).
        Uses of this method in Ember itself are stripped from the ember.prod.js build.
           @method deprecate
        @for @ember/application/deprecations
        @param {String} message A description of the deprecation.
        @param {Boolean} test A boolean. If falsy, the deprecation will be displayed.
        @param {Object} options
        @param {String} options.id A unique id for this deprecation. The id can be
          used by Ember debugging tools to change the behavior (raise, log or silence)
          for that specific deprecation. The id should be namespaced by dots, e.g.
          "view.helper.select".
        @param {string} options.until The version of Ember when this deprecation
          warning will be removed.
        @param {String} [options.url] An optional url to the transition guide on the
          emberjs.com website.
        @static
        @public
        @since 1.0.0
      */

      deprecate = function deprecate(message, test, options) {
        (0, _index.assert)(missingOptionsDeprecation, Boolean(options && (options.id || options.until)));
        (0, _index.assert)(missingOptionsIdDeprecation, Boolean(options.id));
        (0, _index.assert)(missingOptionsUntilDeprecation, Boolean(options.until));
        (0, _handlers.invoke)('deprecate', message, test, options);
      };
    }

  var _default = deprecate;
  _exports.default = _default;
});
enifed("@ember/debug/lib/handlers", ["exports"], function (_exports) {
  "use strict";

  _exports.invoke = _exports.registerHandler = _exports.HANDLERS = void 0;
  let HANDLERS = {};
  _exports.HANDLERS = HANDLERS;

  let registerHandler = () => {};

  _exports.registerHandler = registerHandler;

  let invoke = () => {};

  _exports.invoke = invoke;

  if (true
  /* DEBUG */
  ) {
      _exports.registerHandler = registerHandler = function registerHandler(type, callback) {
        let nextHandler = HANDLERS[type] || (() => {});

        HANDLERS[type] = (message, options) => {
          callback(message, options, nextHandler);
        };
      };

      _exports.invoke = invoke = function invoke(type, message, test, options) {
        if (test) {
          return;
        }

        let handlerForType = HANDLERS[type];

        if (handlerForType) {
          handlerForType(message, options);
        }
      };
    }
});
enifed("@ember/debug/lib/testing", ["exports"], function (_exports) {
  "use strict";

  _exports.isTesting = isTesting;
  _exports.setTesting = setTesting;
  let testing = false;

  function isTesting() {
    return testing;
  }

  function setTesting(value) {
    testing = Boolean(value);
  }
});
enifed("@ember/debug/lib/warn", ["exports", "@ember/debug/index", "@ember/debug/lib/handlers"], function (_exports, _index, _handlers) {
  "use strict";

  _exports.missingOptionsDeprecation = _exports.missingOptionsIdDeprecation = _exports.registerHandler = _exports.default = void 0;

  let registerHandler = () => {};

  _exports.registerHandler = registerHandler;

  let warn = () => {};

  let missingOptionsDeprecation;
  _exports.missingOptionsDeprecation = missingOptionsDeprecation;
  let missingOptionsIdDeprecation;
  /**
  @module @ember/debug
  */

  _exports.missingOptionsIdDeprecation = missingOptionsIdDeprecation;

  if (true
  /* DEBUG */
  ) {
      /**
        Allows for runtime registration of handler functions that override the default warning behavior.
        Warnings are invoked by calls made to [@ember/debug/warn](https://emberjs.com/api/ember/release/classes/@ember%2Fdebug/methods/warn?anchor=warn).
        The following example demonstrates its usage by registering a handler that does nothing overriding Ember's
        default warning behavior.
           ```javascript
        import { registerWarnHandler } from '@ember/debug';
           // next is not called, so no warnings get the default behavior
        registerWarnHandler(() => {});
        ```
           The handler function takes the following arguments:
           <ul>
          <li> <code>message</code> - The message received from the warn call. </li>
          <li> <code>options</code> - An object passed in with the warn call containing additional information including:</li>
            <ul>
              <li> <code>id</code> - An id of the warning in the form of <code>package-name.specific-warning</code>.</li>
            </ul>
          <li> <code>next</code> - A function that calls into the previously registered handler.</li>
        </ul>
           @public
        @static
        @method registerWarnHandler
        @for @ember/debug
        @param handler {Function} A function to handle warnings.
        @since 2.1.0
      */
      _exports.registerHandler = registerHandler = function registerHandler(handler) {
        (0, _handlers.registerHandler)('warn', handler);
      };

      registerHandler(function logWarning(message) {
        /* eslint-disable no-console */
        console.warn(`WARNING: ${message}`);
        /* eslint-enable no-console */
      });
      _exports.missingOptionsDeprecation = missingOptionsDeprecation = 'When calling `warn` you ' + 'must provide an `options` hash as the third parameter.  ' + '`options` should include an `id` property.';
      _exports.missingOptionsIdDeprecation = missingOptionsIdDeprecation = 'When calling `warn` you must provide `id` in options.';
      /**
        Display a warning with the provided message.
           * In a production build, this method is defined as an empty function (NOP).
        Uses of this method in Ember itself are stripped from the ember.prod.js build.
           ```javascript
        import { warn } from '@ember/debug';
        import tomsterCount from './tomster-counter'; // a module in my project
           // Log a warning if we have more than 3 tomsters
        warn('Too many tomsters!', tomsterCount <= 3, {
          id: 'ember-debug.too-many-tomsters'
        });
        ```
           @method warn
        @for @ember/debug
        @static
        @param {String} message A warning to display.
        @param {Boolean} test An optional boolean. If falsy, the warning
          will be displayed.
        @param {Object} options An object that can be used to pass a unique
          `id` for this warning.  The `id` can be used by Ember debugging tools
          to change the behavior (raise, log, or silence) for that specific warning.
          The `id` should be namespaced by dots, e.g. "ember-debug.feature-flag-with-features-stripped"
        @public
        @since 1.0.0
      */

      warn = function warn(message, test, options) {
        if (arguments.length === 2 && typeof test === 'object') {
          options = test;
          test = false;
        }

        (0, _index.assert)(missingOptionsDeprecation, Boolean(options));
        (0, _index.assert)(missingOptionsIdDeprecation, Boolean(options && options.id));
        (0, _handlers.invoke)('warn', message, test, options);
      };
    }

  var _default = warn;
  _exports.default = _default;
});
enifed("@ember/deprecated-features/index", ["exports"], function (_exports) {
  "use strict";

  _exports.APP_CTRL_ROUTER_PROPS = _exports.ALIAS_METHOD = _exports.JQUERY_INTEGRATION = _exports.COMPONENT_MANAGER_STRING_LOOKUP = _exports.TRANSITION_STATE = _exports.ROUTER_EVENTS = _exports.HANDLER_INFOS = _exports.MERGE = _exports.LOGGER = _exports.RUN_SYNC = _exports.EMBER_EXTEND_PROTOTYPES = _exports.SEND_ACTION = void 0;

  /* eslint-disable no-implicit-coercion */
  // These versions should be the version that the deprecation was _introduced_,
  // not the version that the feature will be removed.
  const SEND_ACTION = !!'3.4.0';
  _exports.SEND_ACTION = SEND_ACTION;
  const EMBER_EXTEND_PROTOTYPES = !!'3.2.0-beta.5';
  _exports.EMBER_EXTEND_PROTOTYPES = EMBER_EXTEND_PROTOTYPES;
  const RUN_SYNC = !!'3.0.0-beta.4';
  _exports.RUN_SYNC = RUN_SYNC;
  const LOGGER = !!'3.2.0-beta.1';
  _exports.LOGGER = LOGGER;
  const MERGE = !!'3.6.0-beta.1';
  _exports.MERGE = MERGE;
  const HANDLER_INFOS = !!'3.9.0';
  _exports.HANDLER_INFOS = HANDLER_INFOS;
  const ROUTER_EVENTS = !!'3.9.0';
  _exports.ROUTER_EVENTS = ROUTER_EVENTS;
  const TRANSITION_STATE = !!'3.9.0';
  _exports.TRANSITION_STATE = TRANSITION_STATE;
  const COMPONENT_MANAGER_STRING_LOOKUP = !!'3.8.0';
  _exports.COMPONENT_MANAGER_STRING_LOOKUP = COMPONENT_MANAGER_STRING_LOOKUP;
  const JQUERY_INTEGRATION = !!'3.9.0';
  _exports.JQUERY_INTEGRATION = JQUERY_INTEGRATION;
  const ALIAS_METHOD = !!'3.9.0';
  _exports.ALIAS_METHOD = ALIAS_METHOD;
  const APP_CTRL_ROUTER_PROPS = !!'4.0.0';
  _exports.APP_CTRL_ROUTER_PROPS = APP_CTRL_ROUTER_PROPS;
});
enifed("@ember/error/index", ["exports"], function (_exports) {
  "use strict";

  _exports.default = void 0;

  /**
   @module @ember/error
  */

  /**
    The JavaScript Error object used by Ember.assert.
  
    @class Error
    @namespace Ember
    @extends Error
    @constructor
    @public
  */
  var _default = Error;
  _exports.default = _default;
});
enifed("@ember/polyfills/index", ["exports", "@ember/deprecated-features", "@ember/polyfills/lib/merge", "@ember/polyfills/lib/assign", "@ember/polyfills/lib/weak_set"], function (_exports, _deprecatedFeatures, _merge, _assign, _weak_set) {
  "use strict";

  Object.defineProperty(_exports, "assign", {
    enumerable: true,
    get: function () {
      return _assign.default;
    }
  });
  Object.defineProperty(_exports, "assignPolyfill", {
    enumerable: true,
    get: function () {
      return _assign.assign;
    }
  });
  Object.defineProperty(_exports, "_WeakSet", {
    enumerable: true,
    get: function () {
      return _weak_set.default;
    }
  });
  _exports.merge = void 0;
  let merge = _deprecatedFeatures.MERGE ? _merge.default : undefined; // Export `assignPolyfill` for testing

  _exports.merge = merge;
});
enifed("@ember/polyfills/lib/assign", ["exports"], function (_exports) {
  "use strict";

  _exports.assign = assign;
  _exports.default = void 0;

  /**
   @module @ember/polyfills
  */

  /**
    Copy properties from a source object to a target object. Source arguments remain unchanged.
  
    ```javascript
    import { assign } from '@ember/polyfills';
  
    var a = { first: 'Yehuda' };
    var b = { last: 'Katz' };
    var c = { company: 'Other Company' };
    var d = { company: 'Tilde Inc.' };
    assign(a, b, c, d); // a === { first: 'Yehuda', last: 'Katz', company: 'Tilde Inc.' };
    ```
  
    @method assign
    @for @ember/polyfills
    @param {Object} target The object to assign into
    @param {Object} ...args The objects to copy properties from
    @return {Object}
    @public
    @static
  */
  function assign(target) {
    for (let i = 1; i < arguments.length; i++) {
      let arg = arguments[i];

      if (!arg) {
        continue;
      }

      let updates = Object.keys(arg);

      for (let i = 0; i < updates.length; i++) {
        let prop = updates[i];
        target[prop] = arg[prop];
      }
    }

    return target;
  } // Note: We use the bracket notation so
  //       that the babel plugin does not
  //       transform it.
  // https://www.npmjs.com/package/babel-plugin-transform-object-assign


  const {
    assign: _assign
  } = Object;

  var _default = _assign || assign;

  _exports.default = _default;
});
enifed("@ember/polyfills/lib/merge", ["exports", "@ember/debug"], function (_exports, _debug) {
  "use strict";

  _exports.default = merge;

  /**
    Merge the contents of two objects together into the first object.
  
    ```javascript
    import { merge } from '@ember/polyfills';
  
    merge({ first: 'Tom' }, { last: 'Dale' }); // { first: 'Tom', last: 'Dale' }
    var a = { first: 'Yehuda' };
    var b = { last: 'Katz' };
    merge(a, b); // a == { first: 'Yehuda', last: 'Katz' }, b == { last: 'Katz' }
    ```
  
    @method merge
    @static
    @for @ember/polyfills
    @param {Object} original The object to merge into
    @param {Object} updates The object to copy properties from
    @return {Object}
    @deprecated
    @public
  */
  function merge(original, updates) {
    true && !false && (0, _debug.deprecate)('Use of `merge` has been deprecated. Please use `assign` instead.', false, {
      id: 'ember-polyfills.deprecate-merge',
      until: '4.0.0',
      url: 'https://emberjs.com/deprecations/v3.x/#toc_ember-polyfills-deprecate-merge'
    });

    if (updates === null || typeof updates !== 'object') {
      return original;
    }

    let props = Object.keys(updates);
    let prop;

    for (let i = 0; i < props.length; i++) {
      prop = props[i];
      original[prop] = updates[prop];
    }

    return original;
  }
});
enifed("@ember/polyfills/lib/weak_set", ["exports"], function (_exports) {
  "use strict";

  _exports.default = void 0;

  /* globals WeakSet */
  var _default = typeof WeakSet === 'function' ? WeakSet : class WeakSetPolyFill {
    constructor() {
      this._map = new WeakMap();
    }

    add(val) {
      this._map.set(val, true);

      return this;
    }

    delete(val) {
      return this._map.delete(val);
    }

    has(val) {
      return this._map.has(val);
    }

  };

  _exports.default = _default;
});
enifed("@glimmer/compiler", ["exports", "node-module", "@glimmer/util", "@glimmer/wire-format", "@glimmer/syntax"], function (_exports, _nodeModule, _util, _wireFormat, _syntax) {
  "use strict";

  _exports.precompile = precompile;
  _exports.TemplateVisitor = _exports.TemplateCompiler = _exports.defaultId = void 0;

  class SymbolTable {
    static top() {
      return new ProgramSymbolTable();
    }

    child(locals) {
      let symbols = locals.map(name => this.allocate(name));
      return new BlockSymbolTable(this, locals, symbols);
    }

  }

  class ProgramSymbolTable extends SymbolTable {
    constructor() {
      super(...arguments);
      this.symbols = [];
      this.size = 1;
      this.named = (0, _util.dict)();
      this.blocks = (0, _util.dict)();
    }

    has(_name) {
      return false;
    }

    get(_name) {
      throw (0, _util.unreachable)();
    }

    getLocalsMap() {
      return {};
    }

    getEvalInfo() {
      return [];
    }

    allocateNamed(name) {
      let named = this.named[name];

      if (!named) {
        named = this.named[name] = this.allocate(name);
      }

      return named;
    }

    allocateBlock(name) {
      let block = this.blocks[name];

      if (!block) {
        block = this.blocks[name] = this.allocate(`&${name}`);
      }

      return block;
    }

    allocate(identifier) {
      this.symbols.push(identifier);
      return this.size++;
    }

  }

  class BlockSymbolTable extends SymbolTable {
    constructor(parent, symbols, slots) {
      super();
      this.parent = parent;
      this.symbols = symbols;
      this.slots = slots;
    }

    has(name) {
      return this.symbols.indexOf(name) !== -1 || this.parent.has(name);
    }

    get(name) {
      let slot = this.symbols.indexOf(name);
      return slot === -1 ? this.parent.get(name) : this.slots[slot];
    }

    getLocalsMap() {
      let dict$$1 = this.parent.getLocalsMap();
      this.symbols.forEach(symbol => dict$$1[symbol] = this.get(symbol));
      return dict$$1;
    }

    getEvalInfo() {
      let locals = this.getLocalsMap();
      return Object.keys(locals).map(symbol => locals[symbol]);
    }

    allocateNamed(name) {
      return this.parent.allocateNamed(name);
    }

    allocateBlock(name) {
      return this.parent.allocateBlock(name);
    }

    allocate(identifier) {
      return this.parent.allocate(identifier);
    }

  }
  /**
   * Takes in an AST and outputs a list of actions to be consumed
   * by a compiler. For example, the template
   *
   *     foo{{bar}}<div>baz</div>
   *
   * produces the actions
   *
   *     [['startProgram', [programNode, 0]],
   *      ['text', [textNode, 0, 3]],
   *      ['mustache', [mustacheNode, 1, 3]],
   *      ['openElement', [elementNode, 2, 3, 0]],
   *      ['text', [textNode, 0, 1]],
   *      ['closeElement', [elementNode, 2, 3],
   *      ['endProgram', [programNode]]]
   *
   * This visitor walks the AST depth first and backwards. As
   * a result the bottom-most child template will appear at the
   * top of the actions list whereas the root template will appear
   * at the bottom of the list. For example,
   *
   *     <div>{{#if}}foo{{else}}bar<b></b>{{/if}}</div>
   *
   * produces the actions
   *
   *     [['startProgram', [programNode, 0]],
   *      ['text', [textNode, 0, 2, 0]],
   *      ['openElement', [elementNode, 1, 2, 0]],
   *      ['closeElement', [elementNode, 1, 2]],
   *      ['endProgram', [programNode]],
   *      ['startProgram', [programNode, 0]],
   *      ['text', [textNode, 0, 1]],
   *      ['endProgram', [programNode]],
   *      ['startProgram', [programNode, 2]],
   *      ['openElement', [elementNode, 0, 1, 1]],
   *      ['block', [blockNode, 0, 1]],
   *      ['closeElement', [elementNode, 0, 1]],
   *      ['endProgram', [programNode]]]
   *
   * The state of the traversal is maintained by a stack of frames.
   * Whenever a node with children is entered (either a ProgramNode
   * or an ElementNode) a frame is pushed onto the stack. The frame
   * contains information about the state of the traversal of that
   * node. For example,
   *
   *   - index of the current child node being visited
   *   - the number of mustaches contained within its child nodes
   *   - the list of actions generated by its child nodes
   */


  class Frame {
    constructor() {
      this.parentNode = null;
      this.children = null;
      this.childIndex = null;
      this.childCount = null;
      this.childTemplateCount = 0;
      this.mustacheCount = 0;
      this.actions = [];
      this.blankChildTextNodes = null;
      this.symbols = null;
    }

  }

  class TemplateVisitor {
    constructor() {
      this.frameStack = [];
      this.actions = [];
      this.programDepth = -1;
    }

    visit(node) {
      this[node.type](node);
    } // Traversal methods


    Program(program) {
      this.programDepth++;
      let parentFrame = this.getCurrentFrame();
      let programFrame = this.pushFrame();

      if (!parentFrame) {
        program['symbols'] = SymbolTable.top();
      } else {
        program['symbols'] = parentFrame.symbols.child(program.blockParams);
      }

      let startType, endType;

      if (this.programDepth === 0) {
        startType = 'startProgram';
        endType = 'endProgram';
      } else {
        startType = 'startBlock';
        endType = 'endBlock';
      }

      programFrame.parentNode = program;
      programFrame.children = program.body;
      programFrame.childCount = program.body.length;
      programFrame.blankChildTextNodes = [];
      programFrame.actions.push([endType, [program, this.programDepth]]);
      programFrame.symbols = program['symbols'];

      for (let i = program.body.length - 1; i >= 0; i--) {
        programFrame.childIndex = i;
        this.visit(program.body[i]);
      }

      programFrame.actions.push([startType, [program, programFrame.childTemplateCount, programFrame.blankChildTextNodes.reverse()]]);
      this.popFrame();
      this.programDepth--; // Push the completed template into the global actions list

      if (parentFrame) {
        parentFrame.childTemplateCount++;
      }

      this.actions.push(...programFrame.actions.reverse());
    }

    ElementNode(element) {
      let parentFrame = this.currentFrame;
      let elementFrame = this.pushFrame();
      elementFrame.parentNode = element;
      elementFrame.children = element.children;
      elementFrame.childCount = element.children.length;
      elementFrame.mustacheCount += element.modifiers.length;
      elementFrame.blankChildTextNodes = [];
      elementFrame.symbols = element['symbols'] = parentFrame.symbols.child(element.blockParams);
      let actionArgs = [element, parentFrame.childIndex, parentFrame.childCount];
      elementFrame.actions.push(['closeElement', actionArgs]);

      for (let i = element.attributes.length - 1; i >= 0; i--) {
        this.visit(element.attributes[i]);
      }

      for (let i = element.children.length - 1; i >= 0; i--) {
        elementFrame.childIndex = i;
        this.visit(element.children[i]);
      }

      let open = ['openElement', [...actionArgs, elementFrame.mustacheCount, elementFrame.blankChildTextNodes.reverse()]];
      elementFrame.actions.push(open);
      this.popFrame(); // Propagate the element's frame state to the parent frame

      if (elementFrame.mustacheCount > 0) {
        parentFrame.mustacheCount++;
      }

      parentFrame.childTemplateCount += elementFrame.childTemplateCount;
      parentFrame.actions.push(...elementFrame.actions);
    }

    AttrNode(attr) {
      if (attr.value.type !== 'TextNode') {
        this.currentFrame.mustacheCount++;
      }
    }

    TextNode(text) {
      let frame = this.currentFrame;

      if (text.chars === '') {
        frame.blankChildTextNodes.push(domIndexOf(frame.children, text));
      }

      frame.actions.push(['text', [text, frame.childIndex, frame.childCount]]);
    }

    BlockStatement(node) {
      let frame = this.currentFrame;
      frame.mustacheCount++;
      frame.actions.push(['block', [node, frame.childIndex, frame.childCount]]);

      if (node.inverse) {
        this.visit(node.inverse);
      }

      if (node.program) {
        this.visit(node.program);
      }
    }

    PartialStatement(node) {
      let frame = this.currentFrame;
      frame.mustacheCount++;
      frame.actions.push(['mustache', [node, frame.childIndex, frame.childCount]]);
    }

    CommentStatement(text) {
      let frame = this.currentFrame;
      frame.actions.push(['comment', [text, frame.childIndex, frame.childCount]]);
    }

    MustacheCommentStatement() {// Intentional empty: Handlebars comments should not affect output.
    }

    MustacheStatement(mustache) {
      let frame = this.currentFrame;
      frame.mustacheCount++;
      frame.actions.push(['mustache', [mustache, frame.childIndex, frame.childCount]]);
    } // Frame helpers


    get currentFrame() {
      return this.getCurrentFrame();
    }

    getCurrentFrame() {
      return this.frameStack[this.frameStack.length - 1];
    }

    pushFrame() {
      let frame = new Frame();
      this.frameStack.push(frame);
      return frame;
    }

    popFrame() {
      return this.frameStack.pop();
    }

  } // Returns the index of `domNode` in the `nodes` array, skipping
  // over any nodes which do not represent DOM nodes.


  _exports.TemplateVisitor = TemplateVisitor;

  function domIndexOf(nodes, domNode) {
    let index = -1;

    for (let i = 0; i < nodes.length; i++) {
      let node = nodes[i];

      if (node.type !== 'TextNode' && node.type !== 'ElementNode') {
        continue;
      } else {
        index++;
      }

      if (node === domNode) {
        return index;
      }
    }

    return -1;
  }

  class Block {
    constructor() {
      this.statements = [];
    }

    push(statement) {
      this.statements.push(statement);
    }

  }

  class InlineBlock extends Block {
    constructor(table) {
      super();
      this.table = table;
    }

    toJSON() {
      return {
        statements: this.statements,
        parameters: this.table.slots
      };
    }

  }

  class TemplateBlock extends Block {
    constructor(symbolTable) {
      super();
      this.symbolTable = symbolTable;
      this.type = 'template';
      this.yields = new _util.DictSet();
      this.named = new _util.DictSet();
      this.blocks = [];
      this.hasEval = false;
    }

    push(statement) {
      this.statements.push(statement);
    }

    toJSON() {
      return {
        symbols: this.symbolTable.symbols,
        statements: this.statements,
        hasEval: this.hasEval
      };
    }

  }

  class ComponentBlock extends Block {
    constructor(tag, table, selfClosing) {
      super();
      this.tag = tag;
      this.table = table;
      this.selfClosing = selfClosing;
      this.attributes = [];
      this.arguments = [];
      this.inParams = true;
      this.positionals = [];
    }

    push(statement) {
      if (this.inParams) {
        if ((0, _wireFormat.isFlushElement)(statement)) {
          this.inParams = false;
        } else if ((0, _wireFormat.isArgument)(statement)) {
          this.arguments.push(statement);
        } else if ((0, _wireFormat.isAttribute)(statement)) {
          this.attributes.push(statement);
        } else {
          throw new Error('Compile Error: only parameters allowed before flush-element');
        }
      } else {
        this.statements.push(statement);
      }
    }

    toJSON() {
      let args = this.arguments;
      let keys = args.map(arg => arg[1]);
      let values = args.map(arg => arg[2]);
      let block = this.selfClosing ? null : {
        statements: this.statements,
        parameters: this.table.slots
      };
      return [this.tag, this.attributes, [keys, values], block];
    }

  }

  class Template {
    constructor(symbols) {
      this.block = new TemplateBlock(symbols);
    }

    toJSON() {
      return this.block.toJSON();
    }

  }

  class JavaScriptCompiler {
    constructor(opcodes, symbols, options) {
      this.blocks = new _util.Stack();
      this.values = [];
      this.opcodes = opcodes;
      this.template = new Template(symbols);
      this.options = options;
    }

    static process(opcodes, symbols, options) {
      let compiler = new JavaScriptCompiler(opcodes, symbols, options);
      return compiler.process();
    }

    get currentBlock() {
      return this.blocks.current;
    }

    process() {
      this.opcodes.forEach(op => {
        let opcode = op[0];
        let arg = op[1];

        if (!this[opcode]) {
          throw new Error(`unimplemented ${opcode} on JavaScriptCompiler`);
        }

        this[opcode](arg);
      });
      return this.template;
    } /// Nesting


    startBlock(program) {
      let block = new InlineBlock(program['symbols']);
      this.blocks.push(block);
    }

    endBlock() {
      let {
        template,
        blocks
      } = this;
      let block = blocks.pop();
      template.block.blocks.push(block.toJSON());
    }

    startProgram() {
      this.blocks.push(this.template.block);
    }

    endProgram() {} /// Statements


    text(content) {
      this.push([_wireFormat.Ops.Text, content]);
    }

    append(trusted) {
      this.push([_wireFormat.Ops.Append, this.popValue(), trusted]);
    }

    comment(value) {
      this.push([_wireFormat.Ops.Comment, value]);
    }

    modifier(name) {
      let params = this.popValue();
      let hash = this.popValue();
      this.push([_wireFormat.Ops.Modifier, name, params, hash]);
    }

    block([name, template, inverse]) {
      let params = this.popValue();
      let hash = this.popValue();
      let blocks = this.template.block.blocks;
      this.push([_wireFormat.Ops.Block, name, params, hash, blocks[template], blocks[inverse]]);
    }

    openComponent(element) {
      let tag = this.options && this.options.customizeComponentName ? this.options.customizeComponentName(element.tag) : element.tag;
      let component = new ComponentBlock(tag, element['symbols'], element.selfClosing);
      this.blocks.push(component);
    }

    openSplattedElement(element) {
      let tag = element.tag;

      if (element.blockParams.length > 0) {
        throw new Error(`Compile Error: <${element.tag}> is not a component and doesn't support block parameters`);
      } else {
        this.push([_wireFormat.Ops.OpenSplattedElement, tag]);
      }
    }

    openElement(element) {
      let tag = element.tag;

      if (element.blockParams.length > 0) {
        throw new Error(`Compile Error: <${element.tag}> is not a component and doesn't support block parameters`);
      } else {
        this.push([_wireFormat.Ops.OpenElement, tag]);
      }
    }

    flushElement() {
      this.push([_wireFormat.Ops.FlushElement]);
    }

    closeComponent(_element) {
      if (_element.modifiers.length > 0) {
        throw new Error('Compile Error: Element modifiers are not allowed in components');
      }

      let [tag, attrs, args, block] = this.endComponent();
      this.push([_wireFormat.Ops.Component, tag, attrs, args, block]);
    }

    closeDynamicComponent(_element) {
      let [, attrs, args, block] = this.endComponent();
      this.push([_wireFormat.Ops.DynamicComponent, this.popValue(), attrs, args, block]);
    }

    closeElement(_element) {
      this.push([_wireFormat.Ops.CloseElement]);
    }

    staticAttr([name, namespace]) {
      let value = this.popValue();
      this.push([_wireFormat.Ops.StaticAttr, name, value, namespace]);
    }

    dynamicAttr([name, namespace]) {
      let value = this.popValue();
      this.push([_wireFormat.Ops.DynamicAttr, name, value, namespace]);
    }

    componentAttr([name, namespace]) {
      let value = this.popValue();
      this.push([_wireFormat.Ops.ComponentAttr, name, value, namespace]);
    }

    trustingAttr([name, namespace]) {
      let value = this.popValue();
      this.push([_wireFormat.Ops.TrustingAttr, name, value, namespace]);
    }

    trustingComponentAttr([name, namespace]) {
      let value = this.popValue();
      this.push([_wireFormat.Ops.TrustingComponentAttr, name, value, namespace]);
    }

    staticArg(name) {
      let value = this.popValue();
      this.push([_wireFormat.Ops.StaticArg, name, value]);
    }

    dynamicArg(name) {
      let value = this.popValue();
      this.push([_wireFormat.Ops.DynamicArg, name, value]);
    }

    yield(to) {
      let params = this.popValue();
      this.push([_wireFormat.Ops.Yield, to, params]);
    }

    attrSplat(to) {
      // consume (and disregard) the value pushed for the
      // ...attributes attribute
      this.popValue();
      this.push([_wireFormat.Ops.AttrSplat, to]);
    }

    debugger(evalInfo) {
      this.push([_wireFormat.Ops.Debugger, evalInfo]);
      this.template.block.hasEval = true;
    }

    hasBlock(name) {
      this.pushValue([_wireFormat.Ops.HasBlock, name]);
    }

    hasBlockParams(name) {
      this.pushValue([_wireFormat.Ops.HasBlockParams, name]);
    }

    partial(evalInfo) {
      let params = this.popValue();
      this.push([_wireFormat.Ops.Partial, params[0], evalInfo]);
      this.template.block.hasEval = true;
    } /// Expressions


    literal(value) {
      if (value === undefined) {
        this.pushValue([_wireFormat.Ops.Undefined]);
      } else {
        this.pushValue(value);
      }
    }

    unknown(name) {
      this.pushValue([_wireFormat.Ops.Unknown, name]);
    }

    get([head, path]) {
      this.pushValue([_wireFormat.Ops.Get, head, path]);
    }

    maybeLocal(path) {
      this.pushValue([_wireFormat.Ops.MaybeLocal, path]);
    }

    concat() {
      this.pushValue([_wireFormat.Ops.Concat, this.popValue()]);
    }

    helper(name) {
      let params = this.popValue();
      let hash = this.popValue();
      this.pushValue([_wireFormat.Ops.Helper, name, params, hash]);
    } /// Stack Management Opcodes


    prepareArray(size) {
      let values = [];

      for (let i = 0; i < size; i++) {
        values.push(this.popValue());
      }

      this.pushValue(values);
    }

    prepareObject(size) {
      let keys = new Array(size);
      let values = new Array(size);

      for (let i = 0; i < size; i++) {
        keys[i] = this.popValue();
        values[i] = this.popValue();
      }

      this.pushValue([keys, values]);
    } /// Utilities


    endComponent() {
      let component = this.blocks.pop();
      return component.toJSON();
    }

    push(args) {
      while (args[args.length - 1] === null) {
        args.pop();
      }

      this.currentBlock.push(args);
    }

    pushValue(val) {
      this.values.push(val);
    }

    popValue() {
      return this.values.pop();
    }

  } // There is a small whitelist of namespaced attributes specially
  // enumerated in
  // https://www.w3.org/TR/html/syntax.html#attributes-0
  //
  // > When a foreign element has one of the namespaced attributes given by
  // > the local name and namespace of the first and second cells of a row
  // > from the following table, it must be written using the name given by
  // > the third cell from the same row.
  //
  // In all other cases, colons are interpreted as a regular character
  // with no special meaning:
  //
  // > No other namespaced attribute can be expressed in the HTML syntax.


  const XLINK = 'http://www.w3.org/1999/xlink';
  const XML = 'http://www.w3.org/XML/1998/namespace';
  const XMLNS = 'http://www.w3.org/2000/xmlns/';
  const WHITELIST = {
    'xlink:actuate': XLINK,
    'xlink:arcrole': XLINK,
    'xlink:href': XLINK,
    'xlink:role': XLINK,
    'xlink:show': XLINK,
    'xlink:title': XLINK,
    'xlink:type': XLINK,
    'xml:base': XML,
    'xml:lang': XML,
    'xml:space': XML,
    xmlns: XMLNS,
    'xmlns:xlink': XMLNS
  };

  function getAttrNamespace(attrName) {
    return WHITELIST[attrName] || null;
  }

  class SymbolAllocator {
    constructor(ops) {
      this.ops = ops;
      this.symbolStack = new _util.Stack();
    }

    process() {
      let out = [];
      let {
        ops
      } = this;

      for (let i = 0; i < ops.length; i++) {
        let op = ops[i];
        let result = this.dispatch(op);

        if (result === undefined) {
          out.push(op);
        } else {
          out.push(result);
        }
      }

      return out;
    }

    dispatch(op) {
      let name = op[0];
      let operand = op[1];
      return this[name](operand);
    }

    get symbols() {
      return this.symbolStack.current;
    }

    startProgram(op) {
      this.symbolStack.push(op['symbols']);
    }

    endProgram(_op) {
      this.symbolStack.pop();
    }

    startBlock(op) {
      this.symbolStack.push(op['symbols']);
    }

    endBlock(_op) {
      this.symbolStack.pop();
    }

    flushElement(op) {
      this.symbolStack.push(op['symbols']);
    }

    closeElement(_op) {
      this.symbolStack.pop();
    }

    closeComponent(_op) {
      this.symbolStack.pop();
    }

    closeDynamicComponent(_op) {
      this.symbolStack.pop();
    }

    attrSplat(_op) {
      return ['attrSplat', this.symbols.allocateBlock('attrs')];
    }

    get(op) {
      let [name, rest] = op;

      if (name === 0) {
        return ['get', [0, rest]];
      }

      if (isLocal(name, this.symbols)) {
        let head = this.symbols.get(name);
        return ['get', [head, rest]];
      } else if (name[0] === '@') {
        let head = this.symbols.allocateNamed(name);
        return ['get', [head, rest]];
      } else {
        return ['maybeLocal', [name, ...rest]];
      }
    }

    maybeGet(op) {
      let [name, rest] = op;

      if (name === 0) {
        return ['get', [0, rest]];
      }

      if (isLocal(name, this.symbols)) {
        let head = this.symbols.get(name);
        return ['get', [head, rest]];
      } else if (name[0] === '@') {
        let head = this.symbols.allocateNamed(name);
        return ['get', [head, rest]];
      } else if (rest.length === 0) {
        return ['unknown', name];
      } else {
        return ['maybeLocal', [name, ...rest]];
      }
    }

    yield(op) {
      if (op === 0) {
        throw new Error('Cannot yield to this');
      }

      return ['yield', this.symbols.allocateBlock(op)];
    }

    debugger(_op) {
      return ['debugger', this.symbols.getEvalInfo()];
    }

    hasBlock(op) {
      if (op === 0) {
        throw new Error('Cannot hasBlock this');
      }

      return ['hasBlock', this.symbols.allocateBlock(op)];
    }

    hasBlockParams(op) {
      if (op === 0) {
        throw new Error('Cannot hasBlockParams this');
      }

      return ['hasBlockParams', this.symbols.allocateBlock(op)];
    }

    partial(_op) {
      return ['partial', this.symbols.getEvalInfo()];
    }

    text(_op) {}

    comment(_op) {}

    openComponent(_op) {}

    openElement(_op) {}

    openSplattedElement(_op) {}

    staticArg(_op) {}

    dynamicArg(_op) {}

    staticAttr(_op) {}

    trustingAttr(_op) {}

    trustingComponentAttr(_op) {}

    dynamicAttr(_op) {}

    componentAttr(_op) {}

    modifier(_op) {}

    append(_op) {}

    block(_op) {}

    literal(_op) {}

    helper(_op) {}

    unknown(_op) {}

    maybeLocal(_op) {}

    prepareArray(_op) {}

    prepareObject(_op) {}

    concat(_op) {}

  }

  function isLocal(name, symbols) {
    return symbols && symbols.has(name);
  }

  function isTrustedValue(value) {
    return value.escaped !== undefined && !value.escaped;
  }

  class TemplateCompiler {
    constructor() {
      this.templateId = 0;
      this.templateIds = [];
      this.opcodes = [];
      this.includeMeta = false;
    }

    static compile(ast, options) {
      let templateVisitor = new TemplateVisitor();
      templateVisitor.visit(ast);
      let compiler = new TemplateCompiler();
      let opcodes = compiler.process(templateVisitor.actions);
      let symbols = new SymbolAllocator(opcodes).process();
      return JavaScriptCompiler.process(symbols, ast['symbols'], options);
    }

    process(actions) {
      actions.forEach(([name, ...args]) => {
        if (!this[name]) {
          throw new Error(`Unimplemented ${name} on TemplateCompiler`);
        }

        this[name](...args);
      });
      return this.opcodes;
    }

    startProgram([program]) {
      this.opcode(['startProgram', program], program);
    }

    endProgram() {
      this.opcode(['endProgram', null], null);
    }

    startBlock([program]) {
      this.templateId++;
      this.opcode(['startBlock', program], program);
    }

    endBlock() {
      this.templateIds.push(this.templateId - 1);
      this.opcode(['endBlock', null], null);
    }

    text([action]) {
      this.opcode(['text', action.chars], action);
    }

    comment([action]) {
      this.opcode(['comment', action.value], action);
    }

    openElement([action]) {
      let attributes = action.attributes;
      let hasSplat = false;

      for (let i = 0; i < attributes.length; i++) {
        let attr = attributes[i];

        if (attr.name === '...attributes') {
          hasSplat = true;
          break;
        }
      }

      let actionIsComponent = false;

      if (isDynamicComponent(action)) {
        let head, rest;
        [head, ...rest] = action.tag.split('.');

        if (head === 'this') {
          head = 0;
        }

        this.opcode(['get', [head, rest]]);
        this.opcode(['openComponent', action], action);
        actionIsComponent = true;
      } else if (isComponent(action)) {
        this.opcode(['openComponent', action], action);
        actionIsComponent = true;
      } else if (hasSplat) {
        this.opcode(['openSplattedElement', action], action);
      } else {
        this.opcode(['openElement', action], action);
      }

      let typeAttr = null;
      let attrs = action.attributes;

      for (let i = 0; i < attrs.length; i++) {
        if (attrs[i].name === 'type') {
          typeAttr = attrs[i];
          continue;
        }

        this.attribute([attrs[i]], hasSplat || actionIsComponent);
      }

      if (typeAttr) {
        this.attribute([typeAttr], hasSplat || actionIsComponent);
      }

      this.opcode(['flushElement', action], null);
    }

    closeElement([action]) {
      if (isDynamicComponent(action)) {
        this.opcode(['closeDynamicComponent', action], action);
      } else if (isComponent(action)) {
        this.opcode(['closeComponent', action], action);
      } else if (action.modifiers.length > 0) {
        for (let i = 0; i < action.modifiers.length; i++) {
          this.modifier([action.modifiers[i]]);
        }

        this.opcode(['closeElement', action], action);
      } else {
        this.opcode(['closeElement', action], action);
      }
    }

    attribute([action], isComponent) {
      let {
        name,
        value
      } = action;
      let namespace = getAttrNamespace(name);
      let isStatic = this.prepareAttributeValue(value);

      if (name.charAt(0) === '@') {
        // Arguments
        if (isStatic) {
          this.opcode(['staticArg', name], action);
        } else if (action.value.type === 'MustacheStatement') {
          this.opcode(['dynamicArg', name], action);
        } else {
          this.opcode(['dynamicArg', name], action);
        }
      } else {
        let isTrusting = isTrustedValue(value);

        if (isStatic && name === '...attributes') {
          this.opcode(['attrSplat', null], action);
        } else if (isStatic && !isComponent) {
          this.opcode(['staticAttr', [name, namespace]], action);
        } else if (isTrusting) {
          this.opcode([isComponent ? 'trustingComponentAttr' : 'trustingAttr', [name, namespace]], action);
        } else if (action.value.type === 'MustacheStatement') {
          this.opcode([isComponent ? 'componentAttr' : 'dynamicAttr', [name, null]], action);
        } else {
          this.opcode([isComponent ? 'componentAttr' : 'dynamicAttr', [name, namespace]], action);
        }
      }
    }

    modifier([action]) {
      assertIsSimplePath(action.path, action.loc, 'modifier');
      let {
        path: {
          parts
        }
      } = action;
      this.prepareHelper(action);
      this.opcode(['modifier', parts[0]], action);
    }

    mustache([action]) {
      let {
        path
      } = action;

      if ((0, _syntax.isLiteral)(path)) {
        this.mustacheExpression(action);
        this.opcode(['append', !action.escaped], action);
      } else if (isYield(path)) {
        let to = assertValidYield(action);
        this.yield(to, action);
      } else if (isPartial(path)) {
        let params = assertValidPartial(action);
        this.partial(params, action);
      } else if (isDebugger(path)) {
        assertValidDebuggerUsage(action);
        this.debugger('debugger', action);
      } else {
        this.mustacheExpression(action);
        this.opcode(['append', !action.escaped], action);
      }
    }

    block([action
    /*, index, count*/
    ]) {
      this.prepareHelper(action);
      let templateId = this.templateIds.pop();
      let inverseId = action.inverse === null ? null : this.templateIds.pop();
      this.opcode(['block', [action.path.parts[0], templateId, inverseId]], action);
    } /// Internal actions, not found in the original processed actions


    arg([path]) {
      let {
        parts: [head, ...rest]
      } = path;
      this.opcode(['get', [`@${head}`, rest]], path);
    }

    mustacheExpression(expr) {
      let {
        path
      } = expr;

      if ((0, _syntax.isLiteral)(path)) {
        this.opcode(['literal', path.value], expr);
      } else if (isBuiltInHelper(path)) {
        this.builtInHelper(expr);
      } else if (isArg(path)) {
        this.arg([path]);
      } else if (isHelperInvocation(expr)) {
        this.prepareHelper(expr);
        this.opcode(['helper', path.parts[0]], expr);
      } else if (path.this) {
        this.opcode(['get', [0, path.parts]], expr);
      } else {
        let [head, ...parts] = path.parts;
        this.opcode(['maybeGet', [head, parts]], expr);
      } // } else if (isLocal(path, this.symbols)) {
      //   let [head, ...parts] = path.parts;
      //   this.opcode(['get', [head, parts]], expr);
      // } else if (isSimplePath(path)) {
      //   this.opcode(['unknown', path.parts[0]], expr);
      // } else {
      //   this.opcode(['maybeLocal', path.parts], expr);
      // }

    } /// Internal Syntax


    yield(to, action) {
      this.prepareParams(action.params);
      this.opcode(['yield', to], action);
    }

    debugger(_name, action) {
      this.opcode(['debugger', null], action);
    }

    hasBlock(name, action) {
      this.opcode(['hasBlock', name], action);
    }

    hasBlockParams(name, action) {
      this.opcode(['hasBlockParams', name], action);
    }

    partial(_params, action) {
      this.prepareParams(action.params);
      this.opcode(['partial', null], action);
    }

    builtInHelper(expr) {
      let {
        path
      } = expr;

      if (isHasBlock(path)) {
        let name = assertValidHasBlockUsage(expr.path.original, expr);
        this.hasBlock(name, expr);
      } else if (isHasBlockParams(path)) {
        let name = assertValidHasBlockUsage(expr.path.original, expr);
        this.hasBlockParams(name, expr);
      }
    } /// Expressions, invoked recursively from prepareParams and prepareHash


    SubExpression(expr) {
      if (isBuiltInHelper(expr.path)) {
        this.builtInHelper(expr);
      } else {
        this.prepareHelper(expr);
        this.opcode(['helper', expr.path.parts[0]], expr);
      }
    }

    PathExpression(expr) {
      if (expr.data) {
        this.arg([expr]);
      } else {
        let [head, ...rest] = expr.parts;

        if (expr.this) {
          this.opcode(['get', [0, expr.parts]], expr);
        } else {
          this.opcode(['get', [head, rest]], expr);
        }
      }
    }

    StringLiteral(action) {
      this.opcode(['literal', action.value], action);
    }

    BooleanLiteral(action) {
      this.opcode(['literal', action.value], action);
    }

    NumberLiteral(action) {
      this.opcode(['literal', action.value], action);
    }

    NullLiteral(action) {
      this.opcode(['literal', action.value], action);
    }

    UndefinedLiteral(action) {
      this.opcode(['literal', action.value], action);
    } /// Utilities


    opcode(opcode, action = null) {
      // TODO: This doesn't really work
      if (this.includeMeta && action) {
        opcode.push(this.meta(action));
      }

      this.opcodes.push(opcode);
    }

    prepareHelper(expr) {
      assertIsSimplePath(expr.path, expr.loc, 'helper');
      let {
        params,
        hash
      } = expr;
      this.prepareHash(hash);
      this.prepareParams(params);
    }

    prepareParams(params) {
      if (!params.length) {
        this.opcode(['literal', null], null);
        return;
      }

      for (let i = params.length - 1; i >= 0; i--) {
        let param = params[i];
        this[param.type](param);
      }

      this.opcode(['prepareArray', params.length], null);
    }

    prepareHash(hash) {
      let pairs = hash.pairs;

      if (!pairs.length) {
        this.opcode(['literal', null], null);
        return;
      }

      for (let i = pairs.length - 1; i >= 0; i--) {
        let {
          key,
          value
        } = pairs[i];
        this[value.type](value);
        this.opcode(['literal', key], null);
      }

      this.opcode(['prepareObject', pairs.length], null);
    }

    prepareAttributeValue(value) {
      // returns the static value if the value is static
      switch (value.type) {
        case 'TextNode':
          this.opcode(['literal', value.chars], value);
          return true;

        case 'MustacheStatement':
          this.attributeMustache([value]);
          return false;

        case 'ConcatStatement':
          this.prepareConcatParts(value.parts);
          this.opcode(['concat', null], value);
          return false;
      }
    }

    prepareConcatParts(parts) {
      for (let i = parts.length - 1; i >= 0; i--) {
        let part = parts[i];

        if (part.type === 'MustacheStatement') {
          this.attributeMustache([part]);
        } else if (part.type === 'TextNode') {
          this.opcode(['literal', part.chars], null);
        }
      }

      this.opcode(['prepareArray', parts.length], null);
    }

    attributeMustache([action]) {
      this.mustacheExpression(action);
    }

    meta(node) {
      let loc = node.loc;

      if (!loc) {
        return [];
      }

      let {
        source,
        start,
        end
      } = loc;
      return ['loc', [source || null, [start.line, start.column], [end.line, end.column]]];
    }

  }

  _exports.TemplateCompiler = TemplateCompiler;

  function isHelperInvocation(mustache) {
    return mustache.params && mustache.params.length > 0 || mustache.hash && mustache.hash.pairs.length > 0;
  }

  function isSimplePath({
    parts
  }) {
    return parts.length === 1;
  }

  function isYield(path) {
    return path.original === 'yield';
  }

  function isPartial(path) {
    return path.original === 'partial';
  }

  function isDebugger(path) {
    return path.original === 'debugger';
  }

  function isHasBlock(path) {
    return path.original === 'has-block';
  }

  function isHasBlockParams(path) {
    return path.original === 'has-block-params';
  }

  function isBuiltInHelper(path) {
    return isHasBlock(path) || isHasBlockParams(path);
  }

  function isArg(path) {
    return !!path['data'];
  }

  function isDynamicComponent(element) {
    let open = element.tag.charAt(0);
    let [maybeLocal] = element.tag.split('.');
    let isNamedArgument = open === '@';
    let isLocal = element['symbols'].has(maybeLocal);
    let isThisPath = element.tag.indexOf('this.') === 0;
    return isLocal || isNamedArgument || isThisPath;
  }

  function isComponent(element) {
    let open = element.tag.charAt(0);
    let isPath = element.tag.indexOf('.') > -1;
    let isUpperCase = open === open.toUpperCase() && open !== open.toLowerCase();
    return isUpperCase && !isPath || isDynamicComponent(element);
  }

  function assertIsSimplePath(path, loc, context) {
    if (!isSimplePath(path)) {
      throw new _syntax.SyntaxError(`\`${path.original}\` is not a valid name for a ${context} on line ${loc.start.line}.`, path.loc);
    }
  }

  function assertValidYield(statement) {
    let {
      pairs
    } = statement.hash;

    if (pairs.length === 1 && pairs[0].key !== 'to' || pairs.length > 1) {
      throw new _syntax.SyntaxError(`yield only takes a single named argument: 'to'`, statement.loc);
    } else if (pairs.length === 1 && pairs[0].value.type !== 'StringLiteral') {
      throw new _syntax.SyntaxError(`you can only yield to a literal value`, statement.loc);
    } else if (pairs.length === 0) {
      return 'default';
    } else {
      return pairs[0].value.value;
    }
  }

  function assertValidPartial(statement) {
    let {
      params,
      hash,
      escaped,
      loc
    } = statement;

    if (params && params.length !== 1) {
      throw new _syntax.SyntaxError(`Partial found with no arguments. You must specify a template name. (on line ${loc.start.line})`, statement.loc);
    } else if (hash && hash.pairs.length > 0) {
      throw new _syntax.SyntaxError(`partial does not take any named arguments (on line ${loc.start.line})`, statement.loc);
    } else if (!escaped) {
      throw new _syntax.SyntaxError(`{{{partial ...}}} is not supported, please use {{partial ...}} instead (on line ${loc.start.line})`, statement.loc);
    }

    return params;
  }

  function assertValidHasBlockUsage(type, call) {
    let {
      params,
      hash,
      loc
    } = call;

    if (hash && hash.pairs.length > 0) {
      throw new _syntax.SyntaxError(`${type} does not take any named arguments`, call.loc);
    }

    if (params.length === 0) {
      return 'default';
    } else if (params.length === 1) {
      let param = params[0];

      if (param.type === 'StringLiteral') {
        return param.value;
      } else {
        throw new _syntax.SyntaxError(`you can only yield to a literal value (on line ${loc.start.line})`, call.loc);
      }
    } else {
      throw new _syntax.SyntaxError(`${type} only takes a single positional argument (on line ${loc.start.line})`, call.loc);
    }
  }

  function assertValidDebuggerUsage(statement) {
    let {
      params,
      hash
    } = statement;

    if (hash && hash.pairs.length > 0) {
      throw new _syntax.SyntaxError(`debugger does not take any named arguments`, statement.loc);
    }

    if (params.length === 0) {
      return 'default';
    } else {
      throw new _syntax.SyntaxError(`debugger does not take any positional arguments`, statement.loc);
    }
  }

  const defaultId = (() => {
    if (typeof _nodeModule.require === 'function') {
      try {
        /* tslint:disable:no-require-imports */
        const crypto = (0, _nodeModule.require)("crypto");
        /* tslint:enable:no-require-imports */

        let idFn = src => {
          let hash = crypto.createHash('sha1');
          hash.update(src, 'utf8'); // trim to 6 bytes of data (2^48 - 1)

          return hash.digest('base64').substring(0, 8);
        };

        idFn('test');
        return idFn;
      } catch (e) {}
    }

    return function idFn() {
      return null;
    };
  })();

  _exports.defaultId = defaultId;
  const defaultOptions = {
    id: defaultId,
    meta: {}
  };

  function precompile(string, options = defaultOptions) {
    let ast = (0, _syntax.preprocess)(string, options);
    let {
      meta
    } = options;
    let {
      block
    } = TemplateCompiler.compile(ast, options);
    let idFn = options.id || defaultId;
    let blockJSON = JSON.stringify(block.toJSON());
    let templateJSONObject = {
      id: idFn(JSON.stringify(meta) + blockJSON),
      block: blockJSON,
      meta: meta
    }; // JSON is javascript

    return JSON.stringify(templateJSONObject);
  }
});
enifed("@glimmer/syntax", ["exports", "simple-html-tokenizer", "@glimmer/util", "handlebars"], function (_exports, _simpleHtmlTokenizer, _util, _handlebars) {
  "use strict";

  _exports.preprocess = preprocess;
  _exports.cannotRemoveNode = cannotRemoveNode;
  _exports.cannotReplaceNode = cannotReplaceNode;
  _exports.cannotReplaceOrRemoveInKeyHandlerYet = cannotReplaceOrRemoveInKeyHandlerYet;
  _exports.traverse = traverse;
  _exports.print = build;
  _exports.isLiteral = isLiteral;
  _exports.printLiteral = printLiteral;
  _exports.SyntaxError = _exports.Walker = _exports.TraversalError = _exports.builders = _exports.AST = void 0;

  function buildMustache(path, params, hash, raw, loc) {
    if (typeof path === 'string') {
      path = buildPath(path);
    }

    return {
      type: 'MustacheStatement',
      path,
      params: params || [],
      hash: hash || buildHash([]),
      escaped: !raw,
      loc: buildLoc(loc || null)
    };
  }

  function buildBlock(path, params, hash, program, inverse, loc) {
    return {
      type: 'BlockStatement',
      path: buildPath(path),
      params: params || [],
      hash: hash || buildHash([]),
      program: program || null,
      inverse: inverse || null,
      loc: buildLoc(loc || null)
    };
  }

  function buildElementModifier(path, params, hash, loc) {
    return {
      type: 'ElementModifierStatement',
      path: buildPath(path),
      params: params || [],
      hash: hash || buildHash([]),
      loc: buildLoc(loc || null)
    };
  }

  function buildPartial(name, params, hash, indent, loc) {
    return {
      type: 'PartialStatement',
      name: name,
      params: params || [],
      hash: hash || buildHash([]),
      indent: indent || '',
      strip: {
        open: false,
        close: false
      },
      loc: buildLoc(loc || null)
    };
  }

  function buildComment(value, loc) {
    return {
      type: 'CommentStatement',
      value: value,
      loc: buildLoc(loc || null)
    };
  }

  function buildMustacheComment(value, loc) {
    return {
      type: 'MustacheCommentStatement',
      value: value,
      loc: buildLoc(loc || null)
    };
  }

  function buildConcat(parts, loc) {
    return {
      type: 'ConcatStatement',
      parts: parts || [],
      loc: buildLoc(loc || null)
    };
  }

  function buildElement(tag, attributes, modifiers, children, comments, blockParams, loc) {
    // this is used for backwards compat prior to `blockParams` being added to the AST
    if (Array.isArray(comments)) {
      if (isBlockParms(comments)) {
        blockParams = comments;
        comments = [];
      } else if (isLoc(blockParams)) {
        loc = blockParams;
        blockParams = [];
      }
    } else if (isLoc(comments)) {
      // this is used for backwards compat prior to `comments` being added to the AST
      loc = comments;
      comments = [];
    } else if (isLoc(blockParams)) {
      loc = blockParams;
      blockParams = [];
    } // this is used for backwards compat, prior to `selfClosing` being part of the ElementNode AST


    let selfClosing = false;

    if (typeof tag === 'object') {
      selfClosing = tag.selfClosing;
      tag = tag.name;
    }

    return {
      type: 'ElementNode',
      tag: tag || '',
      selfClosing: selfClosing,
      attributes: attributes || [],
      blockParams: blockParams || [],
      modifiers: modifiers || [],
      comments: comments || [],
      children: children || [],
      loc: buildLoc(loc || null)
    };
  }

  function buildAttr(name, value, loc) {
    return {
      type: 'AttrNode',
      name: name,
      value: value,
      loc: buildLoc(loc || null)
    };
  }

  function buildText(chars, loc) {
    return {
      type: 'TextNode',
      chars: chars || '',
      loc: buildLoc(loc || null)
    };
  } // Expressions


  function buildSexpr(path, params, hash, loc) {
    return {
      type: 'SubExpression',
      path: buildPath(path),
      params: params || [],
      hash: hash || buildHash([]),
      loc: buildLoc(loc || null)
    };
  }

  function buildPath(original, loc) {
    if (typeof original !== 'string') return original;
    let parts = original.split('.');
    let thisHead = false;

    if (parts[0] === 'this') {
      thisHead = true;
      parts = parts.slice(1);
    }

    return {
      type: 'PathExpression',
      original,
      this: thisHead,
      parts,
      data: false,
      loc: buildLoc(loc || null)
    };
  }

  function buildLiteral(type, value, loc) {
    return {
      type,
      value,
      original: value,
      loc: buildLoc(loc || null)
    };
  } // Miscellaneous


  function buildHash(pairs, loc) {
    return {
      type: 'Hash',
      pairs: pairs || [],
      loc: buildLoc(loc || null)
    };
  }

  function buildPair(key, value, loc) {
    return {
      type: 'HashPair',
      key: key,
      value,
      loc: buildLoc(loc || null)
    };
  }

  function buildProgram(body, blockParams, loc) {
    return {
      type: 'Program',
      body: body || [],
      blockParams: blockParams || [],
      loc: buildLoc(loc || null)
    };
  }

  function buildSource(source) {
    return source || null;
  }

  function buildPosition(line, column) {
    return {
      line,
      column
    };
  }

  const SYNTHETIC = {
    source: '(synthetic)',
    start: {
      line: 1,
      column: 0
    },
    end: {
      line: 1,
      column: 0
    }
  };

  function buildLoc(...args) {
    if (args.length === 1) {
      let loc = args[0];

      if (loc && typeof loc === 'object') {
        return {
          source: buildSource(loc.source),
          start: buildPosition(loc.start.line, loc.start.column),
          end: buildPosition(loc.end.line, loc.end.column)
        };
      } else {
        return SYNTHETIC;
      }
    } else {
      let [startLine, startColumn, endLine, endColumn, source] = args;
      return {
        source: buildSource(source),
        start: buildPosition(startLine, startColumn),
        end: buildPosition(endLine, endColumn)
      };
    }
  }

  function isBlockParms(arr) {
    return arr[0] === 'string';
  }

  function isLoc(item) {
    return !Array.isArray(item);
  }

  var b = {
    mustache: buildMustache,
    block: buildBlock,
    partial: buildPartial,
    comment: buildComment,
    mustacheComment: buildMustacheComment,
    element: buildElement,
    elementModifier: buildElementModifier,
    attr: buildAttr,
    text: buildText,
    sexpr: buildSexpr,
    path: buildPath,
    concat: buildConcat,
    hash: buildHash,
    pair: buildPair,
    literal: buildLiteral,
    program: buildProgram,
    loc: buildLoc,
    pos: buildPosition,
    string: literal('StringLiteral'),
    boolean: literal('BooleanLiteral'),
    number: literal('NumberLiteral'),

    undefined() {
      return buildLiteral('UndefinedLiteral', undefined);
    },

    null() {
      return buildLiteral('NullLiteral', null);
    }

  };
  _exports.builders = b;

  function literal(type) {
    return function (value) {
      return buildLiteral(type, value);
    };
  }
  /**
   * Subclass of `Error` with additional information
   * about location of incorrect markup.
   */


  const SyntaxError = function () {
    SyntaxError.prototype = Object.create(Error.prototype);
    SyntaxError.prototype.constructor = SyntaxError;

    function SyntaxError(message, location) {
      let error = Error.call(this, message);
      this.message = message;
      this.stack = error.stack;
      this.location = location;
    }

    return SyntaxError;
  }(); // Regex to validate the identifier for block parameters.
  // Based on the ID validation regex in Handlebars.


  _exports.SyntaxError = SyntaxError;
  let ID_INVERSE_PATTERN = /[!"#%-,\.\/;->@\[-\^`\{-~]/; // Checks the element's attributes to see if it uses block params.
  // If it does, registers the block params with the program and
  // removes the corresponding attributes from the element.

  function parseElementBlockParams(element) {
    let params = parseBlockParams(element);
    if (params) element.blockParams = params;
  }

  function parseBlockParams(element) {
    let l = element.attributes.length;
    let attrNames = [];

    for (let i = 0; i < l; i++) {
      attrNames.push(element.attributes[i].name);
    }

    let asIndex = attrNames.indexOf('as');

    if (asIndex !== -1 && l > asIndex && attrNames[asIndex + 1].charAt(0) === '|') {
      // Some basic validation, since we're doing the parsing ourselves
      let paramsString = attrNames.slice(asIndex).join(' ');

      if (paramsString.charAt(paramsString.length - 1) !== '|' || paramsString.match(/\|/g).length !== 2) {
        throw new SyntaxError("Invalid block parameters syntax: '" + paramsString + "'", element.loc);
      }

      let params = [];

      for (let i = asIndex + 1; i < l; i++) {
        let param = attrNames[i].replace(/\|/g, '');

        if (param !== '') {
          if (ID_INVERSE_PATTERN.test(param)) {
            throw new SyntaxError("Invalid identifier for block parameters: '" + param + "' in '" + paramsString + "'", element.loc);
          }

          params.push(param);
        }
      }

      if (params.length === 0) {
        throw new SyntaxError("Cannot use zero block parameters: '" + paramsString + "'", element.loc);
      }

      element.attributes = element.attributes.slice(0, asIndex);
      return params;
    }

    return null;
  }

  function childrenFor(node) {
    switch (node.type) {
      case 'Program':
        return node.body;

      case 'ElementNode':
        return node.children;
    }
  }

  function appendChild(parent, node) {
    childrenFor(parent).push(node);
  }

  function isLiteral(path) {
    return path.type === 'StringLiteral' || path.type === 'BooleanLiteral' || path.type === 'NumberLiteral' || path.type === 'NullLiteral' || path.type === 'UndefinedLiteral';
  }

  function printLiteral(literal) {
    if (literal.type === 'UndefinedLiteral') {
      return 'undefined';
    } else {
      return JSON.stringify(literal.value);
    }
  }

  const entityParser = new _simpleHtmlTokenizer.EntityParser(_simpleHtmlTokenizer.HTML5NamedCharRefs);

  class Parser {
    constructor(source) {
      this.elementStack = [];
      this.currentAttribute = null;
      this.currentNode = null;
      this.tokenizer = new _simpleHtmlTokenizer.EventedTokenizer(this, entityParser);
      this.source = source.split(/(?:\r\n?|\n)/g);
    }

    get currentAttr() {
      return this.currentAttribute;
    }

    get currentTag() {
      let node = this.currentNode;
      return node;
    }

    get currentStartTag() {
      let node = this.currentNode;
      return node;
    }

    get currentEndTag() {
      let node = this.currentNode;
      return node;
    }

    get currentComment() {
      let node = this.currentNode;
      return node;
    }

    get currentData() {
      let node = this.currentNode;
      return node;
    }

    acceptNode(node) {
      return this[node.type](node);
    }

    currentElement() {
      return this.elementStack[this.elementStack.length - 1];
    }

    sourceForNode(node, endNode) {
      let firstLine = node.loc.start.line - 1;
      let currentLine = firstLine - 1;
      let firstColumn = node.loc.start.column;
      let string = [];
      let line;
      let lastLine;
      let lastColumn;

      if (endNode) {
        lastLine = endNode.loc.end.line - 1;
        lastColumn = endNode.loc.end.column;
      } else {
        lastLine = node.loc.end.line - 1;
        lastColumn = node.loc.end.column;
      }

      while (currentLine < lastLine) {
        currentLine++;
        line = this.source[currentLine];

        if (currentLine === firstLine) {
          if (firstLine === lastLine) {
            string.push(line.slice(firstColumn, lastColumn));
          } else {
            string.push(line.slice(firstColumn));
          }
        } else if (currentLine === lastLine) {
          string.push(line.slice(0, lastColumn));
        } else {
          string.push(line);
        }
      }

      return string.join('\n');
    }

  }

  class HandlebarsNodeVisitors extends Parser {
    constructor() {
      super(...arguments);
      this.cursorCount = 0;
    }

    cursor() {
      return `%cursor:${this.cursorCount++}%`;
    }

    Program(program) {
      let body = [];
      this.cursorCount = 0;
      let node = b.program(body, program.blockParams, program.loc);
      let i,
          l = program.body.length;
      this.elementStack.push(node);

      if (l === 0) {
        return this.elementStack.pop();
      }

      for (i = 0; i < l; i++) {
        this.acceptNode(program.body[i]);
      } // Ensure that that the element stack is balanced properly.


      let poppedNode = this.elementStack.pop();

      if (poppedNode !== node) {
        let elementNode = poppedNode;
        throw new SyntaxError('Unclosed element `' + elementNode.tag + '` (on line ' + elementNode.loc.start.line + ').', elementNode.loc);
      }

      return node;
    }

    BlockStatement(block) {
      if (this.tokenizer['state'] === 'comment') {
        this.appendToCommentData(this.sourceForNode(block));
        return;
      }

      if (this.tokenizer['state'] !== 'comment' && this.tokenizer['state'] !== 'data' && this.tokenizer['state'] !== 'beforeData') {
        throw new SyntaxError('A block may only be used inside an HTML element or another block.', block.loc);
      }

      let {
        path,
        params,
        hash
      } = acceptCallNodes(this, block);
      let program = this.Program(block.program);
      let inverse = block.inverse ? this.Program(block.inverse) : null;

      if (path.original === 'in-element') {
        hash = addInElementHash(this.cursor(), hash, block.loc);
      }

      let node = b.block(path, params, hash, program, inverse, block.loc);
      let parentProgram = this.currentElement();
      appendChild(parentProgram, node);
    }

    MustacheStatement(rawMustache) {
      let {
        tokenizer
      } = this;

      if (tokenizer.state === 'comment') {
        this.appendToCommentData(this.sourceForNode(rawMustache));
        return;
      }

      let mustache;
      let {
        escaped,
        loc
      } = rawMustache;

      if (rawMustache.path.type.match(/Literal$/)) {
        mustache = {
          type: 'MustacheStatement',
          path: this.acceptNode(rawMustache.path),
          params: [],
          hash: b.hash(),
          escaped,
          loc
        };
      } else {
        let {
          path,
          params,
          hash
        } = acceptCallNodes(this, rawMustache);
        mustache = b.mustache(path, params, hash, !escaped, loc);
      }

      switch (tokenizer.state) {
        // Tag helpers
        case "tagOpen"
        /* tagOpen */
        :
        case "tagName"
        /* tagName */
        :
          throw new SyntaxError(`Cannot use mustaches in an elements tagname: \`${this.sourceForNode(rawMustache, rawMustache.path)}\` at L${loc.start.line}:C${loc.start.column}`, mustache.loc);

        case "beforeAttributeName"
        /* beforeAttributeName */
        :
          addElementModifier(this.currentStartTag, mustache);
          break;

        case "attributeName"
        /* attributeName */
        :
        case "afterAttributeName"
        /* afterAttributeName */
        :
          this.beginAttributeValue(false);
          this.finishAttributeValue();
          addElementModifier(this.currentStartTag, mustache);
          tokenizer.transitionTo("beforeAttributeName"
          /* beforeAttributeName */
          );
          break;

        case "afterAttributeValueQuoted"
        /* afterAttributeValueQuoted */
        :
          addElementModifier(this.currentStartTag, mustache);
          tokenizer.transitionTo("beforeAttributeName"
          /* beforeAttributeName */
          );
          break;
        // Attribute values

        case "beforeAttributeValue"
        /* beforeAttributeValue */
        :
          this.beginAttributeValue(false);
          appendDynamicAttributeValuePart(this.currentAttribute, mustache);
          tokenizer.transitionTo("attributeValueUnquoted"
          /* attributeValueUnquoted */
          );
          break;

        case "attributeValueDoubleQuoted"
        /* attributeValueDoubleQuoted */
        :
        case "attributeValueSingleQuoted"
        /* attributeValueSingleQuoted */
        :
        case "attributeValueUnquoted"
        /* attributeValueUnquoted */
        :
          appendDynamicAttributeValuePart(this.currentAttribute, mustache);
          break;
        // TODO: Only append child when the tokenizer state makes
        // sense to do so, otherwise throw an error.

        default:
          appendChild(this.currentElement(), mustache);
      }

      return mustache;
    }

    ContentStatement(content) {
      updateTokenizerLocation(this.tokenizer, content);
      this.tokenizer.tokenizePart(content.value);
      this.tokenizer.flushData();
    }

    CommentStatement(rawComment) {
      let {
        tokenizer
      } = this;

      if (tokenizer.state === "comment"
      /* comment */
      ) {
          this.appendToCommentData(this.sourceForNode(rawComment));
          return null;
        }

      let {
        value,
        loc
      } = rawComment;
      let comment = b.mustacheComment(value, loc);

      switch (tokenizer.state) {
        case "beforeAttributeName"
        /* beforeAttributeName */
        :
          this.currentStartTag.comments.push(comment);
          break;

        case "beforeData"
        /* beforeData */
        :
        case "data"
        /* data */
        :
          appendChild(this.currentElement(), comment);
          break;

        default:
          throw new SyntaxError(`Using a Handlebars comment when in the \`${tokenizer['state']}\` state is not supported: "${comment.value}" on line ${loc.start.line}:${loc.start.column}`, rawComment.loc);
      }

      return comment;
    }

    PartialStatement(partial) {
      let {
        loc
      } = partial;
      throw new SyntaxError(`Handlebars partials are not supported: "${this.sourceForNode(partial, partial.name)}" at L${loc.start.line}:C${loc.start.column}`, partial.loc);
    }

    PartialBlockStatement(partialBlock) {
      let {
        loc
      } = partialBlock;
      throw new SyntaxError(`Handlebars partial blocks are not supported: "${this.sourceForNode(partialBlock, partialBlock.name)}" at L${loc.start.line}:C${loc.start.column}`, partialBlock.loc);
    }

    Decorator(decorator) {
      let {
        loc
      } = decorator;
      throw new SyntaxError(`Handlebars decorators are not supported: "${this.sourceForNode(decorator, decorator.path)}" at L${loc.start.line}:C${loc.start.column}`, decorator.loc);
    }

    DecoratorBlock(decoratorBlock) {
      let {
        loc
      } = decoratorBlock;
      throw new SyntaxError(`Handlebars decorator blocks are not supported: "${this.sourceForNode(decoratorBlock, decoratorBlock.path)}" at L${loc.start.line}:C${loc.start.column}`, decoratorBlock.loc);
    }

    SubExpression(sexpr) {
      let {
        path,
        params,
        hash
      } = acceptCallNodes(this, sexpr);
      return b.sexpr(path, params, hash, sexpr.loc);
    }

    PathExpression(path) {
      let {
        original,
        loc
      } = path;
      let parts;

      if (original.indexOf('/') !== -1) {
        if (original.slice(0, 2) === './') {
          throw new SyntaxError(`Using "./" is not supported in Glimmer and unnecessary: "${path.original}" on line ${loc.start.line}.`, path.loc);
        }

        if (original.slice(0, 3) === '../') {
          throw new SyntaxError(`Changing context using "../" is not supported in Glimmer: "${path.original}" on line ${loc.start.line}.`, path.loc);
        }

        if (original.indexOf('.') !== -1) {
          throw new SyntaxError(`Mixing '.' and '/' in paths is not supported in Glimmer; use only '.' to separate property paths: "${path.original}" on line ${loc.start.line}.`, path.loc);
        }

        parts = [path.parts.join('/')];
      } else if (original === '.') {
        let locationInfo = `L${loc.start.line}:C${loc.start.column}`;
        throw new SyntaxError(`'.' is not a supported path in Glimmer; check for a path with a trailing '.' at ${locationInfo}.`, path.loc);
      } else {
        parts = path.parts;
      }

      let thisHead = false; // This is to fix a bug in the Handlebars AST where the path expressions in
      // `{{this.foo}}` (and similarly `{{foo-bar this.foo named=this.foo}}` etc)
      // are simply turned into `{{foo}}`. The fix is to push it back onto the
      // parts array and let the runtime see the difference. However, we cannot
      // simply use the string `this` as it means literally the property called
      // "this" in the current context (it can be expressed in the syntax as
      // `{{[this]}}`, where the square bracket are generally for this kind of
      // escaping – such as `{{foo.["bar.baz"]}}` would mean lookup a property
      // named literally "bar.baz" on `this.foo`). By convention, we use `null`
      // for this purpose.

      if (original.match(/^this(\..+)?$/)) {
        thisHead = true;
      }

      return {
        type: 'PathExpression',
        original: path.original,
        this: thisHead,
        parts,
        data: path.data,
        loc: path.loc
      };
    }

    Hash(hash) {
      let pairs = [];

      for (let i = 0; i < hash.pairs.length; i++) {
        let pair = hash.pairs[i];
        pairs.push(b.pair(pair.key, this.acceptNode(pair.value), pair.loc));
      }

      return b.hash(pairs, hash.loc);
    }

    StringLiteral(string) {
      return b.literal('StringLiteral', string.value, string.loc);
    }

    BooleanLiteral(boolean) {
      return b.literal('BooleanLiteral', boolean.value, boolean.loc);
    }

    NumberLiteral(number) {
      return b.literal('NumberLiteral', number.value, number.loc);
    }

    UndefinedLiteral(undef) {
      return b.literal('UndefinedLiteral', undefined, undef.loc);
    }

    NullLiteral(nul) {
      return b.literal('NullLiteral', null, nul.loc);
    }

  }

  function calculateRightStrippedOffsets(original, value) {
    if (value === '') {
      // if it is empty, just return the count of newlines
      // in original
      return {
        lines: original.split('\n').length - 1,
        columns: 0
      };
    } // otherwise, return the number of newlines prior to
    // `value`


    let difference = original.split(value)[0];
    let lines = difference.split(/\n/);
    let lineCount = lines.length - 1;
    return {
      lines: lineCount,
      columns: lines[lineCount].length
    };
  }

  function updateTokenizerLocation(tokenizer, content) {
    let line = content.loc.start.line;
    let column = content.loc.start.column;
    let offsets = calculateRightStrippedOffsets(content.original, content.value);
    line = line + offsets.lines;

    if (offsets.lines) {
      column = offsets.columns;
    } else {
      column = column + offsets.columns;
    }

    tokenizer.line = line;
    tokenizer.column = column;
  }

  function acceptCallNodes(compiler, node) {
    let path = compiler.PathExpression(node.path);
    let params = node.params ? node.params.map(e => compiler.acceptNode(e)) : [];
    let hash = node.hash ? compiler.Hash(node.hash) : b.hash();
    return {
      path,
      params,
      hash
    };
  }

  function addElementModifier(element, mustache) {
    let {
      path,
      params,
      hash,
      loc
    } = mustache;

    if (isLiteral(path)) {
      let modifier = `{{${printLiteral(path)}}}`;
      let tag = `<${element.name} ... ${modifier} ...`;
      throw new SyntaxError(`In ${tag}, ${modifier} is not a valid modifier: "${path.original}" on line ${loc && loc.start.line}.`, mustache.loc);
    }

    let modifier = b.elementModifier(path, params, hash, loc);
    element.modifiers.push(modifier);
  }

  function addInElementHash(cursor, hash, loc) {
    let hasNextSibling = false;
    hash.pairs.forEach(pair => {
      if (pair.key === 'guid') {
        throw new SyntaxError('Cannot pass `guid` from user space', loc);
      }

      if (pair.key === 'nextSibling') {
        hasNextSibling = true;
      }
    });
    let guid = b.literal('StringLiteral', cursor);
    let guidPair = b.pair('guid', guid);
    hash.pairs.unshift(guidPair);

    if (!hasNextSibling) {
      let nullLiteral = b.literal('NullLiteral', null);
      let nextSibling = b.pair('nextSibling', nullLiteral);
      hash.pairs.push(nextSibling);
    }

    return hash;
  }

  function appendDynamicAttributeValuePart(attribute, part) {
    attribute.isDynamic = true;
    attribute.parts.push(part);
  }

  function tuple(...args) {
    return args;
  } // ensure stays in sync with typing
  // ParentNode and ChildKey types are derived from VisitorKeysMap


  const visitorKeys = {
    Program: tuple('body'),
    MustacheStatement: tuple('path', 'params', 'hash'),
    BlockStatement: tuple('path', 'params', 'hash', 'program', 'inverse'),
    ElementModifierStatement: tuple('path', 'params', 'hash'),
    PartialStatement: tuple('name', 'params', 'hash'),
    CommentStatement: tuple(),
    MustacheCommentStatement: tuple(),
    ElementNode: tuple('attributes', 'modifiers', 'children', 'comments'),
    AttrNode: tuple('value'),
    TextNode: tuple(),
    ConcatStatement: tuple('parts'),
    SubExpression: tuple('path', 'params', 'hash'),
    PathExpression: tuple(),
    StringLiteral: tuple(),
    BooleanLiteral: tuple(),
    NumberLiteral: tuple(),
    NullLiteral: tuple(),
    UndefinedLiteral: tuple(),
    Hash: tuple('pairs'),
    HashPair: tuple('value')
  };

  const TraversalError = function () {
    TraversalError.prototype = Object.create(Error.prototype);
    TraversalError.prototype.constructor = TraversalError;

    function TraversalError(message, node, parent, key) {
      let error = Error.call(this, message);
      this.key = key;
      this.message = message;
      this.node = node;
      this.parent = parent;
      this.stack = error.stack;
    }

    return TraversalError;
  }();

  _exports.TraversalError = TraversalError;

  function cannotRemoveNode(node, parent, key) {
    return new TraversalError('Cannot remove a node unless it is part of an array', node, parent, key);
  }

  function cannotReplaceNode(node, parent, key) {
    return new TraversalError('Cannot replace a node with multiple nodes unless it is part of an array', node, parent, key);
  }

  function cannotReplaceOrRemoveInKeyHandlerYet(node, key) {
    return new TraversalError('Replacing and removing in key handlers is not yet supported.', node, null, key);
  }

  function getEnterFunction(handler) {
    return typeof handler === 'function' ? handler : handler.enter;
  }

  function getExitFunction(handler) {
    return typeof handler !== 'function' ? handler.exit : undefined;
  }

  function getKeyHandler(handler, key) {
    let keyVisitor = typeof handler !== 'function' ? handler.keys : undefined;
    if (keyVisitor === undefined) return;
    let keyHandler = keyVisitor[key];

    if (keyHandler !== undefined) {
      // widen specific key to all keys
      return keyHandler;
    }

    return keyVisitor.All;
  }

  function getNodeHandler(visitor, nodeType) {
    let handler = visitor[nodeType];

    if (handler !== undefined) {
      // widen specific Node to all nodes
      return handler;
    }

    return visitor.All;
  }

  function visitNode(visitor, node) {
    let handler = getNodeHandler(visitor, node.type);
    let enter;
    let exit;

    if (handler !== undefined) {
      enter = getEnterFunction(handler);
      exit = getExitFunction(handler);
    }

    let result;

    if (enter !== undefined) {
      result = enter(node);
    }

    if (result !== undefined && result !== null) {
      if (JSON.stringify(node) === JSON.stringify(result)) {
        result = undefined;
      } else if (Array.isArray(result)) {
        visitArray(visitor, result);
        return result;
      } else {
        return visitNode(visitor, result) || result;
      }
    }

    if (result === undefined) {
      let keys = visitorKeys[node.type];

      for (let i = 0; i < keys.length; i++) {
        // we know if it has child keys we can widen to a ParentNode
        visitKey(visitor, handler, node, keys[i]);
      }

      if (exit !== undefined) {
        result = exit(node);
      }
    }

    return result;
  }

  function visitKey(visitor, handler, node, key) {
    let value = node[key];

    if (!value) {
      return;
    }

    let keyEnter;
    let keyExit;

    if (handler !== undefined) {
      let keyHandler = getKeyHandler(handler, key);

      if (keyHandler !== undefined) {
        keyEnter = getEnterFunction(keyHandler);
        keyExit = getExitFunction(keyHandler);
      }
    }

    if (keyEnter !== undefined) {
      if (keyEnter(node, key) !== undefined) {
        throw cannotReplaceOrRemoveInKeyHandlerYet(node, key);
      }
    }

    if (Array.isArray(value)) {
      visitArray(visitor, value);
    } else {
      let result = visitNode(visitor, value);

      if (result !== undefined) {
        assignKey(node, key, result);
      }
    }

    if (keyExit !== undefined) {
      if (keyExit(node, key) !== undefined) {
        throw cannotReplaceOrRemoveInKeyHandlerYet(node, key);
      }
    }
  }

  function visitArray(visitor, array) {
    for (let i = 0; i < array.length; i++) {
      let result = visitNode(visitor, array[i]);

      if (result !== undefined) {
        i += spliceArray(array, i, result) - 1;
      }
    }
  }

  function assignKey(node, key, result) {
    if (result === null) {
      throw cannotRemoveNode(node[key], node, key);
    } else if (Array.isArray(result)) {
      if (result.length === 1) {
        node[key] = result[0];
      } else {
        if (result.length === 0) {
          throw cannotRemoveNode(node[key], node, key);
        } else {
          throw cannotReplaceNode(node[key], node, key);
        }
      }
    } else {
      node[key] = result;
    }
  }

  function spliceArray(array, index, result) {
    if (result === null) {
      array.splice(index, 1);
      return 0;
    } else if (Array.isArray(result)) {
      array.splice(index, 1, ...result);
      return result.length;
    } else {
      array.splice(index, 1, result);
      return 1;
    }
  }

  function traverse(node, visitor) {
    visitNode(visitor, node);
  }

  const ATTR_VALUE_REGEX_TEST = /[\xA0"&]/;
  const ATTR_VALUE_REGEX_REPLACE = new RegExp(ATTR_VALUE_REGEX_TEST.source, 'g');
  const TEXT_REGEX_TEST = /[\xA0&<>]/;
  const TEXT_REGEX_REPLACE = new RegExp(TEXT_REGEX_TEST.source, 'g');

  function attrValueReplacer(char) {
    switch (char.charCodeAt(0)) {
      case 160
      /* NBSP */
      :
        return '&nbsp;';

      case 34
      /* QUOT */
      :
        return '&quot;';

      case 38
      /* AMP */
      :
        return '&amp;';

      default:
        return char;
    }
  }

  function textReplacer(char) {
    switch (char.charCodeAt(0)) {
      case 160
      /* NBSP */
      :
        return '&nbsp;';

      case 38
      /* AMP */
      :
        return '&amp;';

      case 60
      /* LT */
      :
        return '&lt;';

      case 62
      /* GT */
      :
        return '&gt;';

      default:
        return char;
    }
  }

  function escapeAttrValue(attrValue) {
    if (ATTR_VALUE_REGEX_TEST.test(attrValue)) {
      return attrValue.replace(ATTR_VALUE_REGEX_REPLACE, attrValueReplacer);
    }

    return attrValue;
  }

  function escapeText(text) {
    if (TEXT_REGEX_TEST.test(text)) {
      return text.replace(TEXT_REGEX_REPLACE, textReplacer);
    }

    return text;
  }

  function unreachable() {
    throw new Error('unreachable');
  }

  function build(ast) {
    if (!ast) {
      return '';
    }

    const output = [];

    switch (ast.type) {
      case 'Program':
        {
          const chainBlock = ast['chained'] && ast.body[0];

          if (chainBlock) {
            chainBlock['chained'] = true;
          }

          const body = buildEach(ast.body).join('');
          output.push(body);
        }
        break;

      case 'ElementNode':
        output.push('<', ast.tag);

        if (ast.attributes.length) {
          output.push(' ', buildEach(ast.attributes).join(' '));
        }

        if (ast.modifiers.length) {
          output.push(' ', buildEach(ast.modifiers).join(' '));
        }

        if (ast.comments.length) {
          output.push(' ', buildEach(ast.comments).join(' '));
        }

        if (ast.blockParams.length) {
          output.push(' ', 'as', ' ', `|${ast.blockParams.join(' ')}|`);
        }

        if (voidMap[ast.tag]) {
          if (ast.selfClosing) {
            output.push(' /');
          }

          output.push('>');
        } else {
          output.push('>');
          output.push.apply(output, buildEach(ast.children));
          output.push('</', ast.tag, '>');
        }

        break;

      case 'AttrNode':
        if (ast.value.type === 'TextNode') {
          if (ast.value.chars !== '') {
            output.push(ast.name, '=');
            output.push('"', escapeAttrValue(ast.value.chars), '"');
          } else {
            output.push(ast.name);
          }
        } else {
          output.push(ast.name, '='); // ast.value is mustache or concat

          output.push(build(ast.value));
        }

        break;

      case 'ConcatStatement':
        output.push('"');
        ast.parts.forEach(node => {
          if (node.type === 'TextNode') {
            output.push(escapeAttrValue(node.chars));
          } else {
            output.push(build(node));
          }
        });
        output.push('"');
        break;

      case 'TextNode':
        output.push(escapeText(ast.chars));
        break;

      case 'MustacheStatement':
        {
          output.push(compactJoin(['{{', pathParams(ast), '}}']));
        }
        break;

      case 'MustacheCommentStatement':
        {
          output.push(compactJoin(['{{!--', ast.value, '--}}']));
        }
        break;

      case 'ElementModifierStatement':
        {
          output.push(compactJoin(['{{', pathParams(ast), '}}']));
        }
        break;

      case 'PathExpression':
        output.push(ast.original);
        break;

      case 'SubExpression':
        {
          output.push('(', pathParams(ast), ')');
        }
        break;

      case 'BooleanLiteral':
        output.push(ast.value ? 'true' : 'false');
        break;

      case 'BlockStatement':
        {
          const lines = [];

          if (ast['chained']) {
            lines.push(['{{else ', pathParams(ast), '}}'].join(''));
          } else {
            lines.push(openBlock(ast));
          }

          lines.push(build(ast.program));

          if (ast.inverse) {
            if (!ast.inverse['chained']) {
              lines.push('{{else}}');
            }

            lines.push(build(ast.inverse));
          }

          if (!ast['chained']) {
            lines.push(closeBlock(ast));
          }

          output.push(lines.join(''));
        }
        break;

      case 'PartialStatement':
        {
          output.push(compactJoin(['{{>', pathParams(ast), '}}']));
        }
        break;

      case 'CommentStatement':
        {
          output.push(compactJoin(['<!--', ast.value, '-->']));
        }
        break;

      case 'StringLiteral':
        {
          output.push(`"${ast.value}"`);
        }
        break;

      case 'NumberLiteral':
        {
          output.push(String(ast.value));
        }
        break;

      case 'UndefinedLiteral':
        {
          output.push('undefined');
        }
        break;

      case 'NullLiteral':
        {
          output.push('null');
        }
        break;

      case 'Hash':
        {
          output.push(ast.pairs.map(pair => {
            return build(pair);
          }).join(' '));
        }
        break;

      case 'HashPair':
        {
          output.push(`${ast.key}=${build(ast.value)}`);
        }
        break;
    }

    return output.join('');
  }

  function compact(array) {
    const newArray = [];
    array.forEach(a => {
      if (typeof a !== 'undefined' && a !== null && a !== '') {
        newArray.push(a);
      }
    });
    return newArray;
  }

  function buildEach(asts) {
    return asts.map(build);
  }

  function pathParams(ast) {
    let path;

    switch (ast.type) {
      case 'MustacheStatement':
      case 'SubExpression':
      case 'ElementModifierStatement':
      case 'BlockStatement':
        if (isLiteral(ast.path)) {
          return String(ast.path.value);
        }

        path = build(ast.path);
        break;

      case 'PartialStatement':
        path = build(ast.name);
        break;

      default:
        return unreachable();
    }

    return compactJoin([path, buildEach(ast.params).join(' '), build(ast.hash)], ' ');
  }

  function compactJoin(array, delimiter) {
    return compact(array).join(delimiter || '');
  }

  function blockParams(block) {
    const params = block.program.blockParams;

    if (params.length) {
      return ` as |${params.join(' ')}|`;
    }

    return null;
  }

  function openBlock(block) {
    return ['{{#', pathParams(block), blockParams(block), '}}'].join('');
  }

  function closeBlock(block) {
    return ['{{/', build(block.path), '}}'].join('');
  }

  class Walker {
    constructor(order) {
      this.order = order;
      this.stack = [];
    }

    visit(node, callback) {
      if (!node) {
        return;
      }

      this.stack.push(node);

      if (this.order === 'post') {
        this.children(node, callback);
        callback(node, this);
      } else {
        callback(node, this);
        this.children(node, callback);
      }

      this.stack.pop();
    }

    children(node, callback) {
      let visitor = visitors[node.type];

      if (visitor) {
        visitor(this, node, callback);
      }
    }

  }

  _exports.Walker = Walker;
  let visitors = {
    Program(walker, node, callback) {
      for (let i = 0; i < node.body.length; i++) {
        walker.visit(node.body[i], callback);
      }
    },

    ElementNode(walker, node, callback) {
      for (let i = 0; i < node.children.length; i++) {
        walker.visit(node.children[i], callback);
      }
    },

    BlockStatement(walker, node, callback) {
      walker.visit(node.program, callback);
      walker.visit(node.inverse || null, callback);
    }

  };
  const voidMap = Object.create(null);
  let voidTagNames = 'area base br col command embed hr img input keygen link meta param source track wbr';
  voidTagNames.split(' ').forEach(tagName => {
    voidMap[tagName] = true;
  });

  class TokenizerEventHandlers extends HandlebarsNodeVisitors {
    constructor() {
      super(...arguments);
      this.tagOpenLine = 0;
      this.tagOpenColumn = 0;
    }

    reset() {
      this.currentNode = null;
    } // Comment


    beginComment() {
      this.currentNode = b.comment('');
      this.currentNode.loc = {
        source: null,
        start: b.pos(this.tagOpenLine, this.tagOpenColumn),
        end: null
      };
    }

    appendToCommentData(char) {
      this.currentComment.value += char;
    }

    finishComment() {
      this.currentComment.loc.end = b.pos(this.tokenizer.line, this.tokenizer.column);
      appendChild(this.currentElement(), this.currentComment);
    } // Data


    beginData() {
      this.currentNode = b.text();
      this.currentNode.loc = {
        source: null,
        start: b.pos(this.tokenizer.line, this.tokenizer.column),
        end: null
      };
    }

    appendToData(char) {
      this.currentData.chars += char;
    }

    finishData() {
      this.currentData.loc.end = b.pos(this.tokenizer.line, this.tokenizer.column);
      appendChild(this.currentElement(), this.currentData);
    } // Tags - basic


    tagOpen() {
      this.tagOpenLine = this.tokenizer.line;
      this.tagOpenColumn = this.tokenizer.column;
    }

    beginStartTag() {
      this.currentNode = {
        type: 'StartTag',
        name: '',
        attributes: [],
        modifiers: [],
        comments: [],
        selfClosing: false,
        loc: SYNTHETIC
      };
    }

    beginEndTag() {
      this.currentNode = {
        type: 'EndTag',
        name: '',
        attributes: [],
        modifiers: [],
        comments: [],
        selfClosing: false,
        loc: SYNTHETIC
      };
    }

    finishTag() {
      let {
        line,
        column
      } = this.tokenizer;
      let tag = this.currentTag;
      tag.loc = b.loc(this.tagOpenLine, this.tagOpenColumn, line, column);

      if (tag.type === 'StartTag') {
        this.finishStartTag();

        if (voidMap[tag.name] || tag.selfClosing) {
          this.finishEndTag(true);
        }
      } else if (tag.type === 'EndTag') {
        this.finishEndTag(false);
      }
    }

    finishStartTag() {
      let {
        name,
        attributes,
        modifiers,
        comments,
        selfClosing
      } = this.currentStartTag;
      let loc = b.loc(this.tagOpenLine, this.tagOpenColumn);
      let element = b.element({
        name,
        selfClosing
      }, attributes, modifiers, [], comments, [], loc);
      this.elementStack.push(element);
    }

    finishEndTag(isVoid) {
      let tag = this.currentTag;
      let element = this.elementStack.pop();
      let parent = this.currentElement();
      validateEndTag(tag, element, isVoid);
      element.loc.end.line = this.tokenizer.line;
      element.loc.end.column = this.tokenizer.column;
      parseElementBlockParams(element);
      appendChild(parent, element);
    }

    markTagAsSelfClosing() {
      this.currentTag.selfClosing = true;
    } // Tags - name


    appendToTagName(char) {
      this.currentTag.name += char;
    } // Tags - attributes


    beginAttribute() {
      let tag = this.currentTag;

      if (tag.type === 'EndTag') {
        throw new SyntaxError(`Invalid end tag: closing tag must not have attributes, ` + `in \`${tag.name}\` (on line ${this.tokenizer.line}).`, tag.loc);
      }

      this.currentAttribute = {
        name: '',
        parts: [],
        isQuoted: false,
        isDynamic: false,
        start: b.pos(this.tokenizer.line, this.tokenizer.column),
        valueStartLine: 0,
        valueStartColumn: 0
      };
    }

    appendToAttributeName(char) {
      this.currentAttr.name += char;
    }

    beginAttributeValue(isQuoted) {
      this.currentAttr.isQuoted = isQuoted;
      this.currentAttr.valueStartLine = this.tokenizer.line;
      this.currentAttr.valueStartColumn = this.tokenizer.column;
    }

    appendToAttributeValue(char) {
      let parts = this.currentAttr.parts;
      let lastPart = parts[parts.length - 1];

      if (lastPart && lastPart.type === 'TextNode') {
        lastPart.chars += char; // update end location for each added char

        lastPart.loc.end.line = this.tokenizer.line;
        lastPart.loc.end.column = this.tokenizer.column;
      } else {
        // initially assume the text node is a single char
        let loc = b.loc(this.tokenizer.line, this.tokenizer.column, this.tokenizer.line, this.tokenizer.column); // correct for `\n` as first char

        if (char === '\n') {
          loc.start.line -= 1;
          loc.start.column = lastPart ? lastPart.loc.end.column : this.currentAttr.valueStartColumn;
        }

        let text = b.text(char, loc);
        parts.push(text);
      }
    }

    finishAttributeValue() {
      let {
        name,
        parts,
        isQuoted,
        isDynamic,
        valueStartLine,
        valueStartColumn
      } = this.currentAttr;
      let value = assembleAttributeValue(parts, isQuoted, isDynamic, this.tokenizer.line);
      value.loc = b.loc(valueStartLine, valueStartColumn, this.tokenizer.line, this.tokenizer.column);
      let loc = b.loc(this.currentAttr.start.line, this.currentAttr.start.column, this.tokenizer.line, this.tokenizer.column);
      let attribute = b.attr(name, value, loc);
      this.currentStartTag.attributes.push(attribute);
    }

    reportSyntaxError(message) {
      throw new SyntaxError(`Syntax error at line ${this.tokenizer.line} col ${this.tokenizer.column}: ${message}`, b.loc(this.tokenizer.line, this.tokenizer.column));
    }

  }

  function assembleAttributeValue(parts, isQuoted, isDynamic, line) {
    if (isDynamic) {
      if (isQuoted) {
        return assembleConcatenatedValue(parts);
      } else {
        if (parts.length === 1 || parts.length === 2 && parts[1].type === 'TextNode' && parts[1].chars === '/') {
          return parts[0];
        } else {
          throw new SyntaxError(`An unquoted attribute value must be a string or a mustache, ` + `preceeded by whitespace or a '=' character, and ` + `followed by whitespace, a '>' character, or '/>' (on line ${line})`, b.loc(line, 0));
        }
      }
    } else {
      return parts.length > 0 ? parts[0] : b.text('');
    }
  }

  function assembleConcatenatedValue(parts) {
    for (let i = 0; i < parts.length; i++) {
      let part = parts[i];

      if (part.type !== 'MustacheStatement' && part.type !== 'TextNode') {
        throw new SyntaxError('Unsupported node in quoted attribute value: ' + part['type'], part.loc);
      }
    }

    return b.concat(parts);
  }

  function validateEndTag(tag, element, selfClosing) {
    let error;

    if (voidMap[tag.name] && !selfClosing) {
      // EngTag is also called by StartTag for void and self-closing tags (i.e.
      // <input> or <br />, so we need to check for that here. Otherwise, we would
      // throw an error for those cases.
      error = 'Invalid end tag ' + formatEndTagInfo(tag) + ' (void elements cannot have end tags).';
    } else if (element.tag === undefined) {
      error = 'Closing tag ' + formatEndTagInfo(tag) + ' without an open tag.';
    } else if (element.tag !== tag.name) {
      error = 'Closing tag ' + formatEndTagInfo(tag) + ' did not match last open tag `' + element.tag + '` (on line ' + element.loc.start.line + ').';
    }

    if (error) {
      throw new SyntaxError(error, element.loc);
    }
  }

  function formatEndTagInfo(tag) {
    return '`' + tag.name + '` (on line ' + tag.loc.end.line + ')';
  }

  const syntax = {
    parse: preprocess,
    builders: b,
    print: build,
    traverse,
    Walker
  };

  function preprocess(html, options) {
    const parseOptions = options ? options.parseOptions : {};
    let ast = typeof html === 'object' ? html : (0, _handlebars.parse)(html, parseOptions);
    let program = new TokenizerEventHandlers(html).acceptNode(ast);

    if (options && options.plugins && options.plugins.ast) {
      for (let i = 0, l = options.plugins.ast.length; i < l; i++) {
        let transform = options.plugins.ast[i];
        let env = (0, _util.assign)({}, options, {
          syntax
        }, {
          plugins: undefined
        });
        let pluginResult = transform(env);
        traverse(program, pluginResult.visitor);
      }
    }

    return program;
  }

  var nodes =
  /*#__PURE__*/
  Object.freeze({}); // used by ember-compiler

  _exports.AST = nodes;
});
enifed("@glimmer/util", ["exports"], function (_exports) {
  "use strict";

  _exports.assert = debugAssert;
  _exports.assign = assign;
  _exports.fillNulls = fillNulls;
  _exports.ensureGuid = ensureGuid;
  _exports.initializeGuid = initializeGuid;
  _exports.dict = dict;
  _exports.unwrap = unwrap;
  _exports.expect = expect;
  _exports.unreachable = unreachable;
  _exports.EMPTY_ARRAY = _exports.ListSlice = _exports.ListNode = _exports.LinkedList = _exports.EMPTY_SLICE = _exports.DictSet = _exports.Stack = void 0;

  function unwrap(val) {
    if (val === null || val === undefined) throw new Error(`Expected value to be present`);
    return val;
  }

  function expect(val, message) {
    if (val === null || val === undefined) throw new Error(message);
    return val;
  }

  function unreachable(message = 'unreachable') {
    return new Error(message);
  } // import Logger from './logger';
  // let alreadyWarned = false;


  function debugAssert(test, msg) {
    // if (!alreadyWarned) {
    //   alreadyWarned = true;
    //   Logger.warn("Don't leave debug assertions on in public builds");
    // }
    if (!test) {
      throw new Error(msg || 'assertion failure');
    }
  }

  const {
    keys: objKeys
  } = Object;

  function assign(obj) {
    for (let i = 1; i < arguments.length; i++) {
      let assignment = arguments[i];
      if (assignment === null || typeof assignment !== 'object') continue;
      let keys = objKeys(assignment);

      for (let j = 0; j < keys.length; j++) {
        let key = keys[j];
        obj[key] = assignment[key];
      }
    }

    return obj;
  }

  function fillNulls(count) {
    let arr = new Array(count);

    for (let i = 0; i < count; i++) {
      arr[i] = null;
    }

    return arr;
  }

  let GUID = 0;

  function initializeGuid(object) {
    return object._guid = ++GUID;
  }

  function ensureGuid(object) {
    return object._guid || initializeGuid(object);
  }

  function dict() {
    return Object.create(null);
  }

  class DictSet {
    constructor() {
      this.dict = dict();
    }

    add(obj) {
      if (typeof obj === 'string') this.dict[obj] = obj;else this.dict[ensureGuid(obj)] = obj;
      return this;
    }

    delete(obj) {
      if (typeof obj === 'string') delete this.dict[obj];else if (obj._guid) delete this.dict[obj._guid];
    }

  }

  _exports.DictSet = DictSet;

  class Stack {
    constructor() {
      this.stack = [];
      this.current = null;
    }

    get size() {
      return this.stack.length;
    }

    push(item) {
      this.current = item;
      this.stack.push(item);
    }

    pop() {
      let item = this.stack.pop();
      let len = this.stack.length;
      this.current = len === 0 ? null : this.stack[len - 1];
      return item === undefined ? null : item;
    }

    isEmpty() {
      return this.stack.length === 0;
    }

  }

  _exports.Stack = Stack;

  class ListNode {
    constructor(value) {
      this.next = null;
      this.prev = null;
      this.value = value;
    }

  }

  _exports.ListNode = ListNode;

  class LinkedList {
    constructor() {
      this.clear();
    }

    head() {
      return this._head;
    }

    tail() {
      return this._tail;
    }

    clear() {
      this._head = this._tail = null;
    }

    toArray() {
      let out = [];
      this.forEachNode(n => out.push(n));
      return out;
    }

    nextNode(node) {
      return node.next;
    }

    forEachNode(callback) {
      let node = this._head;

      while (node !== null) {
        callback(node);
        node = node.next;
      }
    }

    insertBefore(node, reference = null) {
      if (reference === null) return this.append(node);
      if (reference.prev) reference.prev.next = node;else this._head = node;
      node.prev = reference.prev;
      node.next = reference;
      reference.prev = node;
      return node;
    }

    append(node) {
      let tail = this._tail;

      if (tail) {
        tail.next = node;
        node.prev = tail;
        node.next = null;
      } else {
        this._head = node;
      }

      return this._tail = node;
    }

    remove(node) {
      if (node.prev) node.prev.next = node.next;else this._head = node.next;
      if (node.next) node.next.prev = node.prev;else this._tail = node.prev;
      return node;
    }

  }

  _exports.LinkedList = LinkedList;

  class ListSlice {
    constructor(head, tail) {
      this._head = head;
      this._tail = tail;
    }

    forEachNode(callback) {
      let node = this._head;

      while (node !== null) {
        callback(node);
        node = this.nextNode(node);
      }
    }

    head() {
      return this._head;
    }

    tail() {
      return this._tail;
    }

    toArray() {
      let out = [];
      this.forEachNode(n => out.push(n));
      return out;
    }

    nextNode(node) {
      if (node === this._tail) return null;
      return node.next;
    }

  }

  _exports.ListSlice = ListSlice;
  const EMPTY_SLICE = new ListSlice(null, null);
  _exports.EMPTY_SLICE = EMPTY_SLICE;
  const EMPTY_ARRAY = Object.freeze([]);
  _exports.EMPTY_ARRAY = EMPTY_ARRAY;
});
enifed("@glimmer/wire-format", ["exports"], function (_exports) {
  "use strict";

  _exports.is = is;
  _exports.isAttribute = isAttribute;
  _exports.isArgument = isArgument;
  _exports.Ops = _exports.isMaybeLocal = _exports.isGet = _exports.isFlushElement = void 0;
  var Opcodes;
  _exports.Ops = Opcodes;

  (function (Opcodes) {
    // Statements
    Opcodes[Opcodes["Text"] = 0] = "Text";
    Opcodes[Opcodes["Append"] = 1] = "Append";
    Opcodes[Opcodes["Comment"] = 2] = "Comment";
    Opcodes[Opcodes["Modifier"] = 3] = "Modifier";
    Opcodes[Opcodes["Block"] = 4] = "Block";
    Opcodes[Opcodes["Component"] = 5] = "Component";
    Opcodes[Opcodes["DynamicComponent"] = 6] = "DynamicComponent";
    Opcodes[Opcodes["OpenElement"] = 7] = "OpenElement";
    Opcodes[Opcodes["OpenSplattedElement"] = 8] = "OpenSplattedElement";
    Opcodes[Opcodes["FlushElement"] = 9] = "FlushElement";
    Opcodes[Opcodes["CloseElement"] = 10] = "CloseElement";
    Opcodes[Opcodes["StaticAttr"] = 11] = "StaticAttr";
    Opcodes[Opcodes["DynamicAttr"] = 12] = "DynamicAttr";
    Opcodes[Opcodes["ComponentAttr"] = 13] = "ComponentAttr";
    Opcodes[Opcodes["AttrSplat"] = 14] = "AttrSplat";
    Opcodes[Opcodes["Yield"] = 15] = "Yield";
    Opcodes[Opcodes["Partial"] = 16] = "Partial";
    Opcodes[Opcodes["DynamicArg"] = 17] = "DynamicArg";
    Opcodes[Opcodes["StaticArg"] = 18] = "StaticArg";
    Opcodes[Opcodes["TrustingAttr"] = 19] = "TrustingAttr";
    Opcodes[Opcodes["TrustingComponentAttr"] = 20] = "TrustingComponentAttr";
    Opcodes[Opcodes["Debugger"] = 21] = "Debugger";
    Opcodes[Opcodes["ClientSideStatement"] = 22] = "ClientSideStatement"; // Expressions

    Opcodes[Opcodes["Unknown"] = 23] = "Unknown";
    Opcodes[Opcodes["Get"] = 24] = "Get";
    Opcodes[Opcodes["MaybeLocal"] = 25] = "MaybeLocal";
    Opcodes[Opcodes["HasBlock"] = 26] = "HasBlock";
    Opcodes[Opcodes["HasBlockParams"] = 27] = "HasBlockParams";
    Opcodes[Opcodes["Undefined"] = 28] = "Undefined";
    Opcodes[Opcodes["Helper"] = 29] = "Helper";
    Opcodes[Opcodes["Concat"] = 30] = "Concat";
    Opcodes[Opcodes["ClientSideExpression"] = 31] = "ClientSideExpression";
  })(Opcodes || (_exports.Ops = Opcodes = {}));

  function is(variant) {
    return function (value) {
      return Array.isArray(value) && value[0] === variant;
    };
  } // Statements


  const isFlushElement = is(Opcodes.FlushElement);
  _exports.isFlushElement = isFlushElement;

  function isAttribute(val) {
    return val[0] === Opcodes.StaticAttr || val[0] === Opcodes.DynamicAttr || val[0] === Opcodes.ComponentAttr || val[0] === Opcodes.TrustingAttr || val[0] === Opcodes.TrustingComponentAttr || val[0] === Opcodes.AttrSplat;
  }

  function isArgument(val) {
    return val[0] === Opcodes.StaticArg || val[0] === Opcodes.DynamicArg;
  } // Expressions


  const isGet = is(Opcodes.Get);
  _exports.isGet = isGet;
  const isMaybeLocal = is(Opcodes.MaybeLocal);
  _exports.isMaybeLocal = isMaybeLocal;
});
enifed("ember-babel", ["exports"], function (_exports) {
  "use strict";

  _exports.classCallCheck = classCallCheck;
  _exports.inheritsLoose = inheritsLoose;
  _exports.taggedTemplateLiteralLoose = taggedTemplateLiteralLoose;
  _exports.createClass = createClass;
  _exports.assertThisInitialized = assertThisInitialized;
  _exports.possibleConstructorReturn = possibleConstructorReturn;
  _exports.objectDestructuringEmpty = objectDestructuringEmpty;
  const setPrototypeOf = Object.setPrototypeOf;

  function classCallCheck(instance, Constructor) {
    if (true
    /* DEBUG */
    ) {
        if (!(instance instanceof Constructor)) {
          throw new TypeError('Cannot call a class as a function');
        }
      }
  }
  /*
    Overrides default `inheritsLoose` to _also_ call `Object.setPrototypeOf`.
    This is needed so that we can use `loose` option with the
    `@babel/plugin-transform-classes` (because we want simple assignment to the
    prototype whereever possible) but also keep our constructor based prototypal
    inheritance working properly
  */


  function inheritsLoose(subClass, superClass) {
    if (true
    /* DEBUG */
    ) {
        if (typeof superClass !== 'function' && superClass !== null) {
          throw new TypeError('Super expression must either be null or a function');
        }
      }

    subClass.prototype = Object.create(superClass === null ? null : superClass.prototype, {
      constructor: {
        value: subClass,
        writable: true,
        configurable: true
      }
    });

    if (superClass !== null) {
      setPrototypeOf(subClass, superClass);
    }
  }

  function taggedTemplateLiteralLoose(strings, raw) {
    if (!raw) {
      raw = strings.slice(0);
    }

    strings.raw = raw;
    return strings;
  }

  function _defineProperties(target, props) {
    for (var i = 0; i < props.length; i++) {
      var descriptor = props[i];
      descriptor.enumerable = descriptor.enumerable || false;
      descriptor.configurable = true;
      if ('value' in descriptor) descriptor.writable = true;
      Object.defineProperty(target, descriptor.key, descriptor);
    }
  }
  /*
    Differs from default implementation by avoiding boolean coercion of
    `protoProps` and `staticProps`.
  */


  function createClass(Constructor, protoProps, staticProps) {
    if (protoProps !== null && protoProps !== undefined) {
      _defineProperties(Constructor.prototype, protoProps);
    }

    if (staticProps !== null && staticProps !== undefined) {
      _defineProperties(Constructor, staticProps);
    }

    return Constructor;
  }

  function assertThisInitialized(self) {
    if (true
    /* DEBUG */
    && self === void 0) {
      throw new ReferenceError("this hasn't been initialised - super() hasn't been called");
    }

    return self;
  }
  /*
    Adds `DEBUG` guard to error being thrown, and avoids boolean coercion of `call`.
  */


  function possibleConstructorReturn(self, call) {
    if (typeof call === 'object' && call !== null || typeof call === 'function') {
      return call;
    }

    return assertThisInitialized(self);
  }

  function objectDestructuringEmpty(obj) {
    if (true
    /* DEBUG */
    && (obj === null || obj === undefined)) {
      throw new TypeError('Cannot destructure undefined');
    }
  }
});
enifed("ember-template-compiler/index", ["exports", "@ember/-internals/environment", "@ember/canary-features", "ember/version", "ember-template-compiler/lib/compat", "ember-template-compiler/lib/system/precompile", "ember-template-compiler/lib/system/compile", "ember-template-compiler/lib/system/compile-options", "ember-template-compiler/lib/plugins/index", "ember-template-compiler/lib/system/bootstrap", "ember-template-compiler/lib/system/initializer"], function (_exports, _environment, _canaryFeatures, _version, _compat, _precompile, _compile, _compileOptions, _index, _bootstrap, _initializer) {
  "use strict";

  Object.defineProperty(_exports, "precompile", {
    enumerable: true,
    get: function () {
      return _precompile.default;
    }
  });
  Object.defineProperty(_exports, "compile", {
    enumerable: true,
    get: function () {
      return _compile.default;
    }
  });
  Object.defineProperty(_exports, "compileOptions", {
    enumerable: true,
    get: function () {
      return _compileOptions.default;
    }
  });
  Object.defineProperty(_exports, "registerPlugin", {
    enumerable: true,
    get: function () {
      return _compileOptions.registerPlugin;
    }
  });
  Object.defineProperty(_exports, "unregisterPlugin", {
    enumerable: true,
    get: function () {
      return _compileOptions.unregisterPlugin;
    }
  });
  Object.defineProperty(_exports, "defaultPlugins", {
    enumerable: true,
    get: function () {
      return _index.default;
    }
  });
  _exports._Ember = void 0;

  const _Ember = typeof _environment.context.imports.Ember === 'object' && _environment.context.imports.Ember || {}; // private API used by ember-cli-htmlbars to setup ENV and FEATURES


  _exports._Ember = _Ember;

  if (!_Ember.ENV) {
    _Ember.ENV = _environment.ENV;
  }

  if (!_Ember.FEATURES) {
    _Ember.FEATURES = _canaryFeatures.FEATURES;
  }

  if (!_Ember.VERSION) {
    _Ember.VERSION = _version.default;
  } // used for adding Ember.Handlebars.compile for backwards compat


  (0, _compat.default)(_Ember);
});
enifed("ember-template-compiler/lib/compat", ["exports", "ember-template-compiler/lib/system/compile", "ember-template-compiler/lib/system/compile-options", "ember-template-compiler/lib/system/precompile"], function (_exports, _compile, _compileOptions, _precompile) {
  "use strict";

  _exports.default = setupGlobal;

  function setupGlobal(Ember) {
    let EmberHandlebars = Ember.Handlebars;

    if (!EmberHandlebars) {
      Ember.Handlebars = EmberHandlebars = {};
    }

    let EmberHTMLBars = Ember.HTMLBars;

    if (!EmberHTMLBars) {
      Ember.HTMLBars = EmberHTMLBars = {};
    }

    EmberHTMLBars.precompile = EmberHandlebars.precompile = _precompile.default;
    EmberHTMLBars.compile = EmberHandlebars.compile = _compile.default;
    EmberHTMLBars.registerPlugin = _compileOptions.registerPlugin;
  }
});
enifed("ember-template-compiler/lib/plugins/assert-if-helper-without-arguments", ["exports", "@ember/debug", "ember-template-compiler/lib/system/calculate-location-display"], function (_exports, _debug, _calculateLocationDisplay) {
  "use strict";

  _exports.default = assertIfHelperWithoutArguments;

  function assertIfHelperWithoutArguments(env) {
    let {
      moduleName
    } = env.meta;
    return {
      name: 'assert-if-helper-without-arguments',
      visitor: {
        BlockStatement(node) {
          if (isInvalidBlockIf(node)) {
            true && !false && (0, _debug.assert)(`${blockAssertMessage(node.path.original)} ${(0, _calculateLocationDisplay.default)(moduleName, node.loc)}`);
          }
        },

        MustacheStatement(node) {
          if (isInvalidInlineIf(node)) {
            true && !false && (0, _debug.assert)(`${inlineAssertMessage(node.path.original)} ${(0, _calculateLocationDisplay.default)(moduleName, node.loc)}`);
          }
        },

        SubExpression(node) {
          if (isInvalidInlineIf(node)) {
            true && !false && (0, _debug.assert)(`${inlineAssertMessage(node.path.original)} ${(0, _calculateLocationDisplay.default)(moduleName, node.loc)}`);
          }
        }

      }
    };
  }

  function blockAssertMessage(original) {
    return `#${original} requires a single argument.`;
  }

  function inlineAssertMessage(original) {
    return `The inline form of the '${original}' helper expects two or three arguments.`;
  }

  function isInvalidInlineIf(node) {
    return node.path.original === 'if' && (!node.params || node.params.length < 2 || node.params.length > 3);
  }

  function isInvalidBlockIf(node) {
    return node.path.original === 'if' && (!node.params || node.params.length !== 1);
  }
});
enifed("ember-template-compiler/lib/plugins/assert-input-helper-without-block", ["exports", "@ember/debug", "ember-template-compiler/lib/system/calculate-location-display"], function (_exports, _debug, _calculateLocationDisplay) {
  "use strict";

  _exports.default = errorOnInputWithContent;

  function errorOnInputWithContent(env) {
    let {
      moduleName
    } = env.meta;
    return {
      name: 'assert-input-helper-without-block',
      visitor: {
        BlockStatement(node) {
          if (node.path.original !== 'input') {
            return;
          }

          true && !false && (0, _debug.assert)(assertMessage(moduleName, node));
        }

      }
    };
  }

  function assertMessage(moduleName, node) {
    let sourceInformation = (0, _calculateLocationDisplay.default)(moduleName, node.loc);
    return `The {{input}} helper cannot be used in block form. ${sourceInformation}`;
  }
});
enifed("ember-template-compiler/lib/plugins/assert-local-variable-shadowing-helper-invocation", ["exports", "@ember/debug", "ember-template-compiler/lib/system/calculate-location-display"], function (_exports, _debug, _calculateLocationDisplay) {
  "use strict";

  _exports.default = assertLocalVariableShadowingHelperInvocation;

  function assertLocalVariableShadowingHelperInvocation(env) {
    let {
      moduleName
    } = env.meta;
    let locals = [];
    return {
      name: 'assert-local-variable-shadowing-helper-invocation',
      visitor: {
        Program: {
          enter(node) {
            locals.push(node.blockParams);
          },

          exit() {
            locals.pop();
          }

        },
        ElementNode: {
          keys: {
            children: {
              enter(node) {
                locals.push(node.blockParams);
              },

              exit() {
                locals.pop();
              }

            }
          }
        },

        MustacheStatement(node) {
          if (isPath(node.path) && hasArguments(node)) {
            let name = node.path.parts[0];
            let type = 'helper';
            true && !!isLocalVariable(node.path, locals) && (0, _debug.assert)(`${messageFor(name, type)} ${(0, _calculateLocationDisplay.default)(moduleName, node.loc)}`, !isLocalVariable(node.path, locals));
          }
        },

        SubExpression(node) {
          let name = node.path.parts[0];
          let type = 'helper';
          true && !!isLocalVariable(node.path, locals) && (0, _debug.assert)(`${messageFor(name, type)} ${(0, _calculateLocationDisplay.default)(moduleName, node.loc)}`, !isLocalVariable(node.path, locals));
        },

        ElementModifierStatement(node) {
          let name = node.path.parts[0];
          let type = 'modifier';
          true && !!isLocalVariable(node.path, locals) && (0, _debug.assert)(`${messageFor(name, type)} ${(0, _calculateLocationDisplay.default)(moduleName, node.loc)}`, !isLocalVariable(node.path, locals));
        }

      }
    };
  }

  function isLocalVariable(node, locals) {
    return !node.this && node.parts.length === 1 && hasLocalVariable(node.parts[0], locals);
  }

  function hasLocalVariable(name, locals) {
    return locals.some(names => names.indexOf(name) !== -1);
  }

  function messageFor(name, type) {
    return `Cannot invoke the \`${name}\` ${type} because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict.`;
  }

  function isPath(node) {
    return node.type === 'PathExpression';
  }

  function hasArguments(node) {
    return node.params.length > 0 || node.hash.pairs.length > 0;
  }
});
enifed("ember-template-compiler/lib/plugins/assert-reserved-named-arguments", ["exports", "@ember/debug", "ember-template-compiler/lib/system/calculate-location-display"], function (_exports, _debug, _calculateLocationDisplay) {
  "use strict";

  _exports.default = assertReservedNamedArguments;
  const RESERVED = ['@arguments', '@args', '@block', '@else'];
  let isReserved, assertMessage;

  function assertReservedNamedArguments(env) {
    let {
      moduleName
    } = env.meta;
    return {
      name: 'assert-reserved-named-arguments',
      visitor: {
        PathExpression({
          original,
          loc
        }) {
          if (isReserved(original)) {
            true && !false && (0, _debug.assert)(`${assertMessage(original)} ${(0, _calculateLocationDisplay.default)(moduleName, loc)}`);
          }
        }

      }
    };
  }

  if (true
  /* EMBER_GLIMMER_NAMED_ARGUMENTS */
  ) {
      isReserved = name => RESERVED.indexOf(name) !== -1 || Boolean(name.match(/^@[^a-z]/));

      assertMessage = name => `'${name}' is reserved.`;
    } else {
    isReserved = name => name[0] === '@';

    assertMessage = name => `'${name}' is not a valid path.`;
  }
});
enifed("ember-template-compiler/lib/plugins/assert-splattribute-expression", ["exports", "@ember/debug", "ember-template-compiler/lib/system/calculate-location-display"], function (_exports, _debug, _calculateLocationDisplay) {
  "use strict";

  _exports.default = assertSplattributeExpressions;

  function assertSplattributeExpressions(env) {
    let {
      moduleName
    } = env.meta;
    return {
      name: 'assert-splattribute-expressions',
      visitor: {
        AttrNode({
          name,
          loc
        }) {
          if (!true
          /* EMBER_GLIMMER_ANGLE_BRACKET_INVOCATION */
          && name === '...attributes') {
            true && !false && (0, _debug.assert)(`${errorMessage()} ${(0, _calculateLocationDisplay.default)(moduleName, loc)}`);
          }
        },

        PathExpression({
          original,
          loc
        }) {
          if (original === '...attributes') {
            true && !false && (0, _debug.assert)(`${errorMessage()} ${(0, _calculateLocationDisplay.default)(moduleName, loc)}`);
          }
        }

      }
    };
  }

  function errorMessage() {
    if (true
    /* EMBER_GLIMMER_ANGLE_BRACKET_INVOCATION */
    ) {
        return `Using "...attributes" can only be used in the element position e.g. <div ...attributes />. It cannot be used as a path.`;
      }

    return `...attributes is an invalid path`;
  }
});
enifed("ember-template-compiler/lib/plugins/deprecate-send-action", ["exports", "@ember/debug", "@ember/deprecated-features", "ember-template-compiler/lib/system/calculate-location-display"], function (_exports, _debug, _deprecatedFeatures, _calculateLocationDisplay) {
  "use strict";

  _exports.default = deprecateSendAction;
  const EVENTS = ['insert-newline', 'enter', 'escape-press', 'focus-in', 'focus-out', 'key-press', 'key-up', 'key-down'];

  function deprecateSendAction(env) {
    if (_deprecatedFeatures.SEND_ACTION) {
      let {
        moduleName
      } = env.meta;

      let deprecationMessage = (node, evName, action) => {
        let sourceInformation = (0, _calculateLocationDisplay.default)(moduleName, node.loc);
        return `Please refactor \`{{input ${evName}="${action}"}}\` to \`{{input ${evName}=(action "${action}")}}\. ${sourceInformation}`;
      };

      return {
        name: 'deprecate-send-action',
        visitor: {
          MustacheStatement(node) {
            if (node.path.original !== 'input') {
              return;
            }

            node.hash.pairs.forEach(pair => {
              if (EVENTS.indexOf(pair.key) > -1 && pair.value.type === 'StringLiteral') {
                true && !false && (0, _debug.deprecate)(deprecationMessage(node, pair.key, pair.value.original), false, {
                  id: 'ember-component.send-action',
                  until: '4.0.0',
                  url: 'https://emberjs.com/deprecations/v3.x#toc_ember-component-send-action'
                });
              }
            });
          }

        }
      };
    }

    return;
  }
});
enifed("ember-template-compiler/lib/plugins/index", ["exports", "ember-template-compiler/lib/plugins/assert-if-helper-without-arguments", "ember-template-compiler/lib/plugins/assert-input-helper-without-block", "ember-template-compiler/lib/plugins/assert-local-variable-shadowing-helper-invocation", "ember-template-compiler/lib/plugins/assert-reserved-named-arguments", "ember-template-compiler/lib/plugins/assert-splattribute-expression", "ember-template-compiler/lib/plugins/deprecate-send-action", "ember-template-compiler/lib/plugins/transform-action-syntax", "ember-template-compiler/lib/plugins/transform-attrs-into-args", "ember-template-compiler/lib/plugins/transform-component-invocation", "ember-template-compiler/lib/plugins/transform-each-in-into-each", "ember-template-compiler/lib/plugins/transform-has-block-syntax", "ember-template-compiler/lib/plugins/transform-in-element", "ember-template-compiler/lib/plugins/transform-inline-link-to", "ember-template-compiler/lib/plugins/transform-input-type-syntax", "ember-template-compiler/lib/plugins/transform-old-class-binding-syntax", "ember-template-compiler/lib/plugins/transform-quoted-bindings-into-just-bindings", "@ember/deprecated-features"], function (_exports, _assertIfHelperWithoutArguments, _assertInputHelperWithoutBlock, _assertLocalVariableShadowingHelperInvocation, _assertReservedNamedArguments, _assertSplattributeExpression, _deprecateSendAction, _transformActionSyntax, _transformAttrsIntoArgs, _transformComponentInvocation, _transformEachInIntoEach, _transformHasBlockSyntax, _transformInElement, _transformInlineLinkTo, _transformInputTypeSyntax, _transformOldClassBindingSyntax, _transformQuotedBindingsIntoJustBindings, _deprecatedFeatures) {
  "use strict";

  _exports.default = void 0;
  const transforms = [_transformComponentInvocation.default, _transformInlineLinkTo.default, _transformOldClassBindingSyntax.default, _transformQuotedBindingsIntoJustBindings.default, _assertReservedNamedArguments.default, _transformActionSyntax.default, _transformInputTypeSyntax.default, _transformAttrsIntoArgs.default, _transformEachInIntoEach.default, _transformHasBlockSyntax.default, _assertLocalVariableShadowingHelperInvocation.default, _assertInputHelperWithoutBlock.default, _transformInElement.default, _assertIfHelperWithoutArguments.default, _assertSplattributeExpression.default];

  if (_deprecatedFeatures.SEND_ACTION) {
    transforms.push(_deprecateSendAction.default);
  }

  var _default = Object.freeze(transforms);

  _exports.default = _default;
});
enifed("ember-template-compiler/lib/plugins/transform-action-syntax", ["exports"], function (_exports) {
  "use strict";

  _exports.default = transformActionSyntax;

  /**
   @module ember
  */

  /**
    A Glimmer2 AST transformation that replaces all instances of
  
    ```handlebars
   <button {{action 'foo'}}>
   <button onblur={{action 'foo'}}>
   <button onblur={{action (action 'foo') 'bar'}}>
    ```
  
    with
  
    ```handlebars
   <button {{action this 'foo'}}>
   <button onblur={{action this 'foo'}}>
   <button onblur={{action this (action this 'foo') 'bar'}}>
    ```
  
    @private
    @class TransformActionSyntax
  */
  function transformActionSyntax({
    syntax
  }) {
    let {
      builders: b
    } = syntax;
    return {
      name: 'transform-action-syntax',
      visitor: {
        ElementModifierStatement(node) {
          if (isAction(node)) {
            insertThisAsFirstParam(node, b);
          }
        },

        MustacheStatement(node) {
          if (isAction(node)) {
            insertThisAsFirstParam(node, b);
          }
        },

        SubExpression(node) {
          if (isAction(node)) {
            insertThisAsFirstParam(node, b);
          }
        }

      }
    };
  }

  function isAction(node) {
    return node.path.original === 'action';
  }

  function insertThisAsFirstParam(node, builders) {
    node.params.unshift(builders.path('this'));
  }
});
enifed("ember-template-compiler/lib/plugins/transform-attrs-into-args", ["exports"], function (_exports) {
  "use strict";

  _exports.default = transformAttrsIntoArgs;

  /**
   @module ember
  */

  /**
    A Glimmer2 AST transformation that replaces all instances of
  
    ```handlebars
   {{attrs.foo.bar}}
    ```
  
    to
  
    ```handlebars
   {{@foo.bar}}
    ```
  
    as well as `{{#if attrs.foo}}`, `{{deeply (nested attrs.foobar.baz)}}`,
    `{{this.attrs.foo}}` etc
  
    @private
    @class TransformAttrsToProps
  */
  function transformAttrsIntoArgs(env) {
    let {
      builders: b
    } = env.syntax;
    let stack = [[]];
    return {
      name: 'transform-attrs-into-args',
      visitor: {
        Program: {
          enter(node) {
            let parent = stack[stack.length - 1];
            stack.push(parent.concat(node.blockParams));
          },

          exit() {
            stack.pop();
          }

        },

        PathExpression(node) {
          if (isAttrs(node, stack[stack.length - 1])) {
            let path = b.path(node.original.substr(6));
            path.original = `@${path.original}`;
            path.data = true;
            return path;
          }
        }

      }
    };
  }

  function isAttrs(node, symbols) {
    let name = node.parts[0];

    if (symbols.indexOf(name) !== -1) {
      return false;
    }

    if (name === 'attrs') {
      if (node.this === true) {
        node.parts.shift();
        node.original = node.original.slice(5);
      }

      return true;
    }

    return false;
  }
});
enifed("ember-template-compiler/lib/plugins/transform-component-invocation", ["exports", "ember-template-compiler/lib/system/calculate-location-display"], function (_exports, _calculateLocationDisplay) {
  "use strict";

  _exports.default = transformComponentInvocation;

  /**
    Transforms unambigious invocations of closure components to be wrapped with
    the component helper. Once these syntaxes are fully supported by Glimmer VM
    natively, this transform can be removed.
  
    ```handlebars
    {{!-- this.foo is not a legal helper/component name --}}
    {{this.foo "with" some="args"}}
    ```
  
    with
  
    ```handlebars
    {{component this.foo "with" some="args"}}
    ```
  
    and
  
    ```handlebars
    {{!-- this.foo is not a legal helper/component name --}}
    {{#this.foo}}...{{/this.foo}}
    ```
  
    with
  
    ```handlebars
    {{#component this.foo}}...{{/component}}
    ```
  
    and
  
    ```handlebars
    {{!-- foo.bar is not a legal helper/component name --}}
    {{foo.bar "with" some="args"}}
    ```
  
    with
  
    ```handlebars
    {{component foo.bar "with" some="args"}}
    ```
  
    and
  
    ```handlebars
    {{!-- foo.bar is not a legal helper/component name --}}
    {{#foo.bar}}...{{/foo.bar}}
    ```
  
    with
  
    ```handlebars
    {{#component foo.bar}}...{{/component}}
    ```
  
    and
  
    ```handlebars
    {{!-- @foo is not a legal helper/component name --}}
    {{@foo "with" some="args"}}
    ```
  
    with
  
    ```handlebars
    {{component @foo "with" some="args"}}
    ```
  
    and
  
    ```handlebars
    {{!-- @foo is not a legal helper/component name --}}
    {{#@foo}}...{{/@foo}}
    ```
  
    with
  
    ```handlebars
    {{#component @foo}}...{{/component}}
    ```
  
    and
  
    ```handlebars
    {{#let ... as |foo|}}
      {{!-- foo is a local variable --}}
      {{foo "with" some="args"}}
    {{/let}}
    ```
  
    with
  
    ```handlebars
    {{#let ... as |foo|}}
      {{component foo "with" some="args"}}
    {{/let}}
    ```
  
    and
  
    ```handlebars
    {{#let ... as |foo|}}
      {{!-- foo is a local variable --}}
      {{#foo}}...{{/foo}}
    {{/let}}
    ```
  
    with
  
    ```handlebars
    {{#let ... as |foo|}}
      {{#component foo}}...{{/component}}
    {{/let}}
    ```
  
    @private
    @class TransFormComponentInvocation
  */
  function transformComponentInvocation(env) {
    let {
      moduleName
    } = env.meta;
    let {
      builders: b
    } = env.syntax;
    let locals = [];
    let isAttrs = false;
    return {
      name: 'transform-component-invocation',
      visitor: {
        Program: {
          enter(node) {
            locals.push(node.blockParams);
          },

          exit() {
            locals.pop();
          }

        },
        ElementNode: {
          keys: {
            attributes: {
              enter() {
                isAttrs = true;
              },

              exit() {
                isAttrs = false;
              }

            },
            children: {
              enter(node) {
                locals.push(node.blockParams);
              },

              exit() {
                locals.pop();
              }

            }
          }
        },

        BlockStatement(node) {
          if (isBlockInvocation(node, locals)) {
            wrapInComponent(moduleName, node, b);
          }
        },

        MustacheStatement(node) {
          if (!isAttrs && isInlineInvocation(node, locals)) {
            wrapInComponent(moduleName, node, b);
          }
        }

      }
    };
  }

  function isInlineInvocation(node, locals) {
    let {
      path
    } = node;
    return isPath(path) && isIllegalName(path, locals) && hasArguments(node);
  }

  function isPath(node) {
    return node.type === 'PathExpression';
  }

  function isIllegalName(node, locals) {
    return isThisPath(node) || isDotPath(node) || isNamedArg(node) || isLocalVariable(node, locals);
  }

  function isThisPath(node) {
    return node.this === true;
  }

  function isDotPath(node) {
    return node.parts.length > 1;
  }

  function isNamedArg(node) {
    return node.data === true;
  }

  function isLocalVariable(node, locals) {
    return !node.this && hasLocalVariable(node.parts[0], locals);
  }

  function hasLocalVariable(name, locals) {
    return locals.some(names => names.indexOf(name) !== -1);
  }

  function hasArguments(node) {
    return node.params.length > 0 || node.hash.pairs.length > 0;
  }

  function isBlockInvocation(node, locals) {
    return isIllegalName(node.path, locals);
  }

  function wrapInAssertion(moduleName, node, b) {
    let error = b.string(`expected \`${node.original}\` to be a contextual component but found a string. Did you mean \`(component ${node.original})\`? ${(0, _calculateLocationDisplay.default)(moduleName, node.loc)}`);
    return b.sexpr(b.path('-assert-implicit-component-helper-argument'), [node, error], b.hash(), node.loc);
  }

  function wrapInComponent(moduleName, node, b) {
    let component = wrapInAssertion(moduleName, node.path, b);
    node.path = b.path('component');
    node.params.unshift(component);
  }
});
enifed("ember-template-compiler/lib/plugins/transform-each-in-into-each", ["exports"], function (_exports) {
  "use strict";

  _exports.default = transformEachInIntoEach;

  /**
   @module ember
  */

  /**
    A Glimmer2 AST transformation that replaces all instances of
  
    ```handlebars
   {{#each-in iterableThing as |key value|}}
    ```
  
    with
  
    ```handlebars
   {{#each (-each-in iterableThing) as |value key|}}
    ```
  
    @private
    @class TransformHasBlockSyntax
  */
  function transformEachInIntoEach(env) {
    let {
      builders: b
    } = env.syntax;
    return {
      name: 'transform-each-in-into-each',
      visitor: {
        BlockStatement(node) {
          if (node.path.original === 'each-in') {
            node.params[0] = b.sexpr(b.path('-each-in'), [node.params[0]]);
            let blockParams = node.program.blockParams;

            if (!blockParams || blockParams.length === 0) {// who uses {{#each-in}} without block params?!
            } else if (blockParams.length === 1) {
              // insert a dummy variable for the first slot
              // pick a name that won't parse so it won't shadow any real variables
              blockParams = ['( unused value )', blockParams[0]];
            } else {
              let key = blockParams.shift();
              let value = blockParams.shift();
              blockParams = [value, key, ...blockParams];
            }

            node.program.blockParams = blockParams;
            return b.block(b.path('each'), node.params, node.hash, node.program, node.inverse, node.loc);
          }
        }

      }
    };
  }
});
enifed("ember-template-compiler/lib/plugins/transform-has-block-syntax", ["exports"], function (_exports) {
  "use strict";

  _exports.default = transformHasBlockSyntax;

  /**
   @module ember
  */

  /**
    A Glimmer2 AST transformation that replaces all instances of
  
    ```handlebars
   {{hasBlock}}
    ```
  
    with
  
    ```handlebars
   {{has-block}}
    ```
  
    @private
    @class TransformHasBlockSyntax
  */
  const TRANSFORMATIONS = {
    hasBlock: 'has-block',
    hasBlockParams: 'has-block-params'
  };

  function transformHasBlockSyntax(env) {
    let {
      builders: b
    } = env.syntax;
    return {
      name: 'transform-has-block-syntax',
      visitor: {
        PathExpression(node) {
          if (TRANSFORMATIONS[node.original]) {
            return b.sexpr(b.path(TRANSFORMATIONS[node.original]));
          }
        },

        MustacheStatement(node) {
          if (typeof node.path.original === 'string' && TRANSFORMATIONS[node.path.original]) {
            return b.mustache(b.path(TRANSFORMATIONS[node.path.original]), node.params, node.hash, undefined, node.loc);
          }
        },

        SubExpression(node) {
          if (TRANSFORMATIONS[node.path.original]) {
            return b.sexpr(b.path(TRANSFORMATIONS[node.path.original]), node.params, node.hash);
          }
        }

      }
    };
  }
});
enifed("ember-template-compiler/lib/plugins/transform-in-element", ["exports", "@ember/debug", "ember-template-compiler/lib/system/calculate-location-display"], function (_exports, _debug, _calculateLocationDisplay) {
  "use strict";

  _exports.default = transformInElement;

  /**
   @module ember
  */

  /**
    glimmer-vm has made the `in-element` API public from its perspective (in
    https://github.com/glimmerjs/glimmer-vm/pull/619) so in glimmer-vm the
    correct keyword to use is `in-element`, however Ember is still working through
    its form of `in-element` (see https://github.com/emberjs/rfcs/pull/287).
  
    There are enough usages of the pre-existing private API (`{{-in-element`) in
    the wild that we need to transform `{{-in-element` into `{{in-element` during
    template transpilation, but since RFC#287 is not landed and enabled by default we _also_ need
    to prevent folks from starting to use `{{in-element` "for realz".
  
    Tranforms:
  
    ```handlebars
    {{#-in-element someElement}}
      {{modal-display text=text}}
    {{/-in-element}}
    ```
  
    into:
  
    ```handlebars
    {{#in-element someElement}}
      {{modal-display text=text}}
    {{/in-element}}
    ```
  
    And issues a build time assertion for:
  
    ```handlebars
    {{#in-element someElement}}
      {{modal-display text=text}}
    {{/in-element}}
    ```
  
    @private
    @class TransformHasBlockSyntax
  */
  function transformInElement(env) {
    let {
      moduleName
    } = env.meta;
    let {
      builders: b
    } = env.syntax;
    let cursorCount = 0;
    return {
      name: 'transform-in-element',
      visitor: {
        BlockStatement(node) {
          if (node.path.original === 'in-element') {
            true && !false && (0, _debug.assert)(assertMessage(moduleName, node));
          } else if (node.path.original === '-in-element') {
            node.path.original = 'in-element';
            node.path.parts = ['in-element']; // replicate special hash arguments added here:
            // https://github.com/glimmerjs/glimmer-vm/blob/ba9b37d44b85fa1385eeeea71910ff5798198c8e/packages/%40glimmer/syntax/lib/parser/handlebars-node-visitors.ts#L340-L363

            let hasNextSibling = false;
            let hash = node.hash;
            hash.pairs.forEach(pair => {
              if (pair.key === 'nextSibling') {
                hasNextSibling = true;
              }
            });
            let guid = b.literal('StringLiteral', `%cursor:${cursorCount++}%`);
            let guidPair = b.pair('guid', guid);
            hash.pairs.unshift(guidPair);

            if (!hasNextSibling) {
              let nullLiteral = b.literal('NullLiteral', null);
              let nextSibling = b.pair('nextSibling', nullLiteral);
              hash.pairs.push(nextSibling);
            }
          }
        }

      }
    };
  }

  function assertMessage(moduleName, node) {
    let sourceInformation = (0, _calculateLocationDisplay.default)(moduleName, node.loc);
    return `The {{in-element}} helper cannot be used. ${sourceInformation}`;
  }
});
enifed("ember-template-compiler/lib/plugins/transform-inline-link-to", ["exports"], function (_exports) {
  "use strict";

  _exports.default = transformInlineLinkTo;

  function buildProgram(b, content, loc) {
    return b.program([buildStatement(b, content, loc)], undefined, loc);
  }

  function buildStatement(b, content, loc) {
    switch (content.type) {
      case 'PathExpression':
        return b.mustache(content, undefined, undefined, undefined, loc);

      case 'SubExpression':
        return b.mustache(content.path, content.params, content.hash, undefined, loc);
      // The default case handles literals.

      default:
        return b.text(`${content.value}`, loc);
    }
  }

  function unsafeHtml(b, expr) {
    return b.sexpr('-html-safe', [expr]);
  }

  function transformInlineLinkTo(env) {
    let {
      builders: b
    } = env.syntax;
    return {
      name: 'transform-inline-link-to',
      visitor: {
        MustacheStatement(node) {
          if (node.path.original === 'link-to') {
            let content = node.escaped ? node.params[0] : unsafeHtml(b, node.params[0]);
            return b.block('link-to', node.params.slice(1), node.hash, buildProgram(b, content, node.loc), null, node.loc);
          }
        }

      }
    };
  }
});
enifed("ember-template-compiler/lib/plugins/transform-input-type-syntax", ["exports"], function (_exports) {
  "use strict";

  _exports.default = transformInputTypeSyntax;

  /**
   @module ember
  */

  /**
    A Glimmer2 AST transformation that replaces all instances of
  
    ```handlebars
   {{input type=boundType}}
    ```
  
    with
  
    ```handlebars
   {{input (-input-type boundType) type=boundType}}
    ```
  
    Note that the type parameters is not removed as the -input-type helpers
    is only used to select the component class. The component still needs
    the type parameter to function.
  
    @private
    @class TransformInputTypeSyntax
  */
  function transformInputTypeSyntax(env) {
    let b = env.syntax.builders;
    return {
      name: 'transform-input-type-syntax',
      visitor: {
        MustacheStatement(node) {
          if (isInput(node)) {
            insertTypeHelperParameter(node, b);
          }
        }

      }
    };
  }

  function isInput(node) {
    return node.path.original === 'input';
  }

  function insertTypeHelperParameter(node, builders) {
    let pairs = node.hash.pairs;
    let pair = null;

    for (let i = 0; i < pairs.length; i++) {
      if (pairs[i].key === 'type') {
        pair = pairs[i];
        break;
      }
    }

    if (pair && pair.value.type !== 'StringLiteral') {
      node.params.unshift(builders.sexpr('-input-type', [pair.value], undefined, pair.loc));
    }
  }
});
enifed("ember-template-compiler/lib/plugins/transform-old-class-binding-syntax", ["exports"], function (_exports) {
  "use strict";

  _exports.default = transformOldClassBindingSyntax;

  function transformOldClassBindingSyntax(env) {
    let b = env.syntax.builders;
    return {
      name: 'transform-old-class-binding-syntax',
      visitor: {
        MustacheStatement(node) {
          process(b, node);
        },

        BlockStatement(node) {
          process(b, node);
        }

      }
    };
  }

  function process(b, node) {
    let allOfTheMicrosyntaxes = [];
    let allOfTheMicrosyntaxIndexes = [];
    let classPair;
    each(node.hash.pairs, (pair, index) => {
      let {
        key
      } = pair;

      if (key === 'classBinding' || key === 'classNameBindings') {
        allOfTheMicrosyntaxIndexes.push(index);
        allOfTheMicrosyntaxes.push(pair);
      } else if (key === 'class') {
        classPair = pair;
      }
    });

    if (allOfTheMicrosyntaxes.length === 0) {
      return;
    }

    let classValue = [];

    if (classPair) {
      classValue.push(classPair.value);
      classValue.push(b.string(' '));
    } else {
      classPair = b.pair('class', null);
      node.hash.pairs.push(classPair);
    }

    each(allOfTheMicrosyntaxIndexes, index => {
      node.hash.pairs.splice(index, 1);
    });
    each(allOfTheMicrosyntaxes, ({
      value
    }) => {
      let sexprs = []; // TODO: add helpful deprecation when both `classNames` and `classNameBindings` can
      // be removed.

      if (value.type === 'StringLiteral') {
        let microsyntax = parseMicrosyntax(value.original);
        buildSexprs(microsyntax, sexprs, b);
        classValue.push(...sexprs);
      }
    });
    let hash = b.hash();
    classPair.value = b.sexpr(b.path('concat'), classValue, hash);
  }

  function buildSexprs(microsyntax, sexprs, b) {
    for (let i = 0; i < microsyntax.length; i++) {
      let [propName, activeClass, inactiveClass] = microsyntax[i];
      let sexpr; // :my-class-name microsyntax for static values

      if (propName === '') {
        sexpr = b.string(activeClass);
      } else {
        let params = [b.path(propName)];

        if (activeClass || activeClass === '') {
          params.push(b.string(activeClass));
        } else {
          let sexprParams = [b.string(propName), b.path(propName)];
          let hash = b.hash();

          if (activeClass !== undefined) {
            hash.pairs.push(b.pair('activeClass', b.string(activeClass)));
          }

          if (inactiveClass !== undefined) {
            hash.pairs.push(b.pair('inactiveClass', b.string(inactiveClass)));
          }

          params.push(b.sexpr(b.path('-normalize-class'), sexprParams, hash));
        }

        if (inactiveClass || inactiveClass === '') {
          params.push(b.string(inactiveClass));
        }

        sexpr = b.sexpr(b.path('if'), params);
      }

      sexprs.push(sexpr);
      sexprs.push(b.string(' '));
    }
  }

  function each(list, callback) {
    for (let i = 0; i < list.length; i++) {
      callback(list[i], i);
    }
  }

  function parseMicrosyntax(string) {
    let segments = string.split(' ');
    let ret = [];

    for (let i = 0; i < segments.length; i++) {
      ret[i] = segments[i].split(':');
    }

    return ret;
  }
});
enifed("ember-template-compiler/lib/plugins/transform-quoted-bindings-into-just-bindings", ["exports"], function (_exports) {
  "use strict";

  _exports.default = transformQuotedBindingsIntoJustBindings;

  function transformQuotedBindingsIntoJustBindings()
  /* env */
  {
    return {
      name: 'transform-quoted-bindings-into-just-bindings',
      visitor: {
        ElementNode(node) {
          let styleAttr = getStyleAttr(node);

          if (!validStyleAttr(styleAttr)) {
            return;
          }

          styleAttr.value = styleAttr.value.parts[0];
        }

      }
    };
  }

  function validStyleAttr(attr) {
    if (!attr) {
      return false;
    }

    let value = attr.value;

    if (!value || value.type !== 'ConcatStatement' || value.parts.length !== 1) {
      return false;
    }

    let onlyPart = value.parts[0];
    return onlyPart.type === 'MustacheStatement';
  }

  function getStyleAttr(node) {
    let attributes = node.attributes;

    for (let i = 0; i < attributes.length; i++) {
      if (attributes[i].name === 'style') {
        return attributes[i];
      }
    }

    return undefined;
  }
});
enifed("ember-template-compiler/lib/system/bootstrap", ["exports", "ember-template-compiler/lib/system/compile"], function (_exports, _compile) {
  "use strict";

  _exports.default = void 0;

  /**
  @module ember
  */

  /**
    Find templates stored in the head tag as script tags and make them available
    to `Ember.CoreView` in the global `Ember.TEMPLATES` object.
  
    Script tags with `text/x-handlebars` will be compiled
    with Ember's template compiler and are suitable for use as a view's template.
  
    @private
    @method bootstrap
    @for Ember.HTMLBars
    @static
    @param ctx
  */
  function bootstrap({
    context,
    hasTemplate,
    setTemplate
  }) {
    if (!context) {
      context = document;
    }

    let selector = 'script[type="text/x-handlebars"]';
    let elements = context.querySelectorAll(selector);

    for (let i = 0; i < elements.length; i++) {
      let script = elements[i]; // Get the name of the script
      // First look for data-template-name attribute, then fall back to its
      // id if no name is found.

      let templateName = script.getAttribute('data-template-name') || script.getAttribute('id') || 'application';
      let template;
      template = (0, _compile.default)(script.innerHTML, {
        moduleName: templateName
      }); // Check if template of same name already exists.

      if (hasTemplate(templateName)) {
        throw new Error(`Template named "${templateName}" already exists.`);
      } // For templates which have a name, we save them and then remove them from the DOM.


      setTemplate(templateName, template); // Remove script tag from DOM.

      script.parentNode.removeChild(script);
    }
  }

  var _default = bootstrap;
  _exports.default = _default;
});
enifed("ember-template-compiler/lib/system/calculate-location-display", ["exports"], function (_exports) {
  "use strict";

  _exports.default = calculateLocationDisplay;

  function calculateLocationDisplay(moduleName, loc) {
    let moduleInfo = '';

    if (moduleName) {
      moduleInfo += `'${moduleName}' `;
    }

    if (loc) {
      let {
        column,
        line
      } = loc.start || {
        line: undefined,
        column: undefined
      };

      if (line !== undefined && column !== undefined) {
        if (moduleName) {
          // only prepend @ if the moduleName was present
          moduleInfo += '@ ';
        }

        moduleInfo += `L${line}:C${column}`;
      }
    }

    if (moduleInfo) {
      moduleInfo = `(${moduleInfo}) `;
    }

    return moduleInfo;
  }
});
enifed("ember-template-compiler/lib/system/compile-options", ["exports", "@ember/polyfills", "ember-template-compiler/lib/plugins/index", "ember-template-compiler/lib/system/dasherize-component-name"], function (_exports, _polyfills, _index, _dasherizeComponentName) {
  "use strict";

  _exports.default = compileOptions;
  _exports.registerPlugin = registerPlugin;
  _exports.unregisterPlugin = unregisterPlugin;
  let USER_PLUGINS = [];

  function compileOptions(_options) {
    let options = (0, _polyfills.assign)({
      meta: {}
    }, _options, {
      customizeComponentName(tagname) {
        return _dasherizeComponentName.default.get(tagname);
      }

    }); // move `moduleName` into `meta` property

    if (options.moduleName) {
      let meta = options.meta;
      meta.moduleName = options.moduleName;
    }

    if (!options.plugins) {
      options.plugins = {
        ast: [...USER_PLUGINS, ..._index.default]
      };
    } else {
      let potententialPugins = [...USER_PLUGINS, ..._index.default];
      let providedPlugins = options.plugins.ast.map(plugin => wrapLegacyPluginIfNeeded(plugin));
      let pluginsToAdd = potententialPugins.filter(plugin => {
        return options.plugins.ast.indexOf(plugin) === -1;
      });
      options.plugins.ast = providedPlugins.concat(pluginsToAdd);
    }

    return options;
  }

  function wrapLegacyPluginIfNeeded(_plugin) {
    let plugin = _plugin;

    if (_plugin.prototype && _plugin.prototype.transform) {
      const pluginFunc = env => {
        let pluginInstantiated = false;
        return {
          name: _plugin.constructor && _plugin.constructor.name,
          visitor: {
            Program(node) {
              if (!pluginInstantiated) {
                pluginInstantiated = true;
                let plugin = new _plugin(env);
                plugin.syntax = env.syntax;
                return plugin.transform(node);
              }
            }

          }
        };
      };

      pluginFunc.__raw = _plugin;
      plugin = pluginFunc;
    }

    return plugin;
  }

  function registerPlugin(type, _plugin) {
    if (type !== 'ast') {
      throw new Error(`Attempting to register ${_plugin} as "${type}" which is not a valid Glimmer plugin type.`);
    }

    for (let i = 0; i < USER_PLUGINS.length; i++) {
      let PLUGIN = USER_PLUGINS[i];

      if (PLUGIN === _plugin || PLUGIN.__raw === _plugin) {
        return;
      }
    }

    let plugin = wrapLegacyPluginIfNeeded(_plugin);
    USER_PLUGINS = [plugin, ...USER_PLUGINS];
  }

  function unregisterPlugin(type, PluginClass) {
    if (type !== 'ast') {
      throw new Error(`Attempting to unregister ${PluginClass} as "${type}" which is not a valid Glimmer plugin type.`);
    }

    USER_PLUGINS = USER_PLUGINS.filter(plugin => plugin !== PluginClass && plugin.__raw !== PluginClass);
  }
});
enifed("ember-template-compiler/lib/system/compile", ["exports", "require", "ember-template-compiler/lib/system/precompile"], function (_exports, _require, _precompile) {
  "use strict";

  _exports.default = compile;

  /**
  @module ember
  */
  let template;
  /**
    Uses HTMLBars `compile` function to process a string into a compiled template.
  
    This is not present in production builds.
  
    @private
    @method compile
    @param {String} templateString This is the string to be compiled by HTMLBars.
    @param {Object} options This is an options hash to augment the compiler options.
  */

  function compile(templateString, options) {
    if (!template && (0, _require.has)('@ember/-internals/glimmer')) {
      // tslint:disable-next-line:no-require-imports
      template = (0, _require.default)("@ember/-internals/glimmer").template;
    }

    if (!template) {
      throw new Error('Cannot call `compile` with only the template compiler loaded. Please load `ember.debug.js` or `ember.prod.js` prior to calling `compile`.');
    }

    let precompiledTemplateString = (0, _precompile.default)(templateString, options);
    let templateJS = new Function(`return ${precompiledTemplateString}`)();
    return template(templateJS);
  }
});
enifed("ember-template-compiler/lib/system/dasherize-component-name", ["exports", "@ember/-internals/utils"], function (_exports, _utils) {
  "use strict";

  _exports.default = void 0;

  /*
    This diverges from `Ember.String.dasherize` so that`<XFoo />` can resolve to `x-foo`.
    `Ember.String.dasherize` would resolve it to `xfoo`..
  */
  const SIMPLE_DASHERIZE_REGEXP = /[A-Z]/g;
  const ALPHA = /[A-Za-z0-9]/;

  var _default = new _utils.Cache(1000, key => key.replace(SIMPLE_DASHERIZE_REGEXP, (char, index) => {
    if (index === 0 || !ALPHA.test(key[index - 1])) {
      return char.toLowerCase();
    }

    return `-${char.toLowerCase()}`;
  }));

  _exports.default = _default;
});
enifed("ember-template-compiler/lib/system/initializer", ["require", "ember-template-compiler/lib/system/bootstrap"], function (_require, _bootstrap) {
  "use strict";

  // Globals mode template compiler
  if ((0, _require.has)('@ember/application') && (0, _require.has)('@ember/-internals/browser-environment') && (0, _require.has)('@ember/-internals/glimmer')) {
    // tslint:disable:no-require-imports
    let emberEnv = (0, _require.default)("@ember/-internals/browser-environment");
    let emberGlimmer = (0, _require.default)("@ember/-internals/glimmer");
    let emberApp = (0, _require.default)("@ember/application");
    let Application = emberApp.default;
    let {
      hasTemplate,
      setTemplate
    } = emberGlimmer;
    let {
      hasDOM
    } = emberEnv;
    Application.initializer({
      name: 'domTemplates',

      initialize() {
        let context;

        if (hasDOM) {
          context = document;
        }

        (0, _bootstrap.default)({
          context,
          hasTemplate,
          setTemplate
        });
      }

    });
  }
});
enifed("ember-template-compiler/lib/system/precompile", ["exports", "@glimmer/compiler", "ember-template-compiler/lib/system/compile-options"], function (_exports, _compiler, _compileOptions) {
  "use strict";

  _exports.default = precompile;

  /**
  @module ember
  */

  /**
    Uses HTMLBars `compile` function to process a string into a compiled template string.
    The returned string must be passed through `Ember.HTMLBars.template`.
  
    This is not present in production builds.
  
    @private
    @method precompile
    @param {String} templateString This is the string to be compiled by HTMLBars.
  */
  function precompile(templateString, options) {
    return (0, _compiler.precompile)(templateString, (0, _compileOptions.default)(options));
  }
});
enifed("ember-template-compiler/tests/plugins/assert-if-helper-without-arguments-test", ["ember-template-compiler/index", "internal-test-helpers"], function (_index, _internalTestHelpers) {
  "use strict";

  (0, _internalTestHelpers.moduleFor)('ember-template-compiler: assert-if-helper-without-argument', class extends _internalTestHelpers.AbstractTestCase {
    [`@test block if helper expects one argument`]() {
      expectAssertion(() => {
        (0, _index.compile)(`{{#if}}aVal{{/if}}`, {
          moduleName: 'baz/foo-bar'
        });
      }, `#if requires a single argument. ('baz/foo-bar' @ L1:C0) `);
      expectAssertion(() => {
        (0, _index.compile)(`{{#if val1 val2}}aVal{{/if}}`, {
          moduleName: 'baz/foo-bar'
        });
      }, `#if requires a single argument. ('baz/foo-bar' @ L1:C0) `);
      expectAssertion(() => {
        (0, _index.compile)(`{{#if}}aVal{{/if}}`, {
          moduleName: 'baz/foo-bar'
        });
      }, `#if requires a single argument. ('baz/foo-bar' @ L1:C0) `);
    }

    [`@test inline if helper expects between one and three arguments`]() {
      expectAssertion(() => {
        (0, _index.compile)(`{{if}}`, {
          moduleName: 'baz/foo-bar'
        });
      }, `The inline form of the 'if' helper expects two or three arguments. ('baz/foo-bar' @ L1:C0) `);
      (0, _index.compile)(`{{if foo bar baz}}`, {
        moduleName: 'baz/foo-bar'
      });
    }

    ['@test subexpression if helper expects between one and three arguments']() {
      expectAssertion(() => {
        (0, _index.compile)(`{{input foo=(if)}}`, {
          moduleName: 'baz/foo-bar'
        });
      }, `The inline form of the 'if' helper expects two or three arguments. ('baz/foo-bar' @ L1:C12) `);
      (0, _index.compile)(`{{some-thing foo=(if foo bar baz)}}`, {
        moduleName: 'baz/foo-bar'
      });
    }

  });
});
enifed("ember-template-compiler/tests/plugins/assert-input-helper-without-block-test", ["ember-template-compiler/index", "internal-test-helpers"], function (_index, _internalTestHelpers) {
  "use strict";

  (0, _internalTestHelpers.moduleFor)('ember-template-compiler: assert-input-helper-without-block', class extends _internalTestHelpers.AbstractTestCase {
    ['@test Using {{#input}}{{/input}} is not valid']() {
      let expectedMessage = `The {{input}} helper cannot be used in block form. ('baz/foo-bar' @ L1:C0) `;
      expectAssertion(() => {
        (0, _index.compile)('{{#input value="123"}}Completely invalid{{/input}}', {
          moduleName: 'baz/foo-bar'
        });
      }, expectedMessage);
    }

  });
});
enifed("ember-template-compiler/tests/plugins/assert-local-variable-shadowing-helper-invocation-test", ["ember-template-compiler/index", "internal-test-helpers"], function (_index, _internalTestHelpers) {
  "use strict";

  (0, _internalTestHelpers.moduleFor)('ember-template-compiler: assert-local-variable-shadowing-helper-invocation', class extends _internalTestHelpers.AbstractTestCase {
    [`@test block statements shadowing sub-expression invocations`]() {
      expectAssertion(() => {
        (0, _index.compile)(`
          {{#let foo as |foo|}}
            {{concat (foo)}}
          {{/let}}`, {
          moduleName: 'baz/foo-bar'
        });
      }, `Cannot invoke the \`foo\` helper because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict. ('baz/foo-bar' @ L3:C21) `);
      expectAssertion(() => {
        (0, _index.compile)(`
          {{#let foo as |foo|}}
            {{concat (foo bar baz)}}
          {{/let}}`, {
          moduleName: 'baz/foo-bar'
        });
      }, `Cannot invoke the \`foo\` helper because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict. ('baz/foo-bar' @ L3:C21) `); // Not shadowed

      (0, _index.compile)(`
        {{#let foo as |foo|}}{{/let}}
        {{concat (foo)}}
        {{concat (foo bar baz)}}`, {
        moduleName: 'baz/foo-bar'
      }); // Not invocations

      (0, _index.compile)(`
        {{#let foo as |foo|}}
          {{concat foo}}
        {{/let}}`, {
        moduleName: 'baz/foo-bar'
      });
      (0, _index.compile)(`
        {{#let (concat foo) as |concat|}}
          {{input value=concat}}
        {{/let}}`, {
        moduleName: 'baz/foo-bar'
      });
    }

    [`@test element nodes shadowing sub-expression invocations`]() {
      expectAssertion(() => {
        (0, _index.compile)(`
          <Foo as |foo|>
            {{concat (foo)}}
          </Foo>`, {
          moduleName: 'baz/foo-bar'
        });
      }, `Cannot invoke the \`foo\` helper because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict. ('baz/foo-bar' @ L3:C21) `);
      expectAssertion(() => {
        (0, _index.compile)(`
          <Foo as |foo|>
            {{concat (foo bar baz)}}
          </Foo>`, {
          moduleName: 'baz/foo-bar'
        });
      }, `Cannot invoke the \`foo\` helper because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict. ('baz/foo-bar' @ L3:C21) `); // Not shadowed

      (0, _index.compile)(`
        <Foo as |foo|></Foo>
        {{concat (foo)}}
        {{concat (foo bar baz)}}`, {
        moduleName: 'baz/foo-bar'
      }); // Not invocations

      (0, _index.compile)(`
        <Foo as |foo|>
          {{concat foo}}
        </Foo>`, {
        moduleName: 'baz/foo-bar'
      });
      (0, _index.compile)(`
        <Foo foo={{concat foo}} as |concat|>
          {{input value=concat}}
        </Foo>`, {
        moduleName: 'baz/foo-bar'
      });
    }

    [`@test deeply nested sub-expression invocations`]() {
      expectAssertion(() => {
        (0, _index.compile)(`
          {{#let foo as |foo|}}
            <FooBar as |bar|>
              {{#each items as |baz|}}
                {{concat (foo)}}
              {{/each}}
            </FooBar>
          {{/let}}`, {
          moduleName: 'baz/foo-bar'
        });
      }, `Cannot invoke the \`foo\` helper because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict. ('baz/foo-bar' @ L5:C25) `);
      expectAssertion(() => {
        (0, _index.compile)(`
          {{#let foo as |foo|}}
            <FooBar as |bar|>
              {{#each items as |baz|}}
                {{concat (foo bar baz)}}
              {{/each}}
            </FooBar>
          {{/let}}`, {
          moduleName: 'baz/foo-bar'
        });
      }, `Cannot invoke the \`foo\` helper because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict. ('baz/foo-bar' @ L5:C25) `); // Not shadowed

      (0, _index.compile)(`
        {{#let foo as |foo|}}
          <FooBar as |bar|>
            {{#each items as |baz|}}
            {{/each}}
            {{concat (baz)}}
            {{concat (baz bat)}}
          </FooBar>
          {{concat (bar)}}
          {{concat (bar baz bat)}}
        {{/let}}
        {{concat (foo)}}
        {{concat (foo bar baz bat)}}`, {
        moduleName: 'baz/foo-bar'
      }); // Not invocations

      (0, _index.compile)(`
        {{#let foo as |foo|}}
          <FooBar as |bar|>
            {{#each items as |baz|}}
              {{concat foo}}
            {{/each}}
          </FooBar>
        {{/let}}`, {
        moduleName: 'baz/foo-bar'
      });
      (0, _index.compile)(`
        {{#let (foo foo) as |foo|}}
          <FooBar bar=(bar bar) as |bar|>
            {{#each (baz baz) as |baz|}}
              {{concat foo bar baz}}
            {{/each}}
          </FooBar>
        {{/let}}`, {
        moduleName: 'baz/foo-bar'
      });
    }

    [`@test block statements shadowing attribute sub-expression invocations`]() {
      expectAssertion(() => {
        (0, _index.compile)(`
          {{#let foo as |foo|}}
            <div class={{concat (foo bar baz)}} />
          {{/let}}`, {
          moduleName: 'baz/foo-bar'
        });
      }, `Cannot invoke the \`foo\` helper because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict. ('baz/foo-bar' @ L3:C32) `); // Not shadowed

      (0, _index.compile)(`
        {{#let foo as |foo|}}{{/let}}
        <div class={{concat (foo)}} />
        <div class={{concat (foo bar baz)}} />`, {
        moduleName: 'baz/foo-bar'
      }); // Not invocations

      (0, _index.compile)(`
        {{#let foo as |foo|}}
          <div class={{concat foo}} />
        {{/let}}`, {
        moduleName: 'baz/foo-bar'
      });
      (0, _index.compile)(`
        {{#let (foo foo) as |foo|}}
          <div class={{concat foo}} />
        {{/let}}`, {
        moduleName: 'baz/foo-bar'
      });
    }

    [`@test element nodes shadowing attribute sub-expression invocations`]() {
      expectAssertion(() => {
        (0, _index.compile)(`
          <Foo as |foo|>
            <div class={{concat (foo bar baz)}} />
          </Foo>`, {
          moduleName: 'baz/foo-bar'
        });
      }, `Cannot invoke the \`foo\` helper because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict. ('baz/foo-bar' @ L3:C32) `); // Not shadowed

      (0, _index.compile)(`
        <Foo as |foo|></Foo>
        <div class={{concat (foo)}} />
        <div class={{concat (foo bar baz)}} />`, {
        moduleName: 'baz/foo-bar'
      }); // Not invocations

      (0, _index.compile)(`
        <Foo as |foo|>
          <div class={{concat foo}} />
        </Foo>`, {
        moduleName: 'baz/foo-bar'
      });
      (0, _index.compile)(`
        <Foo foo={{foo foo}} as |foo|>
          <div class={{concat foo}} />
        </Foo>`, {
        moduleName: 'baz/foo-bar'
      });
    }

    [`@test deeply nested attribute sub-expression invocations`]() {
      expectAssertion(() => {
        (0, _index.compile)(`
          {{#let foo as |foo|}}
            <FooBar as |bar|>
              {{#each items as |baz|}}
                <div class={{concat (foo bar baz)}} />
              {{/each}}
            </FooBar>
          {{/let}}`, {
          moduleName: 'baz/foo-bar'
        });
      }, `Cannot invoke the \`foo\` helper because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict. ('baz/foo-bar' @ L5:C36) `); // Not shadowed

      (0, _index.compile)(`
        {{#let foo as |foo|}}
          <FooBar as |bar|>
            {{#each items as |baz|}}
            {{/each}}
            <div class={{concat (baz)}} />
            <div class={{concat (baz bat)}} />
          </FooBar>
          <div class={{concat (bar)}} />
          <div class={{concat (bar baz bat)}} />
        {{/let}}
        <div class={{concat (foo)}} />
        <div class={{concat (foo bar baz bat)}} />`, {
        moduleName: 'baz/foo-bar'
      }); // Not invocations

      (0, _index.compile)(`
        {{#let foo as |foo|}}
          <FooBar as |bar|>
            {{#each items as |baz|}}
              <div class={{concat foo}} />
            {{/each}}
          </FooBar>
        {{/let}}`, {
        moduleName: 'baz/foo-bar'
      });
      (0, _index.compile)(`
        {{#let (foo foo) as |foo|}}
          <FooBar bar=(bar bar) as |bar|>
            {{#each (baz baz) as |baz|}}
              <div class={{concat foo bar baz}} />
            {{/each}}
          </FooBar>
        {{/let}}`, {
        moduleName: 'baz/foo-bar'
      });
    }

    [`@test block statements shadowing attribute mustache invocations`]() {
      expectAssertion(() => {
        (0, _index.compile)(`
          {{#let foo as |foo|}}
            <div class={{foo bar baz}} />
          {{/let}}`, {
          moduleName: 'baz/foo-bar'
        });
      }, `Cannot invoke the \`foo\` helper because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict. ('baz/foo-bar' @ L3:C23) `); // Not shadowed

      (0, _index.compile)(`
        {{#let foo as |foo|}}{{/let}}
        <div class={{foo}} />
        <div class={{foo bar baz}} />`, {
        moduleName: 'baz/foo-bar'
      }); // Not invocations

      (0, _index.compile)(`
        {{#let foo as |foo|}}
          <div class={{foo}} />
        {{/let}}`, {
        moduleName: 'baz/foo-bar'
      });
      (0, _index.compile)(`
        {{#let (concat foo) as |concat|}}
          <div class={{concat}} />
        {{/let}}`, {
        moduleName: 'baz/foo-bar'
      });
    }

    [`@test element nodes shadowing attribute mustache invocations`]() {
      expectAssertion(() => {
        (0, _index.compile)(`
          <Foo as |foo|>
            <div class={{foo bar baz}} />
          </Foo>`, {
          moduleName: 'baz/foo-bar'
        });
      }, `Cannot invoke the \`foo\` helper because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict. ('baz/foo-bar' @ L3:C23) `); // Not shadowed

      (0, _index.compile)(`
        <Foo as |foo|></Foo>
        <div class={{foo}} />
        <div class={{foo bar baz}} />`, {
        moduleName: 'baz/foo-bar'
      }); // Not invocations

      (0, _index.compile)(`
        <Foo as |foo|>
          <div class={{foo}} />
        </Foo>`, {
        moduleName: 'baz/foo-bar'
      });
      (0, _index.compile)(`
        <Foo foo={{concat foo}} as |concat|>
          <div class={{concat}} />
        </Foo>`, {
        moduleName: 'baz/foo-bar'
      });
    }

    [`@test deeply nested attribute mustache invocations`]() {
      expectAssertion(() => {
        (0, _index.compile)(`
          {{#let foo as |foo|}}
            <FooBar as |bar|>
              {{#each items as |baz|}}
                <div class={{foo bar baz}} />
              {{/each}}
            </FooBar>
          {{/let}}`, {
          moduleName: 'baz/foo-bar'
        });
      }, `Cannot invoke the \`foo\` helper because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict. ('baz/foo-bar' @ L5:C27) `); // Not shadowed

      (0, _index.compile)(`
        {{#let foo as |foo|}}
          <FooBar as |bar|>
            {{#each items as |baz|}}
            {{/each}}
            <div class={{baz}} />
            <div class={{baz bat}} />
          </FooBar>
          <div class={{bar}} />
          <div class={{bar baz bat}} />
        {{/let}}
        <div class={{foo}} />
        <div class={{foo bar baz bat}} />`, {
        moduleName: 'baz/foo-bar'
      }); // Not invocations

      (0, _index.compile)(`
        {{#let foo as |foo|}}
          <FooBar as |bar|>
            {{#each items as |baz|}}
              <div class={{foo}} />
            {{/each}}
          </FooBar>
        {{/let}}`, {
        moduleName: 'baz/foo-bar'
      });
      (0, _index.compile)(`
        {{#let (foo foo) as |foo|}}
          <FooBar bar=(bar bar) as |bar|>
            {{#each (baz baz) as |baz|}}
              <div foo={{foo}} bar={{bar}} baz={{baz}} />
            {{/each}}
          </FooBar>
        {{/let}}`, {
        moduleName: 'baz/foo-bar'
      });
    }

    [`@test block statements shadowing mustache invocations`](assert) {
      // These are fine, because they should already be considered contextual
      // component invocations, not helper invocations
      assert.expect(0);
      (0, _index.compile)(`
        {{#let foo as |foo|}}
          {{foo}}
        {{/let}}`, {
        moduleName: 'baz/foo-bar'
      });
      (0, _index.compile)(`
        {{#let foo as |foo|}}
          {{foo bar baz}}
        {{/let}}`, {
        moduleName: 'baz/foo-bar'
      });
    }

    [`@test element nodes shadowing mustache invocations`](assert) {
      // These are fine, because they should already be considered contextual
      // component invocations, not helper invocations
      assert.expect(0);
      (0, _index.compile)(`
        <Foo as |foo|>
          {{foo}}
        </Foo>`, {
        moduleName: 'baz/foo-bar'
      });
      (0, _index.compile)(`
        <Foo as |foo|>
          {{foo bar baz}}
        </Foo>`, {
        moduleName: 'baz/foo-bar'
      });
    }

    [`@test deeply nested mustache invocations`](assert) {
      // These are fine, because they should already be considered contextual
      // component invocations, not helper invocations
      assert.expect(0);
      (0, _index.compile)(`
        {{#let foo as |foo|}}
          <FooBar as |bar|>
            {{#each items as |baz|}}
              {{foo}}
            {{/each}}
          </FooBar>
        {{/let}}`, {
        moduleName: 'baz/foo-bar'
      });
      (0, _index.compile)(`
        {{#let foo as |foo|}}
          <FooBar as |bar|>
            {{#each items as |baz|}}
              {{foo bar baz}}
            {{/each}}
          </FooBar>
        {{/let}}`, {
        moduleName: 'baz/foo-bar'
      });
    }

    [`@test block statements shadowing modifier invocations`]() {
      expectAssertion(() => {
        (0, _index.compile)(`
          {{#let foo as |foo|}}
            <div {{foo}} />
          {{/let}}`, {
          moduleName: 'baz/foo-bar'
        });
      }, `Cannot invoke the \`foo\` modifier because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict. ('baz/foo-bar' @ L3:C17) `);
      expectAssertion(() => {
        (0, _index.compile)(`
          {{#let foo as |foo|}}
            <div {{foo bar baz}} />
          {{/let}}`, {
          moduleName: 'baz/foo-bar'
        });
      }, `Cannot invoke the \`foo\` modifier because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict. ('baz/foo-bar' @ L3:C17) `); // Not shadowed

      (0, _index.compile)(`
        {{#let foo as |foo|}}{{/let}}
        <div {{foo}} />
        <div {{foo bar baz}} />`, {
        moduleName: 'baz/foo-bar'
      });
    }

    [`@test element nodes shadowing modifier invocations`]() {
      expectAssertion(() => {
        (0, _index.compile)(`
          <Foo as |foo|>
            <div {{foo}} />
          </Foo>`, {
          moduleName: 'baz/foo-bar'
        });
      }, `Cannot invoke the \`foo\` modifier because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict. ('baz/foo-bar' @ L3:C17) `);
      expectAssertion(() => {
        (0, _index.compile)(`
          <Foo as |foo|>
            <div {{foo bar baz}} />
          </Foo>`, {
          moduleName: 'baz/foo-bar'
        });
      }, `Cannot invoke the \`foo\` modifier because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict. ('baz/foo-bar' @ L3:C17) `); // Not shadowed

      (0, _index.compile)(`
        <Foo as |foo|></Foo>
        <div {{foo}} />
        <div {{foo bar baz}} />`, {
        moduleName: 'baz/foo-bar'
      });
    }

    [`@test deeply nested modifier invocations`]() {
      expectAssertion(() => {
        (0, _index.compile)(`
          {{#let foo as |foo|}}
            <FooBar as |bar|>
              {{#each items as |baz|}}
                <div {{foo}} />
              {{/each}}
            </FooBar>
          {{/let}}`, {
          moduleName: 'baz/foo-bar'
        });
      }, `Cannot invoke the \`foo\` modifier because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict. ('baz/foo-bar' @ L5:C21) `);
      expectAssertion(() => {
        (0, _index.compile)(`
          {{#let foo as |foo|}}
            <FooBar as |bar|>
              {{#each items as |baz|}}
                <div {{foo bar baz}} />
              {{/each}}
            </FooBar>
          {{/let}}`, {
          moduleName: 'baz/foo-bar'
        });
      }, `Cannot invoke the \`foo\` modifier because it was shadowed by a local variable (i.e. a block param) with the same name. Please rename the local variable to resolve the conflict. ('baz/foo-bar' @ L5:C21) `); // Not shadowed

      (0, _index.compile)(`
        {{#let foo as |foo|}}
          <FooBar as |bar|>
            {{#each items as |baz|}}
            {{/each}}
            <div {{baz}} />
            <div {{baz bat}} />
          </FooBar>
          <div {{bar}} />
          <div {{bar baz bat}} />
        {{/let}}
        <div {{foo}} />
        <div {{foo bar baz bat}} />`, {
        moduleName: 'baz/foo-bar'
      });
    }

  });
});
enifed("ember-template-compiler/tests/plugins/assert-reserved-named-arguments-test", ["ember-template-compiler/index", "internal-test-helpers"], function (_index, _internalTestHelpers) {
  "use strict";

  if (true
  /* EMBER_GLIMMER_NAMED_ARGUMENTS */
  ) {
      (0, _internalTestHelpers.moduleFor)('ember-template-compiler: assert-reserved-named-arguments (EMBER_GLIMMER_NAMED_ARGUMENTS) ', class extends _internalTestHelpers.AbstractTestCase {
        [`@test '@arguments' is reserved`]() {
          expectAssertion(() => {
            (0, _index.compile)(`{{@arguments}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@arguments' is reserved. ('baz/foo-bar' @ L1:C2) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{#if @arguments}}Yup{{/if}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@arguments' is reserved. ('baz/foo-bar' @ L1:C6) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{input type=(if @arguments "bar" "baz")}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@arguments' is reserved. ('baz/foo-bar' @ L1:C17) `);
        }

        [`@test '@args' is reserved`]() {
          expectAssertion(() => {
            (0, _index.compile)(`{{@args}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@args' is reserved. ('baz/foo-bar' @ L1:C2) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{#if @args}}Yup{{/if}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@args' is reserved. ('baz/foo-bar' @ L1:C6) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{input type=(if @args "bar" "baz")}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@args' is reserved. ('baz/foo-bar' @ L1:C17) `);
        }

        [`@test '@block' is reserved`]() {
          expectAssertion(() => {
            (0, _index.compile)(`{{@block}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@block' is reserved. ('baz/foo-bar' @ L1:C2) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{#if @block}}Yup{{/if}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@block' is reserved. ('baz/foo-bar' @ L1:C6) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{input type=(if @block "bar" "baz")}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@block' is reserved. ('baz/foo-bar' @ L1:C17) `);
        }

        [`@test '@else' is reserved`]() {
          expectAssertion(() => {
            (0, _index.compile)(`{{@else}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@else' is reserved. ('baz/foo-bar' @ L1:C2) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{#if @else}}Yup{{/if}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@else' is reserved. ('baz/foo-bar' @ L1:C6) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{input type=(if @else "bar" "baz")}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@else' is reserved. ('baz/foo-bar' @ L1:C17) `);
        } // anything else that doesn't start with a lower case letter


        [`@test '@Arguments' is reserved`]() {
          expectAssertion(() => {
            (0, _index.compile)(`{{@Arguments}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@Arguments' is reserved. ('baz/foo-bar' @ L1:C2) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{#if @Arguments}}Yup{{/if}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@Arguments' is reserved. ('baz/foo-bar' @ L1:C6) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{input type=(if @Arguments "bar" "baz")}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@Arguments' is reserved. ('baz/foo-bar' @ L1:C17) `);
        }

        [`@test '@Args' is reserved`]() {
          expectAssertion(() => {
            (0, _index.compile)(`{{@Args}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@Args' is reserved. ('baz/foo-bar' @ L1:C2) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{#if @Args}}Yup{{/if}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@Args' is reserved. ('baz/foo-bar' @ L1:C6) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{input type=(if @Args "bar" "baz")}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@Args' is reserved. ('baz/foo-bar' @ L1:C17) `);
        }

        [`@test '@FOO' is reserved`]() {
          expectAssertion(() => {
            (0, _index.compile)(`{{@FOO}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@FOO' is reserved. ('baz/foo-bar' @ L1:C2) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{#if @FOO}}Yup{{/if}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@FOO' is reserved. ('baz/foo-bar' @ L1:C6) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{input type=(if @FOO "bar" "baz")}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@FOO' is reserved. ('baz/foo-bar' @ L1:C17) `);
        }

        [`@test '@Foo' is reserved`]() {
          expectAssertion(() => {
            (0, _index.compile)(`{{@Foo}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@Foo' is reserved. ('baz/foo-bar' @ L1:C2) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{#if @Foo}}Yup{{/if}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@Foo' is reserved. ('baz/foo-bar' @ L1:C6) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{input type=(if @Foo "bar" "baz")}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@Foo' is reserved. ('baz/foo-bar' @ L1:C17) `);
        }

        [`@test '@.' is reserved`]() {
          expectAssertion(() => {
            (0, _index.compile)(`{{@.}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@.' is reserved. ('baz/foo-bar' @ L1:C2) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{#if @.}}Yup{{/if}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@.' is reserved. ('baz/foo-bar' @ L1:C6) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{input type=(if @. "bar" "baz")}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@.' is reserved. ('baz/foo-bar' @ L1:C17) `);
        }

        [`@test '@_' is reserved`]() {
          expectAssertion(() => {
            (0, _index.compile)(`{{@_}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@_' is reserved. ('baz/foo-bar' @ L1:C2) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{#if @_}}Yup{{/if}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@_' is reserved. ('baz/foo-bar' @ L1:C6) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{input type=(if @_ "bar" "baz")}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@_' is reserved. ('baz/foo-bar' @ L1:C17) `);
        }

        [`@test '@-' is reserved`]() {
          expectAssertion(() => {
            (0, _index.compile)(`{{@-}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@-' is reserved. ('baz/foo-bar' @ L1:C2) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{#if @-}}Yup{{/if}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@-' is reserved. ('baz/foo-bar' @ L1:C6) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{input type=(if @- "bar" "baz")}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@-' is reserved. ('baz/foo-bar' @ L1:C17) `);
        }

        [`@test '@$' is reserved`]() {
          expectAssertion(() => {
            (0, _index.compile)(`{{@$}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@$' is reserved. ('baz/foo-bar' @ L1:C2) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{#if @$}}Yup{{/if}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@$' is reserved. ('baz/foo-bar' @ L1:C6) `);
          expectAssertion(() => {
            (0, _index.compile)(`{{input type=(if @$ "bar" "baz")}}`, {
              moduleName: 'baz/foo-bar'
            });
          }, `'@$' is reserved. ('baz/foo-bar' @ L1:C17) `);
        }

        [`@test '@' is de facto reserved (parse error)`](assert) {
          assert.throws(() => {
            (0, _index.compile)('{{@}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
          assert.throws(() => {
            (0, _index.compile)('{{#if @}}Yup{{/if}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
          assert.throws(() => {
            (0, _index.compile)('{{input type=(if @ "bar" "baz")}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
        }

        [`@test '@0' is de facto reserved (parse error)`](assert) {
          assert.throws(() => {
            (0, _index.compile)('{{@0}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
          assert.throws(() => {
            (0, _index.compile)('{{#if @0}}Yup{{/if}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
          assert.throws(() => {
            (0, _index.compile)('{{input type=(if @0 "bar" "baz")}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
        }

        [`@test '@1' is de facto reserved (parse error)`](assert) {
          assert.throws(() => {
            (0, _index.compile)('{{@1}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
          assert.throws(() => {
            (0, _index.compile)('{{#if @1}}Yup{{/if}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
          assert.throws(() => {
            (0, _index.compile)('{{input type=(if @1 "bar" "baz")}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
        }

        [`@test '@2' is de facto reserved (parse error)`](assert) {
          assert.throws(() => {
            (0, _index.compile)('{{@2}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
          assert.throws(() => {
            (0, _index.compile)('{{#if @2}}Yup{{/if}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
          assert.throws(() => {
            (0, _index.compile)('{{input type=(if @2 "bar" "baz")}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
        }

        [`@test '@@' is de facto reserved (parse error)`](assert) {
          assert.throws(() => {
            (0, _index.compile)('{{@@}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
          assert.throws(() => {
            (0, _index.compile)('{{#if @@}}Yup{{/if}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
          assert.throws(() => {
            (0, _index.compile)('{{input type=(if @@ "bar" "baz")}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
        }

        [`@test '@=' is de facto reserved (parse error)`](assert) {
          assert.throws(() => {
            (0, _index.compile)('{{@=}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
          assert.throws(() => {
            (0, _index.compile)('{{#if @=}}Yup{{/if}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
          assert.throws(() => {
            (0, _index.compile)('{{input type=(if @= "bar" "baz")}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
        }

        [`@test '@!' is de facto reserved (parse error)`](assert) {
          assert.throws(() => {
            (0, _index.compile)('{{@!}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
          assert.throws(() => {
            (0, _index.compile)('{{#if @!}}Yup{{/if}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
          assert.throws(() => {
            (0, _index.compile)('{{input type=(if @! "bar" "baz")}}', {
              moduleName: 'baz/foo-bar'
            });
          }, /Expecting 'ID'/);
        }

      });
    } else {
    (0, _internalTestHelpers.moduleFor)('ember-template-compiler: assert-reserved-named-arguments', class extends _internalTestHelpers.AbstractTestCase {
      ['@test Paths beginning with @ are not valid']() {
        expectAssertion(() => {
          (0, _index.compile)('{{@foo}}', {
            moduleName: 'baz/foo-bar'
          });
        }, `'@foo' is not a valid path. ('baz/foo-bar' @ L1:C2) `);
        expectAssertion(() => {
          (0, _index.compile)('{{#if @foo}}Yup{{/if}}', {
            moduleName: 'baz/foo-bar'
          });
        }, `'@foo' is not a valid path. ('baz/foo-bar' @ L1:C6) `);
        expectAssertion(() => {
          (0, _index.compile)('{{input type=(if @foo "bar" "baz")}}', {
            moduleName: 'baz/foo-bar'
          });
        }, `'@foo' is not a valid path. ('baz/foo-bar' @ L1:C17) `);
      }

    });
  }
});
enifed("ember-template-compiler/tests/plugins/assert-splattribute-expression-test", ["internal-test-helpers", "ember-template-compiler/index"], function (_internalTestHelpers, _index) {
  "use strict";

  (0, _internalTestHelpers.moduleFor)('ember-template-compiler: assert-splattribute-expression', class extends _internalTestHelpers.AbstractTestCase {
    expectedMessage(locInfo) {
      return true
      /* EMBER_GLIMMER_ANGLE_BRACKET_INVOCATION */
      ? `Using "...attributes" can only be used in the element position e.g. <div ...attributes />. It cannot be used as a path. (${locInfo}) ` : `...attributes is an invalid path (${locInfo}) `;
    }

    '@test ...attributes is in element space'(assert) {
      if (true
      /* EMBER_GLIMMER_ANGLE_BRACKET_INVOCATION */
      ) {
          assert.expect(0);
          (0, _index.compile)('<div ...attributes>Foo</div>');
        } else {
        expectAssertion(() => {
          (0, _index.compile)('<div ...attributes>Foo</div>');
        }, this.expectedMessage('L1:C5'));
      }
    }

    '@test {{...attributes}} is not valid'() {
      expectAssertion(() => {
        (0, _index.compile)('<div>{{...attributes}}</div>', {
          moduleName: 'foo-bar'
        });
      }, this.expectedMessage(`'foo-bar' @ L1:C7`));
    }

    '@test {{...attributes}} is not valid path expression'() {
      expectAssertion(() => {
        (0, _index.compile)('<div>{{...attributes}}</div>', {
          moduleName: 'foo-bar'
        });
      }, this.expectedMessage(`'foo-bar' @ L1:C7`));
    }

    '@test {{...attributes}} is not valid modifier'() {
      expectAssertion(() => {
        (0, _index.compile)('<div {{...attributes}}>Wat</div>', {
          moduleName: 'foo-bar'
        });
      }, this.expectedMessage(`'foo-bar' @ L1:C7`));
    }

    '@test {{...attributes}} is not valid attribute'() {
      expectAssertion(() => {
        (0, _index.compile)('<div class={{...attributes}}>Wat</div>', {
          moduleName: 'foo-bar'
        });
      }, this.expectedMessage(`'foo-bar' @ L1:C13`));
    }

  });
});
enifed("ember-template-compiler/tests/plugins/deprecate-send-action-test", ["ember-template-compiler/index", "internal-test-helpers"], function (_index, _internalTestHelpers) {
  "use strict";

  const EVENTS = ['insert-newline', 'enter', 'escape-press', 'focus-in', 'focus-out', 'key-press', 'key-up', 'key-down'];

  class DeprecateSendActionTest extends _internalTestHelpers.AbstractTestCase {}

  EVENTS.forEach(function (e) {
    DeprecateSendActionTest.prototype[`@test Using \`{{input ${e}="actionName"}}\` provides a deprecation`] = function () {
      let expectedMessage = `Please refactor \`{{input ${e}="foo-bar"}}\` to \`{{input ${e}=(action "foo-bar")}}\. ('baz/foo-bar' @ L1:C0) `;
      expectDeprecation(() => {
        (0, _index.compile)(`{{input ${e}="foo-bar"}}`, {
          moduleName: 'baz/foo-bar'
        });
      }, expectedMessage);
    };
  });
  (0, _internalTestHelpers.moduleFor)('ember-template-compiler: deprecate-send-action', DeprecateSendActionTest);
});
enifed("ember-template-compiler/tests/plugins/transform-component-invocation-test", ["ember-template-compiler/index", "internal-test-helpers"], function (_index, _internalTestHelpers) {
  "use strict";

  (0, _internalTestHelpers.moduleFor)('ember-template-compiler: transforms component invocation', class extends _internalTestHelpers.AbstractTestCase {
    ['@test Does not throw a compiler error for component invocations'](assert) {
      assert.expect(0);
      ['{{this.modal open}}', '{{this.modal isOpen=true}}', '{{#this.modal}}Woot{{/this.modal}}', '{{@modal open}}', // RFC#311
      '{{@modal isOpen=true}}', // RFC#311
      '{{#@modal}}Woot{{/@modal}}', // RFC#311
      '{{c.modal open}}', '{{c.modal isOpen=true}}', '{{#c.modal}}Woot{{/c.modal}}', '{{#my-component as |c|}}{{c name="Chad"}}{{/my-component}}', // RFC#311
      '{{#my-component as |c|}}{{c "Chad"}}{{/my-component}}', // RFC#311
      '{{#my-component as |c|}}{{#c}}{{/c}}{{/my-component}}', // RFC#311
      '<input disabled={{true}}>', // GH#15740
      '<td colspan={{3}}></td>'].forEach((layout, i) => {
        (0, _index.compile)(layout, {
          moduleName: `example-${i}`
        });
      });
    }

  });
});
enifed("ember-template-compiler/tests/plugins/transform-inline-link-to-test", ["ember-template-compiler/index", "internal-test-helpers"], function (_index, _internalTestHelpers) {
  "use strict";

  (0, _internalTestHelpers.moduleFor)('ember-template-compiler: inline-link-to', class extends _internalTestHelpers.AbstractTestCase {
    ['@test Can transform an inline {{link-to}} without error'](assert) {
      assert.expect(0);
      (0, _index.compile)(`{{link-to 'foo' 'index'}}`, {
        moduleName: 'foo/bar/baz'
      });
    }

  });
});
enifed("ember-template-compiler/tests/plugins/transform-input-type-syntax-test", ["ember-template-compiler/index", "internal-test-helpers"], function (_index, _internalTestHelpers) {
  "use strict";

  (0, _internalTestHelpers.moduleFor)('ember-template-compiler: input type syntax', class extends _internalTestHelpers.AbstractTestCase {
    ['@test Can compile an {{input}} helper that has a sub-expression value as its type'](assert) {
      assert.expect(0);
      (0, _index.compile)(`{{input type=(if true 'password' 'text')}}`);
    }

    ['@test Can compile an {{input}} helper with a string literal type'](assert) {
      assert.expect(0);
      (0, _index.compile)(`{{input type='text'}}`);
    }

    ['@test Can compile an {{input}} helper with a type stored in a var'](assert) {
      assert.expect(0);
      (0, _index.compile)(`{{input type=_type}}`);
    }

  });
});
enifed("ember-template-compiler/tests/system/bootstrap-test", ["@ember/runloop", "@ember/-internals/glimmer", "ember-template-compiler/lib/system/bootstrap", "internal-test-helpers"], function (_runloop, _glimmer, _bootstrap, _internalTestHelpers) {
  "use strict";

  let component, fixture;

  function checkTemplate(templateName, assert) {
    (0, _runloop.run)(() => (0, _bootstrap.default)({
      context: fixture,
      hasTemplate: _glimmer.hasTemplate,
      setTemplate: _glimmer.setTemplate
    }));
    let template = (0, _glimmer.getTemplate)(templateName);
    let qunitFixture = document.querySelector('#qunit-fixture');
    assert.ok(template, 'template is available on Ember.TEMPLATES');
    assert.notOk(qunitFixture.querySelector('script'), 'script removed');
    let owner = (0, _internalTestHelpers.buildOwner)();
    owner.register('template:-top-level', template);
    owner.register('component:-top-level', _glimmer.Component.extend({
      layoutName: '-top-level',
      firstName: 'Tobias',
      drug: 'teamocil'
    }));
    component = owner.lookup('component:-top-level');
    (0, _internalTestHelpers.runAppend)(component);
    assert.equal(qunitFixture.textContent.trim(), 'Tobias takes teamocil', 'template works');
    (0, _internalTestHelpers.runDestroy)(owner);
  }

  (0, _internalTestHelpers.moduleFor)('ember-templates: bootstrap', class extends _internalTestHelpers.AbstractTestCase {
    constructor() {
      super();
      fixture = document.getElementById('qunit-fixture');
    }

    teardown() {
      (0, _glimmer.setTemplates)({});
      fixture = component = null;
    }

    ['@test template with data-template-name should add a new template to Ember.TEMPLATES'](assert) {
      fixture.innerHTML = '<script type="text/x-handlebars" data-template-name="funkyTemplate">{{firstName}} takes {{drug}}</script>';
      checkTemplate('funkyTemplate', assert);
    }

    ['@test template with id instead of data-template-name should add a new template to Ember.TEMPLATES'](assert) {
      fixture.innerHTML = '<script type="text/x-handlebars" id="funkyTemplate" >{{firstName}} takes {{drug}}</script>';
      checkTemplate('funkyTemplate', assert);
    }

    ['@test template without data-template-name or id should default to application'](assert) {
      fixture.innerHTML = '<script type="text/x-handlebars">{{firstName}} takes {{drug}}</script>';
      checkTemplate('application', assert);
    } // Add this test case, only for typeof Handlebars === 'object';


    [`${typeof Handlebars === 'object' ? '@test' : '@skip'} template with type text/x-raw-handlebars should be parsed`](assert) {
      fixture.innerHTML = '<script type="text/x-raw-handlebars" data-template-name="funkyTemplate">{{name}}</script>';
      (0, _runloop.run)(() => (0, _bootstrap.default)({
        context: fixture,
        hasTemplate: _glimmer.hasTemplate,
        setTemplate: _glimmer.setTemplate
      }));
      let template = (0, _glimmer.getTemplate)('funkyTemplate');
      assert.ok(template, 'template with name funkyTemplate available'); // This won't even work with Ember templates

      assert.equal(template({
        name: 'Tobias'
      }).trim(), 'Tobias');
    }

    ['@test duplicated default application templates should throw exception'](assert) {
      fixture.innerHTML = '<script type="text/x-handlebars">first</script><script type="text/x-handlebars">second</script>';
      assert.throws(() => (0, _bootstrap.default)({
        context: fixture,
        hasTemplate: _glimmer.hasTemplate,
        setTemplate: _glimmer.setTemplate
      }), /Template named "[^"]+" already exists\./, 'duplicate templates should not be allowed');
    }

    ['@test default default application template and id application template present should throw exception'](assert) {
      fixture.innerHTML = '<script type="text/x-handlebars">first</script><script type="text/x-handlebars" id="application">second</script>';
      assert.throws(() => (0, _bootstrap.default)({
        context: fixture,
        hasTemplate: _glimmer.hasTemplate,
        setTemplate: _glimmer.setTemplate
      }), /Template named "[^"]+" already exists\./, 'duplicate templates should not be allowed');
    }

    ['@test default application template and data-template-name application template present should throw exception'](assert) {
      fixture.innerHTML = '<script type="text/x-handlebars">first</script><script type="text/x-handlebars" data-template-name="application">second</script>';
      assert.throws(() => (0, _bootstrap.default)({
        context: fixture,
        hasTemplate: _glimmer.hasTemplate,
        setTemplate: _glimmer.setTemplate
      }), /Template named "[^"]+" already exists\./, 'duplicate templates should not be allowed');
    }

    ['@test duplicated template id should throw exception'](assert) {
      fixture.innerHTML = '<script type="text/x-handlebars" id="funkyTemplate">first</script><script type="text/x-handlebars" id="funkyTemplate">second</script>';
      assert.throws(() => (0, _bootstrap.default)({
        context: fixture,
        hasTemplate: _glimmer.hasTemplate,
        setTemplate: _glimmer.setTemplate
      }), /Template named "[^"]+" already exists\./, 'duplicate templates should not be allowed');
    }

    ['@test duplicated template data-template-name should throw exception'](assert) {
      fixture.innerHTML = '<script type="text/x-handlebars" data-template-name="funkyTemplate">first</script><script type="text/x-handlebars" data-template-name="funkyTemplate">second</script>';
      assert.throws(() => (0, _bootstrap.default)({
        context: fixture,
        hasTemplate: _glimmer.hasTemplate,
        setTemplate: _glimmer.setTemplate
      }), /Template named "[^"]+" already exists\./, 'duplicate templates should not be allowed');
    }

  });
});
enifed("ember-template-compiler/tests/system/compile_options_test", ["ember-template-compiler/index", "internal-test-helpers"], function (_index, _internalTestHelpers) {
  "use strict";

  (0, _internalTestHelpers.moduleFor)('ember-template-compiler: default compile options', class extends _internalTestHelpers.AbstractTestCase {
    ['@test default options are a new copy'](assert) {
      assert.notEqual((0, _index.compileOptions)(), (0, _index.compileOptions)());
    }

    ['@test has default AST plugins'](assert) {
      assert.expect(_index.defaultPlugins.length);
      let plugins = (0, _index.compileOptions)().plugins.ast;

      for (let i = 0; i < _index.defaultPlugins.length; i++) {
        let plugin = _index.defaultPlugins[i];
        assert.ok(plugins.indexOf(plugin) > -1, `includes ${plugin}`);
      }
    }

  });
  let customTransformCounter = 0;

  class LegacyCustomTransform {
    constructor(options) {
      customTransformCounter++;
      this.options = options;
      this.syntax = null;
    }

    transform(ast) {
      let walker = new this.syntax.Walker();
      walker.visit(ast, node => {
        if (node.type !== 'ElementNode') {
          return;
        }

        for (var i = 0; i < node.attributes.length; i++) {
          let attribute = node.attributes[i];

          if (attribute.name === 'data-test') {
            node.attributes.splice(i, 1);
          }
        }
      });
      return ast;
    }

  }

  function customTransform() {
    customTransformCounter++;
    return {
      name: 'remove-data-test',
      visitor: {
        ElementNode(node) {
          for (var i = 0; i < node.attributes.length; i++) {
            let attribute = node.attributes[i];

            if (attribute.name === 'data-test') {
              node.attributes.splice(i, 1);
            }
          }
        }

      }
    };
  }

  class CustomPluginsTests extends _internalTestHelpers.RenderingTestCase {
    afterEach() {
      customTransformCounter = 0;
      return super.afterEach();
    }

    ['@test custom plugins can be used']() {
      this.render('<div data-test="foo" data-blah="derp" class="hahaha"></div>');
      this.assertElement(this.firstChild, {
        tagName: 'div',
        attrs: {
          class: 'hahaha',
          'data-blah': 'derp'
        },
        content: ''
      });
    }

    ['@test wrapped plugins are only invoked once per template'](assert) {
      this.render('<div>{{#if falsey}}nope{{/if}}</div>');
      assert.equal(customTransformCounter, 1, 'transform should only be instantiated once');
    }

  }

  (0, _internalTestHelpers.moduleFor)('ember-template-compiler: registerPlugin with a custom plugins in legacy format', class extends CustomPluginsTests {
    beforeEach() {
      (0, _index.registerPlugin)('ast', LegacyCustomTransform);
    }

    afterEach() {
      (0, _index.unregisterPlugin)('ast', LegacyCustomTransform);
      return super.afterEach();
    }

    ['@test custom registered plugins are deduplicated'](assert) {
      (0, _index.registerPlugin)('ast', LegacyCustomTransform);
      this.registerTemplate('application', '<div data-test="foo" data-blah="derp" class="hahaha"></div>');
      assert.equal(customTransformCounter, 1, 'transform should only be instantiated once');
    }

  });
  (0, _internalTestHelpers.moduleFor)('ember-template-compiler: registerPlugin with a custom plugins', class extends CustomPluginsTests {
    beforeEach() {
      (0, _index.registerPlugin)('ast', customTransform);
    }

    afterEach() {
      (0, _index.unregisterPlugin)('ast', customTransform);
      return super.afterEach();
    }

    ['@test custom registered plugins are deduplicated'](assert) {
      (0, _index.registerPlugin)('ast', customTransform);
      this.registerTemplate('application', '<div data-test="foo" data-blah="derp" class="hahaha"></div>');
      assert.equal(customTransformCounter, 1, 'transform should only be instantiated once');
    }

  });
  (0, _internalTestHelpers.moduleFor)('ember-template-compiler: custom plugins in legacy format passed to compile', class extends _internalTestHelpers.RenderingTestCase {
    // override so that we can provide custom AST plugins to compile
    compile(templateString) {
      return (0, _index.compile)(templateString, {
        plugins: {
          ast: [LegacyCustomTransform]
        }
      });
    }

  });
  (0, _internalTestHelpers.moduleFor)('ember-template-compiler: custom plugins passed to compile', class extends _internalTestHelpers.RenderingTestCase {
    // override so that we can provide custom AST plugins to compile
    compile(templateString) {
      return (0, _index.compile)(templateString, {
        plugins: {
          ast: [customTransform]
        }
      });
    }

  });
});
enifed("ember-template-compiler/tests/system/dasherize-component-name-test", ["ember-template-compiler/lib/system/dasherize-component-name", "internal-test-helpers"], function (_dasherizeComponentName, _internalTestHelpers) {
  "use strict";

  (0, _internalTestHelpers.moduleFor)('dasherize-component-name', class extends _internalTestHelpers.AbstractTestCase {
    ['@test names are correctly dasherized'](assert) {
      assert.equal(_dasherizeComponentName.default.get('Foo'), 'foo');
      assert.equal(_dasherizeComponentName.default.get('foo-bar'), 'foo-bar');
      assert.equal(_dasherizeComponentName.default.get('FooBar'), 'foo-bar');
      assert.equal(_dasherizeComponentName.default.get('F3Bar'), 'f3-bar');
      assert.equal(_dasherizeComponentName.default.get('Foo3Bar'), 'foo3-bar');
      assert.equal(_dasherizeComponentName.default.get('Foo3barBaz'), 'foo3bar-baz');
      assert.equal(_dasherizeComponentName.default.get('FooB3ar'), 'foo-b3ar');
      assert.equal(_dasherizeComponentName.default.get('XBlah'), 'x-blah');
      assert.equal(_dasherizeComponentName.default.get('X-Blah'), 'x-blah');
      assert.equal(_dasherizeComponentName.default.get('Foo::BarBaz'), 'foo::bar-baz');
      assert.equal(_dasherizeComponentName.default.get('Foo::Bar-Baz'), 'foo::bar-baz');
      assert.equal(_dasherizeComponentName.default.get('Foo@BarBaz'), 'foo@bar-baz');
      assert.equal(_dasherizeComponentName.default.get('Foo@Bar-Baz'), 'foo@bar-baz');
    }

  });
});
enifed("ember/version", ["exports"], function (_exports) {
  "use strict";

  _exports.default = void 0;
  var _default = "3.9.1";
  _exports.default = _default;
});
enifed("handlebars", ["exports"], function (_exports) {
  "use strict";

  _exports.parse = parse;
  _exports.parser = void 0;

  // File ignored in coverage tests via setting in .istanbul.yml

  /* Jison generated parser */
  var handlebars = function () {
    var parser = {
      trace: function trace() {},
      yy: {},
      symbols_: {
        "error": 2,
        "root": 3,
        "program": 4,
        "EOF": 5,
        "program_repetition0": 6,
        "statement": 7,
        "mustache": 8,
        "block": 9,
        "rawBlock": 10,
        "partial": 11,
        "partialBlock": 12,
        "content": 13,
        "COMMENT": 14,
        "CONTENT": 15,
        "openRawBlock": 16,
        "rawBlock_repetition_plus0": 17,
        "END_RAW_BLOCK": 18,
        "OPEN_RAW_BLOCK": 19,
        "helperName": 20,
        "openRawBlock_repetition0": 21,
        "openRawBlock_option0": 22,
        "CLOSE_RAW_BLOCK": 23,
        "openBlock": 24,
        "block_option0": 25,
        "closeBlock": 26,
        "openInverse": 27,
        "block_option1": 28,
        "OPEN_BLOCK": 29,
        "openBlock_repetition0": 30,
        "openBlock_option0": 31,
        "openBlock_option1": 32,
        "CLOSE": 33,
        "OPEN_INVERSE": 34,
        "openInverse_repetition0": 35,
        "openInverse_option0": 36,
        "openInverse_option1": 37,
        "openInverseChain": 38,
        "OPEN_INVERSE_CHAIN": 39,
        "openInverseChain_repetition0": 40,
        "openInverseChain_option0": 41,
        "openInverseChain_option1": 42,
        "inverseAndProgram": 43,
        "INVERSE": 44,
        "inverseChain": 45,
        "inverseChain_option0": 46,
        "OPEN_ENDBLOCK": 47,
        "OPEN": 48,
        "mustache_repetition0": 49,
        "mustache_option0": 50,
        "OPEN_UNESCAPED": 51,
        "mustache_repetition1": 52,
        "mustache_option1": 53,
        "CLOSE_UNESCAPED": 54,
        "OPEN_PARTIAL": 55,
        "partialName": 56,
        "partial_repetition0": 57,
        "partial_option0": 58,
        "openPartialBlock": 59,
        "OPEN_PARTIAL_BLOCK": 60,
        "openPartialBlock_repetition0": 61,
        "openPartialBlock_option0": 62,
        "param": 63,
        "sexpr": 64,
        "OPEN_SEXPR": 65,
        "sexpr_repetition0": 66,
        "sexpr_option0": 67,
        "CLOSE_SEXPR": 68,
        "hash": 69,
        "hash_repetition_plus0": 70,
        "hashSegment": 71,
        "ID": 72,
        "EQUALS": 73,
        "blockParams": 74,
        "OPEN_BLOCK_PARAMS": 75,
        "blockParams_repetition_plus0": 76,
        "CLOSE_BLOCK_PARAMS": 77,
        "path": 78,
        "dataName": 79,
        "STRING": 80,
        "NUMBER": 81,
        "BOOLEAN": 82,
        "UNDEFINED": 83,
        "NULL": 84,
        "DATA": 85,
        "pathSegments": 86,
        "SEP": 87,
        "$accept": 0,
        "$end": 1
      },
      terminals_: {
        2: "error",
        5: "EOF",
        14: "COMMENT",
        15: "CONTENT",
        18: "END_RAW_BLOCK",
        19: "OPEN_RAW_BLOCK",
        23: "CLOSE_RAW_BLOCK",
        29: "OPEN_BLOCK",
        33: "CLOSE",
        34: "OPEN_INVERSE",
        39: "OPEN_INVERSE_CHAIN",
        44: "INVERSE",
        47: "OPEN_ENDBLOCK",
        48: "OPEN",
        51: "OPEN_UNESCAPED",
        54: "CLOSE_UNESCAPED",
        55: "OPEN_PARTIAL",
        60: "OPEN_PARTIAL_BLOCK",
        65: "OPEN_SEXPR",
        68: "CLOSE_SEXPR",
        72: "ID",
        73: "EQUALS",
        75: "OPEN_BLOCK_PARAMS",
        77: "CLOSE_BLOCK_PARAMS",
        80: "STRING",
        81: "NUMBER",
        82: "BOOLEAN",
        83: "UNDEFINED",
        84: "NULL",
        85: "DATA",
        87: "SEP"
      },
      productions_: [0, [3, 2], [4, 1], [7, 1], [7, 1], [7, 1], [7, 1], [7, 1], [7, 1], [7, 1], [13, 1], [10, 3], [16, 5], [9, 4], [9, 4], [24, 6], [27, 6], [38, 6], [43, 2], [45, 3], [45, 1], [26, 3], [8, 5], [8, 5], [11, 5], [12, 3], [59, 5], [63, 1], [63, 1], [64, 5], [69, 1], [71, 3], [74, 3], [20, 1], [20, 1], [20, 1], [20, 1], [20, 1], [20, 1], [20, 1], [56, 1], [56, 1], [79, 2], [78, 1], [86, 3], [86, 1], [6, 0], [6, 2], [17, 1], [17, 2], [21, 0], [21, 2], [22, 0], [22, 1], [25, 0], [25, 1], [28, 0], [28, 1], [30, 0], [30, 2], [31, 0], [31, 1], [32, 0], [32, 1], [35, 0], [35, 2], [36, 0], [36, 1], [37, 0], [37, 1], [40, 0], [40, 2], [41, 0], [41, 1], [42, 0], [42, 1], [46, 0], [46, 1], [49, 0], [49, 2], [50, 0], [50, 1], [52, 0], [52, 2], [53, 0], [53, 1], [57, 0], [57, 2], [58, 0], [58, 1], [61, 0], [61, 2], [62, 0], [62, 1], [66, 0], [66, 2], [67, 0], [67, 1], [70, 1], [70, 2], [76, 1], [76, 2]],
      performAction: function anonymous(yytext, yyleng, yylineno, yy, yystate, $$, _$) {
        var $0 = $$.length - 1;

        switch (yystate) {
          case 1:
            return $$[$0 - 1];
            break;

          case 2:
            this.$ = yy.prepareProgram($$[$0]);
            break;

          case 3:
            this.$ = $$[$0];
            break;

          case 4:
            this.$ = $$[$0];
            break;

          case 5:
            this.$ = $$[$0];
            break;

          case 6:
            this.$ = $$[$0];
            break;

          case 7:
            this.$ = $$[$0];
            break;

          case 8:
            this.$ = $$[$0];
            break;

          case 9:
            this.$ = {
              type: 'CommentStatement',
              value: yy.stripComment($$[$0]),
              strip: yy.stripFlags($$[$0], $$[$0]),
              loc: yy.locInfo(this._$)
            };
            break;

          case 10:
            this.$ = {
              type: 'ContentStatement',
              original: $$[$0],
              value: $$[$0],
              loc: yy.locInfo(this._$)
            };
            break;

          case 11:
            this.$ = yy.prepareRawBlock($$[$0 - 2], $$[$0 - 1], $$[$0], this._$);
            break;

          case 12:
            this.$ = {
              path: $$[$0 - 3],
              params: $$[$0 - 2],
              hash: $$[$0 - 1]
            };
            break;

          case 13:
            this.$ = yy.prepareBlock($$[$0 - 3], $$[$0 - 2], $$[$0 - 1], $$[$0], false, this._$);
            break;

          case 14:
            this.$ = yy.prepareBlock($$[$0 - 3], $$[$0 - 2], $$[$0 - 1], $$[$0], true, this._$);
            break;

          case 15:
            this.$ = {
              open: $$[$0 - 5],
              path: $$[$0 - 4],
              params: $$[$0 - 3],
              hash: $$[$0 - 2],
              blockParams: $$[$0 - 1],
              strip: yy.stripFlags($$[$0 - 5], $$[$0])
            };
            break;

          case 16:
            this.$ = {
              path: $$[$0 - 4],
              params: $$[$0 - 3],
              hash: $$[$0 - 2],
              blockParams: $$[$0 - 1],
              strip: yy.stripFlags($$[$0 - 5], $$[$0])
            };
            break;

          case 17:
            this.$ = {
              path: $$[$0 - 4],
              params: $$[$0 - 3],
              hash: $$[$0 - 2],
              blockParams: $$[$0 - 1],
              strip: yy.stripFlags($$[$0 - 5], $$[$0])
            };
            break;

          case 18:
            this.$ = {
              strip: yy.stripFlags($$[$0 - 1], $$[$0 - 1]),
              program: $$[$0]
            };
            break;

          case 19:
            var inverse = yy.prepareBlock($$[$0 - 2], $$[$0 - 1], $$[$0], $$[$0], false, this._$),
                program = yy.prepareProgram([inverse], $$[$0 - 1].loc);
            program.chained = true;
            this.$ = {
              strip: $$[$0 - 2].strip,
              program: program,
              chain: true
            };
            break;

          case 20:
            this.$ = $$[$0];
            break;

          case 21:
            this.$ = {
              path: $$[$0 - 1],
              strip: yy.stripFlags($$[$0 - 2], $$[$0])
            };
            break;

          case 22:
            this.$ = yy.prepareMustache($$[$0 - 3], $$[$0 - 2], $$[$0 - 1], $$[$0 - 4], yy.stripFlags($$[$0 - 4], $$[$0]), this._$);
            break;

          case 23:
            this.$ = yy.prepareMustache($$[$0 - 3], $$[$0 - 2], $$[$0 - 1], $$[$0 - 4], yy.stripFlags($$[$0 - 4], $$[$0]), this._$);
            break;

          case 24:
            this.$ = {
              type: 'PartialStatement',
              name: $$[$0 - 3],
              params: $$[$0 - 2],
              hash: $$[$0 - 1],
              indent: '',
              strip: yy.stripFlags($$[$0 - 4], $$[$0]),
              loc: yy.locInfo(this._$)
            };
            break;

          case 25:
            this.$ = yy.preparePartialBlock($$[$0 - 2], $$[$0 - 1], $$[$0], this._$);
            break;

          case 26:
            this.$ = {
              path: $$[$0 - 3],
              params: $$[$0 - 2],
              hash: $$[$0 - 1],
              strip: yy.stripFlags($$[$0 - 4], $$[$0])
            };
            break;

          case 27:
            this.$ = $$[$0];
            break;

          case 28:
            this.$ = $$[$0];
            break;

          case 29:
            this.$ = {
              type: 'SubExpression',
              path: $$[$0 - 3],
              params: $$[$0 - 2],
              hash: $$[$0 - 1],
              loc: yy.locInfo(this._$)
            };
            break;

          case 30:
            this.$ = {
              type: 'Hash',
              pairs: $$[$0],
              loc: yy.locInfo(this._$)
            };
            break;

          case 31:
            this.$ = {
              type: 'HashPair',
              key: yy.id($$[$0 - 2]),
              value: $$[$0],
              loc: yy.locInfo(this._$)
            };
            break;

          case 32:
            this.$ = yy.id($$[$0 - 1]);
            break;

          case 33:
            this.$ = $$[$0];
            break;

          case 34:
            this.$ = $$[$0];
            break;

          case 35:
            this.$ = {
              type: 'StringLiteral',
              value: $$[$0],
              original: $$[$0],
              loc: yy.locInfo(this._$)
            };
            break;

          case 36:
            this.$ = {
              type: 'NumberLiteral',
              value: Number($$[$0]),
              original: Number($$[$0]),
              loc: yy.locInfo(this._$)
            };
            break;

          case 37:
            this.$ = {
              type: 'BooleanLiteral',
              value: $$[$0] === 'true',
              original: $$[$0] === 'true',
              loc: yy.locInfo(this._$)
            };
            break;

          case 38:
            this.$ = {
              type: 'UndefinedLiteral',
              original: undefined,
              value: undefined,
              loc: yy.locInfo(this._$)
            };
            break;

          case 39:
            this.$ = {
              type: 'NullLiteral',
              original: null,
              value: null,
              loc: yy.locInfo(this._$)
            };
            break;

          case 40:
            this.$ = $$[$0];
            break;

          case 41:
            this.$ = $$[$0];
            break;

          case 42:
            this.$ = yy.preparePath(true, $$[$0], this._$);
            break;

          case 43:
            this.$ = yy.preparePath(false, $$[$0], this._$);
            break;

          case 44:
            $$[$0 - 2].push({
              part: yy.id($$[$0]),
              original: $$[$0],
              separator: $$[$0 - 1]
            });
            this.$ = $$[$0 - 2];
            break;

          case 45:
            this.$ = [{
              part: yy.id($$[$0]),
              original: $$[$0]
            }];
            break;

          case 46:
            this.$ = [];
            break;

          case 47:
            $$[$0 - 1].push($$[$0]);
            break;

          case 48:
            this.$ = [$$[$0]];
            break;

          case 49:
            $$[$0 - 1].push($$[$0]);
            break;

          case 50:
            this.$ = [];
            break;

          case 51:
            $$[$0 - 1].push($$[$0]);
            break;

          case 58:
            this.$ = [];
            break;

          case 59:
            $$[$0 - 1].push($$[$0]);
            break;

          case 64:
            this.$ = [];
            break;

          case 65:
            $$[$0 - 1].push($$[$0]);
            break;

          case 70:
            this.$ = [];
            break;

          case 71:
            $$[$0 - 1].push($$[$0]);
            break;

          case 78:
            this.$ = [];
            break;

          case 79:
            $$[$0 - 1].push($$[$0]);
            break;

          case 82:
            this.$ = [];
            break;

          case 83:
            $$[$0 - 1].push($$[$0]);
            break;

          case 86:
            this.$ = [];
            break;

          case 87:
            $$[$0 - 1].push($$[$0]);
            break;

          case 90:
            this.$ = [];
            break;

          case 91:
            $$[$0 - 1].push($$[$0]);
            break;

          case 94:
            this.$ = [];
            break;

          case 95:
            $$[$0 - 1].push($$[$0]);
            break;

          case 98:
            this.$ = [$$[$0]];
            break;

          case 99:
            $$[$0 - 1].push($$[$0]);
            break;

          case 100:
            this.$ = [$$[$0]];
            break;

          case 101:
            $$[$0 - 1].push($$[$0]);
            break;
        }
      },
      table: [{
        3: 1,
        4: 2,
        5: [2, 46],
        6: 3,
        14: [2, 46],
        15: [2, 46],
        19: [2, 46],
        29: [2, 46],
        34: [2, 46],
        48: [2, 46],
        51: [2, 46],
        55: [2, 46],
        60: [2, 46]
      }, {
        1: [3]
      }, {
        5: [1, 4]
      }, {
        5: [2, 2],
        7: 5,
        8: 6,
        9: 7,
        10: 8,
        11: 9,
        12: 10,
        13: 11,
        14: [1, 12],
        15: [1, 20],
        16: 17,
        19: [1, 23],
        24: 15,
        27: 16,
        29: [1, 21],
        34: [1, 22],
        39: [2, 2],
        44: [2, 2],
        47: [2, 2],
        48: [1, 13],
        51: [1, 14],
        55: [1, 18],
        59: 19,
        60: [1, 24]
      }, {
        1: [2, 1]
      }, {
        5: [2, 47],
        14: [2, 47],
        15: [2, 47],
        19: [2, 47],
        29: [2, 47],
        34: [2, 47],
        39: [2, 47],
        44: [2, 47],
        47: [2, 47],
        48: [2, 47],
        51: [2, 47],
        55: [2, 47],
        60: [2, 47]
      }, {
        5: [2, 3],
        14: [2, 3],
        15: [2, 3],
        19: [2, 3],
        29: [2, 3],
        34: [2, 3],
        39: [2, 3],
        44: [2, 3],
        47: [2, 3],
        48: [2, 3],
        51: [2, 3],
        55: [2, 3],
        60: [2, 3]
      }, {
        5: [2, 4],
        14: [2, 4],
        15: [2, 4],
        19: [2, 4],
        29: [2, 4],
        34: [2, 4],
        39: [2, 4],
        44: [2, 4],
        47: [2, 4],
        48: [2, 4],
        51: [2, 4],
        55: [2, 4],
        60: [2, 4]
      }, {
        5: [2, 5],
        14: [2, 5],
        15: [2, 5],
        19: [2, 5],
        29: [2, 5],
        34: [2, 5],
        39: [2, 5],
        44: [2, 5],
        47: [2, 5],
        48: [2, 5],
        51: [2, 5],
        55: [2, 5],
        60: [2, 5]
      }, {
        5: [2, 6],
        14: [2, 6],
        15: [2, 6],
        19: [2, 6],
        29: [2, 6],
        34: [2, 6],
        39: [2, 6],
        44: [2, 6],
        47: [2, 6],
        48: [2, 6],
        51: [2, 6],
        55: [2, 6],
        60: [2, 6]
      }, {
        5: [2, 7],
        14: [2, 7],
        15: [2, 7],
        19: [2, 7],
        29: [2, 7],
        34: [2, 7],
        39: [2, 7],
        44: [2, 7],
        47: [2, 7],
        48: [2, 7],
        51: [2, 7],
        55: [2, 7],
        60: [2, 7]
      }, {
        5: [2, 8],
        14: [2, 8],
        15: [2, 8],
        19: [2, 8],
        29: [2, 8],
        34: [2, 8],
        39: [2, 8],
        44: [2, 8],
        47: [2, 8],
        48: [2, 8],
        51: [2, 8],
        55: [2, 8],
        60: [2, 8]
      }, {
        5: [2, 9],
        14: [2, 9],
        15: [2, 9],
        19: [2, 9],
        29: [2, 9],
        34: [2, 9],
        39: [2, 9],
        44: [2, 9],
        47: [2, 9],
        48: [2, 9],
        51: [2, 9],
        55: [2, 9],
        60: [2, 9]
      }, {
        20: 25,
        72: [1, 35],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        20: 36,
        72: [1, 35],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        4: 37,
        6: 3,
        14: [2, 46],
        15: [2, 46],
        19: [2, 46],
        29: [2, 46],
        34: [2, 46],
        39: [2, 46],
        44: [2, 46],
        47: [2, 46],
        48: [2, 46],
        51: [2, 46],
        55: [2, 46],
        60: [2, 46]
      }, {
        4: 38,
        6: 3,
        14: [2, 46],
        15: [2, 46],
        19: [2, 46],
        29: [2, 46],
        34: [2, 46],
        44: [2, 46],
        47: [2, 46],
        48: [2, 46],
        51: [2, 46],
        55: [2, 46],
        60: [2, 46]
      }, {
        13: 40,
        15: [1, 20],
        17: 39
      }, {
        20: 42,
        56: 41,
        64: 43,
        65: [1, 44],
        72: [1, 35],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        4: 45,
        6: 3,
        14: [2, 46],
        15: [2, 46],
        19: [2, 46],
        29: [2, 46],
        34: [2, 46],
        47: [2, 46],
        48: [2, 46],
        51: [2, 46],
        55: [2, 46],
        60: [2, 46]
      }, {
        5: [2, 10],
        14: [2, 10],
        15: [2, 10],
        18: [2, 10],
        19: [2, 10],
        29: [2, 10],
        34: [2, 10],
        39: [2, 10],
        44: [2, 10],
        47: [2, 10],
        48: [2, 10],
        51: [2, 10],
        55: [2, 10],
        60: [2, 10]
      }, {
        20: 46,
        72: [1, 35],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        20: 47,
        72: [1, 35],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        20: 48,
        72: [1, 35],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        20: 42,
        56: 49,
        64: 43,
        65: [1, 44],
        72: [1, 35],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        33: [2, 78],
        49: 50,
        65: [2, 78],
        72: [2, 78],
        80: [2, 78],
        81: [2, 78],
        82: [2, 78],
        83: [2, 78],
        84: [2, 78],
        85: [2, 78]
      }, {
        23: [2, 33],
        33: [2, 33],
        54: [2, 33],
        65: [2, 33],
        68: [2, 33],
        72: [2, 33],
        75: [2, 33],
        80: [2, 33],
        81: [2, 33],
        82: [2, 33],
        83: [2, 33],
        84: [2, 33],
        85: [2, 33]
      }, {
        23: [2, 34],
        33: [2, 34],
        54: [2, 34],
        65: [2, 34],
        68: [2, 34],
        72: [2, 34],
        75: [2, 34],
        80: [2, 34],
        81: [2, 34],
        82: [2, 34],
        83: [2, 34],
        84: [2, 34],
        85: [2, 34]
      }, {
        23: [2, 35],
        33: [2, 35],
        54: [2, 35],
        65: [2, 35],
        68: [2, 35],
        72: [2, 35],
        75: [2, 35],
        80: [2, 35],
        81: [2, 35],
        82: [2, 35],
        83: [2, 35],
        84: [2, 35],
        85: [2, 35]
      }, {
        23: [2, 36],
        33: [2, 36],
        54: [2, 36],
        65: [2, 36],
        68: [2, 36],
        72: [2, 36],
        75: [2, 36],
        80: [2, 36],
        81: [2, 36],
        82: [2, 36],
        83: [2, 36],
        84: [2, 36],
        85: [2, 36]
      }, {
        23: [2, 37],
        33: [2, 37],
        54: [2, 37],
        65: [2, 37],
        68: [2, 37],
        72: [2, 37],
        75: [2, 37],
        80: [2, 37],
        81: [2, 37],
        82: [2, 37],
        83: [2, 37],
        84: [2, 37],
        85: [2, 37]
      }, {
        23: [2, 38],
        33: [2, 38],
        54: [2, 38],
        65: [2, 38],
        68: [2, 38],
        72: [2, 38],
        75: [2, 38],
        80: [2, 38],
        81: [2, 38],
        82: [2, 38],
        83: [2, 38],
        84: [2, 38],
        85: [2, 38]
      }, {
        23: [2, 39],
        33: [2, 39],
        54: [2, 39],
        65: [2, 39],
        68: [2, 39],
        72: [2, 39],
        75: [2, 39],
        80: [2, 39],
        81: [2, 39],
        82: [2, 39],
        83: [2, 39],
        84: [2, 39],
        85: [2, 39]
      }, {
        23: [2, 43],
        33: [2, 43],
        54: [2, 43],
        65: [2, 43],
        68: [2, 43],
        72: [2, 43],
        75: [2, 43],
        80: [2, 43],
        81: [2, 43],
        82: [2, 43],
        83: [2, 43],
        84: [2, 43],
        85: [2, 43],
        87: [1, 51]
      }, {
        72: [1, 35],
        86: 52
      }, {
        23: [2, 45],
        33: [2, 45],
        54: [2, 45],
        65: [2, 45],
        68: [2, 45],
        72: [2, 45],
        75: [2, 45],
        80: [2, 45],
        81: [2, 45],
        82: [2, 45],
        83: [2, 45],
        84: [2, 45],
        85: [2, 45],
        87: [2, 45]
      }, {
        52: 53,
        54: [2, 82],
        65: [2, 82],
        72: [2, 82],
        80: [2, 82],
        81: [2, 82],
        82: [2, 82],
        83: [2, 82],
        84: [2, 82],
        85: [2, 82]
      }, {
        25: 54,
        38: 56,
        39: [1, 58],
        43: 57,
        44: [1, 59],
        45: 55,
        47: [2, 54]
      }, {
        28: 60,
        43: 61,
        44: [1, 59],
        47: [2, 56]
      }, {
        13: 63,
        15: [1, 20],
        18: [1, 62]
      }, {
        15: [2, 48],
        18: [2, 48]
      }, {
        33: [2, 86],
        57: 64,
        65: [2, 86],
        72: [2, 86],
        80: [2, 86],
        81: [2, 86],
        82: [2, 86],
        83: [2, 86],
        84: [2, 86],
        85: [2, 86]
      }, {
        33: [2, 40],
        65: [2, 40],
        72: [2, 40],
        80: [2, 40],
        81: [2, 40],
        82: [2, 40],
        83: [2, 40],
        84: [2, 40],
        85: [2, 40]
      }, {
        33: [2, 41],
        65: [2, 41],
        72: [2, 41],
        80: [2, 41],
        81: [2, 41],
        82: [2, 41],
        83: [2, 41],
        84: [2, 41],
        85: [2, 41]
      }, {
        20: 65,
        72: [1, 35],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        26: 66,
        47: [1, 67]
      }, {
        30: 68,
        33: [2, 58],
        65: [2, 58],
        72: [2, 58],
        75: [2, 58],
        80: [2, 58],
        81: [2, 58],
        82: [2, 58],
        83: [2, 58],
        84: [2, 58],
        85: [2, 58]
      }, {
        33: [2, 64],
        35: 69,
        65: [2, 64],
        72: [2, 64],
        75: [2, 64],
        80: [2, 64],
        81: [2, 64],
        82: [2, 64],
        83: [2, 64],
        84: [2, 64],
        85: [2, 64]
      }, {
        21: 70,
        23: [2, 50],
        65: [2, 50],
        72: [2, 50],
        80: [2, 50],
        81: [2, 50],
        82: [2, 50],
        83: [2, 50],
        84: [2, 50],
        85: [2, 50]
      }, {
        33: [2, 90],
        61: 71,
        65: [2, 90],
        72: [2, 90],
        80: [2, 90],
        81: [2, 90],
        82: [2, 90],
        83: [2, 90],
        84: [2, 90],
        85: [2, 90]
      }, {
        20: 75,
        33: [2, 80],
        50: 72,
        63: 73,
        64: 76,
        65: [1, 44],
        69: 74,
        70: 77,
        71: 78,
        72: [1, 79],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        72: [1, 80]
      }, {
        23: [2, 42],
        33: [2, 42],
        54: [2, 42],
        65: [2, 42],
        68: [2, 42],
        72: [2, 42],
        75: [2, 42],
        80: [2, 42],
        81: [2, 42],
        82: [2, 42],
        83: [2, 42],
        84: [2, 42],
        85: [2, 42],
        87: [1, 51]
      }, {
        20: 75,
        53: 81,
        54: [2, 84],
        63: 82,
        64: 76,
        65: [1, 44],
        69: 83,
        70: 77,
        71: 78,
        72: [1, 79],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        26: 84,
        47: [1, 67]
      }, {
        47: [2, 55]
      }, {
        4: 85,
        6: 3,
        14: [2, 46],
        15: [2, 46],
        19: [2, 46],
        29: [2, 46],
        34: [2, 46],
        39: [2, 46],
        44: [2, 46],
        47: [2, 46],
        48: [2, 46],
        51: [2, 46],
        55: [2, 46],
        60: [2, 46]
      }, {
        47: [2, 20]
      }, {
        20: 86,
        72: [1, 35],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        4: 87,
        6: 3,
        14: [2, 46],
        15: [2, 46],
        19: [2, 46],
        29: [2, 46],
        34: [2, 46],
        47: [2, 46],
        48: [2, 46],
        51: [2, 46],
        55: [2, 46],
        60: [2, 46]
      }, {
        26: 88,
        47: [1, 67]
      }, {
        47: [2, 57]
      }, {
        5: [2, 11],
        14: [2, 11],
        15: [2, 11],
        19: [2, 11],
        29: [2, 11],
        34: [2, 11],
        39: [2, 11],
        44: [2, 11],
        47: [2, 11],
        48: [2, 11],
        51: [2, 11],
        55: [2, 11],
        60: [2, 11]
      }, {
        15: [2, 49],
        18: [2, 49]
      }, {
        20: 75,
        33: [2, 88],
        58: 89,
        63: 90,
        64: 76,
        65: [1, 44],
        69: 91,
        70: 77,
        71: 78,
        72: [1, 79],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        65: [2, 94],
        66: 92,
        68: [2, 94],
        72: [2, 94],
        80: [2, 94],
        81: [2, 94],
        82: [2, 94],
        83: [2, 94],
        84: [2, 94],
        85: [2, 94]
      }, {
        5: [2, 25],
        14: [2, 25],
        15: [2, 25],
        19: [2, 25],
        29: [2, 25],
        34: [2, 25],
        39: [2, 25],
        44: [2, 25],
        47: [2, 25],
        48: [2, 25],
        51: [2, 25],
        55: [2, 25],
        60: [2, 25]
      }, {
        20: 93,
        72: [1, 35],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        20: 75,
        31: 94,
        33: [2, 60],
        63: 95,
        64: 76,
        65: [1, 44],
        69: 96,
        70: 77,
        71: 78,
        72: [1, 79],
        75: [2, 60],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        20: 75,
        33: [2, 66],
        36: 97,
        63: 98,
        64: 76,
        65: [1, 44],
        69: 99,
        70: 77,
        71: 78,
        72: [1, 79],
        75: [2, 66],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        20: 75,
        22: 100,
        23: [2, 52],
        63: 101,
        64: 76,
        65: [1, 44],
        69: 102,
        70: 77,
        71: 78,
        72: [1, 79],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        20: 75,
        33: [2, 92],
        62: 103,
        63: 104,
        64: 76,
        65: [1, 44],
        69: 105,
        70: 77,
        71: 78,
        72: [1, 79],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        33: [1, 106]
      }, {
        33: [2, 79],
        65: [2, 79],
        72: [2, 79],
        80: [2, 79],
        81: [2, 79],
        82: [2, 79],
        83: [2, 79],
        84: [2, 79],
        85: [2, 79]
      }, {
        33: [2, 81]
      }, {
        23: [2, 27],
        33: [2, 27],
        54: [2, 27],
        65: [2, 27],
        68: [2, 27],
        72: [2, 27],
        75: [2, 27],
        80: [2, 27],
        81: [2, 27],
        82: [2, 27],
        83: [2, 27],
        84: [2, 27],
        85: [2, 27]
      }, {
        23: [2, 28],
        33: [2, 28],
        54: [2, 28],
        65: [2, 28],
        68: [2, 28],
        72: [2, 28],
        75: [2, 28],
        80: [2, 28],
        81: [2, 28],
        82: [2, 28],
        83: [2, 28],
        84: [2, 28],
        85: [2, 28]
      }, {
        23: [2, 30],
        33: [2, 30],
        54: [2, 30],
        68: [2, 30],
        71: 107,
        72: [1, 108],
        75: [2, 30]
      }, {
        23: [2, 98],
        33: [2, 98],
        54: [2, 98],
        68: [2, 98],
        72: [2, 98],
        75: [2, 98]
      }, {
        23: [2, 45],
        33: [2, 45],
        54: [2, 45],
        65: [2, 45],
        68: [2, 45],
        72: [2, 45],
        73: [1, 109],
        75: [2, 45],
        80: [2, 45],
        81: [2, 45],
        82: [2, 45],
        83: [2, 45],
        84: [2, 45],
        85: [2, 45],
        87: [2, 45]
      }, {
        23: [2, 44],
        33: [2, 44],
        54: [2, 44],
        65: [2, 44],
        68: [2, 44],
        72: [2, 44],
        75: [2, 44],
        80: [2, 44],
        81: [2, 44],
        82: [2, 44],
        83: [2, 44],
        84: [2, 44],
        85: [2, 44],
        87: [2, 44]
      }, {
        54: [1, 110]
      }, {
        54: [2, 83],
        65: [2, 83],
        72: [2, 83],
        80: [2, 83],
        81: [2, 83],
        82: [2, 83],
        83: [2, 83],
        84: [2, 83],
        85: [2, 83]
      }, {
        54: [2, 85]
      }, {
        5: [2, 13],
        14: [2, 13],
        15: [2, 13],
        19: [2, 13],
        29: [2, 13],
        34: [2, 13],
        39: [2, 13],
        44: [2, 13],
        47: [2, 13],
        48: [2, 13],
        51: [2, 13],
        55: [2, 13],
        60: [2, 13]
      }, {
        38: 56,
        39: [1, 58],
        43: 57,
        44: [1, 59],
        45: 112,
        46: 111,
        47: [2, 76]
      }, {
        33: [2, 70],
        40: 113,
        65: [2, 70],
        72: [2, 70],
        75: [2, 70],
        80: [2, 70],
        81: [2, 70],
        82: [2, 70],
        83: [2, 70],
        84: [2, 70],
        85: [2, 70]
      }, {
        47: [2, 18]
      }, {
        5: [2, 14],
        14: [2, 14],
        15: [2, 14],
        19: [2, 14],
        29: [2, 14],
        34: [2, 14],
        39: [2, 14],
        44: [2, 14],
        47: [2, 14],
        48: [2, 14],
        51: [2, 14],
        55: [2, 14],
        60: [2, 14]
      }, {
        33: [1, 114]
      }, {
        33: [2, 87],
        65: [2, 87],
        72: [2, 87],
        80: [2, 87],
        81: [2, 87],
        82: [2, 87],
        83: [2, 87],
        84: [2, 87],
        85: [2, 87]
      }, {
        33: [2, 89]
      }, {
        20: 75,
        63: 116,
        64: 76,
        65: [1, 44],
        67: 115,
        68: [2, 96],
        69: 117,
        70: 77,
        71: 78,
        72: [1, 79],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        33: [1, 118]
      }, {
        32: 119,
        33: [2, 62],
        74: 120,
        75: [1, 121]
      }, {
        33: [2, 59],
        65: [2, 59],
        72: [2, 59],
        75: [2, 59],
        80: [2, 59],
        81: [2, 59],
        82: [2, 59],
        83: [2, 59],
        84: [2, 59],
        85: [2, 59]
      }, {
        33: [2, 61],
        75: [2, 61]
      }, {
        33: [2, 68],
        37: 122,
        74: 123,
        75: [1, 121]
      }, {
        33: [2, 65],
        65: [2, 65],
        72: [2, 65],
        75: [2, 65],
        80: [2, 65],
        81: [2, 65],
        82: [2, 65],
        83: [2, 65],
        84: [2, 65],
        85: [2, 65]
      }, {
        33: [2, 67],
        75: [2, 67]
      }, {
        23: [1, 124]
      }, {
        23: [2, 51],
        65: [2, 51],
        72: [2, 51],
        80: [2, 51],
        81: [2, 51],
        82: [2, 51],
        83: [2, 51],
        84: [2, 51],
        85: [2, 51]
      }, {
        23: [2, 53]
      }, {
        33: [1, 125]
      }, {
        33: [2, 91],
        65: [2, 91],
        72: [2, 91],
        80: [2, 91],
        81: [2, 91],
        82: [2, 91],
        83: [2, 91],
        84: [2, 91],
        85: [2, 91]
      }, {
        33: [2, 93]
      }, {
        5: [2, 22],
        14: [2, 22],
        15: [2, 22],
        19: [2, 22],
        29: [2, 22],
        34: [2, 22],
        39: [2, 22],
        44: [2, 22],
        47: [2, 22],
        48: [2, 22],
        51: [2, 22],
        55: [2, 22],
        60: [2, 22]
      }, {
        23: [2, 99],
        33: [2, 99],
        54: [2, 99],
        68: [2, 99],
        72: [2, 99],
        75: [2, 99]
      }, {
        73: [1, 109]
      }, {
        20: 75,
        63: 126,
        64: 76,
        65: [1, 44],
        72: [1, 35],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        5: [2, 23],
        14: [2, 23],
        15: [2, 23],
        19: [2, 23],
        29: [2, 23],
        34: [2, 23],
        39: [2, 23],
        44: [2, 23],
        47: [2, 23],
        48: [2, 23],
        51: [2, 23],
        55: [2, 23],
        60: [2, 23]
      }, {
        47: [2, 19]
      }, {
        47: [2, 77]
      }, {
        20: 75,
        33: [2, 72],
        41: 127,
        63: 128,
        64: 76,
        65: [1, 44],
        69: 129,
        70: 77,
        71: 78,
        72: [1, 79],
        75: [2, 72],
        78: 26,
        79: 27,
        80: [1, 28],
        81: [1, 29],
        82: [1, 30],
        83: [1, 31],
        84: [1, 32],
        85: [1, 34],
        86: 33
      }, {
        5: [2, 24],
        14: [2, 24],
        15: [2, 24],
        19: [2, 24],
        29: [2, 24],
        34: [2, 24],
        39: [2, 24],
        44: [2, 24],
        47: [2, 24],
        48: [2, 24],
        51: [2, 24],
        55: [2, 24],
        60: [2, 24]
      }, {
        68: [1, 130]
      }, {
        65: [2, 95],
        68: [2, 95],
        72: [2, 95],
        80: [2, 95],
        81: [2, 95],
        82: [2, 95],
        83: [2, 95],
        84: [2, 95],
        85: [2, 95]
      }, {
        68: [2, 97]
      }, {
        5: [2, 21],
        14: [2, 21],
        15: [2, 21],
        19: [2, 21],
        29: [2, 21],
        34: [2, 21],
        39: [2, 21],
        44: [2, 21],
        47: [2, 21],
        48: [2, 21],
        51: [2, 21],
        55: [2, 21],
        60: [2, 21]
      }, {
        33: [1, 131]
      }, {
        33: [2, 63]
      }, {
        72: [1, 133],
        76: 132
      }, {
        33: [1, 134]
      }, {
        33: [2, 69]
      }, {
        15: [2, 12]
      }, {
        14: [2, 26],
        15: [2, 26],
        19: [2, 26],
        29: [2, 26],
        34: [2, 26],
        47: [2, 26],
        48: [2, 26],
        51: [2, 26],
        55: [2, 26],
        60: [2, 26]
      }, {
        23: [2, 31],
        33: [2, 31],
        54: [2, 31],
        68: [2, 31],
        72: [2, 31],
        75: [2, 31]
      }, {
        33: [2, 74],
        42: 135,
        74: 136,
        75: [1, 121]
      }, {
        33: [2, 71],
        65: [2, 71],
        72: [2, 71],
        75: [2, 71],
        80: [2, 71],
        81: [2, 71],
        82: [2, 71],
        83: [2, 71],
        84: [2, 71],
        85: [2, 71]
      }, {
        33: [2, 73],
        75: [2, 73]
      }, {
        23: [2, 29],
        33: [2, 29],
        54: [2, 29],
        65: [2, 29],
        68: [2, 29],
        72: [2, 29],
        75: [2, 29],
        80: [2, 29],
        81: [2, 29],
        82: [2, 29],
        83: [2, 29],
        84: [2, 29],
        85: [2, 29]
      }, {
        14: [2, 15],
        15: [2, 15],
        19: [2, 15],
        29: [2, 15],
        34: [2, 15],
        39: [2, 15],
        44: [2, 15],
        47: [2, 15],
        48: [2, 15],
        51: [2, 15],
        55: [2, 15],
        60: [2, 15]
      }, {
        72: [1, 138],
        77: [1, 137]
      }, {
        72: [2, 100],
        77: [2, 100]
      }, {
        14: [2, 16],
        15: [2, 16],
        19: [2, 16],
        29: [2, 16],
        34: [2, 16],
        44: [2, 16],
        47: [2, 16],
        48: [2, 16],
        51: [2, 16],
        55: [2, 16],
        60: [2, 16]
      }, {
        33: [1, 139]
      }, {
        33: [2, 75]
      }, {
        33: [2, 32]
      }, {
        72: [2, 101],
        77: [2, 101]
      }, {
        14: [2, 17],
        15: [2, 17],
        19: [2, 17],
        29: [2, 17],
        34: [2, 17],
        39: [2, 17],
        44: [2, 17],
        47: [2, 17],
        48: [2, 17],
        51: [2, 17],
        55: [2, 17],
        60: [2, 17]
      }],
      defaultActions: {
        4: [2, 1],
        55: [2, 55],
        57: [2, 20],
        61: [2, 57],
        74: [2, 81],
        83: [2, 85],
        87: [2, 18],
        91: [2, 89],
        102: [2, 53],
        105: [2, 93],
        111: [2, 19],
        112: [2, 77],
        117: [2, 97],
        120: [2, 63],
        123: [2, 69],
        124: [2, 12],
        136: [2, 75],
        137: [2, 32]
      },
      parseError: function parseError(str, hash) {
        throw new Error(str);
      },
      parse: function parse(input) {
        var self = this,
            stack = [0],
            vstack = [null],
            lstack = [],
            table = this.table,
            yytext = "",
            yylineno = 0,
            yyleng = 0,
            recovering = 0;
        this.lexer.setInput(input);
        this.lexer.yy = this.yy;
        this.yy.lexer = this.lexer;
        this.yy.parser = this;
        if (typeof this.lexer.yylloc == "undefined") this.lexer.yylloc = {};
        var yyloc = this.lexer.yylloc;
        lstack.push(yyloc);
        var ranges = this.lexer.options && this.lexer.options.ranges;
        if (typeof this.yy.parseError === "function") this.parseError = this.yy.parseError;

        function lex() {
          var token;
          token = self.lexer.lex() || 1;

          if (typeof token !== "number") {
            token = self.symbols_[token] || token;
          }

          return token;
        }

        var symbol,
            preErrorSymbol,
            state,
            action,
            r,
            yyval = {},
            p,
            len,
            newState,
            expected;

        while (true) {
          state = stack[stack.length - 1];

          if (this.defaultActions[state]) {
            action = this.defaultActions[state];
          } else {
            if (symbol === null || typeof symbol == "undefined") {
              symbol = lex();
            }

            action = table[state] && table[state][symbol];
          }

          if (typeof action === "undefined" || !action.length || !action[0]) {
            var errStr = "";

            if (!recovering) {
              expected = [];

              for (p in table[state]) if (this.terminals_[p] && p > 2) {
                expected.push("'" + this.terminals_[p] + "'");
              }

              if (this.lexer.showPosition) {
                errStr = "Parse error on line " + (yylineno + 1) + ":\n" + this.lexer.showPosition() + "\nExpecting " + expected.join(", ") + ", got '" + (this.terminals_[symbol] || symbol) + "'";
              } else {
                errStr = "Parse error on line " + (yylineno + 1) + ": Unexpected " + (symbol == 1 ? "end of input" : "'" + (this.terminals_[symbol] || symbol) + "'");
              }

              this.parseError(errStr, {
                text: this.lexer.match,
                token: this.terminals_[symbol] || symbol,
                line: this.lexer.yylineno,
                loc: yyloc,
                expected: expected
              });
            }
          }

          if (action[0] instanceof Array && action.length > 1) {
            throw new Error("Parse Error: multiple actions possible at state: " + state + ", token: " + symbol);
          }

          switch (action[0]) {
            case 1:
              stack.push(symbol);
              vstack.push(this.lexer.yytext);
              lstack.push(this.lexer.yylloc);
              stack.push(action[1]);
              symbol = null;

              if (!preErrorSymbol) {
                yyleng = this.lexer.yyleng;
                yytext = this.lexer.yytext;
                yylineno = this.lexer.yylineno;
                yyloc = this.lexer.yylloc;
                if (recovering > 0) recovering--;
              } else {
                symbol = preErrorSymbol;
                preErrorSymbol = null;
              }

              break;

            case 2:
              len = this.productions_[action[1]][1];
              yyval.$ = vstack[vstack.length - len];
              yyval._$ = {
                first_line: lstack[lstack.length - (len || 1)].first_line,
                last_line: lstack[lstack.length - 1].last_line,
                first_column: lstack[lstack.length - (len || 1)].first_column,
                last_column: lstack[lstack.length - 1].last_column
              };

              if (ranges) {
                yyval._$.range = [lstack[lstack.length - (len || 1)].range[0], lstack[lstack.length - 1].range[1]];
              }

              r = this.performAction.call(yyval, yytext, yyleng, yylineno, this.yy, action[1], vstack, lstack);

              if (typeof r !== "undefined") {
                return r;
              }

              if (len) {
                stack = stack.slice(0, -1 * len * 2);
                vstack = vstack.slice(0, -1 * len);
                lstack = lstack.slice(0, -1 * len);
              }

              stack.push(this.productions_[action[1]][0]);
              vstack.push(yyval.$);
              lstack.push(yyval._$);
              newState = table[stack[stack.length - 2]][stack[stack.length - 1]];
              stack.push(newState);
              break;

            case 3:
              return true;
          }
        }

        return true;
      }
    };
    /* Jison generated lexer */

    var lexer = function () {
      var lexer = {
        EOF: 1,
        parseError: function parseError(str, hash) {
          if (this.yy.parser) {
            this.yy.parser.parseError(str, hash);
          } else {
            throw new Error(str);
          }
        },
        setInput: function (input) {
          this._input = input;
          this._more = this._less = this.done = false;
          this.yylineno = this.yyleng = 0;
          this.yytext = this.matched = this.match = '';
          this.conditionStack = ['INITIAL'];
          this.yylloc = {
            first_line: 1,
            first_column: 0,
            last_line: 1,
            last_column: 0
          };
          if (this.options.ranges) this.yylloc.range = [0, 0];
          this.offset = 0;
          return this;
        },
        input: function () {
          var ch = this._input[0];
          this.yytext += ch;
          this.yyleng++;
          this.offset++;
          this.match += ch;
          this.matched += ch;
          var lines = ch.match(/(?:\r\n?|\n).*/g);

          if (lines) {
            this.yylineno++;
            this.yylloc.last_line++;
          } else {
            this.yylloc.last_column++;
          }

          if (this.options.ranges) this.yylloc.range[1]++;
          this._input = this._input.slice(1);
          return ch;
        },
        unput: function (ch) {
          var len = ch.length;
          var lines = ch.split(/(?:\r\n?|\n)/g);
          this._input = ch + this._input;
          this.yytext = this.yytext.substr(0, this.yytext.length - len - 1); //this.yyleng -= len;

          this.offset -= len;
          var oldLines = this.match.split(/(?:\r\n?|\n)/g);
          this.match = this.match.substr(0, this.match.length - 1);
          this.matched = this.matched.substr(0, this.matched.length - 1);
          if (lines.length - 1) this.yylineno -= lines.length - 1;
          var r = this.yylloc.range;
          this.yylloc = {
            first_line: this.yylloc.first_line,
            last_line: this.yylineno + 1,
            first_column: this.yylloc.first_column,
            last_column: lines ? (lines.length === oldLines.length ? this.yylloc.first_column : 0) + oldLines[oldLines.length - lines.length].length - lines[0].length : this.yylloc.first_column - len
          };

          if (this.options.ranges) {
            this.yylloc.range = [r[0], r[0] + this.yyleng - len];
          }

          return this;
        },
        more: function () {
          this._more = true;
          return this;
        },
        less: function (n) {
          this.unput(this.match.slice(n));
        },
        pastInput: function () {
          var past = this.matched.substr(0, this.matched.length - this.match.length);
          return (past.length > 20 ? '...' : '') + past.substr(-20).replace(/\n/g, "");
        },
        upcomingInput: function () {
          var next = this.match;

          if (next.length < 20) {
            next += this._input.substr(0, 20 - next.length);
          }

          return (next.substr(0, 20) + (next.length > 20 ? '...' : '')).replace(/\n/g, "");
        },
        showPosition: function () {
          var pre = this.pastInput();
          var c = new Array(pre.length + 1).join("-");
          return pre + this.upcomingInput() + "\n" + c + "^";
        },
        next: function () {
          if (this.done) {
            return this.EOF;
          }

          if (!this._input) this.done = true;
          var token, match, tempMatch, index, lines;

          if (!this._more) {
            this.yytext = '';
            this.match = '';
          }

          var rules = this._currentRules();

          for (var i = 0; i < rules.length; i++) {
            tempMatch = this._input.match(this.rules[rules[i]]);

            if (tempMatch && (!match || tempMatch[0].length > match[0].length)) {
              match = tempMatch;
              index = i;
              if (!this.options.flex) break;
            }
          }

          if (match) {
            lines = match[0].match(/(?:\r\n?|\n).*/g);
            if (lines) this.yylineno += lines.length;
            this.yylloc = {
              first_line: this.yylloc.last_line,
              last_line: this.yylineno + 1,
              first_column: this.yylloc.last_column,
              last_column: lines ? lines[lines.length - 1].length - lines[lines.length - 1].match(/\r?\n?/)[0].length : this.yylloc.last_column + match[0].length
            };
            this.yytext += match[0];
            this.match += match[0];
            this.matches = match;
            this.yyleng = this.yytext.length;

            if (this.options.ranges) {
              this.yylloc.range = [this.offset, this.offset += this.yyleng];
            }

            this._more = false;
            this._input = this._input.slice(match[0].length);
            this.matched += match[0];
            token = this.performAction.call(this, this.yy, this, rules[index], this.conditionStack[this.conditionStack.length - 1]);
            if (this.done && this._input) this.done = false;
            if (token) return token;else return;
          }

          if (this._input === "") {
            return this.EOF;
          } else {
            return this.parseError('Lexical error on line ' + (this.yylineno + 1) + '. Unrecognized text.\n' + this.showPosition(), {
              text: "",
              token: null,
              line: this.yylineno
            });
          }
        },
        lex: function lex() {
          var r = this.next();

          if (typeof r !== 'undefined') {
            return r;
          } else {
            return this.lex();
          }
        },
        begin: function begin(condition) {
          this.conditionStack.push(condition);
        },
        popState: function popState() {
          return this.conditionStack.pop();
        },
        _currentRules: function _currentRules() {
          return this.conditions[this.conditionStack[this.conditionStack.length - 1]].rules;
        },
        topState: function () {
          return this.conditionStack[this.conditionStack.length - 2];
        },
        pushState: function begin(condition) {
          this.begin(condition);
        }
      };
      lexer.options = {};

      lexer.performAction = function anonymous(yy, yy_, $avoiding_name_collisions, YY_START) {
        function strip(start, end) {
          return yy_.yytext = yy_.yytext.substr(start, yy_.yyleng - end);
        }

        switch ($avoiding_name_collisions) {
          case 0:
            if (yy_.yytext.slice(-2) === "\\\\") {
              strip(0, 1);
              this.begin("mu");
            } else if (yy_.yytext.slice(-1) === "\\") {
              strip(0, 1);
              this.begin("emu");
            } else {
              this.begin("mu");
            }

            if (yy_.yytext) return 15;
            break;

          case 1:
            return 15;
            break;

          case 2:
            this.popState();
            return 15;
            break;

          case 3:
            this.begin('raw');
            return 15;
            break;

          case 4:
            this.popState(); // Should be using `this.topState()` below, but it currently
            // returns the second top instead of the first top. Opened an
            // issue about it at https://github.com/zaach/jison/issues/291

            if (this.conditionStack[this.conditionStack.length - 1] === 'raw') {
              return 15;
            } else {
              yy_.yytext = yy_.yytext.substr(5, yy_.yyleng - 9);
              return 'END_RAW_BLOCK';
            }

            break;

          case 5:
            return 15;
            break;

          case 6:
            this.popState();
            return 14;
            break;

          case 7:
            return 65;
            break;

          case 8:
            return 68;
            break;

          case 9:
            return 19;
            break;

          case 10:
            this.popState();
            this.begin('raw');
            return 23;
            break;

          case 11:
            return 55;
            break;

          case 12:
            return 60;
            break;

          case 13:
            return 29;
            break;

          case 14:
            return 47;
            break;

          case 15:
            this.popState();
            return 44;
            break;

          case 16:
            this.popState();
            return 44;
            break;

          case 17:
            return 34;
            break;

          case 18:
            return 39;
            break;

          case 19:
            return 51;
            break;

          case 20:
            return 48;
            break;

          case 21:
            this.unput(yy_.yytext);
            this.popState();
            this.begin('com');
            break;

          case 22:
            this.popState();
            return 14;
            break;

          case 23:
            return 48;
            break;

          case 24:
            return 73;
            break;

          case 25:
            return 72;
            break;

          case 26:
            return 72;
            break;

          case 27:
            return 87;
            break;

          case 28:
            // ignore whitespace
            break;

          case 29:
            this.popState();
            return 54;
            break;

          case 30:
            this.popState();
            return 33;
            break;

          case 31:
            yy_.yytext = strip(1, 2).replace(/\\"/g, '"');
            return 80;
            break;

          case 32:
            yy_.yytext = strip(1, 2).replace(/\\'/g, "'");
            return 80;
            break;

          case 33:
            return 85;
            break;

          case 34:
            return 82;
            break;

          case 35:
            return 82;
            break;

          case 36:
            return 83;
            break;

          case 37:
            return 84;
            break;

          case 38:
            return 81;
            break;

          case 39:
            return 75;
            break;

          case 40:
            return 77;
            break;

          case 41:
            return 72;
            break;

          case 42:
            yy_.yytext = yy_.yytext.replace(/\\([\\\]])/g, '$1');
            return 72;
            break;

          case 43:
            return 'INVALID';
            break;

          case 44:
            return 5;
            break;
        }
      };

      lexer.rules = [/^(?:[^\x00]*?(?=(\{\{)))/, /^(?:[^\x00]+)/, /^(?:[^\x00]{2,}?(?=(\{\{|\\\{\{|\\\\\{\{|$)))/, /^(?:\{\{\{\{(?=[^\/]))/, /^(?:\{\{\{\{\/[^\s!"#%-,\.\/;->@\[-\^`\{-~]+(?=[=}\s\/.])\}\}\}\})/, /^(?:[^\x00]*?(?=(\{\{\{\{)))/, /^(?:[\s\S]*?--(~)?\}\})/, /^(?:\()/, /^(?:\))/, /^(?:\{\{\{\{)/, /^(?:\}\}\}\})/, /^(?:\{\{(~)?>)/, /^(?:\{\{(~)?#>)/, /^(?:\{\{(~)?#\*?)/, /^(?:\{\{(~)?\/)/, /^(?:\{\{(~)?\^\s*(~)?\}\})/, /^(?:\{\{(~)?\s*else\s*(~)?\}\})/, /^(?:\{\{(~)?\^)/, /^(?:\{\{(~)?\s*else\b)/, /^(?:\{\{(~)?\{)/, /^(?:\{\{(~)?&)/, /^(?:\{\{(~)?!--)/, /^(?:\{\{(~)?![\s\S]*?\}\})/, /^(?:\{\{(~)?\*?)/, /^(?:=)/, /^(?:\.\.)/, /^(?:\.(?=([=~}\s\/.)|])))/, /^(?:[\/.])/, /^(?:\s+)/, /^(?:\}(~)?\}\})/, /^(?:(~)?\}\})/, /^(?:"(\\["]|[^"])*")/, /^(?:'(\\[']|[^'])*')/, /^(?:@)/, /^(?:true(?=([~}\s)])))/, /^(?:false(?=([~}\s)])))/, /^(?:undefined(?=([~}\s)])))/, /^(?:null(?=([~}\s)])))/, /^(?:-?[0-9]+(?:\.[0-9]+)?(?=([~}\s)])))/, /^(?:as\s+\|)/, /^(?:\|)/, /^(?:([^\s!"#%-,\.\/;->@\[-\^`\{-~]+(?=([=~}\s\/.)|]))))/, /^(?:\[(\\\]|[^\]])*\])/, /^(?:.)/, /^(?:$)/];
      lexer.conditions = {
        "mu": {
          "rules": [7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44],
          "inclusive": false
        },
        "emu": {
          "rules": [2],
          "inclusive": false
        },
        "com": {
          "rules": [6],
          "inclusive": false
        },
        "raw": {
          "rules": [3, 4, 5],
          "inclusive": false
        },
        "INITIAL": {
          "rules": [0, 1, 44],
          "inclusive": true
        }
      };
      return lexer;
    }();

    parser.lexer = lexer;

    function Parser() {
      this.yy = {};
    }

    Parser.prototype = parser;
    parser.Parser = Parser;
    return new Parser();
  }();

  _exports.parser = handlebars;
  const errorProps = ['description', 'fileName', 'lineNumber', 'message', 'name', 'number', 'stack'];

  function Exception(message, node) {
    let loc = node && node.loc,
        line,
        column;

    if (loc) {
      line = loc.start.line;
      column = loc.start.column;
      message += ' - ' + line + ':' + column;
    }

    let tmp = Error.prototype.constructor.call(this, message); // Unfortunately errors are not enumerable in Chrome (at least), so `for prop in tmp` doesn't work.

    for (let idx = 0; idx < errorProps.length; idx++) {
      this[errorProps[idx]] = tmp[errorProps[idx]];
    }
    /* istanbul ignore else */


    if (Error.captureStackTrace) {
      Error.captureStackTrace(this, Exception);
    }

    try {
      if (loc) {
        this.lineNumber = line; // Work around issue under safari where we can't directly set the column value

        /* istanbul ignore next */

        if (Object.defineProperty) {
          Object.defineProperty(this, 'column', {
            value: column,
            enumerable: true
          });
        } else {
          this.column = column;
        }
      }
    } catch (nop) {
      /* Ignore if the browser is very particular */
    }
  }

  Exception.prototype = new Error();

  function Visitor() {
    this.parents = [];
  }

  Visitor.prototype = {
    constructor: Visitor,
    mutating: false,
    // Visits a given value. If mutating, will replace the value if necessary.
    acceptKey: function (node, name) {
      let value = this.accept(node[name]);

      if (this.mutating) {
        // Hacky sanity check: This may have a few false positives for type for the helper
        // methods but will generally do the right thing without a lot of overhead.
        if (value && !Visitor.prototype[value.type]) {
          throw new Exception('Unexpected node type "' + value.type + '" found when accepting ' + name + ' on ' + node.type);
        }

        node[name] = value;
      }
    },
    // Performs an accept operation with added sanity check to ensure
    // required keys are not removed.
    acceptRequired: function (node, name) {
      this.acceptKey(node, name);

      if (!node[name]) {
        throw new Exception(node.type + ' requires ' + name);
      }
    },
    // Traverses a given array. If mutating, empty respnses will be removed
    // for child elements.
    acceptArray: function (array) {
      for (let i = 0, l = array.length; i < l; i++) {
        this.acceptKey(array, i);

        if (!array[i]) {
          array.splice(i, 1);
          i--;
          l--;
        }
      }
    },
    accept: function (object) {
      if (!object) {
        return;
      }
      /* istanbul ignore next: Sanity code */


      if (!this[object.type]) {
        throw new Exception('Unknown type: ' + object.type, object);
      }

      if (this.current) {
        this.parents.unshift(this.current);
      }

      this.current = object;
      let ret = this[object.type](object);
      this.current = this.parents.shift();

      if (!this.mutating || ret) {
        return ret;
      } else if (ret !== false) {
        return object;
      }
    },
    Program: function (program) {
      this.acceptArray(program.body);
    },
    MustacheStatement: visitSubExpression,
    Decorator: visitSubExpression,
    BlockStatement: visitBlock,
    DecoratorBlock: visitBlock,
    PartialStatement: visitPartial,
    PartialBlockStatement: function (partial) {
      visitPartial.call(this, partial);
      this.acceptKey(partial, 'program');
    },
    ContentStatement: function ()
    /* content */
    {},
    CommentStatement: function ()
    /* comment */
    {},
    SubExpression: visitSubExpression,
    PathExpression: function ()
    /* path */
    {},
    StringLiteral: function ()
    /* string */
    {},
    NumberLiteral: function ()
    /* number */
    {},
    BooleanLiteral: function ()
    /* bool */
    {},
    UndefinedLiteral: function ()
    /* literal */
    {},
    NullLiteral: function ()
    /* literal */
    {},
    Hash: function (hash) {
      this.acceptArray(hash.pairs);
    },
    HashPair: function (pair) {
      this.acceptRequired(pair, 'value');
    }
  };

  function visitSubExpression(mustache) {
    this.acceptRequired(mustache, 'path');
    this.acceptArray(mustache.params);
    this.acceptKey(mustache, 'hash');
  }

  function visitBlock(block) {
    visitSubExpression.call(this, block);
    this.acceptKey(block, 'program');
    this.acceptKey(block, 'inverse');
  }

  function visitPartial(partial) {
    this.acceptRequired(partial, 'name');
    this.acceptArray(partial.params);
    this.acceptKey(partial, 'hash');
  }

  function WhitespaceControl(options = {}) {
    this.options = options;
  }

  WhitespaceControl.prototype = new Visitor();

  WhitespaceControl.prototype.Program = function (program) {
    const doStandalone = !this.options.ignoreStandalone;
    let isRoot = !this.isRootSeen;
    this.isRootSeen = true;
    let body = program.body;

    for (let i = 0, l = body.length; i < l; i++) {
      let current = body[i],
          strip = this.accept(current);

      if (!strip) {
        continue;
      }

      let _isPrevWhitespace = isPrevWhitespace(body, i, isRoot),
          _isNextWhitespace = isNextWhitespace(body, i, isRoot),
          openStandalone = strip.openStandalone && _isPrevWhitespace,
          closeStandalone = strip.closeStandalone && _isNextWhitespace,
          inlineStandalone = strip.inlineStandalone && _isPrevWhitespace && _isNextWhitespace;

      if (strip.close) {
        omitRight(body, i, true);
      }

      if (strip.open) {
        omitLeft(body, i, true);
      }

      if (doStandalone && inlineStandalone) {
        omitRight(body, i);

        if (omitLeft(body, i)) {
          // If we are on a standalone node, save the indent info for partials
          if (current.type === 'PartialStatement') {
            // Pull out the whitespace from the final line
            current.indent = /([ \t]+$)/.exec(body[i - 1].original)[1];
          }
        }
      }

      if (doStandalone && openStandalone) {
        omitRight((current.program || current.inverse).body); // Strip out the previous content node if it's whitespace only

        omitLeft(body, i);
      }

      if (doStandalone && closeStandalone) {
        // Always strip the next node
        omitRight(body, i);
        omitLeft((current.inverse || current.program).body);
      }
    }

    return program;
  };

  WhitespaceControl.prototype.BlockStatement = WhitespaceControl.prototype.DecoratorBlock = WhitespaceControl.prototype.PartialBlockStatement = function (block) {
    this.accept(block.program);
    this.accept(block.inverse); // Find the inverse program that is involed with whitespace stripping.

    let program = block.program || block.inverse,
        inverse = block.program && block.inverse,
        firstInverse = inverse,
        lastInverse = inverse;

    if (inverse && inverse.chained) {
      firstInverse = inverse.body[0].program; // Walk the inverse chain to find the last inverse that is actually in the chain.

      while (lastInverse.chained) {
        lastInverse = lastInverse.body[lastInverse.body.length - 1].program;
      }
    }

    let strip = {
      open: block.openStrip.open,
      close: block.closeStrip.close,
      // Determine the standalone candiacy. Basically flag our content as being possibly standalone
      // so our parent can determine if we actually are standalone
      openStandalone: isNextWhitespace(program.body),
      closeStandalone: isPrevWhitespace((firstInverse || program).body)
    };

    if (block.openStrip.close) {
      omitRight(program.body, null, true);
    }

    if (inverse) {
      let inverseStrip = block.inverseStrip;

      if (inverseStrip.open) {
        omitLeft(program.body, null, true);
      }

      if (inverseStrip.close) {
        omitRight(firstInverse.body, null, true);
      }

      if (block.closeStrip.open) {
        omitLeft(lastInverse.body, null, true);
      } // Find standalone else statments


      if (!this.options.ignoreStandalone && isPrevWhitespace(program.body) && isNextWhitespace(firstInverse.body)) {
        omitLeft(program.body);
        omitRight(firstInverse.body);
      }
    } else if (block.closeStrip.open) {
      omitLeft(program.body, null, true);
    }

    return strip;
  };

  WhitespaceControl.prototype.Decorator = WhitespaceControl.prototype.MustacheStatement = function (mustache) {
    return mustache.strip;
  };

  WhitespaceControl.prototype.PartialStatement = WhitespaceControl.prototype.CommentStatement = function (node) {
    /* istanbul ignore next */
    let strip = node.strip || {};
    return {
      inlineStandalone: true,
      open: strip.open,
      close: strip.close
    };
  };

  function isPrevWhitespace(body, i, isRoot) {
    if (i === undefined) {
      i = body.length;
    } // Nodes that end with newlines are considered whitespace (but are special
    // cased for strip operations)


    let prev = body[i - 1],
        sibling = body[i - 2];

    if (!prev) {
      return isRoot;
    }

    if (prev.type === 'ContentStatement') {
      return (sibling || !isRoot ? /\r?\n\s*?$/ : /(^|\r?\n)\s*?$/).test(prev.original);
    }
  }

  function isNextWhitespace(body, i, isRoot) {
    if (i === undefined) {
      i = -1;
    }

    let next = body[i + 1],
        sibling = body[i + 2];

    if (!next) {
      return isRoot;
    }

    if (next.type === 'ContentStatement') {
      return (sibling || !isRoot ? /^\s*?\r?\n/ : /^\s*?(\r?\n|$)/).test(next.original);
    }
  } // Marks the node to the right of the position as omitted.
  // I.e. {{foo}}' ' will mark the ' ' node as omitted.
  //
  // If i is undefined, then the first child will be marked as such.
  //
  // If mulitple is truthy then all whitespace will be stripped out until non-whitespace
  // content is met.


  function omitRight(body, i, multiple) {
    let current = body[i == null ? 0 : i + 1];

    if (!current || current.type !== 'ContentStatement' || !multiple && current.rightStripped) {
      return;
    }

    let original = current.value;
    current.value = current.value.replace(multiple ? /^\s+/ : /^[ \t]*\r?\n?/, '');
    current.rightStripped = current.value !== original;
  } // Marks the node to the left of the position as omitted.
  // I.e. ' '{{foo}} will mark the ' ' node as omitted.
  //
  // If i is undefined then the last child will be marked as such.
  //
  // If mulitple is truthy then all whitespace will be stripped out until non-whitespace
  // content is met.


  function omitLeft(body, i, multiple) {
    let current = body[i == null ? body.length - 1 : i - 1];

    if (!current || current.type !== 'ContentStatement' || !multiple && current.leftStripped) {
      return;
    } // We omit the last node if it's whitespace only and not preceeded by a non-content node.


    let original = current.value;
    current.value = current.value.replace(multiple ? /\s+$/ : /[ \t]+$/, '');
    current.leftStripped = current.value !== original;
    return current.leftStripped;
  }

  function validateClose(open, close) {
    close = close.path ? close.path.original : close;

    if (open.path.original !== close) {
      let errorNode = {
        loc: open.path.loc
      };
      throw new Exception(open.path.original + " doesn't match " + close, errorNode);
    }
  }

  function SourceLocation(source, locInfo) {
    this.source = source;
    this.start = {
      line: locInfo.first_line,
      column: locInfo.first_column
    };
    this.end = {
      line: locInfo.last_line,
      column: locInfo.last_column
    };
  }

  function id(token) {
    if (/^\[.*\]$/.test(token)) {
      return token.substr(1, token.length - 2);
    } else {
      return token;
    }
  }

  function stripFlags(open, close) {
    return {
      open: open.charAt(2) === '~',
      close: close.charAt(close.length - 3) === '~'
    };
  }

  function stripComment(comment) {
    return comment.replace(/^\{\{~?!-?-?/, '').replace(/-?-?~?\}\}$/, '');
  }

  function preparePath(data, parts, loc) {
    loc = this.locInfo(loc);
    let original = data ? '@' : '',
        dig = [],
        depth = 0;

    for (let i = 0, l = parts.length; i < l; i++) {
      let part = parts[i].part,
          // If we have [] syntax then we do not treat path references as operators,
      // i.e. foo.[this] resolves to approximately context.foo['this']
      isLiteral = parts[i].original !== part;
      original += (parts[i].separator || '') + part;

      if (!isLiteral && (part === '..' || part === '.' || part === 'this')) {
        if (dig.length > 0) {
          throw new Exception('Invalid path: ' + original, {
            loc
          });
        } else if (part === '..') {
          depth++;
        }
      } else {
        dig.push(part);
      }
    }

    return {
      type: 'PathExpression',
      data,
      depth,
      parts: dig,
      original,
      loc
    };
  }

  function prepareMustache(path, params, hash, open, strip, locInfo) {
    // Must use charAt to support IE pre-10
    let escapeFlag = open.charAt(3) || open.charAt(2),
        escaped = escapeFlag !== '{' && escapeFlag !== '&';
    let decorator = /\*/.test(open);
    return {
      type: decorator ? 'Decorator' : 'MustacheStatement',
      path,
      params,
      hash,
      escaped,
      strip,
      loc: this.locInfo(locInfo)
    };
  }

  function prepareRawBlock(openRawBlock, contents, close, locInfo) {
    validateClose(openRawBlock, close);
    locInfo = this.locInfo(locInfo);
    let program = {
      type: 'Program',
      body: contents,
      strip: {},
      loc: locInfo
    };
    return {
      type: 'BlockStatement',
      path: openRawBlock.path,
      params: openRawBlock.params,
      hash: openRawBlock.hash,
      program,
      openStrip: {},
      inverseStrip: {},
      closeStrip: {},
      loc: locInfo
    };
  }

  function prepareBlock(openBlock, program, inverseAndProgram, close, inverted, locInfo) {
    if (close && close.path) {
      validateClose(openBlock, close);
    }

    let decorator = /\*/.test(openBlock.open);
    program.blockParams = openBlock.blockParams;
    let inverse, inverseStrip;

    if (inverseAndProgram) {
      if (decorator) {
        throw new Exception('Unexpected inverse block on decorator', inverseAndProgram);
      }

      if (inverseAndProgram.chain) {
        inverseAndProgram.program.body[0].closeStrip = close.strip;
      }

      inverseStrip = inverseAndProgram.strip;
      inverse = inverseAndProgram.program;
    }

    if (inverted) {
      inverted = inverse;
      inverse = program;
      program = inverted;
    }

    return {
      type: decorator ? 'DecoratorBlock' : 'BlockStatement',
      path: openBlock.path,
      params: openBlock.params,
      hash: openBlock.hash,
      program,
      inverse,
      openStrip: openBlock.strip,
      inverseStrip,
      closeStrip: close && close.strip,
      loc: this.locInfo(locInfo)
    };
  }

  function prepareProgram(statements, loc) {
    if (!loc && statements.length) {
      const firstLoc = statements[0].loc,
            lastLoc = statements[statements.length - 1].loc;
      /* istanbul ignore else */

      if (firstLoc && lastLoc) {
        loc = {
          source: firstLoc.source,
          start: {
            line: firstLoc.start.line,
            column: firstLoc.start.column
          },
          end: {
            line: lastLoc.end.line,
            column: lastLoc.end.column
          }
        };
      }
    }

    return {
      type: 'Program',
      body: statements,
      strip: {},
      loc: loc
    };
  }

  function preparePartialBlock(open, program, close, locInfo) {
    validateClose(open, close);
    return {
      type: 'PartialBlockStatement',
      name: open.path,
      params: open.params,
      hash: open.hash,
      program,
      openStrip: open.strip,
      closeStrip: close && close.strip,
      loc: this.locInfo(locInfo)
    };
  }

  var Helpers =
  /*#__PURE__*/
  Object.freeze({
    SourceLocation: SourceLocation,
    id: id,
    stripFlags: stripFlags,
    stripComment: stripComment,
    preparePath: preparePath,
    prepareMustache: prepareMustache,
    prepareRawBlock: prepareRawBlock,
    prepareBlock: prepareBlock,
    prepareProgram: prepareProgram,
    preparePartialBlock: preparePartialBlock
  });

  function extend(obj
  /* , ...source */
  ) {
    for (let i = 1; i < arguments.length; i++) {
      for (let key in arguments[i]) {
        if (Object.prototype.hasOwnProperty.call(arguments[i], key)) {
          obj[key] = arguments[i][key];
        }
      }
    }

    return obj;
  }

  let toString = Object.prototype.toString; // Sourced from lodash
  // https://github.com/bestiejs/lodash/blob/master/LICENSE.txt

  /* eslint-disable func-style */

  let isFunction = function (value) {
    return typeof value === 'function';
  }; // fallback for older versions of Chrome and Safari

  /* istanbul ignore next */


  if (isFunction(/x/)) {
    isFunction = function (value) {
      return typeof value === 'function' && toString.call(value) === '[object Function]';
    };
  }

  let yy = {};
  extend(yy, Helpers);

  function parse(input, options) {
    // Just return if an already-compiled AST was passed in.
    if (input.type === 'Program') {
      return input;
    }

    handlebars.yy = yy; // Altering the shared object here, but this is ok as parser is a sync operation

    yy.locInfo = function (locInfo) {
      return new yy.SourceLocation(options && options.srcName, locInfo);
    };

    let strip = new WhitespaceControl(options);
    return strip.accept(handlebars.parse(input));
  }
});
/*global enifed, module */
enifed('node-module', ['exports'], function(_exports) {
  var IS_NODE = typeof module === 'object' && typeof module.require === 'function';
  if (IS_NODE) {
    _exports.require = module.require;
    _exports.module = module;
    _exports.IS_NODE = IS_NODE;
  } else {
    _exports.require = null;
    _exports.module = null;
    _exports.IS_NODE = IS_NODE;
  }
});

enifed("simple-html-tokenizer", ["exports"], function (_exports) {
  "use strict";

  _exports.tokenize = tokenize;
  _exports.Tokenizer = _exports.EventedTokenizer = _exports.EntityParser = _exports.HTML5NamedCharRefs = void 0;

  /**
   * generated from https://raw.githubusercontent.com/w3c/html/26b5126f96f736f796b9e29718138919dd513744/entities.json
   * do not edit
   */
  var namedCharRefs = {
    Aacute: "Á",
    aacute: "á",
    Abreve: "Ă",
    abreve: "ă",
    ac: "∾",
    acd: "∿",
    acE: "∾̳",
    Acirc: "Â",
    acirc: "â",
    acute: "´",
    Acy: "А",
    acy: "а",
    AElig: "Æ",
    aelig: "æ",
    af: "\u2061",
    Afr: "𝔄",
    afr: "𝔞",
    Agrave: "À",
    agrave: "à",
    alefsym: "ℵ",
    aleph: "ℵ",
    Alpha: "Α",
    alpha: "α",
    Amacr: "Ā",
    amacr: "ā",
    amalg: "⨿",
    amp: "&",
    AMP: "&",
    andand: "⩕",
    And: "⩓",
    and: "∧",
    andd: "⩜",
    andslope: "⩘",
    andv: "⩚",
    ang: "∠",
    ange: "⦤",
    angle: "∠",
    angmsdaa: "⦨",
    angmsdab: "⦩",
    angmsdac: "⦪",
    angmsdad: "⦫",
    angmsdae: "⦬",
    angmsdaf: "⦭",
    angmsdag: "⦮",
    angmsdah: "⦯",
    angmsd: "∡",
    angrt: "∟",
    angrtvb: "⊾",
    angrtvbd: "⦝",
    angsph: "∢",
    angst: "Å",
    angzarr: "⍼",
    Aogon: "Ą",
    aogon: "ą",
    Aopf: "𝔸",
    aopf: "𝕒",
    apacir: "⩯",
    ap: "≈",
    apE: "⩰",
    ape: "≊",
    apid: "≋",
    apos: "'",
    ApplyFunction: "\u2061",
    approx: "≈",
    approxeq: "≊",
    Aring: "Å",
    aring: "å",
    Ascr: "𝒜",
    ascr: "𝒶",
    Assign: "≔",
    ast: "*",
    asymp: "≈",
    asympeq: "≍",
    Atilde: "Ã",
    atilde: "ã",
    Auml: "Ä",
    auml: "ä",
    awconint: "∳",
    awint: "⨑",
    backcong: "≌",
    backepsilon: "϶",
    backprime: "‵",
    backsim: "∽",
    backsimeq: "⋍",
    Backslash: "∖",
    Barv: "⫧",
    barvee: "⊽",
    barwed: "⌅",
    Barwed: "⌆",
    barwedge: "⌅",
    bbrk: "⎵",
    bbrktbrk: "⎶",
    bcong: "≌",
    Bcy: "Б",
    bcy: "б",
    bdquo: "„",
    becaus: "∵",
    because: "∵",
    Because: "∵",
    bemptyv: "⦰",
    bepsi: "϶",
    bernou: "ℬ",
    Bernoullis: "ℬ",
    Beta: "Β",
    beta: "β",
    beth: "ℶ",
    between: "≬",
    Bfr: "𝔅",
    bfr: "𝔟",
    bigcap: "⋂",
    bigcirc: "◯",
    bigcup: "⋃",
    bigodot: "⨀",
    bigoplus: "⨁",
    bigotimes: "⨂",
    bigsqcup: "⨆",
    bigstar: "★",
    bigtriangledown: "▽",
    bigtriangleup: "△",
    biguplus: "⨄",
    bigvee: "⋁",
    bigwedge: "⋀",
    bkarow: "⤍",
    blacklozenge: "⧫",
    blacksquare: "▪",
    blacktriangle: "▴",
    blacktriangledown: "▾",
    blacktriangleleft: "◂",
    blacktriangleright: "▸",
    blank: "␣",
    blk12: "▒",
    blk14: "░",
    blk34: "▓",
    block: "█",
    bne: "=⃥",
    bnequiv: "≡⃥",
    bNot: "⫭",
    bnot: "⌐",
    Bopf: "𝔹",
    bopf: "𝕓",
    bot: "⊥",
    bottom: "⊥",
    bowtie: "⋈",
    boxbox: "⧉",
    boxdl: "┐",
    boxdL: "╕",
    boxDl: "╖",
    boxDL: "╗",
    boxdr: "┌",
    boxdR: "╒",
    boxDr: "╓",
    boxDR: "╔",
    boxh: "─",
    boxH: "═",
    boxhd: "┬",
    boxHd: "╤",
    boxhD: "╥",
    boxHD: "╦",
    boxhu: "┴",
    boxHu: "╧",
    boxhU: "╨",
    boxHU: "╩",
    boxminus: "⊟",
    boxplus: "⊞",
    boxtimes: "⊠",
    boxul: "┘",
    boxuL: "╛",
    boxUl: "╜",
    boxUL: "╝",
    boxur: "└",
    boxuR: "╘",
    boxUr: "╙",
    boxUR: "╚",
    boxv: "│",
    boxV: "║",
    boxvh: "┼",
    boxvH: "╪",
    boxVh: "╫",
    boxVH: "╬",
    boxvl: "┤",
    boxvL: "╡",
    boxVl: "╢",
    boxVL: "╣",
    boxvr: "├",
    boxvR: "╞",
    boxVr: "╟",
    boxVR: "╠",
    bprime: "‵",
    breve: "˘",
    Breve: "˘",
    brvbar: "¦",
    bscr: "𝒷",
    Bscr: "ℬ",
    bsemi: "⁏",
    bsim: "∽",
    bsime: "⋍",
    bsolb: "⧅",
    bsol: "\\",
    bsolhsub: "⟈",
    bull: "•",
    bullet: "•",
    bump: "≎",
    bumpE: "⪮",
    bumpe: "≏",
    Bumpeq: "≎",
    bumpeq: "≏",
    Cacute: "Ć",
    cacute: "ć",
    capand: "⩄",
    capbrcup: "⩉",
    capcap: "⩋",
    cap: "∩",
    Cap: "⋒",
    capcup: "⩇",
    capdot: "⩀",
    CapitalDifferentialD: "ⅅ",
    caps: "∩︀",
    caret: "⁁",
    caron: "ˇ",
    Cayleys: "ℭ",
    ccaps: "⩍",
    Ccaron: "Č",
    ccaron: "č",
    Ccedil: "Ç",
    ccedil: "ç",
    Ccirc: "Ĉ",
    ccirc: "ĉ",
    Cconint: "∰",
    ccups: "⩌",
    ccupssm: "⩐",
    Cdot: "Ċ",
    cdot: "ċ",
    cedil: "¸",
    Cedilla: "¸",
    cemptyv: "⦲",
    cent: "¢",
    centerdot: "·",
    CenterDot: "·",
    cfr: "𝔠",
    Cfr: "ℭ",
    CHcy: "Ч",
    chcy: "ч",
    check: "✓",
    checkmark: "✓",
    Chi: "Χ",
    chi: "χ",
    circ: "ˆ",
    circeq: "≗",
    circlearrowleft: "↺",
    circlearrowright: "↻",
    circledast: "⊛",
    circledcirc: "⊚",
    circleddash: "⊝",
    CircleDot: "⊙",
    circledR: "®",
    circledS: "Ⓢ",
    CircleMinus: "⊖",
    CirclePlus: "⊕",
    CircleTimes: "⊗",
    cir: "○",
    cirE: "⧃",
    cire: "≗",
    cirfnint: "⨐",
    cirmid: "⫯",
    cirscir: "⧂",
    ClockwiseContourIntegral: "∲",
    CloseCurlyDoubleQuote: "”",
    CloseCurlyQuote: "’",
    clubs: "♣",
    clubsuit: "♣",
    colon: ":",
    Colon: "∷",
    Colone: "⩴",
    colone: "≔",
    coloneq: "≔",
    comma: ",",
    commat: "@",
    comp: "∁",
    compfn: "∘",
    complement: "∁",
    complexes: "ℂ",
    cong: "≅",
    congdot: "⩭",
    Congruent: "≡",
    conint: "∮",
    Conint: "∯",
    ContourIntegral: "∮",
    copf: "𝕔",
    Copf: "ℂ",
    coprod: "∐",
    Coproduct: "∐",
    copy: "©",
    COPY: "©",
    copysr: "℗",
    CounterClockwiseContourIntegral: "∳",
    crarr: "↵",
    cross: "✗",
    Cross: "⨯",
    Cscr: "𝒞",
    cscr: "𝒸",
    csub: "⫏",
    csube: "⫑",
    csup: "⫐",
    csupe: "⫒",
    ctdot: "⋯",
    cudarrl: "⤸",
    cudarrr: "⤵",
    cuepr: "⋞",
    cuesc: "⋟",
    cularr: "↶",
    cularrp: "⤽",
    cupbrcap: "⩈",
    cupcap: "⩆",
    CupCap: "≍",
    cup: "∪",
    Cup: "⋓",
    cupcup: "⩊",
    cupdot: "⊍",
    cupor: "⩅",
    cups: "∪︀",
    curarr: "↷",
    curarrm: "⤼",
    curlyeqprec: "⋞",
    curlyeqsucc: "⋟",
    curlyvee: "⋎",
    curlywedge: "⋏",
    curren: "¤",
    curvearrowleft: "↶",
    curvearrowright: "↷",
    cuvee: "⋎",
    cuwed: "⋏",
    cwconint: "∲",
    cwint: "∱",
    cylcty: "⌭",
    dagger: "†",
    Dagger: "‡",
    daleth: "ℸ",
    darr: "↓",
    Darr: "↡",
    dArr: "⇓",
    dash: "‐",
    Dashv: "⫤",
    dashv: "⊣",
    dbkarow: "⤏",
    dblac: "˝",
    Dcaron: "Ď",
    dcaron: "ď",
    Dcy: "Д",
    dcy: "д",
    ddagger: "‡",
    ddarr: "⇊",
    DD: "ⅅ",
    dd: "ⅆ",
    DDotrahd: "⤑",
    ddotseq: "⩷",
    deg: "°",
    Del: "∇",
    Delta: "Δ",
    delta: "δ",
    demptyv: "⦱",
    dfisht: "⥿",
    Dfr: "𝔇",
    dfr: "𝔡",
    dHar: "⥥",
    dharl: "⇃",
    dharr: "⇂",
    DiacriticalAcute: "´",
    DiacriticalDot: "˙",
    DiacriticalDoubleAcute: "˝",
    DiacriticalGrave: "`",
    DiacriticalTilde: "˜",
    diam: "⋄",
    diamond: "⋄",
    Diamond: "⋄",
    diamondsuit: "♦",
    diams: "♦",
    die: "¨",
    DifferentialD: "ⅆ",
    digamma: "ϝ",
    disin: "⋲",
    div: "÷",
    divide: "÷",
    divideontimes: "⋇",
    divonx: "⋇",
    DJcy: "Ђ",
    djcy: "ђ",
    dlcorn: "⌞",
    dlcrop: "⌍",
    dollar: "$",
    Dopf: "𝔻",
    dopf: "𝕕",
    Dot: "¨",
    dot: "˙",
    DotDot: "⃜",
    doteq: "≐",
    doteqdot: "≑",
    DotEqual: "≐",
    dotminus: "∸",
    dotplus: "∔",
    dotsquare: "⊡",
    doublebarwedge: "⌆",
    DoubleContourIntegral: "∯",
    DoubleDot: "¨",
    DoubleDownArrow: "⇓",
    DoubleLeftArrow: "⇐",
    DoubleLeftRightArrow: "⇔",
    DoubleLeftTee: "⫤",
    DoubleLongLeftArrow: "⟸",
    DoubleLongLeftRightArrow: "⟺",
    DoubleLongRightArrow: "⟹",
    DoubleRightArrow: "⇒",
    DoubleRightTee: "⊨",
    DoubleUpArrow: "⇑",
    DoubleUpDownArrow: "⇕",
    DoubleVerticalBar: "∥",
    DownArrowBar: "⤓",
    downarrow: "↓",
    DownArrow: "↓",
    Downarrow: "⇓",
    DownArrowUpArrow: "⇵",
    DownBreve: "̑",
    downdownarrows: "⇊",
    downharpoonleft: "⇃",
    downharpoonright: "⇂",
    DownLeftRightVector: "⥐",
    DownLeftTeeVector: "⥞",
    DownLeftVectorBar: "⥖",
    DownLeftVector: "↽",
    DownRightTeeVector: "⥟",
    DownRightVectorBar: "⥗",
    DownRightVector: "⇁",
    DownTeeArrow: "↧",
    DownTee: "⊤",
    drbkarow: "⤐",
    drcorn: "⌟",
    drcrop: "⌌",
    Dscr: "𝒟",
    dscr: "𝒹",
    DScy: "Ѕ",
    dscy: "ѕ",
    dsol: "⧶",
    Dstrok: "Đ",
    dstrok: "đ",
    dtdot: "⋱",
    dtri: "▿",
    dtrif: "▾",
    duarr: "⇵",
    duhar: "⥯",
    dwangle: "⦦",
    DZcy: "Џ",
    dzcy: "џ",
    dzigrarr: "⟿",
    Eacute: "É",
    eacute: "é",
    easter: "⩮",
    Ecaron: "Ě",
    ecaron: "ě",
    Ecirc: "Ê",
    ecirc: "ê",
    ecir: "≖",
    ecolon: "≕",
    Ecy: "Э",
    ecy: "э",
    eDDot: "⩷",
    Edot: "Ė",
    edot: "ė",
    eDot: "≑",
    ee: "ⅇ",
    efDot: "≒",
    Efr: "𝔈",
    efr: "𝔢",
    eg: "⪚",
    Egrave: "È",
    egrave: "è",
    egs: "⪖",
    egsdot: "⪘",
    el: "⪙",
    Element: "∈",
    elinters: "⏧",
    ell: "ℓ",
    els: "⪕",
    elsdot: "⪗",
    Emacr: "Ē",
    emacr: "ē",
    empty: "∅",
    emptyset: "∅",
    EmptySmallSquare: "◻",
    emptyv: "∅",
    EmptyVerySmallSquare: "▫",
    emsp13: " ",
    emsp14: " ",
    emsp: " ",
    ENG: "Ŋ",
    eng: "ŋ",
    ensp: " ",
    Eogon: "Ę",
    eogon: "ę",
    Eopf: "𝔼",
    eopf: "𝕖",
    epar: "⋕",
    eparsl: "⧣",
    eplus: "⩱",
    epsi: "ε",
    Epsilon: "Ε",
    epsilon: "ε",
    epsiv: "ϵ",
    eqcirc: "≖",
    eqcolon: "≕",
    eqsim: "≂",
    eqslantgtr: "⪖",
    eqslantless: "⪕",
    Equal: "⩵",
    equals: "=",
    EqualTilde: "≂",
    equest: "≟",
    Equilibrium: "⇌",
    equiv: "≡",
    equivDD: "⩸",
    eqvparsl: "⧥",
    erarr: "⥱",
    erDot: "≓",
    escr: "ℯ",
    Escr: "ℰ",
    esdot: "≐",
    Esim: "⩳",
    esim: "≂",
    Eta: "Η",
    eta: "η",
    ETH: "Ð",
    eth: "ð",
    Euml: "Ë",
    euml: "ë",
    euro: "€",
    excl: "!",
    exist: "∃",
    Exists: "∃",
    expectation: "ℰ",
    exponentiale: "ⅇ",
    ExponentialE: "ⅇ",
    fallingdotseq: "≒",
    Fcy: "Ф",
    fcy: "ф",
    female: "♀",
    ffilig: "ﬃ",
    fflig: "ﬀ",
    ffllig: "ﬄ",
    Ffr: "𝔉",
    ffr: "𝔣",
    filig: "ﬁ",
    FilledSmallSquare: "◼",
    FilledVerySmallSquare: "▪",
    fjlig: "fj",
    flat: "♭",
    fllig: "ﬂ",
    fltns: "▱",
    fnof: "ƒ",
    Fopf: "𝔽",
    fopf: "𝕗",
    forall: "∀",
    ForAll: "∀",
    fork: "⋔",
    forkv: "⫙",
    Fouriertrf: "ℱ",
    fpartint: "⨍",
    frac12: "½",
    frac13: "⅓",
    frac14: "¼",
    frac15: "⅕",
    frac16: "⅙",
    frac18: "⅛",
    frac23: "⅔",
    frac25: "⅖",
    frac34: "¾",
    frac35: "⅗",
    frac38: "⅜",
    frac45: "⅘",
    frac56: "⅚",
    frac58: "⅝",
    frac78: "⅞",
    frasl: "⁄",
    frown: "⌢",
    fscr: "𝒻",
    Fscr: "ℱ",
    gacute: "ǵ",
    Gamma: "Γ",
    gamma: "γ",
    Gammad: "Ϝ",
    gammad: "ϝ",
    gap: "⪆",
    Gbreve: "Ğ",
    gbreve: "ğ",
    Gcedil: "Ģ",
    Gcirc: "Ĝ",
    gcirc: "ĝ",
    Gcy: "Г",
    gcy: "г",
    Gdot: "Ġ",
    gdot: "ġ",
    ge: "≥",
    gE: "≧",
    gEl: "⪌",
    gel: "⋛",
    geq: "≥",
    geqq: "≧",
    geqslant: "⩾",
    gescc: "⪩",
    ges: "⩾",
    gesdot: "⪀",
    gesdoto: "⪂",
    gesdotol: "⪄",
    gesl: "⋛︀",
    gesles: "⪔",
    Gfr: "𝔊",
    gfr: "𝔤",
    gg: "≫",
    Gg: "⋙",
    ggg: "⋙",
    gimel: "ℷ",
    GJcy: "Ѓ",
    gjcy: "ѓ",
    gla: "⪥",
    gl: "≷",
    glE: "⪒",
    glj: "⪤",
    gnap: "⪊",
    gnapprox: "⪊",
    gne: "⪈",
    gnE: "≩",
    gneq: "⪈",
    gneqq: "≩",
    gnsim: "⋧",
    Gopf: "𝔾",
    gopf: "𝕘",
    grave: "`",
    GreaterEqual: "≥",
    GreaterEqualLess: "⋛",
    GreaterFullEqual: "≧",
    GreaterGreater: "⪢",
    GreaterLess: "≷",
    GreaterSlantEqual: "⩾",
    GreaterTilde: "≳",
    Gscr: "𝒢",
    gscr: "ℊ",
    gsim: "≳",
    gsime: "⪎",
    gsiml: "⪐",
    gtcc: "⪧",
    gtcir: "⩺",
    gt: ">",
    GT: ">",
    Gt: "≫",
    gtdot: "⋗",
    gtlPar: "⦕",
    gtquest: "⩼",
    gtrapprox: "⪆",
    gtrarr: "⥸",
    gtrdot: "⋗",
    gtreqless: "⋛",
    gtreqqless: "⪌",
    gtrless: "≷",
    gtrsim: "≳",
    gvertneqq: "≩︀",
    gvnE: "≩︀",
    Hacek: "ˇ",
    hairsp: " ",
    half: "½",
    hamilt: "ℋ",
    HARDcy: "Ъ",
    hardcy: "ъ",
    harrcir: "⥈",
    harr: "↔",
    hArr: "⇔",
    harrw: "↭",
    Hat: "^",
    hbar: "ℏ",
    Hcirc: "Ĥ",
    hcirc: "ĥ",
    hearts: "♥",
    heartsuit: "♥",
    hellip: "…",
    hercon: "⊹",
    hfr: "𝔥",
    Hfr: "ℌ",
    HilbertSpace: "ℋ",
    hksearow: "⤥",
    hkswarow: "⤦",
    hoarr: "⇿",
    homtht: "∻",
    hookleftarrow: "↩",
    hookrightarrow: "↪",
    hopf: "𝕙",
    Hopf: "ℍ",
    horbar: "―",
    HorizontalLine: "─",
    hscr: "𝒽",
    Hscr: "ℋ",
    hslash: "ℏ",
    Hstrok: "Ħ",
    hstrok: "ħ",
    HumpDownHump: "≎",
    HumpEqual: "≏",
    hybull: "⁃",
    hyphen: "‐",
    Iacute: "Í",
    iacute: "í",
    ic: "\u2063",
    Icirc: "Î",
    icirc: "î",
    Icy: "И",
    icy: "и",
    Idot: "İ",
    IEcy: "Е",
    iecy: "е",
    iexcl: "¡",
    iff: "⇔",
    ifr: "𝔦",
    Ifr: "ℑ",
    Igrave: "Ì",
    igrave: "ì",
    ii: "ⅈ",
    iiiint: "⨌",
    iiint: "∭",
    iinfin: "⧜",
    iiota: "℩",
    IJlig: "Ĳ",
    ijlig: "ĳ",
    Imacr: "Ī",
    imacr: "ī",
    image: "ℑ",
    ImaginaryI: "ⅈ",
    imagline: "ℐ",
    imagpart: "ℑ",
    imath: "ı",
    Im: "ℑ",
    imof: "⊷",
    imped: "Ƶ",
    Implies: "⇒",
    incare: "℅",
    in: "∈",
    infin: "∞",
    infintie: "⧝",
    inodot: "ı",
    intcal: "⊺",
    int: "∫",
    Int: "∬",
    integers: "ℤ",
    Integral: "∫",
    intercal: "⊺",
    Intersection: "⋂",
    intlarhk: "⨗",
    intprod: "⨼",
    InvisibleComma: "\u2063",
    InvisibleTimes: "\u2062",
    IOcy: "Ё",
    iocy: "ё",
    Iogon: "Į",
    iogon: "į",
    Iopf: "𝕀",
    iopf: "𝕚",
    Iota: "Ι",
    iota: "ι",
    iprod: "⨼",
    iquest: "¿",
    iscr: "𝒾",
    Iscr: "ℐ",
    isin: "∈",
    isindot: "⋵",
    isinE: "⋹",
    isins: "⋴",
    isinsv: "⋳",
    isinv: "∈",
    it: "\u2062",
    Itilde: "Ĩ",
    itilde: "ĩ",
    Iukcy: "І",
    iukcy: "і",
    Iuml: "Ï",
    iuml: "ï",
    Jcirc: "Ĵ",
    jcirc: "ĵ",
    Jcy: "Й",
    jcy: "й",
    Jfr: "𝔍",
    jfr: "𝔧",
    jmath: "ȷ",
    Jopf: "𝕁",
    jopf: "𝕛",
    Jscr: "𝒥",
    jscr: "𝒿",
    Jsercy: "Ј",
    jsercy: "ј",
    Jukcy: "Є",
    jukcy: "є",
    Kappa: "Κ",
    kappa: "κ",
    kappav: "ϰ",
    Kcedil: "Ķ",
    kcedil: "ķ",
    Kcy: "К",
    kcy: "к",
    Kfr: "𝔎",
    kfr: "𝔨",
    kgreen: "ĸ",
    KHcy: "Х",
    khcy: "х",
    KJcy: "Ќ",
    kjcy: "ќ",
    Kopf: "𝕂",
    kopf: "𝕜",
    Kscr: "𝒦",
    kscr: "𝓀",
    lAarr: "⇚",
    Lacute: "Ĺ",
    lacute: "ĺ",
    laemptyv: "⦴",
    lagran: "ℒ",
    Lambda: "Λ",
    lambda: "λ",
    lang: "⟨",
    Lang: "⟪",
    langd: "⦑",
    langle: "⟨",
    lap: "⪅",
    Laplacetrf: "ℒ",
    laquo: "«",
    larrb: "⇤",
    larrbfs: "⤟",
    larr: "←",
    Larr: "↞",
    lArr: "⇐",
    larrfs: "⤝",
    larrhk: "↩",
    larrlp: "↫",
    larrpl: "⤹",
    larrsim: "⥳",
    larrtl: "↢",
    latail: "⤙",
    lAtail: "⤛",
    lat: "⪫",
    late: "⪭",
    lates: "⪭︀",
    lbarr: "⤌",
    lBarr: "⤎",
    lbbrk: "❲",
    lbrace: "{",
    lbrack: "[",
    lbrke: "⦋",
    lbrksld: "⦏",
    lbrkslu: "⦍",
    Lcaron: "Ľ",
    lcaron: "ľ",
    Lcedil: "Ļ",
    lcedil: "ļ",
    lceil: "⌈",
    lcub: "{",
    Lcy: "Л",
    lcy: "л",
    ldca: "⤶",
    ldquo: "“",
    ldquor: "„",
    ldrdhar: "⥧",
    ldrushar: "⥋",
    ldsh: "↲",
    le: "≤",
    lE: "≦",
    LeftAngleBracket: "⟨",
    LeftArrowBar: "⇤",
    leftarrow: "←",
    LeftArrow: "←",
    Leftarrow: "⇐",
    LeftArrowRightArrow: "⇆",
    leftarrowtail: "↢",
    LeftCeiling: "⌈",
    LeftDoubleBracket: "⟦",
    LeftDownTeeVector: "⥡",
    LeftDownVectorBar: "⥙",
    LeftDownVector: "⇃",
    LeftFloor: "⌊",
    leftharpoondown: "↽",
    leftharpoonup: "↼",
    leftleftarrows: "⇇",
    leftrightarrow: "↔",
    LeftRightArrow: "↔",
    Leftrightarrow: "⇔",
    leftrightarrows: "⇆",
    leftrightharpoons: "⇋",
    leftrightsquigarrow: "↭",
    LeftRightVector: "⥎",
    LeftTeeArrow: "↤",
    LeftTee: "⊣",
    LeftTeeVector: "⥚",
    leftthreetimes: "⋋",
    LeftTriangleBar: "⧏",
    LeftTriangle: "⊲",
    LeftTriangleEqual: "⊴",
    LeftUpDownVector: "⥑",
    LeftUpTeeVector: "⥠",
    LeftUpVectorBar: "⥘",
    LeftUpVector: "↿",
    LeftVectorBar: "⥒",
    LeftVector: "↼",
    lEg: "⪋",
    leg: "⋚",
    leq: "≤",
    leqq: "≦",
    leqslant: "⩽",
    lescc: "⪨",
    les: "⩽",
    lesdot: "⩿",
    lesdoto: "⪁",
    lesdotor: "⪃",
    lesg: "⋚︀",
    lesges: "⪓",
    lessapprox: "⪅",
    lessdot: "⋖",
    lesseqgtr: "⋚",
    lesseqqgtr: "⪋",
    LessEqualGreater: "⋚",
    LessFullEqual: "≦",
    LessGreater: "≶",
    lessgtr: "≶",
    LessLess: "⪡",
    lesssim: "≲",
    LessSlantEqual: "⩽",
    LessTilde: "≲",
    lfisht: "⥼",
    lfloor: "⌊",
    Lfr: "𝔏",
    lfr: "𝔩",
    lg: "≶",
    lgE: "⪑",
    lHar: "⥢",
    lhard: "↽",
    lharu: "↼",
    lharul: "⥪",
    lhblk: "▄",
    LJcy: "Љ",
    ljcy: "љ",
    llarr: "⇇",
    ll: "≪",
    Ll: "⋘",
    llcorner: "⌞",
    Lleftarrow: "⇚",
    llhard: "⥫",
    lltri: "◺",
    Lmidot: "Ŀ",
    lmidot: "ŀ",
    lmoustache: "⎰",
    lmoust: "⎰",
    lnap: "⪉",
    lnapprox: "⪉",
    lne: "⪇",
    lnE: "≨",
    lneq: "⪇",
    lneqq: "≨",
    lnsim: "⋦",
    loang: "⟬",
    loarr: "⇽",
    lobrk: "⟦",
    longleftarrow: "⟵",
    LongLeftArrow: "⟵",
    Longleftarrow: "⟸",
    longleftrightarrow: "⟷",
    LongLeftRightArrow: "⟷",
    Longleftrightarrow: "⟺",
    longmapsto: "⟼",
    longrightarrow: "⟶",
    LongRightArrow: "⟶",
    Longrightarrow: "⟹",
    looparrowleft: "↫",
    looparrowright: "↬",
    lopar: "⦅",
    Lopf: "𝕃",
    lopf: "𝕝",
    loplus: "⨭",
    lotimes: "⨴",
    lowast: "∗",
    lowbar: "_",
    LowerLeftArrow: "↙",
    LowerRightArrow: "↘",
    loz: "◊",
    lozenge: "◊",
    lozf: "⧫",
    lpar: "(",
    lparlt: "⦓",
    lrarr: "⇆",
    lrcorner: "⌟",
    lrhar: "⇋",
    lrhard: "⥭",
    lrm: "\u200e",
    lrtri: "⊿",
    lsaquo: "‹",
    lscr: "𝓁",
    Lscr: "ℒ",
    lsh: "↰",
    Lsh: "↰",
    lsim: "≲",
    lsime: "⪍",
    lsimg: "⪏",
    lsqb: "[",
    lsquo: "‘",
    lsquor: "‚",
    Lstrok: "Ł",
    lstrok: "ł",
    ltcc: "⪦",
    ltcir: "⩹",
    lt: "<",
    LT: "<",
    Lt: "≪",
    ltdot: "⋖",
    lthree: "⋋",
    ltimes: "⋉",
    ltlarr: "⥶",
    ltquest: "⩻",
    ltri: "◃",
    ltrie: "⊴",
    ltrif: "◂",
    ltrPar: "⦖",
    lurdshar: "⥊",
    luruhar: "⥦",
    lvertneqq: "≨︀",
    lvnE: "≨︀",
    macr: "¯",
    male: "♂",
    malt: "✠",
    maltese: "✠",
    Map: "⤅",
    map: "↦",
    mapsto: "↦",
    mapstodown: "↧",
    mapstoleft: "↤",
    mapstoup: "↥",
    marker: "▮",
    mcomma: "⨩",
    Mcy: "М",
    mcy: "м",
    mdash: "—",
    mDDot: "∺",
    measuredangle: "∡",
    MediumSpace: " ",
    Mellintrf: "ℳ",
    Mfr: "𝔐",
    mfr: "𝔪",
    mho: "℧",
    micro: "µ",
    midast: "*",
    midcir: "⫰",
    mid: "∣",
    middot: "·",
    minusb: "⊟",
    minus: "−",
    minusd: "∸",
    minusdu: "⨪",
    MinusPlus: "∓",
    mlcp: "⫛",
    mldr: "…",
    mnplus: "∓",
    models: "⊧",
    Mopf: "𝕄",
    mopf: "𝕞",
    mp: "∓",
    mscr: "𝓂",
    Mscr: "ℳ",
    mstpos: "∾",
    Mu: "Μ",
    mu: "μ",
    multimap: "⊸",
    mumap: "⊸",
    nabla: "∇",
    Nacute: "Ń",
    nacute: "ń",
    nang: "∠⃒",
    nap: "≉",
    napE: "⩰̸",
    napid: "≋̸",
    napos: "ŉ",
    napprox: "≉",
    natural: "♮",
    naturals: "ℕ",
    natur: "♮",
    nbsp: " ",
    nbump: "≎̸",
    nbumpe: "≏̸",
    ncap: "⩃",
    Ncaron: "Ň",
    ncaron: "ň",
    Ncedil: "Ņ",
    ncedil: "ņ",
    ncong: "≇",
    ncongdot: "⩭̸",
    ncup: "⩂",
    Ncy: "Н",
    ncy: "н",
    ndash: "–",
    nearhk: "⤤",
    nearr: "↗",
    neArr: "⇗",
    nearrow: "↗",
    ne: "≠",
    nedot: "≐̸",
    NegativeMediumSpace: "​",
    NegativeThickSpace: "​",
    NegativeThinSpace: "​",
    NegativeVeryThinSpace: "​",
    nequiv: "≢",
    nesear: "⤨",
    nesim: "≂̸",
    NestedGreaterGreater: "≫",
    NestedLessLess: "≪",
    NewLine: "\u000a",
    nexist: "∄",
    nexists: "∄",
    Nfr: "𝔑",
    nfr: "𝔫",
    ngE: "≧̸",
    nge: "≱",
    ngeq: "≱",
    ngeqq: "≧̸",
    ngeqslant: "⩾̸",
    nges: "⩾̸",
    nGg: "⋙̸",
    ngsim: "≵",
    nGt: "≫⃒",
    ngt: "≯",
    ngtr: "≯",
    nGtv: "≫̸",
    nharr: "↮",
    nhArr: "⇎",
    nhpar: "⫲",
    ni: "∋",
    nis: "⋼",
    nisd: "⋺",
    niv: "∋",
    NJcy: "Њ",
    njcy: "њ",
    nlarr: "↚",
    nlArr: "⇍",
    nldr: "‥",
    nlE: "≦̸",
    nle: "≰",
    nleftarrow: "↚",
    nLeftarrow: "⇍",
    nleftrightarrow: "↮",
    nLeftrightarrow: "⇎",
    nleq: "≰",
    nleqq: "≦̸",
    nleqslant: "⩽̸",
    nles: "⩽̸",
    nless: "≮",
    nLl: "⋘̸",
    nlsim: "≴",
    nLt: "≪⃒",
    nlt: "≮",
    nltri: "⋪",
    nltrie: "⋬",
    nLtv: "≪̸",
    nmid: "∤",
    NoBreak: "\u2060",
    NonBreakingSpace: " ",
    nopf: "𝕟",
    Nopf: "ℕ",
    Not: "⫬",
    not: "¬",
    NotCongruent: "≢",
    NotCupCap: "≭",
    NotDoubleVerticalBar: "∦",
    NotElement: "∉",
    NotEqual: "≠",
    NotEqualTilde: "≂̸",
    NotExists: "∄",
    NotGreater: "≯",
    NotGreaterEqual: "≱",
    NotGreaterFullEqual: "≧̸",
    NotGreaterGreater: "≫̸",
    NotGreaterLess: "≹",
    NotGreaterSlantEqual: "⩾̸",
    NotGreaterTilde: "≵",
    NotHumpDownHump: "≎̸",
    NotHumpEqual: "≏̸",
    notin: "∉",
    notindot: "⋵̸",
    notinE: "⋹̸",
    notinva: "∉",
    notinvb: "⋷",
    notinvc: "⋶",
    NotLeftTriangleBar: "⧏̸",
    NotLeftTriangle: "⋪",
    NotLeftTriangleEqual: "⋬",
    NotLess: "≮",
    NotLessEqual: "≰",
    NotLessGreater: "≸",
    NotLessLess: "≪̸",
    NotLessSlantEqual: "⩽̸",
    NotLessTilde: "≴",
    NotNestedGreaterGreater: "⪢̸",
    NotNestedLessLess: "⪡̸",
    notni: "∌",
    notniva: "∌",
    notnivb: "⋾",
    notnivc: "⋽",
    NotPrecedes: "⊀",
    NotPrecedesEqual: "⪯̸",
    NotPrecedesSlantEqual: "⋠",
    NotReverseElement: "∌",
    NotRightTriangleBar: "⧐̸",
    NotRightTriangle: "⋫",
    NotRightTriangleEqual: "⋭",
    NotSquareSubset: "⊏̸",
    NotSquareSubsetEqual: "⋢",
    NotSquareSuperset: "⊐̸",
    NotSquareSupersetEqual: "⋣",
    NotSubset: "⊂⃒",
    NotSubsetEqual: "⊈",
    NotSucceeds: "⊁",
    NotSucceedsEqual: "⪰̸",
    NotSucceedsSlantEqual: "⋡",
    NotSucceedsTilde: "≿̸",
    NotSuperset: "⊃⃒",
    NotSupersetEqual: "⊉",
    NotTilde: "≁",
    NotTildeEqual: "≄",
    NotTildeFullEqual: "≇",
    NotTildeTilde: "≉",
    NotVerticalBar: "∤",
    nparallel: "∦",
    npar: "∦",
    nparsl: "⫽⃥",
    npart: "∂̸",
    npolint: "⨔",
    npr: "⊀",
    nprcue: "⋠",
    nprec: "⊀",
    npreceq: "⪯̸",
    npre: "⪯̸",
    nrarrc: "⤳̸",
    nrarr: "↛",
    nrArr: "⇏",
    nrarrw: "↝̸",
    nrightarrow: "↛",
    nRightarrow: "⇏",
    nrtri: "⋫",
    nrtrie: "⋭",
    nsc: "⊁",
    nsccue: "⋡",
    nsce: "⪰̸",
    Nscr: "𝒩",
    nscr: "𝓃",
    nshortmid: "∤",
    nshortparallel: "∦",
    nsim: "≁",
    nsime: "≄",
    nsimeq: "≄",
    nsmid: "∤",
    nspar: "∦",
    nsqsube: "⋢",
    nsqsupe: "⋣",
    nsub: "⊄",
    nsubE: "⫅̸",
    nsube: "⊈",
    nsubset: "⊂⃒",
    nsubseteq: "⊈",
    nsubseteqq: "⫅̸",
    nsucc: "⊁",
    nsucceq: "⪰̸",
    nsup: "⊅",
    nsupE: "⫆̸",
    nsupe: "⊉",
    nsupset: "⊃⃒",
    nsupseteq: "⊉",
    nsupseteqq: "⫆̸",
    ntgl: "≹",
    Ntilde: "Ñ",
    ntilde: "ñ",
    ntlg: "≸",
    ntriangleleft: "⋪",
    ntrianglelefteq: "⋬",
    ntriangleright: "⋫",
    ntrianglerighteq: "⋭",
    Nu: "Ν",
    nu: "ν",
    num: "#",
    numero: "№",
    numsp: " ",
    nvap: "≍⃒",
    nvdash: "⊬",
    nvDash: "⊭",
    nVdash: "⊮",
    nVDash: "⊯",
    nvge: "≥⃒",
    nvgt: ">⃒",
    nvHarr: "⤄",
    nvinfin: "⧞",
    nvlArr: "⤂",
    nvle: "≤⃒",
    nvlt: "<⃒",
    nvltrie: "⊴⃒",
    nvrArr: "⤃",
    nvrtrie: "⊵⃒",
    nvsim: "∼⃒",
    nwarhk: "⤣",
    nwarr: "↖",
    nwArr: "⇖",
    nwarrow: "↖",
    nwnear: "⤧",
    Oacute: "Ó",
    oacute: "ó",
    oast: "⊛",
    Ocirc: "Ô",
    ocirc: "ô",
    ocir: "⊚",
    Ocy: "О",
    ocy: "о",
    odash: "⊝",
    Odblac: "Ő",
    odblac: "ő",
    odiv: "⨸",
    odot: "⊙",
    odsold: "⦼",
    OElig: "Œ",
    oelig: "œ",
    ofcir: "⦿",
    Ofr: "𝔒",
    ofr: "𝔬",
    ogon: "˛",
    Ograve: "Ò",
    ograve: "ò",
    ogt: "⧁",
    ohbar: "⦵",
    ohm: "Ω",
    oint: "∮",
    olarr: "↺",
    olcir: "⦾",
    olcross: "⦻",
    oline: "‾",
    olt: "⧀",
    Omacr: "Ō",
    omacr: "ō",
    Omega: "Ω",
    omega: "ω",
    Omicron: "Ο",
    omicron: "ο",
    omid: "⦶",
    ominus: "⊖",
    Oopf: "𝕆",
    oopf: "𝕠",
    opar: "⦷",
    OpenCurlyDoubleQuote: "“",
    OpenCurlyQuote: "‘",
    operp: "⦹",
    oplus: "⊕",
    orarr: "↻",
    Or: "⩔",
    or: "∨",
    ord: "⩝",
    order: "ℴ",
    orderof: "ℴ",
    ordf: "ª",
    ordm: "º",
    origof: "⊶",
    oror: "⩖",
    orslope: "⩗",
    orv: "⩛",
    oS: "Ⓢ",
    Oscr: "𝒪",
    oscr: "ℴ",
    Oslash: "Ø",
    oslash: "ø",
    osol: "⊘",
    Otilde: "Õ",
    otilde: "õ",
    otimesas: "⨶",
    Otimes: "⨷",
    otimes: "⊗",
    Ouml: "Ö",
    ouml: "ö",
    ovbar: "⌽",
    OverBar: "‾",
    OverBrace: "⏞",
    OverBracket: "⎴",
    OverParenthesis: "⏜",
    para: "¶",
    parallel: "∥",
    par: "∥",
    parsim: "⫳",
    parsl: "⫽",
    part: "∂",
    PartialD: "∂",
    Pcy: "П",
    pcy: "п",
    percnt: "%",
    period: ".",
    permil: "‰",
    perp: "⊥",
    pertenk: "‱",
    Pfr: "𝔓",
    pfr: "𝔭",
    Phi: "Φ",
    phi: "φ",
    phiv: "ϕ",
    phmmat: "ℳ",
    phone: "☎",
    Pi: "Π",
    pi: "π",
    pitchfork: "⋔",
    piv: "ϖ",
    planck: "ℏ",
    planckh: "ℎ",
    plankv: "ℏ",
    plusacir: "⨣",
    plusb: "⊞",
    pluscir: "⨢",
    plus: "+",
    plusdo: "∔",
    plusdu: "⨥",
    pluse: "⩲",
    PlusMinus: "±",
    plusmn: "±",
    plussim: "⨦",
    plustwo: "⨧",
    pm: "±",
    Poincareplane: "ℌ",
    pointint: "⨕",
    popf: "𝕡",
    Popf: "ℙ",
    pound: "£",
    prap: "⪷",
    Pr: "⪻",
    pr: "≺",
    prcue: "≼",
    precapprox: "⪷",
    prec: "≺",
    preccurlyeq: "≼",
    Precedes: "≺",
    PrecedesEqual: "⪯",
    PrecedesSlantEqual: "≼",
    PrecedesTilde: "≾",
    preceq: "⪯",
    precnapprox: "⪹",
    precneqq: "⪵",
    precnsim: "⋨",
    pre: "⪯",
    prE: "⪳",
    precsim: "≾",
    prime: "′",
    Prime: "″",
    primes: "ℙ",
    prnap: "⪹",
    prnE: "⪵",
    prnsim: "⋨",
    prod: "∏",
    Product: "∏",
    profalar: "⌮",
    profline: "⌒",
    profsurf: "⌓",
    prop: "∝",
    Proportional: "∝",
    Proportion: "∷",
    propto: "∝",
    prsim: "≾",
    prurel: "⊰",
    Pscr: "𝒫",
    pscr: "𝓅",
    Psi: "Ψ",
    psi: "ψ",
    puncsp: " ",
    Qfr: "𝔔",
    qfr: "𝔮",
    qint: "⨌",
    qopf: "𝕢",
    Qopf: "ℚ",
    qprime: "⁗",
    Qscr: "𝒬",
    qscr: "𝓆",
    quaternions: "ℍ",
    quatint: "⨖",
    quest: "?",
    questeq: "≟",
    quot: "\"",
    QUOT: "\"",
    rAarr: "⇛",
    race: "∽̱",
    Racute: "Ŕ",
    racute: "ŕ",
    radic: "√",
    raemptyv: "⦳",
    rang: "⟩",
    Rang: "⟫",
    rangd: "⦒",
    range: "⦥",
    rangle: "⟩",
    raquo: "»",
    rarrap: "⥵",
    rarrb: "⇥",
    rarrbfs: "⤠",
    rarrc: "⤳",
    rarr: "→",
    Rarr: "↠",
    rArr: "⇒",
    rarrfs: "⤞",
    rarrhk: "↪",
    rarrlp: "↬",
    rarrpl: "⥅",
    rarrsim: "⥴",
    Rarrtl: "⤖",
    rarrtl: "↣",
    rarrw: "↝",
    ratail: "⤚",
    rAtail: "⤜",
    ratio: "∶",
    rationals: "ℚ",
    rbarr: "⤍",
    rBarr: "⤏",
    RBarr: "⤐",
    rbbrk: "❳",
    rbrace: "}",
    rbrack: "]",
    rbrke: "⦌",
    rbrksld: "⦎",
    rbrkslu: "⦐",
    Rcaron: "Ř",
    rcaron: "ř",
    Rcedil: "Ŗ",
    rcedil: "ŗ",
    rceil: "⌉",
    rcub: "}",
    Rcy: "Р",
    rcy: "р",
    rdca: "⤷",
    rdldhar: "⥩",
    rdquo: "”",
    rdquor: "”",
    rdsh: "↳",
    real: "ℜ",
    realine: "ℛ",
    realpart: "ℜ",
    reals: "ℝ",
    Re: "ℜ",
    rect: "▭",
    reg: "®",
    REG: "®",
    ReverseElement: "∋",
    ReverseEquilibrium: "⇋",
    ReverseUpEquilibrium: "⥯",
    rfisht: "⥽",
    rfloor: "⌋",
    rfr: "𝔯",
    Rfr: "ℜ",
    rHar: "⥤",
    rhard: "⇁",
    rharu: "⇀",
    rharul: "⥬",
    Rho: "Ρ",
    rho: "ρ",
    rhov: "ϱ",
    RightAngleBracket: "⟩",
    RightArrowBar: "⇥",
    rightarrow: "→",
    RightArrow: "→",
    Rightarrow: "⇒",
    RightArrowLeftArrow: "⇄",
    rightarrowtail: "↣",
    RightCeiling: "⌉",
    RightDoubleBracket: "⟧",
    RightDownTeeVector: "⥝",
    RightDownVectorBar: "⥕",
    RightDownVector: "⇂",
    RightFloor: "⌋",
    rightharpoondown: "⇁",
    rightharpoonup: "⇀",
    rightleftarrows: "⇄",
    rightleftharpoons: "⇌",
    rightrightarrows: "⇉",
    rightsquigarrow: "↝",
    RightTeeArrow: "↦",
    RightTee: "⊢",
    RightTeeVector: "⥛",
    rightthreetimes: "⋌",
    RightTriangleBar: "⧐",
    RightTriangle: "⊳",
    RightTriangleEqual: "⊵",
    RightUpDownVector: "⥏",
    RightUpTeeVector: "⥜",
    RightUpVectorBar: "⥔",
    RightUpVector: "↾",
    RightVectorBar: "⥓",
    RightVector: "⇀",
    ring: "˚",
    risingdotseq: "≓",
    rlarr: "⇄",
    rlhar: "⇌",
    rlm: "\u200f",
    rmoustache: "⎱",
    rmoust: "⎱",
    rnmid: "⫮",
    roang: "⟭",
    roarr: "⇾",
    robrk: "⟧",
    ropar: "⦆",
    ropf: "𝕣",
    Ropf: "ℝ",
    roplus: "⨮",
    rotimes: "⨵",
    RoundImplies: "⥰",
    rpar: ")",
    rpargt: "⦔",
    rppolint: "⨒",
    rrarr: "⇉",
    Rrightarrow: "⇛",
    rsaquo: "›",
    rscr: "𝓇",
    Rscr: "ℛ",
    rsh: "↱",
    Rsh: "↱",
    rsqb: "]",
    rsquo: "’",
    rsquor: "’",
    rthree: "⋌",
    rtimes: "⋊",
    rtri: "▹",
    rtrie: "⊵",
    rtrif: "▸",
    rtriltri: "⧎",
    RuleDelayed: "⧴",
    ruluhar: "⥨",
    rx: "℞",
    Sacute: "Ś",
    sacute: "ś",
    sbquo: "‚",
    scap: "⪸",
    Scaron: "Š",
    scaron: "š",
    Sc: "⪼",
    sc: "≻",
    sccue: "≽",
    sce: "⪰",
    scE: "⪴",
    Scedil: "Ş",
    scedil: "ş",
    Scirc: "Ŝ",
    scirc: "ŝ",
    scnap: "⪺",
    scnE: "⪶",
    scnsim: "⋩",
    scpolint: "⨓",
    scsim: "≿",
    Scy: "С",
    scy: "с",
    sdotb: "⊡",
    sdot: "⋅",
    sdote: "⩦",
    searhk: "⤥",
    searr: "↘",
    seArr: "⇘",
    searrow: "↘",
    sect: "§",
    semi: ";",
    seswar: "⤩",
    setminus: "∖",
    setmn: "∖",
    sext: "✶",
    Sfr: "𝔖",
    sfr: "𝔰",
    sfrown: "⌢",
    sharp: "♯",
    SHCHcy: "Щ",
    shchcy: "щ",
    SHcy: "Ш",
    shcy: "ш",
    ShortDownArrow: "↓",
    ShortLeftArrow: "←",
    shortmid: "∣",
    shortparallel: "∥",
    ShortRightArrow: "→",
    ShortUpArrow: "↑",
    shy: "\u00ad",
    Sigma: "Σ",
    sigma: "σ",
    sigmaf: "ς",
    sigmav: "ς",
    sim: "∼",
    simdot: "⩪",
    sime: "≃",
    simeq: "≃",
    simg: "⪞",
    simgE: "⪠",
    siml: "⪝",
    simlE: "⪟",
    simne: "≆",
    simplus: "⨤",
    simrarr: "⥲",
    slarr: "←",
    SmallCircle: "∘",
    smallsetminus: "∖",
    smashp: "⨳",
    smeparsl: "⧤",
    smid: "∣",
    smile: "⌣",
    smt: "⪪",
    smte: "⪬",
    smtes: "⪬︀",
    SOFTcy: "Ь",
    softcy: "ь",
    solbar: "⌿",
    solb: "⧄",
    sol: "/",
    Sopf: "𝕊",
    sopf: "𝕤",
    spades: "♠",
    spadesuit: "♠",
    spar: "∥",
    sqcap: "⊓",
    sqcaps: "⊓︀",
    sqcup: "⊔",
    sqcups: "⊔︀",
    Sqrt: "√",
    sqsub: "⊏",
    sqsube: "⊑",
    sqsubset: "⊏",
    sqsubseteq: "⊑",
    sqsup: "⊐",
    sqsupe: "⊒",
    sqsupset: "⊐",
    sqsupseteq: "⊒",
    square: "□",
    Square: "□",
    SquareIntersection: "⊓",
    SquareSubset: "⊏",
    SquareSubsetEqual: "⊑",
    SquareSuperset: "⊐",
    SquareSupersetEqual: "⊒",
    SquareUnion: "⊔",
    squarf: "▪",
    squ: "□",
    squf: "▪",
    srarr: "→",
    Sscr: "𝒮",
    sscr: "𝓈",
    ssetmn: "∖",
    ssmile: "⌣",
    sstarf: "⋆",
    Star: "⋆",
    star: "☆",
    starf: "★",
    straightepsilon: "ϵ",
    straightphi: "ϕ",
    strns: "¯",
    sub: "⊂",
    Sub: "⋐",
    subdot: "⪽",
    subE: "⫅",
    sube: "⊆",
    subedot: "⫃",
    submult: "⫁",
    subnE: "⫋",
    subne: "⊊",
    subplus: "⪿",
    subrarr: "⥹",
    subset: "⊂",
    Subset: "⋐",
    subseteq: "⊆",
    subseteqq: "⫅",
    SubsetEqual: "⊆",
    subsetneq: "⊊",
    subsetneqq: "⫋",
    subsim: "⫇",
    subsub: "⫕",
    subsup: "⫓",
    succapprox: "⪸",
    succ: "≻",
    succcurlyeq: "≽",
    Succeeds: "≻",
    SucceedsEqual: "⪰",
    SucceedsSlantEqual: "≽",
    SucceedsTilde: "≿",
    succeq: "⪰",
    succnapprox: "⪺",
    succneqq: "⪶",
    succnsim: "⋩",
    succsim: "≿",
    SuchThat: "∋",
    sum: "∑",
    Sum: "∑",
    sung: "♪",
    sup1: "¹",
    sup2: "²",
    sup3: "³",
    sup: "⊃",
    Sup: "⋑",
    supdot: "⪾",
    supdsub: "⫘",
    supE: "⫆",
    supe: "⊇",
    supedot: "⫄",
    Superset: "⊃",
    SupersetEqual: "⊇",
    suphsol: "⟉",
    suphsub: "⫗",
    suplarr: "⥻",
    supmult: "⫂",
    supnE: "⫌",
    supne: "⊋",
    supplus: "⫀",
    supset: "⊃",
    Supset: "⋑",
    supseteq: "⊇",
    supseteqq: "⫆",
    supsetneq: "⊋",
    supsetneqq: "⫌",
    supsim: "⫈",
    supsub: "⫔",
    supsup: "⫖",
    swarhk: "⤦",
    swarr: "↙",
    swArr: "⇙",
    swarrow: "↙",
    swnwar: "⤪",
    szlig: "ß",
    Tab: "\u0009",
    target: "⌖",
    Tau: "Τ",
    tau: "τ",
    tbrk: "⎴",
    Tcaron: "Ť",
    tcaron: "ť",
    Tcedil: "Ţ",
    tcedil: "ţ",
    Tcy: "Т",
    tcy: "т",
    tdot: "⃛",
    telrec: "⌕",
    Tfr: "𝔗",
    tfr: "𝔱",
    there4: "∴",
    therefore: "∴",
    Therefore: "∴",
    Theta: "Θ",
    theta: "θ",
    thetasym: "ϑ",
    thetav: "ϑ",
    thickapprox: "≈",
    thicksim: "∼",
    ThickSpace: "  ",
    ThinSpace: " ",
    thinsp: " ",
    thkap: "≈",
    thksim: "∼",
    THORN: "Þ",
    thorn: "þ",
    tilde: "˜",
    Tilde: "∼",
    TildeEqual: "≃",
    TildeFullEqual: "≅",
    TildeTilde: "≈",
    timesbar: "⨱",
    timesb: "⊠",
    times: "×",
    timesd: "⨰",
    tint: "∭",
    toea: "⤨",
    topbot: "⌶",
    topcir: "⫱",
    top: "⊤",
    Topf: "𝕋",
    topf: "𝕥",
    topfork: "⫚",
    tosa: "⤩",
    tprime: "‴",
    trade: "™",
    TRADE: "™",
    triangle: "▵",
    triangledown: "▿",
    triangleleft: "◃",
    trianglelefteq: "⊴",
    triangleq: "≜",
    triangleright: "▹",
    trianglerighteq: "⊵",
    tridot: "◬",
    trie: "≜",
    triminus: "⨺",
    TripleDot: "⃛",
    triplus: "⨹",
    trisb: "⧍",
    tritime: "⨻",
    trpezium: "⏢",
    Tscr: "𝒯",
    tscr: "𝓉",
    TScy: "Ц",
    tscy: "ц",
    TSHcy: "Ћ",
    tshcy: "ћ",
    Tstrok: "Ŧ",
    tstrok: "ŧ",
    twixt: "≬",
    twoheadleftarrow: "↞",
    twoheadrightarrow: "↠",
    Uacute: "Ú",
    uacute: "ú",
    uarr: "↑",
    Uarr: "↟",
    uArr: "⇑",
    Uarrocir: "⥉",
    Ubrcy: "Ў",
    ubrcy: "ў",
    Ubreve: "Ŭ",
    ubreve: "ŭ",
    Ucirc: "Û",
    ucirc: "û",
    Ucy: "У",
    ucy: "у",
    udarr: "⇅",
    Udblac: "Ű",
    udblac: "ű",
    udhar: "⥮",
    ufisht: "⥾",
    Ufr: "𝔘",
    ufr: "𝔲",
    Ugrave: "Ù",
    ugrave: "ù",
    uHar: "⥣",
    uharl: "↿",
    uharr: "↾",
    uhblk: "▀",
    ulcorn: "⌜",
    ulcorner: "⌜",
    ulcrop: "⌏",
    ultri: "◸",
    Umacr: "Ū",
    umacr: "ū",
    uml: "¨",
    UnderBar: "_",
    UnderBrace: "⏟",
    UnderBracket: "⎵",
    UnderParenthesis: "⏝",
    Union: "⋃",
    UnionPlus: "⊎",
    Uogon: "Ų",
    uogon: "ų",
    Uopf: "𝕌",
    uopf: "𝕦",
    UpArrowBar: "⤒",
    uparrow: "↑",
    UpArrow: "↑",
    Uparrow: "⇑",
    UpArrowDownArrow: "⇅",
    updownarrow: "↕",
    UpDownArrow: "↕",
    Updownarrow: "⇕",
    UpEquilibrium: "⥮",
    upharpoonleft: "↿",
    upharpoonright: "↾",
    uplus: "⊎",
    UpperLeftArrow: "↖",
    UpperRightArrow: "↗",
    upsi: "υ",
    Upsi: "ϒ",
    upsih: "ϒ",
    Upsilon: "Υ",
    upsilon: "υ",
    UpTeeArrow: "↥",
    UpTee: "⊥",
    upuparrows: "⇈",
    urcorn: "⌝",
    urcorner: "⌝",
    urcrop: "⌎",
    Uring: "Ů",
    uring: "ů",
    urtri: "◹",
    Uscr: "𝒰",
    uscr: "𝓊",
    utdot: "⋰",
    Utilde: "Ũ",
    utilde: "ũ",
    utri: "▵",
    utrif: "▴",
    uuarr: "⇈",
    Uuml: "Ü",
    uuml: "ü",
    uwangle: "⦧",
    vangrt: "⦜",
    varepsilon: "ϵ",
    varkappa: "ϰ",
    varnothing: "∅",
    varphi: "ϕ",
    varpi: "ϖ",
    varpropto: "∝",
    varr: "↕",
    vArr: "⇕",
    varrho: "ϱ",
    varsigma: "ς",
    varsubsetneq: "⊊︀",
    varsubsetneqq: "⫋︀",
    varsupsetneq: "⊋︀",
    varsupsetneqq: "⫌︀",
    vartheta: "ϑ",
    vartriangleleft: "⊲",
    vartriangleright: "⊳",
    vBar: "⫨",
    Vbar: "⫫",
    vBarv: "⫩",
    Vcy: "В",
    vcy: "в",
    vdash: "⊢",
    vDash: "⊨",
    Vdash: "⊩",
    VDash: "⊫",
    Vdashl: "⫦",
    veebar: "⊻",
    vee: "∨",
    Vee: "⋁",
    veeeq: "≚",
    vellip: "⋮",
    verbar: "|",
    Verbar: "‖",
    vert: "|",
    Vert: "‖",
    VerticalBar: "∣",
    VerticalLine: "|",
    VerticalSeparator: "❘",
    VerticalTilde: "≀",
    VeryThinSpace: " ",
    Vfr: "𝔙",
    vfr: "𝔳",
    vltri: "⊲",
    vnsub: "⊂⃒",
    vnsup: "⊃⃒",
    Vopf: "𝕍",
    vopf: "𝕧",
    vprop: "∝",
    vrtri: "⊳",
    Vscr: "𝒱",
    vscr: "𝓋",
    vsubnE: "⫋︀",
    vsubne: "⊊︀",
    vsupnE: "⫌︀",
    vsupne: "⊋︀",
    Vvdash: "⊪",
    vzigzag: "⦚",
    Wcirc: "Ŵ",
    wcirc: "ŵ",
    wedbar: "⩟",
    wedge: "∧",
    Wedge: "⋀",
    wedgeq: "≙",
    weierp: "℘",
    Wfr: "𝔚",
    wfr: "𝔴",
    Wopf: "𝕎",
    wopf: "𝕨",
    wp: "℘",
    wr: "≀",
    wreath: "≀",
    Wscr: "𝒲",
    wscr: "𝓌",
    xcap: "⋂",
    xcirc: "◯",
    xcup: "⋃",
    xdtri: "▽",
    Xfr: "𝔛",
    xfr: "𝔵",
    xharr: "⟷",
    xhArr: "⟺",
    Xi: "Ξ",
    xi: "ξ",
    xlarr: "⟵",
    xlArr: "⟸",
    xmap: "⟼",
    xnis: "⋻",
    xodot: "⨀",
    Xopf: "𝕏",
    xopf: "𝕩",
    xoplus: "⨁",
    xotime: "⨂",
    xrarr: "⟶",
    xrArr: "⟹",
    Xscr: "𝒳",
    xscr: "𝓍",
    xsqcup: "⨆",
    xuplus: "⨄",
    xutri: "△",
    xvee: "⋁",
    xwedge: "⋀",
    Yacute: "Ý",
    yacute: "ý",
    YAcy: "Я",
    yacy: "я",
    Ycirc: "Ŷ",
    ycirc: "ŷ",
    Ycy: "Ы",
    ycy: "ы",
    yen: "¥",
    Yfr: "𝔜",
    yfr: "𝔶",
    YIcy: "Ї",
    yicy: "ї",
    Yopf: "𝕐",
    yopf: "𝕪",
    Yscr: "𝒴",
    yscr: "𝓎",
    YUcy: "Ю",
    yucy: "ю",
    yuml: "ÿ",
    Yuml: "Ÿ",
    Zacute: "Ź",
    zacute: "ź",
    Zcaron: "Ž",
    zcaron: "ž",
    Zcy: "З",
    zcy: "з",
    Zdot: "Ż",
    zdot: "ż",
    zeetrf: "ℨ",
    ZeroWidthSpace: "​",
    Zeta: "Ζ",
    zeta: "ζ",
    zfr: "𝔷",
    Zfr: "ℨ",
    ZHcy: "Ж",
    zhcy: "ж",
    zigrarr: "⇝",
    zopf: "𝕫",
    Zopf: "ℤ",
    Zscr: "𝒵",
    zscr: "𝓏",
    zwj: "\u200d",
    zwnj: "\u200c"
  };
  _exports.HTML5NamedCharRefs = namedCharRefs;
  var HEXCHARCODE = /^#[xX]([A-Fa-f0-9]+)$/;
  var CHARCODE = /^#([0-9]+)$/;
  var NAMED = /^([A-Za-z0-9]+)$/;

  var EntityParser =
  /** @class */
  function () {
    function EntityParser(named) {
      this.named = named;
    }

    EntityParser.prototype.parse = function (entity) {
      if (!entity) {
        return;
      }

      var matches = entity.match(HEXCHARCODE);

      if (matches) {
        return String.fromCharCode(parseInt(matches[1], 16));
      }

      matches = entity.match(CHARCODE);

      if (matches) {
        return String.fromCharCode(parseInt(matches[1], 10));
      }

      matches = entity.match(NAMED);

      if (matches) {
        return this.named[matches[1]];
      }
    };

    return EntityParser;
  }();

  _exports.EntityParser = EntityParser;
  var WSP = /[\t\n\f ]/;
  var ALPHA = /[A-Za-z]/;
  var CRLF = /\r\n?/g;

  function isSpace(char) {
    return WSP.test(char);
  }

  function isAlpha(char) {
    return ALPHA.test(char);
  }

  function preprocessInput(input) {
    return input.replace(CRLF, '\n');
  }

  var EventedTokenizer =
  /** @class */
  function () {
    function EventedTokenizer(delegate, entityParser) {
      this.delegate = delegate;
      this.entityParser = entityParser;
      this.state = "beforeData"
      /* beforeData */
      ;
      this.line = -1;
      this.column = -1;
      this.input = '';
      this.index = -1;
      this.tagNameBuffer = '';
      this.states = {
        beforeData: function () {
          var char = this.peek();

          if (char === '<') {
            this.transitionTo("tagOpen"
            /* tagOpen */
            );
            this.markTagStart();
            this.consume();
          } else {
            if (char === '\n') {
              var tag = this.tagNameBuffer.toLowerCase();

              if (tag === 'pre' || tag === 'textarea') {
                this.consume();
              }
            }

            this.transitionTo("data"
            /* data */
            );
            this.delegate.beginData();
          }
        },
        data: function () {
          var char = this.peek();

          if (char === '<') {
            this.delegate.finishData();
            this.transitionTo("tagOpen"
            /* tagOpen */
            );
            this.markTagStart();
            this.consume();
          } else if (char === '&') {
            this.consume();
            this.delegate.appendToData(this.consumeCharRef() || '&');
          } else {
            this.consume();
            this.delegate.appendToData(char);
          }
        },
        tagOpen: function () {
          var char = this.consume();

          if (char === '!') {
            this.transitionTo("markupDeclarationOpen"
            /* markupDeclarationOpen */
            );
          } else if (char === '/') {
            this.transitionTo("endTagOpen"
            /* endTagOpen */
            );
          } else if (char === '@' || isAlpha(char)) {
            this.transitionTo("tagName"
            /* tagName */
            );
            this.tagNameBuffer = '';
            this.delegate.beginStartTag();
            this.appendToTagName(char);
          }
        },
        markupDeclarationOpen: function () {
          var char = this.consume();

          if (char === '-' && this.input.charAt(this.index) === '-') {
            this.consume();
            this.transitionTo("commentStart"
            /* commentStart */
            );
            this.delegate.beginComment();
          }
        },
        commentStart: function () {
          var char = this.consume();

          if (char === '-') {
            this.transitionTo("commentStartDash"
            /* commentStartDash */
            );
          } else if (char === '>') {
            this.delegate.finishComment();
            this.transitionTo("beforeData"
            /* beforeData */
            );
          } else {
            this.delegate.appendToCommentData(char);
            this.transitionTo("comment"
            /* comment */
            );
          }
        },
        commentStartDash: function () {
          var char = this.consume();

          if (char === '-') {
            this.transitionTo("commentEnd"
            /* commentEnd */
            );
          } else if (char === '>') {
            this.delegate.finishComment();
            this.transitionTo("beforeData"
            /* beforeData */
            );
          } else {
            this.delegate.appendToCommentData('-');
            this.transitionTo("comment"
            /* comment */
            );
          }
        },
        comment: function () {
          var char = this.consume();

          if (char === '-') {
            this.transitionTo("commentEndDash"
            /* commentEndDash */
            );
          } else {
            this.delegate.appendToCommentData(char);
          }
        },
        commentEndDash: function () {
          var char = this.consume();

          if (char === '-') {
            this.transitionTo("commentEnd"
            /* commentEnd */
            );
          } else {
            this.delegate.appendToCommentData('-' + char);
            this.transitionTo("comment"
            /* comment */
            );
          }
        },
        commentEnd: function () {
          var char = this.consume();

          if (char === '>') {
            this.delegate.finishComment();
            this.transitionTo("beforeData"
            /* beforeData */
            );
          } else {
            this.delegate.appendToCommentData('--' + char);
            this.transitionTo("comment"
            /* comment */
            );
          }
        },
        tagName: function () {
          var char = this.consume();

          if (isSpace(char)) {
            this.transitionTo("beforeAttributeName"
            /* beforeAttributeName */
            );
          } else if (char === '/') {
            this.transitionTo("selfClosingStartTag"
            /* selfClosingStartTag */
            );
          } else if (char === '>') {
            this.delegate.finishTag();
            this.transitionTo("beforeData"
            /* beforeData */
            );
          } else {
            this.appendToTagName(char);
          }
        },
        beforeAttributeName: function () {
          var char = this.peek();

          if (isSpace(char)) {
            this.consume();
            return;
          } else if (char === '/') {
            this.transitionTo("selfClosingStartTag"
            /* selfClosingStartTag */
            );
            this.consume();
          } else if (char === '>') {
            this.consume();
            this.delegate.finishTag();
            this.transitionTo("beforeData"
            /* beforeData */
            );
          } else if (char === '=') {
            this.delegate.reportSyntaxError('attribute name cannot start with equals sign');
            this.transitionTo("attributeName"
            /* attributeName */
            );
            this.delegate.beginAttribute();
            this.consume();
            this.delegate.appendToAttributeName(char);
          } else {
            this.transitionTo("attributeName"
            /* attributeName */
            );
            this.delegate.beginAttribute();
          }
        },
        attributeName: function () {
          var char = this.peek();

          if (isSpace(char)) {
            this.transitionTo("afterAttributeName"
            /* afterAttributeName */
            );
            this.consume();
          } else if (char === '/') {
            this.delegate.beginAttributeValue(false);
            this.delegate.finishAttributeValue();
            this.consume();
            this.transitionTo("selfClosingStartTag"
            /* selfClosingStartTag */
            );
          } else if (char === '=') {
            this.transitionTo("beforeAttributeValue"
            /* beforeAttributeValue */
            );
            this.consume();
          } else if (char === '>') {
            this.delegate.beginAttributeValue(false);
            this.delegate.finishAttributeValue();
            this.consume();
            this.delegate.finishTag();
            this.transitionTo("beforeData"
            /* beforeData */
            );
          } else if (char === '"' || char === "'" || char === '<') {
            this.delegate.reportSyntaxError(char + ' is not a valid character within attribute names');
            this.consume();
            this.delegate.appendToAttributeName(char);
          } else {
            this.consume();
            this.delegate.appendToAttributeName(char);
          }
        },
        afterAttributeName: function () {
          var char = this.peek();

          if (isSpace(char)) {
            this.consume();
            return;
          } else if (char === '/') {
            this.delegate.beginAttributeValue(false);
            this.delegate.finishAttributeValue();
            this.consume();
            this.transitionTo("selfClosingStartTag"
            /* selfClosingStartTag */
            );
          } else if (char === '=') {
            this.consume();
            this.transitionTo("beforeAttributeValue"
            /* beforeAttributeValue */
            );
          } else if (char === '>') {
            this.delegate.beginAttributeValue(false);
            this.delegate.finishAttributeValue();
            this.consume();
            this.delegate.finishTag();
            this.transitionTo("beforeData"
            /* beforeData */
            );
          } else {
            this.delegate.beginAttributeValue(false);
            this.delegate.finishAttributeValue();
            this.transitionTo("attributeName"
            /* attributeName */
            );
            this.delegate.beginAttribute();
            this.consume();
            this.delegate.appendToAttributeName(char);
          }
        },
        beforeAttributeValue: function () {
          var char = this.peek();

          if (isSpace(char)) {
            this.consume();
          } else if (char === '"') {
            this.transitionTo("attributeValueDoubleQuoted"
            /* attributeValueDoubleQuoted */
            );
            this.delegate.beginAttributeValue(true);
            this.consume();
          } else if (char === "'") {
            this.transitionTo("attributeValueSingleQuoted"
            /* attributeValueSingleQuoted */
            );
            this.delegate.beginAttributeValue(true);
            this.consume();
          } else if (char === '>') {
            this.delegate.beginAttributeValue(false);
            this.delegate.finishAttributeValue();
            this.consume();
            this.delegate.finishTag();
            this.transitionTo("beforeData"
            /* beforeData */
            );
          } else {
            this.transitionTo("attributeValueUnquoted"
            /* attributeValueUnquoted */
            );
            this.delegate.beginAttributeValue(false);
            this.consume();
            this.delegate.appendToAttributeValue(char);
          }
        },
        attributeValueDoubleQuoted: function () {
          var char = this.consume();

          if (char === '"') {
            this.delegate.finishAttributeValue();
            this.transitionTo("afterAttributeValueQuoted"
            /* afterAttributeValueQuoted */
            );
          } else if (char === '&') {
            this.delegate.appendToAttributeValue(this.consumeCharRef() || '&');
          } else {
            this.delegate.appendToAttributeValue(char);
          }
        },
        attributeValueSingleQuoted: function () {
          var char = this.consume();

          if (char === "'") {
            this.delegate.finishAttributeValue();
            this.transitionTo("afterAttributeValueQuoted"
            /* afterAttributeValueQuoted */
            );
          } else if (char === '&') {
            this.delegate.appendToAttributeValue(this.consumeCharRef() || '&');
          } else {
            this.delegate.appendToAttributeValue(char);
          }
        },
        attributeValueUnquoted: function () {
          var char = this.peek();

          if (isSpace(char)) {
            this.delegate.finishAttributeValue();
            this.consume();
            this.transitionTo("beforeAttributeName"
            /* beforeAttributeName */
            );
          } else if (char === '/') {
            this.delegate.finishAttributeValue();
            this.consume();
            this.transitionTo("selfClosingStartTag"
            /* selfClosingStartTag */
            );
          } else if (char === '&') {
            this.consume();
            this.delegate.appendToAttributeValue(this.consumeCharRef() || '&');
          } else if (char === '>') {
            this.delegate.finishAttributeValue();
            this.consume();
            this.delegate.finishTag();
            this.transitionTo("beforeData"
            /* beforeData */
            );
          } else {
            this.consume();
            this.delegate.appendToAttributeValue(char);
          }
        },
        afterAttributeValueQuoted: function () {
          var char = this.peek();

          if (isSpace(char)) {
            this.consume();
            this.transitionTo("beforeAttributeName"
            /* beforeAttributeName */
            );
          } else if (char === '/') {
            this.consume();
            this.transitionTo("selfClosingStartTag"
            /* selfClosingStartTag */
            );
          } else if (char === '>') {
            this.consume();
            this.delegate.finishTag();
            this.transitionTo("beforeData"
            /* beforeData */
            );
          } else {
            this.transitionTo("beforeAttributeName"
            /* beforeAttributeName */
            );
          }
        },
        selfClosingStartTag: function () {
          var char = this.peek();

          if (char === '>') {
            this.consume();
            this.delegate.markTagAsSelfClosing();
            this.delegate.finishTag();
            this.transitionTo("beforeData"
            /* beforeData */
            );
          } else {
            this.transitionTo("beforeAttributeName"
            /* beforeAttributeName */
            );
          }
        },
        endTagOpen: function () {
          var char = this.consume();

          if (char === '@' || isAlpha(char)) {
            this.transitionTo("tagName"
            /* tagName */
            );
            this.tagNameBuffer = '';
            this.delegate.beginEndTag();
            this.appendToTagName(char);
          }
        }
      };
      this.reset();
    }

    EventedTokenizer.prototype.reset = function () {
      this.transitionTo("beforeData"
      /* beforeData */
      );
      this.input = '';
      this.index = 0;
      this.line = 1;
      this.column = 0;
      this.delegate.reset();
    };

    EventedTokenizer.prototype.transitionTo = function (state) {
      this.state = state;
    };

    EventedTokenizer.prototype.tokenize = function (input) {
      this.reset();
      this.tokenizePart(input);
      this.tokenizeEOF();
    };

    EventedTokenizer.prototype.tokenizePart = function (input) {
      this.input += preprocessInput(input);

      while (this.index < this.input.length) {
        var handler = this.states[this.state];

        if (handler !== undefined) {
          handler.call(this);
        } else {
          throw new Error("unhandled state " + this.state);
        }
      }
    };

    EventedTokenizer.prototype.tokenizeEOF = function () {
      this.flushData();
    };

    EventedTokenizer.prototype.flushData = function () {
      if (this.state === 'data') {
        this.delegate.finishData();
        this.transitionTo("beforeData"
        /* beforeData */
        );
      }
    };

    EventedTokenizer.prototype.peek = function () {
      return this.input.charAt(this.index);
    };

    EventedTokenizer.prototype.consume = function () {
      var char = this.peek();
      this.index++;

      if (char === '\n') {
        this.line++;
        this.column = 0;
      } else {
        this.column++;
      }

      return char;
    };

    EventedTokenizer.prototype.consumeCharRef = function () {
      var endIndex = this.input.indexOf(';', this.index);

      if (endIndex === -1) {
        return;
      }

      var entity = this.input.slice(this.index, endIndex);
      var chars = this.entityParser.parse(entity);

      if (chars) {
        var count = entity.length; // consume the entity chars

        while (count) {
          this.consume();
          count--;
        } // consume the `;`


        this.consume();
        return chars;
      }
    };

    EventedTokenizer.prototype.markTagStart = function () {
      this.delegate.tagOpen();
    };

    EventedTokenizer.prototype.appendToTagName = function (char) {
      this.tagNameBuffer += char;
      this.delegate.appendToTagName(char);
    };

    return EventedTokenizer;
  }();

  _exports.EventedTokenizer = EventedTokenizer;

  var Tokenizer =
  /** @class */
  function () {
    function Tokenizer(entityParser, options) {
      if (options === void 0) {
        options = {};
      }

      this.options = options;
      this.token = null;
      this.startLine = 1;
      this.startColumn = 0;
      this.tokens = [];
      this.tokenizer = new EventedTokenizer(this, entityParser);
      this._currentAttribute = undefined;
    }

    Tokenizer.prototype.tokenize = function (input) {
      this.tokens = [];
      this.tokenizer.tokenize(input);
      return this.tokens;
    };

    Tokenizer.prototype.tokenizePart = function (input) {
      this.tokens = [];
      this.tokenizer.tokenizePart(input);
      return this.tokens;
    };

    Tokenizer.prototype.tokenizeEOF = function () {
      this.tokens = [];
      this.tokenizer.tokenizeEOF();
      return this.tokens[0];
    };

    Tokenizer.prototype.reset = function () {
      this.token = null;
      this.startLine = 1;
      this.startColumn = 0;
    };

    Tokenizer.prototype.current = function () {
      var token = this.token;

      if (token === null) {
        throw new Error('token was unexpectedly null');
      }

      if (arguments.length === 0) {
        return token;
      }

      for (var i = 0; i < arguments.length; i++) {
        if (token.type === arguments[i]) {
          return token;
        }
      }

      throw new Error("token type was unexpectedly " + token.type);
    };

    Tokenizer.prototype.push = function (token) {
      this.token = token;
      this.tokens.push(token);
    };

    Tokenizer.prototype.currentAttribute = function () {
      return this._currentAttribute;
    };

    Tokenizer.prototype.addLocInfo = function () {
      if (this.options.loc) {
        this.current().loc = {
          start: {
            line: this.startLine,
            column: this.startColumn
          },
          end: {
            line: this.tokenizer.line,
            column: this.tokenizer.column
          }
        };
      }

      this.startLine = this.tokenizer.line;
      this.startColumn = this.tokenizer.column;
    }; // Data


    Tokenizer.prototype.beginData = function () {
      this.push({
        type: "Chars"
        /* Chars */
        ,
        chars: ''
      });
    };

    Tokenizer.prototype.appendToData = function (char) {
      this.current("Chars"
      /* Chars */
      ).chars += char;
    };

    Tokenizer.prototype.finishData = function () {
      this.addLocInfo();
    }; // Comment


    Tokenizer.prototype.beginComment = function () {
      this.push({
        type: "Comment"
        /* Comment */
        ,
        chars: ''
      });
    };

    Tokenizer.prototype.appendToCommentData = function (char) {
      this.current("Comment"
      /* Comment */
      ).chars += char;
    };

    Tokenizer.prototype.finishComment = function () {
      this.addLocInfo();
    }; // Tags - basic


    Tokenizer.prototype.tagOpen = function () {};

    Tokenizer.prototype.beginStartTag = function () {
      this.push({
        type: "StartTag"
        /* StartTag */
        ,
        tagName: '',
        attributes: [],
        selfClosing: false
      });
    };

    Tokenizer.prototype.beginEndTag = function () {
      this.push({
        type: "EndTag"
        /* EndTag */
        ,
        tagName: ''
      });
    };

    Tokenizer.prototype.finishTag = function () {
      this.addLocInfo();
    };

    Tokenizer.prototype.markTagAsSelfClosing = function () {
      this.current("StartTag"
      /* StartTag */
      ).selfClosing = true;
    }; // Tags - name


    Tokenizer.prototype.appendToTagName = function (char) {
      this.current("StartTag"
      /* StartTag */
      , "EndTag"
      /* EndTag */
      ).tagName += char;
    }; // Tags - attributes


    Tokenizer.prototype.beginAttribute = function () {
      this._currentAttribute = ['', '', false];
    };

    Tokenizer.prototype.appendToAttributeName = function (char) {
      this.currentAttribute()[0] += char;
    };

    Tokenizer.prototype.beginAttributeValue = function (isQuoted) {
      this.currentAttribute()[2] = isQuoted;
    };

    Tokenizer.prototype.appendToAttributeValue = function (char) {
      this.currentAttribute()[1] += char;
    };

    Tokenizer.prototype.finishAttributeValue = function () {
      this.current("StartTag"
      /* StartTag */
      ).attributes.push(this._currentAttribute);
    };

    Tokenizer.prototype.reportSyntaxError = function (message) {
      this.current().syntaxError = message;
    };

    return Tokenizer;
  }();

  _exports.Tokenizer = Tokenizer;

  function tokenize(input, options) {
    var tokenizer = new Tokenizer(new EntityParser(namedCharRefs), options);
    return tokenizer.tokenize(input);
  }
});
(function (m) { if (typeof module === "object" && module.exports) { module.exports = m } }(requireModule('ember-template-compiler')));


}());
//# sourceMappingURL=ember-template-compiler.map
