var fs             = require('fs');
var path           = require('path');
var EventEmitter   = require('events').EventEmitter;
var sane           = require('sane');
var Promise        = require('rsvp').Promise;
var printSlowTrees = require('broccoli-slow-trees');
var logger         = require('heimdalljs-logger')('broccoli-sane-watcher');
var heimdall       = require('heimdalljs');

function defaultFilterFunction(name) {
  return /^[^\.]/.test(name);
}

module.exports = Watcher;

function Watcher(builder, options) {
  logger.info('initialize: %o', options);
  this.builder = builder;
  this.options = options || {};
  this.options.filter = this.options.filter || defaultFilterFunction;
  this.watched = Object.create(null);
  this.timeout = null;
  this.changedFiles = [];
  this.sequence = this.build();
}

Watcher.prototype = Object.create(EventEmitter.prototype);

// gathers rapid changes as one build
Watcher.prototype.scheduleBuild = function (filePath) {
  this.changedFiles.push(filePath);
  if (this.timeout) {
    logger.info('debounce scheduleBuild: %s', filePath);
    return;
  }

  logger.info('scheduleBuild: %s', filePath);

  // we want the timeout to start now before we wait for the current build
  var timeout = new Promise(function (resolve) {
    this.timeout = setTimeout(resolve, this.options.debounce || 100);
  }.bind(this));

  var build = function() {
    this.timeout = null;
    return this.build(filePath);
  }.bind(this);

  // we want the build to wait first for the current build, then the timeout
  function timoutThenBuild() {
    return timeout.then(build);
  }
  // we want the current promise to be waiting for the current build regardless if it fails or not
  // can't use finally because we want to be able to affect the result.
  this.sequence = this.sequence.then(timoutThenBuild, timoutThenBuild);
};

Watcher.prototype.build = function Watcher_build(filePath) {
  logger.info('build: %s', filePath);
  var addWatchDir = this.addWatchDir.bind(this);
  var triggerChange = this.triggerChange.bind(this);
  var triggerError = this.triggerError.bind(this);
  var heimdallNode;

  function verboseOutput(run) {
    if (this.options.verbose) {
      printSlowTrees(run.graph.__heimdall__);
    }

    return run;
  }

  function cleanup(run) {
    // guard against `build` rejecting
    if (heimdallNode) {
      // remove the heimdall subtree for this build so we don't leak.  If
      // BROCCOLI_VIZ=1 then we have already output the json in `verboseOutput`.
      heimdallNode.remove();
    }

    return run;
  }

  function totalTime(hash) {
    hash.totalTime = sum(hash.graph.__heimdall__, function(node) {
      return node.stats.time.self;
    });

    return hash;
  }

  function appendFilePath(hash) {
    hash.filePath = filePath;
    return hash;
  }

  function saveNode(hash) {
    heimdallNode = hash.graph.__heimdall__;

    return hash;
  }

  var changedFiles = this.changedFiles;
  this.changedFiles = [];

  var annotation = {
    type: !!filePath ? 'rebuild' : 'initial',
    reason: 'watcher',
    primaryFile: filePath,
    changedFiles: changedFiles,
  };

  logger.info('%o', annotation);

  return this.builder
    .build(addWatchDir, annotation)
    .then(saveNode)
    .then(totalTime)
    .then(appendFilePath)
    .then(triggerChange, triggerError)
    .then(verboseOutput.bind(this))
    .finally(cleanup);
};

function sum(node, cb) {
  var total = 0;
  node.visitPreOrder(function(node) {
    total += cb(node);
  });
  return total;
}

Watcher.prototype.addWatchDir = function Watcher_addWatchDir(dir) {
  if (this.watched[dir]) {
    logger.info('addWatchDir: (not added duplicate) %s', dir);
    return;
  }

  logger.info('addWatchDir: %s', dir);

  if (!fs.existsSync(dir)) {
    throw new Error('Attempting to watch missing directory: ' + dir);
  }

  var watcher = new sane(dir, this.options);

  watcher.on('change', this.onFileChanged.bind(this));
  watcher.on('add', this.onFileAdded.bind(this));
  watcher.on('delete', this.onFileDeleted.bind(this));
  watcher.on('error', this.onError.bind(this));

  this.watched[dir] = watcher;
};

function makeOnChanged (log) {
  return function (filePath, root) {
    if (this.options.filter(path.basename(filePath), filePath, root)) {
      if (this.options.verbose) console.log(log, filePath);
      this.scheduleBuild(path.join(root, filePath));
    }
  };
}

Watcher.prototype.onFileChanged = makeOnChanged('file changed');
Watcher.prototype.onFileAdded = makeOnChanged('file added');
Watcher.prototype.onFileDeleted = makeOnChanged('file deleted');

Watcher.prototype.onError = function(error) {
  this.emit('error', error);
};

Watcher.prototype.triggerChange = function (hash) {
  logger.info('triggerChange');
  this.emit('change', hash);
  return hash;
};

Watcher.prototype.triggerError = function (error) {
  logger.info('triggerError %o', error);
  this.emit('error', error);
  throw error;
};

Watcher.prototype.close = function () {
  logger.info('close');
  clearTimeout(this.timeout);
  var watched = this.watched;
  for (var dir in watched) {
    if (!watched[dir]) continue;
    var watcher = watched[dir];
    delete watched[dir];
    watcher.close();
  }
};

Watcher.prototype.then = function(success, fail) {
  return this.sequence.then(success, fail);
};
;
