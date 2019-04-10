'use strict';

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; }

var fs = require('fs');
var path = require('path');
var read = fs.readFileSync;
var exists = fs.existsSync;
var join = path.join;
var debug = require('./debug')('tabtab:complete');
var assign = require('object-assign');
var uniq = require('lodash.uniq');
var difference = require('lodash.difference');

var _require = require('events');

var EventEmitter = _require.EventEmitter;

var CacheMixin = require('./cache');

// Public: Complete class. This is the main API to interract with the
// completion system and extends EventEmitter.
//
// Examples
//
//   var complete = new Complete({
//     name: 'binary-name'
//   });
//
//   complete.on('list', function(data, done) {
//     return done(null, ['completion', 'result', 'here']);
//   });

var Complete = function (_EventEmitter) {
  _inherits(Complete, _EventEmitter);

  _createClass(Complete, [{
    key: 'defaults',


    // Public: Options defaults.
    //
    // Returns the binary name being completed. Uses process.title if not the
    // default "node" value, or attempt to determine the package name from
    // package.json file.
    get: function get() {
      return {
        name: process.title !== 'node' ? process.title : ''
      };
    }

    // Public: Complete constructor, accepts options hash with
    //
    // options -  Accepts options hash (default: {})
    //
    // Examples
    //
    //   new Complete({
    //     // the String package name being completed, defaults to process.title
    //     // (if not node default) or will attempt to determine parent's
    //     // package.json location and extract the name from it.
    //     name: 'foobar'
    //
    //     // Enable / Disable cache (defaults: true)
    //     cache: true,
    //
    //    // Cache Time To Live duration in ms (default: 5min)
    //     ttl: 1000 * 60 * 5
    //   });

  }]);

  function Complete(options) {
    _classCallCheck(this, Complete);

    var _this = _possibleConstructorReturn(this, (Complete.__proto__ || Object.getPrototypeOf(Complete)).call(this));

    _this.options = options || _this.defaults;
    _this.options.name = _this.options.name || _this.resolve('name');
    _this.options.cache = typeof _this.options.cache !== 'undefined' ? _this.options.cache : true;
    _this.options.ttl = _this.options.ttl || 1000 * 60 * 5;

    _this._results = [];
    _this._completions = [];
    return _this;
  }

  _createClass(Complete, [{
    key: 'start',
    value: function start() {
      this.handle();
    }

    // Public: Takes care of all the completion plumbing mechanism.
    //
    // It checks the environment to determine if we act in plumbing mode or not,
    // to parse COMP args and emit the appropriated events to gather completion
    // results.
    //
    // options - options hash to pass to self#parseEnv

  }, {
    key: 'handle',
    value: function handle(options) {
      var _this2 = this;

      options = assign({}, options, this.options);
      var name = options.name;
      if (!name) throw new Error('Cannot determine package name');

      var env = this.env = this.parseEnv(options);
      if (env.args[0] !== 'completion') return;

      var line = env.line.replace(name, '').trim();
      var first = line.split(' ')[0];
      if (first) first = first.trim();

      var event = (first || env.prev || name).trim();
      var cache = this.cache(env.line);

      if (this.options.cache && cache) {
        return process.nextTick(function () {
          // this.addCompletions(cache.value);
          _this2.send(event + ':cache', env);
          debug('cache', cache.value);
          _this2.output(cache.value);
        });
      }

      if (!env.complete) {
        return debug('Completion command but without COMP args');
      }

      process.nextTick(function () {
        _this2.completePackage(env);

        var prev = env.prev.trim();
        name = name.trim();

        // Keeps emitting event only if previous one is not being listened to.
        // Emits in series: first, prev and name.
        _this2.send(event, env, _this2.recv.bind(_this2));

        if (prev !== event) _this2.send(prev, env, _this2.recv.bind(_this2));
        if (name !== event && name !== prev) _this2.send(name, env, _this2.recv.bind(_this2));

        if (_this2.options.cache) {
          _this2.cache(env.line, _this2._results);
          _this2.writeToStore(_this2.cacheStore);
        }
      });
    }
  }, {
    key: 'output',
    value: function output(results) {
      var _this3 = this;

      var shell = (process.env.SHELL || '').split('/').slice(-1)[0];

      if (shell === 'bash') {
        results = results.filter(function (res) {
          return res.indexOf(_this3.env.last) === 0;
        });
      }

      // create a duplicate-free version of results
      results = uniq(results);

      // only include results different from past completion results
      results = difference(results, this._completions);
      this._completions = this._completions.concat(results);

      debug('results:', results, this.env);
      console.log(results.join('\n'));
    }
  }, {
    key: 'send',
    value: function send(evt, env, done) {
      var res = this.emit(evt, env, done);
      debug('Emit evt: %s (listener: %s)', evt, res);
      return res;
    }
  }, {
    key: 'completePackage',
    value: function completePackage(env, stop) {
      var config = this.resolve('tabtab');
      if (!config) return;

      var pkgname = config[this.options.name];

      var last = (env.last || env.prev).trim();
      var prop = last || this.options.name;
      if (!prop) return;

      if (stop) {
        var first = env.line.split(' ')[0];
        var results = config[first];
        if (!results) return;
        return this.recv(null, results, env);
      }

      // Keeps looking up for completion only if previous one have not returned
      // any results.
      var command = config[prop];
      var completions = this.recv(null, command, env);

      if (!completions) {
        if (last && !stop) {
          var reg = new RegExp('\\s*' + last + '\\s*$');
          var line = env.line.replace(reg, '');
          this.completePackage(this.parseEnv({
            env: {
              COMP_LINE: line,
              COMP_WORD: env.words,
              COMP_POINT: env.point
            }
          }), true);
        }
      }

      return true;
    }

    // Public: Completions handlers
    //
    // will call back this function with an Array of completion items.
    //
    // err -          Error object
    // completions -  The Array of String completion results to write to stdout
    // env -          Env object as parsed by parseEnv

  }, {
    key: 'recv',
    value: function recv(err, completions, env) {
      if (!completions) return;

      env = env || this.env;

      debug('Received %s', completions);
      if (err) return this.emit('error', err);

      this.addCompletions(completions);
      this.output(this._results);

      return completions;
    }
  }, {
    key: 'addCompletions',
    value: function addCompletions(completions) {
      completions = Array.isArray(completions) ? completions : [completions];
      var shell = (process.env.SHELL || '').split('/').slice(-1)[0];

      completions = completions.map(this.completionItem).map(function (item) {
        return shell === 'zsh' ? item.name.replace(/:/g, '\\:') + ':' + item.description : shell === 'fish' ? item.name + '\t' + item.description : item.name;
      });

      this._results = this._results.concat(completions);
    }
  }, {
    key: 'completionItem',
    value: function completionItem(str) {
      debug('completion item', str, typeof str === 'undefined' ? 'undefined' : _typeof(str));

      if (typeof str !== 'string') return str;
      var shell = (process.env.SHELL || '').split('/').slice(-1)[0];

      var parts = str.split(/(\\)?:/);
      var name = parts[0];
      var desc = parts.slice(-1)[0];

      if (desc === name) {
        desc = '';
      }

      if (shell === 'zsh' && /\\/.test(str)) {
        name = name + '\\';
      }

      return {
        name: name,
        description: desc
      };
    }

    // Public: Main utility to extract information from command line arguments and
    // Environment variables, namely COMP args in "plumbing" mode.
    //
    // options -  The options hash as parsed by minimist, plus an env property
    //            representing user environment (default: { env: process.env })
    //    :_      - The arguments Array parsed by minimist (positional arguments)
    //    :env    - The environment Object that holds COMP args (default: process.env)
    //
    // Examples
    //
    //   var env = complete.parseEnv();
    //   // env:
    //   // args        the positional arguments used
    //   // complete    A Boolean indicating whether we act in "plumbing mode" or not
    //   // words       The Number of words in the completed line
    //   // point       A Number indicating cursor position
    //   // line        The String input line
    //   // partial     The String part of line preceding cursor position
    //   // last        The last String word of the line
    //   // lastPartial The last word String of partial
    //   // prev        The String word preceding last
    //
    // Returns the data env object.

  }, {
    key: 'parseEnv',
    value: function parseEnv(options) {
      options = options || {};
      options = assign({}, options, this.options);
      var args = options._ || process.argv.slice(2);
      var env = options.env || process.env;

      var cword = Number(env.COMP_CWORD);
      var point = Number(env.COMP_POINT);
      var line = env.COMP_LINE || '';

      if (isNaN(cword)) cword = 0;
      if (isNaN(point)) point = 0;

      var partial = line.slice(0, point);

      var parts = line.split(' ');
      var prev = parts.slice(0, -1).slice(-1)[0];

      var last = parts.slice(-1).join('');
      var lastPartial = partial.split(' ').slice(-1).join('');

      var complete = args[0] === 'completion';

      if (!env.COMP_CWORD || typeof env.COMP_POINT === 'undefined' || typeof env.COMP_LINE === 'undefined') {
        complete = false;
      }

      return {
        args: args,
        complete: complete,
        words: cword,
        point: point,
        line: line,
        partial: partial,
        last: last,
        lastPartial: lastPartial,
        prev: prev || ''
      };
    }

    // Public: Script templating helper
    //
    // Outputs npm's completion script with pkgname and completer placeholder
    // replaced.
    //
    // name     - The String package/binary name
    // complete - The String completer name, usualy the same as name above. Can
    //            differ to delegate the completion behavior to another command.
    //
    // Returns the script content with placeholders replaced

  }, {
    key: 'script',
    value: function script(name, completer, shell) {
      return read(join(__dirname, '../scripts/' + (shell || 'completion') + '.sh'), 'utf8').replace(/\{pkgname\}/g, name).replace(/{completer}/g, completer);
    }

    // Public: Recursively walk up the `module.parent` chain to find original file.

  }, {
    key: 'findParent',
    value: function findParent(module, last) {
      if (!module.parent) return module;
      return this.findParent(module.parent);
    }

    // Public: Recursively walk up the directories, untill it finds the `file`
    // provided, or reach the user $HOME dir.

  }, {
    key: 'findUp',
    value: function findUp(file, dir) {
      dir = path.resolve(dir || './');

      // stop at user $HOME dir
      if (dir === process.env.HOME) return;
      if (exists(join(dir, file))) return join(dir, file);
      return this.findUp(file, path.dirname(dir));
    }

    // Public: package name resolver.
    //
    // When options.name is not defined, this gets called to attempt to determine
    // completer name.
    //
    // It'll attempt to follow the module chain and find the package.json file to
    // determine the command name being completed.

  }, {
    key: 'resolve',
    value: function resolve(prop) {
      // `module` is special node builtin
      var parent = this.findParent(module);
      if (!parent) return;

      var dirname = path.dirname(parent.filename);

      // was invoked by cli tabtab, fallback to local package.json
      if (parent.filename === path.join(__dirname, '../bin/tabtab')) {
        dirname = path.resolve();
      }

      var jsonfile = this.findUp('package.json', dirname);
      if (!jsonfile) return;

      return require(jsonfile)[prop];
    }
  }]);

  return Complete;
}(EventEmitter);

assign(Complete.prototype, CacheMixin);

module.exports = Complete;