'use strict';

var _commands = require('./commands');

var _commands2 = _interopRequireDefault(_commands);

function _interopRequireDefault(obj) { return obj && obj.__esModule ? obj : { default: obj }; }

var debug = require('./debug')('tabtab');
var minimist = require('minimist');
var npmlog = require('npmlog');

var opts = minimist(process.argv.slice(2), {
  alias: {
    h: 'help',
    v: 'version'
  }
});

var cmd = opts._[0];

var commands = new _commands2.default(opts);
var allowed = commands.allowed;

if (opts.help) {
  console.log(commands.help());
  process.exit(0);
} else if (opts.version) {
  console.log(commands.help());
  process.exit(0);
} else if (allowed.indexOf(cmd) !== -1) {
  // debug('Run command %s with options', cmd, opts);
  commands[cmd](opts);
} else {
  console.log(commands.help());
}