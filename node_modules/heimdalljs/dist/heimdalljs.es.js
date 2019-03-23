import { Promise as Promise$1 } from 'rsvp';

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
var now = void 0;
if ((typeof performance === 'undefined' ? 'undefined' : _typeof(performance)) === 'object' && typeof performance.now === 'function') {
  now = function now() {
    return performance.now.call(performance);
  };
} else {
  now = Date.now || function () {
    return new Date().getTime();
  };
}

var dateOffset = now();

function timeFromDate() {
  var timeMS = now() - dateOffset;

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
      return new Promise$1(function (resolve) {
        return resolve(callback.call(context, cookie._node.stats.own));
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

setupSession(process);

var index = new Heimdall(process._heimdall_session_2);

export default index;
