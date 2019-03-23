/******/ (function(modules) { // webpackBootstrap
/******/ 	// The module cache
/******/ 	var installedModules = {};

/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {

/******/ 		// Check if module is in cache
/******/ 		if(installedModules[moduleId])
/******/ 			return installedModules[moduleId].exports;

/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = installedModules[moduleId] = {
/******/ 			exports: {},
/******/ 			id: moduleId,
/******/ 			loaded: false
/******/ 		};

/******/ 		// Execute the module function
/******/ 		modules[moduleId].call(module.exports, module, module.exports, __webpack_require__);

/******/ 		// Flag the module as loaded
/******/ 		module.loaded = true;

/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}


/******/ 	// expose the modules object (__webpack_modules__)
/******/ 	__webpack_require__.m = modules;

/******/ 	// expose the module cache
/******/ 	__webpack_require__.c = installedModules;

/******/ 	// __webpack_public_path__
/******/ 	__webpack_require__.p = "";

/******/ 	// Load entry module and return exports
/******/ 	return __webpack_require__(0);
/******/ })
/************************************************************************/
/******/ ([
/* 0 */
/***/ (function(module, exports, __webpack_require__) {

	/**
	 * worker must be started as a child process or a web worker.
	 * It listens for RPC messages from the parent process.
	 */

	// create a worker API for sending and receiving messages which works both on
	// node.js and in the browser
	var worker = {};
	if (typeof self !== 'undefined' && typeof postMessage === 'function' && typeof addEventListener === 'function') {
	  // worker in the browser
	  worker.on = function (event, callback) {
	    addEventListener(event, function (message) {
	      callback(message.data);
	    })
	  };
	  worker.send = function (message) {
	    postMessage(message);
	  };
	}
	else if (typeof process !== 'undefined') {
	  // node.js

	  var WorkerThreads;
	  try {
	    WorkerThreads = __webpack_require__(!(function webpackMissingModule() { var e = new Error("Cannot find module \"worker_threads\""); e.code = 'MODULE_NOT_FOUND'; throw e; }()));
	  } catch(error) {
	    if (typeof error === 'object' && error !== null && error.code == 'MODULE_NOT_FOUND') {
	      // no worker_threads, fallback to sub-process based workers
	    } else {
	      throw error;
	    }
	  }

	  if (WorkerThreads &&
	    /* if there is a parentPort, we are in a WorkerThread */
	    WorkerThreads.parentPort !== null) {
	    var parentPort  = WorkerThreads.parentPort;
	    worker.send = parentPort.postMessage.bind(parentPort);
	    worker.on = parentPort.on.bind(parentPort);
	  } else {
	    worker.on = process.on.bind(process);
	    worker.send = process.send.bind(process);
	  }
	}
	else {
	  throw new Error('Script must be executed as a worker');
	}

	function convertError(error) {
	  return Object.getOwnPropertyNames(error).reduce(function(product, name) {
	    return Object.defineProperty(product, name, {
		value: error[name],
		enumerable: true
	    });
	  }, {});
	}

	/**
	 * Test whether a value is a Promise via duck typing.
	 * @param {*} value
	 * @returns {boolean} Returns true when given value is an object
	 *                    having functions `then` and `catch`.
	 */
	function isPromise(value) {
	  return value && (typeof value.then === 'function') && (typeof value.catch === 'function');
	}

	// functions available externally
	worker.methods = {};

	/**
	 * Execute a function with provided arguments
	 * @param {String} fn     Stringified function
	 * @param {Array} [args]  Function arguments
	 * @returns {*}
	 */
	worker.methods.run = function run(fn, args) {
	  var f = eval('(' + fn + ')');
	  return f.apply(f, args);
	};

	/**
	 * Get a list with methods available on this worker
	 * @return {String[]} methods
	 */
	worker.methods.methods = function methods() {
	  return Object.keys(worker.methods);
	};

	worker.on('message', function (request) {
	  try {
	    var method = worker.methods[request.method];

	    if (method) {
	      // execute the function
	      var result = method.apply(method, request.params);

	      if (isPromise(result)) {
	        // promise returned, resolve this and then return
	        result
	            .then(function (result) {
	              worker.send({
	                id: request.id,
	                result: result,
	                error: null
	              });
	            })
	            .catch(function (err) {
	              worker.send({
	                id: request.id,
	                result: null,
	                error: convertError(err)
	              });
	            });
	      }
	      else {
	        // immediate result
	        worker.send({
	          id: request.id,
	          result: result,
	          error: null
	        });
	      }
	    }
	    else {
	      throw new Error('Unknown method "' + request.method + '"');
	    }
	  }
	  catch (err) {
	    worker.send({
	      id: request.id,
	      result: null,
	      error: convertError(err)
	    });
	  }
	});

	/**
	 * Register methods to the worker
	 * @param {Object} methods
	 */
	worker.register = function (methods) {

	  if (methods) {
	    for (var name in methods) {
	      if (methods.hasOwnProperty(name)) {
	        worker.methods[name] = methods[name];
	      }
	    }
	  }

	  worker.send('ready');

	};

	if (true) {
	  exports.add = worker.register;
	}


/***/ })
/******/ ]);