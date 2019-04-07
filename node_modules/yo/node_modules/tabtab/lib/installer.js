const debug = require('./debug')('tabtab:installer')


import fs from                'fs'
import path from              'path'
import inquirer from          'inquirer';
import { spawn, exec } from   'child_process';
import mkdirp from            'mkdirp';

let errmsg = `
Error: You don't have permission to write to :destination.
Try running with sudo instead:

  sudo ${process.argv.join(' ')}
`;

// Public: Manage installation / setup of completion scripts.
//
// pkg-config --variable=completionsdir bash-completion
// pkg-config --variable=compatdir bash-completion
export default class Installer {
  get home() {
    return process.env[(process.platform == 'win32') ? 'USERPROFILE' : 'HOME'];
  }

  constructor(options, complete) {
    this.options = options || {};
    this.complete = complete;
  }

  // Called on install command.
  //
  // Performs the installation process.
  handle(name, options) {
    this.options.name = name;

    if (options.stdout) return new Promise((r, errback) => {
      return this.writeTo({
        destination: 'stdout'
      });
    });

    return this.prompt()
      .then(this.writeTo.bind(this));
  }

  writeTo(data) {
    var destination = data.destination;
    debug('Installing completion script to %s directory', destination);

    var script = this.complete.script(this.options.name, this.options.completer || this.options.name, this.template);

    if (destination === 'stdout') return process.stdout.write('\n\n' + script + '\n');

    if (destination === 'bashrc') destination = path.join(this.home, '.bashrc');
    else if (destination === 'zshrc') destination = path.join(this.home, '.zshrc');
    else if (destination === 'fish') destination = path.join(this.home, '.config/fish/config.fish');
    else if (destination === 'fishdir') destination = path.join(this.home, '.config/fish/completions', this.options.name + '.fish');
    else destination = path.join(destination, this.options.name);

    return new Promise(this.createStream.bind(this, destination))
      .then(this.installCompletion.bind(this, destination));
  }

  createStream(destination, r, errback) {
    // debug('Check %s destination', destination);
    var flags = 'a';
    fs.stat(destination, (err, stat) => {
      if (err && err.code === 'ENOENT') flags = 'w';
      else if (err) return errback(err);

      mkdirp(path.dirname(destination), (err) => {
        if (err) return errback(err);

        var out = fs.createWriteStream(destination, { flags });
        out.on('error', (err) => {
          if (err.code === 'EACCES') {
            console.error(errmsg.replace(':destination', destination));
          }
          return errback(err);
        });

        out.on('open', () => {
          debug('Installing completion script to %s directory', destination);
          debug('Writing to %s file in %s mode', destination, flags === 'a' ? 'append' : 'write');
          r(out);
        });
      });
    });
  }

  installCompletion(destination, out) {
    var name = this.options.name;
    var script = this.complete.script(name, this.options.completer || name, this.template);
    var filename = path.join(__dirname, '../.completions', name + '.' + this.template);
    debug('Writing actual completion script to %s', filename);

    // First write internal completion script in a local .comletions directory
    // in this module. This gets sourced in user scripts after, to avoid
    // cluttering bash/zshrc files with too much boilerplate.
    return new Promise((r, errback) => {
      fs.writeFile(filename, script, (err) => {
        if (err) return errback(err);

        var regex = new RegExp(`tabtab source for ${name}`);
        fs.readFile(destination, 'utf8', (err, content) => {
          if (err) return errback(err);
          if (regex.test(content)) {
            return debug('Already installed %s in %s', name, destination);
          }

          console.error('\n[tabtab] Adding source line to load %s\nin %s\n', filename, destination);

          out.write('\n');
          debug('. %s > %s', filename, destination);
          out.write('\n# tabtab source for ' + name + ' package');
          out.write('\n# uninstall by removing these lines or running ');
          out.write('`tabtab uninstall ' + name + '`');

          if (this.template === 'fish') {
            out.write('\n[ -f ' + filename + ' ]; and . ' + filename);
          } else if (this.template === 'zsh') {
            out.write('\n[[ -f ' + filename + ' ]] && . ' + filename);
          } else {
            out.write('\n[ -f ' + filename + ' ] && . ' + filename);
          }
        });
      });
    });
  }

  // Prompts user for installation location.
  prompt() {
    var choices = [{
      name:   'Nowhere. Just output to STDOUT',
      value:  'stdout',
      short:  'stdout'
    }];

    var prompts = [{
      message: 'Where do you want to setup the completion script',
      name: 'destination',
      type: 'list',
      choices: choices
    }];

    return this.shell()
      .then((entries) => {
        prompts[0].choices = choices.concat(entries);
        return inquirer.prompt(prompts);
      });
  }

  // Shell adapters.
  //
  // Supported:
  //
  // - bash   - Asks pkg-config for completion directories
  // - zsh    - Lookup $fpath environment variable
  // - fish   - Lookup for $fish_complete_path
  shell() {
    return new Promise((r, errback) => {
      var shell = process.env.SHELL;
      if (shell) shell = shell.split('/').slice(-1)[0];

      return this[shell]().then(r)
        .catch(errback);
    });
  }

  fish() {
    debug('Fish shell detected');
    this.template = 'fish';
    return new Promise((r, errback) => {
      var dir = path.join(this.home, '.config/fish/completions');
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

  bash() {
    debug('Bash shell detected');
    this.template = 'bash';
    var entries = [{
      name:   'Bash config file (~/.bashrc)',
      value:  'bashrc',
      short:  'bash'
    }];

    return this.completionsdir()
      .then((dir) => {
        debug(dir);
        if (dir) {
          entries.push({
            name: 'Bash completionsdir ( ' + dir + ' )',
            value: dir
          });
        }
        return this.compatdir();
      })
      .then((dir) => {
        if (dir) {
          entries.push({
            name: 'Bash compatdir ( ' + dir + ' )',
            value: dir
          });
        }

        return entries;
      });
  }

  zsh() {
    debug('Zsh shell detected');
    this.template = 'zsh';
    return new Promise((r, errback) => {
      var dir = '/usr/local/share/zsh/site-functions';
      return r([{
        name:   'Zsh config file (~/.zshrc)',
        value:  'zshrc',
        short:  'zsh'
      }, {
        name: 'Zsh completion directory (' + dir + ')',
        value: dir
      }]);
    });
  }

  // Bash

  // Public: pkg-config wrapper
  pkgconfig(variable) {
    return new Promise((r, errback) => {
      var cmd = `pkg-config --variable=${variable} bash-completion`;
      debug('cmd', cmd);
      exec(cmd, function(err, stdout, stderr) {
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
  completionsdir() {
    debug('Asking pkg-config for completionsdir');
    return this.pkgconfig('completionsdir');
  }

  // Returns the pkg-config variable for "compatdir" and bash-completion
  compatdir() {
    debug('Asking pkg-config for compatdir');
    return this.pkgconfig('compatdir');
  }
}
