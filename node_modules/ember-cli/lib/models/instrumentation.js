'use strict';

const fs = require('fs-extra');
const chalk = require('chalk');
const heimdallGraph = require('heimdalljs-graph');
const utilsInstrumentation = require('../utilities/instrumentation');
const logger = require('heimdalljs-logger')('ember-cli:instrumentation');

let vizEnabled = utilsInstrumentation.vizEnabled;
let instrumentationEnabled = utilsInstrumentation.instrumentationEnabled;

function _enableFSMonitorIfInstrumentationEnabled(config) {
  let monitor;
  if (instrumentationEnabled(config)) {
    const FSMonitor = require('heimdalljs-fs-monitor');
    monitor = new FSMonitor();
    monitor.start();
  }
  return monitor;
}

_enableFSMonitorIfInstrumentationEnabled();


class Instrumentation {
  /**
     An instance of this class is used for invoking the instrumentation
     hooks on addons.

     The instrumentation types currently supported are:

     * init
     * build
     * command
     * shutdown

     @class Instrumentation
     @private
  */
  constructor(options) {
    this.isVizEnabled = vizEnabled;
    this.isEnabled = instrumentationEnabled;

    this.ui = options.ui;

    // project constructor will set up bidirectional link
    this.project = null;

    this.instrumentations = {
      init: options.initInstrumentation,
      build: {
        token: null,
        node: null,
        count: 0,
      },
      command: {
        token: null,
        node: null,
      },
      shutdown: {
        token: null,
        node: null,
      },
    };

    this._heimdall = null;

    if (!options.initInstrumentation && this.isEnabled()) {
      this.instrumentations.init = {
        token: null,
        node: null,
      };
      this.start('init');
    }
  }

  _buildSummary(tree, result, resultAnnotation) {
    let buildSteps = 0;
    let totalTime = 0;

    let node;
    let statName;
    let statValue;
    let nodeItr;
    let statsItr;
    let nextNode;
    let nextStat;

    for (nodeItr = tree.dfsIterator(); ;) {
      nextNode = nodeItr.next();
      if (nextNode.done) { break; }

      node = nextNode.value;
      if (node.label.broccoliNode && !node.label.broccoliCachedNode) {
        ++buildSteps;
      }

      for (statsItr = node.statsIterator(); ;) {
        nextStat = statsItr.next();
        if (nextStat.done) { break; }

        statName = nextStat.value[0];
        statValue = nextStat.value[1];

        if (statName === 'time.self') {
          totalTime += statValue;
        }
      }
    }

    let summary = {
      build: {
        type: resultAnnotation.type,
        count: this.instrumentations.build.count,
        outputChangedFiles: null,
      },
      platform: {
        name: process.platform,
      },
      output: null,
      totalTime,
      buildSteps,
    };

    if (result) {
      summary.build.outputChangedFiles = result.outputChanges;
      summary.output = result.directory;
    }

    if (resultAnnotation.type === 'rebuild') {
      summary.build.primaryFile = resultAnnotation.primaryFile;
      summary.build.changedFileCount = resultAnnotation.changedFiles.length;
      summary.build.changedFiles = resultAnnotation.changedFiles.slice(0, 10);
    }

    return summary;
  }

  _initSummary(tree) {
    return {
      totalTime: totalTime(tree),
      platform: {
        name: process.platform,
      },
    };
  }

  _commandSummary(tree, commandName, commandArgs) {
    return {
      name: commandName,
      args: commandArgs,
      totalTime: totalTime(tree),
      platform: {
        name: process.platform,
      },
    };
  }

  _shutdownSummary(tree) {
    return {
      totalTime: totalTime(tree),
      platform: {
        name: process.platform,
      },
    };
  }

  _instrumentationFor(name) {
    let instr = this.instrumentations[name];
    if (!instr) {
      throw new Error(`No such instrumentation "${name}"`);
    }
    return instr;
  }

  _instrumentationTreeFor(name) {
    return heimdallGraph.loadFromNode(this.instrumentations[name].node);
  }

  _invokeAddonHook(name, instrumentationInfo) {
    if (this.project && this.project.addons.length) {
      this.project.addons.forEach(addon => {
        if (typeof addon.instrumentation === 'function') {
          addon.instrumentation(name, instrumentationInfo);
        }
      });
    }
  }

  _writeInstrumentation(name, instrumentationInfo) {
    if (!vizEnabled()) { return; }

    let filename = `instrumentation.${name}`;
    if (name === 'build') {
      filename += `.${this.instrumentations.build.count}`;
    }
    filename = `${filename}.json`;
    fs.writeJsonSync(filename, {
      summary: instrumentationInfo.summary,
      // we want to change this to tree, to be consistent with the hook, but first
      // we must update broccoli-viz
      // see see https://github.com/ember-cli/broccoli-viz/issues/35
      nodes: instrumentationInfo.tree.toJSON().nodes,
    });
  }

  start(name) {
    if (!instrumentationEnabled()) { return; }

    let instr = this._instrumentationFor(name);
    this._heimdall = this._heimdall || require('heimdalljs');

    if (instr.node) {
      // don't leak nodes during build.  We have already reported on this in the
      // previous stopAndReport so no data is lost
      instr.node.remove();
    }

    let token = this._heimdall.start({ name, emberCLI: true });
    instr.token = token;
    instr.node = this._heimdall.current;
  }

  stopAndReport(name) {
    if (!instrumentationEnabled()) { return; }

    let instr = this._instrumentationFor(name);
    if (!instr.token) {
      throw new Error(`Cannot stop instrumentation "${name}".  It has not started.`);
    }
    try {
      instr.token.stop();
    } catch (e) {
      this.ui.writeLine(chalk.red(`Error reporting instrumentation '${name}'.`));
      logger.error(e.stack);
      return;
    }

    let instrSummaryName = `_${name}Summary`;
    if (!this[instrSummaryName]) {
      throw new Error(`No summary found for "${name}"`);
    }

    let tree = this._instrumentationTreeFor(name);
    let args = Array.prototype.slice.call(arguments, 1);
    args.unshift(tree);

    let instrInfo = {
      summary: this[instrSummaryName].apply(this, args),
      tree,
    };

    this._invokeAddonHook(name, instrInfo);
    this._writeInstrumentation(name, instrInfo);

    if (name === 'build') {
      instr.count++;
    }
  }
}

function totalTime(tree) {
  let totalTime = 0;
  let nodeItr;
  let node;
  let statName;
  let statValue;
  let statsItr;
  let nextNode;
  let nextStat;

  for (nodeItr = tree.dfsIterator(); ;) {
    nextNode = nodeItr.next();
    if (nextNode.done) { break; }

    node = nextNode.value;

    for (statsItr = node.statsIterator(); ;) {
      nextStat = statsItr.next();
      if (nextStat.done) { break; }

      statName = nextStat.value[0];
      statValue = nextStat.value[1];

      if (statName === 'time.self') {
        totalTime += statValue;
      }
    }
  }

  return totalTime;
}

// exported for testing
Instrumentation._enableFSMonitorIfInstrumentationEnabled = _enableFSMonitorIfInstrumentationEnabled;
Instrumentation._vizEnabled = vizEnabled();
Instrumentation._instrumentationEnabled = instrumentationEnabled();

module.exports = Instrumentation;
