'use strict';

Object.defineProperty(exports, "__esModule", {
  value: true
});

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

var _fs = require('fs');

var _fs2 = _interopRequireDefault(_fs);

var _path = require('path');

var _path2 = _interopRequireDefault(_path);

var _inquirer = require('inquirer');

var _inquirer2 = _interopRequireDefault(_inquirer);

var _child_process = require('child_process');

var _mkdirp = require('mkdirp');

var _mkdirp2 = _interopRequireDefault(_mkdirp);

function _interopRequireDefault(obj) { return obj && obj.__esModule ? obj : { default: obj }; }

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var debug = require('./debug')('tabtab:installer');

var errmsg = '\nError: You don\'t have permission to write to :destination.\nTry running with sudo instead:\n\n  sudo ' + process.argv.join(' ') + '\n';

// Public: Manage installation / setup of completion scripts.
//
// pkg-config --variable=completionsdir bash-completion
// pkg-config --variable=compatdir bash-completion

var Installer = function () {
  _createClass(Installer, [{
    key: 'home',
    get: function get() {
      return process.env[process.platform == 'win32' ? 'USERPROFILE' : 'HOME'];
    }
  }]);

  function Installer(options, complete) {
    _classCallCheck(this, Installer);

    this.options = options || {};
    this.complete = complete;

    this._shell = (process.env.SHELL || '').split('/').slice(-1)[0];
    this.dest = this._shell === 'zsh' ? 'zshrc' : this._shell === 'bash' ? 'bashrc' : 'fish';

    this.dest = this.dest.replace('~', process.env.HOME);
  }

  // Called on install command.
  //
  // Performs the installation process.


  _createClass(Installer, [{
    key: 'handle',
    value: function handle(name, options) {
      var _this = this;

      this.options.name = name;

      if (options.stdout) return new Promise(function (r, errback) {
        return _this.writeTo({
          destination: 'stdout'
        });
      });

      if (options.auto) {
        this.template = this._shell;
        return this.writeTo({ destination: this.dest });
      }

      return this.prompt().then(this.writeTo.bind(this));
    }
  }, {
    key: 'writeTo',
    value: function writeTo(data) {
      var destination = data.destination;
      debug('Installing completion script to %s directory', destination);

      var script = this.complete.script(this.options.name, this.options.completer || this.options.name, this.template);

      if (destination === 'stdout') return process.stdout.write('\n\n' + script + '\n');

      if (destination === 'bashrc') destination = _path2.default.join(this.home, '.bashrc');else if (destination === 'zshrc') destination = _path2.default.join(this.home, '.zshrc');else if (destination === 'fish') destination = _path2.default.join(this.home, '.config/fish/config.fish');else if (destination === 'fishdir') destination = _path2.default.join(this.home, '.config/fish/completions', this.options.name + '.fish');else destination = _path2.default.join(destination, this.options.name);

      return new Promise(this.createStream.bind(this, destination)).then(this.installCompletion.bind(this, destination));
    }
  }, {
    key: 'createStream',
    value: function createStream(destination, r, errback) {
      // debug('Check %s destination', destination);
      var flags = 'a';
      _fs2.default.stat(destination, function (err, stat) {
        if (err && err.code === 'ENOENT') flags = 'w';else if (err) return errback(err);

        (0, _mkdirp2.default)(_path2.default.dirname(destination), function (err) {
          if (err) return errback(err);

          var out = _fs2.default.createWriteStream(destination, { flags: flags });
          out.on('error', function (err) {
            if (err.code === 'EACCES') {
              console.error(errmsg.replace(':destination', destination));
            }
            return errback(err);
          });

          out.on('open', function () {
            debug('Installing completion script to %s directory', destination);
            debug('Writing to %s file in %s mode', destination, flags === 'a' ? 'append' : 'write');
            r(out);
          });
        });
      });
    }
  }, {
    key: 'installCompletion',
    value: function installCompletion(destination, out) {
      var _this2 = this;

      var name = this.options.name;
      var script = this.complete.script(name, this.options.completer || name, this.template);
      var filename = _path2.default.join(__dirname, '../.completions', name + '.' + this.template);
      if (process.platform === 'win32') {
        filename = filename.replace(/\\/g, '/');
      } // win32: replace backslashes with forward slashes
      debug('Writing actual completion script to %s', filename);

      // First write internal completion script in a local .comletions directory
      // in this module. This gets sourced in user scripts after, to avoid
      // cluttering bash/zshrc files with too much boilerplate.
      return new Promise(function (r, errback) {
        _fs2.default.writeFile(filename, script, function (err) {
          if (err) return errback(err);

          var regex = new RegExp('tabtab source for ' + name);
          _fs2.default.readFile(destination, 'utf8', function (err, content) {
            if (err) return errback(err);
            if (regex.test(content)) {
              return debug('Already installed %s in %s', name, destination);
            }

            console.error('\n[tabtab] Adding source line to load %s\nin %s\n', filename, destination);

            debug('. %s > %s', filename, destination);
            out.write('\n# tabtab source for ' + name + ' package');
            out.write('\n# uninstall by removing these lines or running ');
            out.write('`tabtab uninstall ' + name + '`');

            if (_this2.template === 'fish') {
              out.write('\n[ -f ' + filename + ' ]; and . ' + filename);
            } else if (_this2.template === 'zsh') {
              out.write('\n[[ -f ' + filename + ' ]] && . ' + filename);
            } else {
              out.write('\n[ -f ' + filename + ' ] && . ' + filename);
            }
          });
        });
      });
    }
  }, {
    key: 'uninstallCompletion',
    value: function uninstallCompletion(destination) {
      var _this3 = this;

      return new Promise(function (r, errback) {
        _fs2.default.readFile(destination, 'utf8', function (err, body) {
          if (err) return errback(err);
          r(body);
        });
      }).then(function (body) {
        var lines = body.split(/\r?\n/);

        debug('Uninstall', _this3.options);
        var name = _this3.options.name;
        var reg = new RegExp('(tabtab source for ' + name + ' package|`tabtab uninstall ' + name + '`|tabtab/.completions/' + name + ')');
        lines = lines.filter(function (line) {
          return !reg.test(line);
        });

        return lines.join('\n');
      }).then(function (content) {
        return new Promise(function (r, errback) {
          _fs2.default.writeFile(destination, content, function (err) {
            if (err) return errback(err);
            debug('%s sucesfully updated to remove tabtab', destination);
          });
        });
      });
    }

    // Prompts user for installation location.

  }, {
    key: 'prompt',
    value: function prompt() {
      var choices = [{
        name: 'Nowhere. Just output to STDOUT',
        value: 'stdout',
        short: 'stdout'
      }];

      var prompts = [{
        message: 'Where do you want to setup the completion script',
        name: 'destination',
        type: 'list',
        choices: choices
      }];

      return this.shell().then(function (entries) {
        prompts[0].choices = choices.concat(entries);
        return _inquirer2.default.prompt(prompts);
      });
    }

    // Shell adapters.
    //
    // Supported:
    //
    // - bash   - Asks pkg-config for completion directories
    // - zsh    - Lookup $fpath environment variable
    // - fish   - Lookup for $fish_complete_path

  }, {
    key: 'shell',
    value: function shell() {
      var _this4 = this;

      return new Promise(function (r, errback) {
        var shell = process.env.SHELL;
        if (shell) shell = shell.split(process.platform !== 'win32' ? '/' : '\\').slice(-1)[0];

        return _this4[shell]().then(r).catch(errback);
      });
    }
  }, {
    key: 'fish',
    value: function fish() {
      var _this5 = this;

      debug('Fish shell detected');
      this.template = 'fish';
      return new Promise(function (r, errback) {
        var dir = _path2.default.join(_this5.home, '.config/fish/completions');
        return r([{
          name: 'Fish config file (~/.config/fish/config.fish)',
          value: 'fish',
          short: 'fish'
        }, {
          name: 'Fish completion directory (' + dir + ')',
          value: 'fishdir',
          short: 'fish'
        }]);
      });
    }
  }, {
    key: 'bash',
    value: function bash() {
      var _this6 = this;

      debug('Bash shell detected');
      this.template = 'bash';
      var entries = [{
        name: 'Bash config file (~/.bashrc)',
        value: 'bashrc',
        short: 'bash'
      }];

      return this.completionsdir().then(function (dir) {
        debug(dir);
        if (dir) {
          entries.push({
            name: 'Bash completionsdir ( ' + dir + ' )',
            value: dir
          });
        }
        return _this6.compatdir();
      }).then(function (dir) {
        if (dir) {
          entries.push({
            name: 'Bash compatdir ( ' + dir + ' )',
            value: dir
          });
        }

        return entries;
      });
    }
  }, {
    key: 'zsh',
    value: function zsh() {
      debug('Zsh shell detected');
      this.template = 'zsh';
      return new Promise(function (r, errback) {
        var dir = '/usr/local/share/zsh/site-functions';
        return r([{
          name: 'Zsh config file (~/.zshrc)',
          value: 'zshrc',
          short: 'zsh'
        }, {
          name: 'Zsh completion directory (' + dir + ')',
          value: dir
        }]);
      });
    }

    // Bash

    // Public: pkg-config wrapper

  }, {
    key: 'pkgconfig',
    value: function pkgconfig(variable) {
      return new Promise(function (r, errback) {
        var cmd = 'pkg-config --variable=' + variable + ' bash-completion';
        debug('cmd', cmd);
        (0, _child_process.exec)(cmd, function (err, stdout, stderr) {
          if (err) {
            // silently fail if pkg-config bash-completion returns an error,
            // meaning bash-completion is either not installed or misconfigured
            // with PKG_CONFIG_PATH
            return r();
          }
          stdout = stdout.trim();
          debug('Got %s for %s', stdout, variable);
          r(stdout);
        });
      });
    }

    // Returns the pkg-config variable for "completionsdir" and bash-completion

  }, {
    key: 'completionsdir',
    value: function completionsdir() {
      debug('Asking pkg-config for completionsdir');
      return this.pkgconfig('completionsdir');
    }

    // Returns the pkg-config variable for "compatdir" and bash-completion

  }, {
    key: 'compatdir',
    value: function compatdir() {
      debug('Asking pkg-config for compatdir');
      return this.pkgconfig('compatdir');
    }
  }]);

  return Installer;
}();

exports.default = Installer;
module.exports = exports['default'];