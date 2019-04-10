const fs    = require('fs');
const path  = require('path');
const util  = require('util');
const dbg   = require('debug');

// process.env.TABTAB_DEBUG = process.env.TABTAB_DEBUG || '/tmp/tabtab.log';

let out = process.env.TABTAB_DEBUG ?
  fs.createWriteStream(process.env.TABTAB_DEBUG, { flags: 'a' }) :
  null;

module.exports = debug;

// Internal: Facade to debug module, which provides the exact same interface.
//
// The added benefit is with the TABTAB_DEBUG environment variable, which when
// defined, will write debug output to the specified filename.
//
// Usefull when debugging tab completion, as logs on stdout / stderr are either
// shallowed or used as tab completion results.
//
// namespace - The String namespace to use when TABTAB_DEBUG is not defined,
//             delegates to debug module.
//
// Examples
//
//    // Use with following command to redirect output to file
//    // TABTAB_DEBUG="debug.log" tabtab ...
//    debug('Foo');
function debug(namespace) {
  var log = dbg(namespace);
  return (...args) => {
    out && out.write(util.format.apply(util, args) + '\n');
    out || log.apply(null, args);
  }
}
