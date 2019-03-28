'use strict';

var padRight = require('pad-right');
var utils = require('../utils');
var bold = utils.colors.bold;
var cyan = utils.colors.cyan;
var gray = utils.colors.gray;

/**
 * Show a help menu.
 *
 * ```sh
 * $ app --help
 * ```
 * @name help
 * @api public
 */

module.exports = function(app, base, options) {
  return function(val, key, config, next) {
    config.run = false;
    console.log(format(help(app, base, options)));
    next();
  };
};

/**
 * Create `help` documentation
 */

function help(app, base, options) {
  var configname = base.option('help.configname') || app.constructor.name.toLowerCase();
  var appname = base.option('help.appname') || base._name;
  var command = base.option('help.command') || appname;

  return {
    heading: `Usage: ${cyan(command + ' <command> [options]')}`
      + '\n'
      + `\nCommand: ${configname} or tasks to run`
      + `\n\nOptions:`,

    examples: 'Examples:'
      + '\n'
      + gray(`\n  # run ${configname} "foo"`)
      + bold(`\n  $ ${command} foo`)
      + '\n'
      + gray(`\n  # run task "bar" from ${configname} "foo"`)
      + bold(`\n  $ ${command} foo:bar`)
      + '\n'
      + gray(`\n  # run multiple tasks from ${configname} "foo"`)
      + bold(`\n  $ ${command} foo:bar,baz,qux`)
      + '\n'
      + gray(`\n  # run a sub-generator from ${configname} "foo"`)
      + bold(`\n  $ ${command} foo.abc`)
      + '\n'
      + gray(`\n  # run task "xyz" from sub-generator "foo.abc"`)
      + bold(`\n  $ ${command} foo.abc:xyz`)
      + '\n'
      + gray(`\n  ${upper(appname)} attempts to automatically determine if "foo" is a task or ${configname}.`
      + `\n  If there is a conflict, you can force ${appname} to run ${configname} "foo"`
      + `\n  by specifying its default task. Example: \`$ ${command} foo:default\``)
      + '\n',

    options: {
      config: {
        description: `Save a configuration value to the \`${command}\` object in package.json`,
        example: '--config=toc:false',
        short: 'c'
      },
      cwd: {
        description: 'Set or display the current working directory',
        example: '',
        short: null
      },
      data: {
        description: 'Define data. API equivalent of `app.data()`',
        example: '',
        short: 'd'
      },
      diff: {
        hide: true,
        description: 'this command is not enabled yet',
        example: '',
        short: null
      },
      disable: {
        description: 'Disable an option. API equivalent of "app.disable(\'foo\')"',
        example: '',
        short: null
      },
      enable: {
        description: 'Enable an option. API equivalent of "app.enable(\'foo\')"',
        example: '',
        short: null
      },
      global: {
        description: 'Save a global configuration value to use as a default',
        example: '--global=toc:false',
        short: 'g'
      },
      help: {
        description: 'Display this help menu',
        example: '',
        short: 'h'
      },
      init: {
        description: 'Prompts for configuration values and stores the answers',
        example: '',
        short: 'i'
      },
      option: {
        description: 'Define options. API equivalent of `app.option()`',
        example: '',
        short: 'o'
      },
      run: {
        description: 'Force tasks to run regardless of command line flags used',
        example: '',
        short: null
      },
      silent: {
        description: 'Silence all tasks and updaters in the terminal',
        example: '--silent',
        short: 'S'
      },
      show: {
        description: 'Display the value of <key>',
        example: '--show <key>',
        command: 'key',
        short: null
      },
      version: {
        description: 'Display the current version of ' + appname,
        example: '',
        short: 'V'
      },
      verbose: {
        description: 'Display all verbose logging messages',
        example: '',
        short: 'v'
      }
    },
    footer: ''
  };
}

function format(help) {
  var heading = help.heading || '';
  var options = help.options || {};
  var footer = help.footer || '';
  var optsList = '';

  for (var key in options) {
    if (options.hasOwnProperty(key)) {
      var val = options[key];
      if (val.hide === true) continue;
      if (val.command) {
        key += ' <' + val.command + '>';
      }

      optsList += toFlag(key, val.short);
      optsList += val.description + '\n';
    }
  }

  var res = '\n' + heading + '\n\n';
  res += optsList + '\n';
  res += footer;
  return res + help.examples;
}

function upper(str) {
  return str.charAt(0).toUpperCase() + str.slice(1);
}

function toFlag(key, short, max) {
  var str = '  --' + key + shortKey(short);
  return padRight(str, 20, ' ');
}

function shortKey(sh) {
  return sh ? (', '+ '-' + sh) : '    ';
}
