'use strict';

const Promise = require('rsvp').Promise;
const chalk = require('chalk');
const spawn = require('child_process').spawn;
const defaults = require('ember-cli-lodash-subset').defaults;
const killCliProcess = require('./kill-cli-process');
const logOnFailure = require('./log-on-failure');
let debug = require('heimdalljs-logger')('run-command');

module.exports = function run(/* command, args, options */) {
  let command = arguments[0];
  let args = Array.prototype.slice.call(arguments, 1);
  let options = {};

  if (typeof args[args.length - 1] === 'object') {
    options = args.pop();
  }

  options = defaults(options, {
    // If true, pass through stdout/stderr.
    // If false, only pass through stdout/stderr if the current test fails.
    verbose: false,

    onOutput(string) {
      options.log(string);
    },

    onError(string) {
      options.log(chalk.red(string));
    },

    log(string) {
      debug.debug(string);
      if (options.verbose) {
        console.log(string);
      } else {
        logOnFailure(string);
      }
    },
  });

  return new Promise(function(resolve, reject) {
    options.log(`      Running: ${command} ${args.join(' ')} in: ${process.cwd()}`);

    let opts = {};
    if (process.platform === 'win32') {
      args = [`"${command}"`].concat(args);
      command = 'node';
      opts.windowsVerbatimArguments = true;
      opts.stdio = [null, null, null, 'ipc'];
    }
    if (options.env) {
      opts.env = defaults(options.env, process.env);
    }

    debug.info("command: %s, args: %o", command, args);
    let child = spawn(command, args, opts);
    let result = {
      output: [],
      errors: [],
      code: null,
    };

    if (options.onChildSpawned) {
      let onChildSpawnedPromise = new Promise(function(childSpawnedResolve, childSpawnedReject) {
        try {
          options.onChildSpawned(child).then(childSpawnedResolve, childSpawnedReject);
        } catch (err) {
          childSpawnedReject(err);
        }
      });
      onChildSpawnedPromise
        .then(function() {
          if (options.killAfterChildSpawnedPromiseResolution) {
            killCliProcess(child);
          }
        }, function(err) {
          result.testingError = err;
          if (options.killAfterChildSpawnedPromiseResolution) {
            killCliProcess(child);
          }
        });
    }

    child.stdout.on('data', function(data) {
      let string = data.toString();

      options.onOutput(string, child);

      result.output.push(string);
    });

    child.stderr.on('data', function(data) {
      let string = data.toString();

      options.onError(string, child);

      result.errors.push(string);
    });

    child.on('close', function(code, signal) {
      result.code = code;
      result.signal = signal;

      if (code === 0) {
        resolve(result);
      } else {
        reject(result);
      }
    });
  });
};
