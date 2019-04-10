'use strict';

var walkSync = require('walk-sync');
var FSTree = require('fs-tree-diff');
var mkdirp = require('mkdirp');
var fs = require('fs');
var debug = require('debug')('tree-sync');

module.exports = TreeSync;

function TreeSync(input, output, options) {
  this._input = input;
  this._output = output;
  this._options = options || {};
  this._walkSyncOpts = {};
  this._hasSynced = false;
  this._lastInput = FSTree.fromEntries([]);

  // Pass through whitelisted options to walk-sync.
  if (this._options.globs) {
    this._walkSyncOpts.globs = options.globs;
  }
  if (this._options.ignore) {
    this._walkSyncOpts.ignore = options.ignore;
  }

  debug('initializing TreeSync:  %s -> %s', this._input, this._output);
}

TreeSync.prototype.sync = function() {
  mkdirp.sync(this._output);
  mkdirp.sync(this._input);

  debug('syncing %s -> %s', this._input, this._output);

  var input = FSTree.fromEntries(walkSync.entries(this._input, this._walkSyncOpts));
  var output = FSTree.fromEntries(walkSync.entries(this._output, this._walkSyncOpts));

  debug('walked %s %dms and  %s %dms', this._input, input.size, this._output, output.size);

  var isFirstSync = !this._hasSynced;
  var operations = output.calculatePatch(input).filter(function(operation) {
    if (operation[0] === 'change') {
      return isFirstSync;
    } else {
      return true;
    }
  });

  var inputOperations = this._lastInput.calculatePatch(input).filter(function(operation) {
    return operation[0] === 'change';
  });

  this._lastInput = input;

  operations = operations.concat(inputOperations);

  debug('calc operations %d', operations.length);

  operations.forEach(function(patch) {
    var operation = patch[0];
    var pathname = patch[1];
    var entry = patch[2];

    var inputFullpath = this._input + '/' + pathname;
    var outputFullpath = this._output + '/' + pathname;

    switch(operation) {
      case 'create' :
        return fs.writeFileSync(outputFullpath, fs.readFileSync(inputFullpath), { mode: entry.mode });
      case 'change' :
        return fs.writeFileSync(outputFullpath, fs.readFileSync(inputFullpath), { mode: entry.mode });
      case 'mkdir' :
        try {
          return fs.mkdirSync(outputFullpath);
        } catch(e) {
          if (e && e.code === 'EEXIST') { /* do nothing */ }
          else { throw e; }
        }
        break;
      case 'unlink':
        try {
          return fs.unlinkSync(outputFullpath);
        } catch(e) {
          if (e && e.code === 'ENOENT') { /* do nothing */ }
          else { throw e; }
        }
        break;
      case 'rmdir':
        return fs.rmdirSync(outputFullpath);
      default:
        throw TypeError('Unknown operation:' + operation + ' on path: ' + pathname);
    }
  }, this);

  this._hasSynced = true;
  debug('applied patches: %d', operations.length);

  // Return only type and name; don't want downstream relying on entries.
  return operations.map(function (op) {
    return op.slice(0,2);
  });
};
