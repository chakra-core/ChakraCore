var environment = require('./lib/environment');

/**
 * Create a new worker pool
 * @param {Object} [options]
 * @returns {Pool} pool
 */
exports.pool = function pool(script, options) {
  var Pool = require('./lib/Pool');

  return new Pool(script, options);
};

/**
 * Create a worker and optionally register a set of methods to the worker.
 * @param {Object} [methods]
 */
exports.worker = function worker(methods) {
  var worker = require('./lib/worker');
  worker.add(methods);
};

/**
 * Create a promise.
 * @type {Promise} promise
 */
exports.Promise = require('./lib/Promise');

exports.platform = environment.platform;
exports.isMainThread = environment.isMainThread;
exports.cpus = environment.cpus;