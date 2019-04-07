'use strict';

Object.defineProperty(exports, "__esModule", {
  value: true
});

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

var _installer = require('../installer');

var _installer2 = _interopRequireDefault(_installer);

function _interopRequireDefault(obj) { return obj && obj.__esModule ? obj : { default: obj }; }

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var fs = require('fs');
var path = require('path');
var join = path.join;
var debug = require('../debug')('tabtab:commands');
var Complete = require('../complete');

var _require = require('fs');

var read = _require.readFileSync;
var exists = _require.existsSync;

// Public: Commands class that defines the various command available with tabtab(1)
//
// Examples
//
//   // TODO: document all methods
//
//   var commands = new Commands({});
//   commands.install();

var Commands = function () {
  _createClass(Commands, [{
    key: 'allowed',
    get: function get() {
      return ['install', 'uninstall', 'list', 'search', 'add', 'rm', 'completion'];
    }

    // Public: Creates a new command instance. this.complete is an instance of
    // Complete.

  }]);

  function Commands(options) {
    _classCallCheck(this, Commands);

    this.options = options || {};
    this.complete = new Complete(this.options);
    this.installer = new _installer2.default(this.options, this.complete);
  }

  // Commands

  // Public: Fow now, just output to the console


  _createClass(Commands, [{
    key: 'install',
    value: function install(options) {
      options = options || {};
      var script = this.complete.script(this.name, this.name || 'tabtab');

      if (process.platform === 'win32') {
        return debug('Windows shell completion not yet supported, skipping completion install');
      }

      var shell = process.env.SHELL;
      if (shell) shell = shell.split('/').slice(-1)[0];
      if (!this.installer[shell]) {
        return debug('User shell %s not supported, skipping completion install', shell);
      }

      this.installer.handle(this.options.name || this.complete.resolve('name'), options).catch(function (e) {
        console.error('oh oh', e.stack);
        process.exit(1);
      });
    }

    // Public: Delegates to this.handle

  }, {
    key: 'completion',
    value: function completion(options) {
      options = options || this.options;
      return this.complete.handle(options);
    }

    // Public: to be implemented.

  }, {
    key: 'uninstall',
    value: function uninstall() {}

    // Public: to be implemented.

  }, {
    key: 'search',
    value: function search() {}

    // Public: to be implemented.

  }, {
    key: 'list',
    value: function list() {}

    // Public: to be implemented.

  }, {
    key: 'add',
    value: function add() {}

    // Public: to be implemented.

  }, {
    key: 'rm',
    value: function rm() {}

    // Public: --help output
    //
    // Returns the String output

  }, {
    key: 'help',
    value: function help() {
      return '\n    $ tabtab <command> [options]\n\n    Options:\n      -h, --help              Show this help output\n      -v, --version           Show package version\n      -s, --silent            Silent mode for commands like install\n      -y, --yes               Skips confirmation prompts\n\n    Commands:\n\n      install                 Install and enable completion file on user system\n      uninstall               Undo the install command\n      list                    List the completion files managed by tabtab\n      search                  Search npm registry for tabtab completion files / dictionaries\n      add                     Install additional completion files / dictionaries\n      rm/remove               Uninstall completion file / dictionnary\n    ';
    }
  }]);

  return Commands;
}();

exports.default = Commands;