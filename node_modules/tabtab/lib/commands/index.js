const fs        = require('fs');
const path      = require('path');
const join      = path.join;
const debug     = require('../debug')('tabtab:commands')
const Complete  = require('../complete');

import Installer from '../installer';

const {
  readFileSync: read,
  existsSync: exists
} = require('fs');

// Public: Commands class that defines the various command available with tabtab(1)
//
// Examples
//
//   // TODO: document all methods
//
//   var commands = new Commands({});
//   commands.install();
export default class Commands {

  get allowed() {
    return ['install', 'uninstall', 'list', 'search', 'add', 'rm', 'completion'];
  }

  // Public: Creates a new command instance. this.complete is an instance of
  // Complete.
  constructor(options) {
    this.options = options || {};
    this.complete = new Complete(this.options);
    this.installer = new Installer(this.options, this.complete);
    this.shell = (process.env.SHELL || '').split('/').slice(-1)[0];
  }

  // Commands

  // Public: Fow now, just output to the console
  install(options) {
    options = options || {};
    var script = this.complete.script(this.name, this.name || 'tabtab');

    var shell = process.env.SHELL;
    if (shell) shell = shell.split((process.platform !== 'win32') ? '/' : '\\').slice(-1)[0];
    if (!this.installer[shell]) {
      return debug('User shell %s not supported, skipping completion install', shell);
    }

    this.installer
      .handle(this.options.name || this.complete.resolve('name'), options)
      .catch((e) => {
        console.error('oh oh', e.stack);
        process.exit(1);
      });
  }

  // Public: Delegates to this.handle
  completion(options) {
    options = options || this.options;
    return this.complete.handle(options);
  }

  // Public: to be implemented.
  uninstall(options) {
    debug('Trigger uninstall command', options);
    if (!options.auto) throw new Error('Uninstall only available with --auto flag');

    var dest = this.shell === 'zsh' ? '~/.zshrc' :
      this.shell === 'bash' ? '~/.bashrc' :
      '~/.config/fish/config.fish';

    // win32 ?
    dest = dest.replace('~', process.env.HOME);

    debug('Destination:', dest);
    this.installer.uninstallCompletion(dest)
      .catch(e => {
        throw e
      })
      .then(() => {
        console.log('uninstall end');
      });

  }

  // Public: to be implemented.
  search() {}

  // Public: to be implemented.
  list() {}

  // Public: to be implemented.
  add() {}

  // Public: to be implemented.
  rm() {}

  // Public: --help output
  //
  // Returns the String output
  help() {
    return `
    $ tabtab <command> [options]

    Options:
      -h, --help              Show this help output
      -v, --version           Show package version
      --name                  Binary name tabtab should complete
      --auto                  Use default SHELL configuration file
                              (~/.bashrc, ~/.zshrc or ~/.config/fish/config.fish)

    Commands:

      install                 Install and enable completion file on user system
      uninstall               Undo the install command
    `;
      // list                    List the completion files managed by tabtab
      // search                  Search npm registry for tabtab completion files / dictionaries
      // add                     Install additional completion files / dictionaries
      // rm/remove               Uninstall completion file / dictionnary
  }
}
