'use strict';

const transpiler = require('@babel/core');
const path = require('path');
const workerpool = require('workerpool');
const Promise = require('rsvp').Promise;
const debugGenerator = require('heimdalljs-logger');

const JOBS = Number(process.env.JOBS) || require('os').cpus().length;

const loggerName = 'broccoli-persistent-filter:babel:parallel-api';
const _logger = debugGenerator(loggerName);

const babelCoreVersion = getBabelVersion();

// return the version of Babel that will be used by this plugin
function getBabelVersion() {
  return require('@babel/core/package.json').version;
}

function getWorkerPool() {
  let pool;
  let globalPoolID = 'v2/broccoli-babel-transpiler/workerpool/babel-core-' + babelCoreVersion;
  let existingPool = process[globalPoolID];

  if (existingPool) {
    pool = existingPool;
  } else {
    pool = workerpool.pool(path.join(__dirname, 'worker.js'), { nodeWorker: 'auto', maxWorkers: JOBS });
    process[globalPoolID] = pool;
  }
  return pool;
}

function implementsParallelAPI(object) {
  const type = typeof object;
  const hasProperties = type === 'function' || (type === 'object' && object !== null) || Array.isArray(object);

  return hasProperties &&
    object._parallelBabel !== null &&
    typeof object._parallelBabel === 'object' &&
    typeof object._parallelBabel.requireFile === 'string';
}

function isSerializable(value) {
  if (value === null) {
    return true;
  } else if (implementsParallelAPI(value)) {
    return true;
  } else if (Array.isArray(value)) {
    return value.every(isSerializable);
  } else {
    switch (typeof value) {
      case 'string':
      case 'number':
      case 'boolean':  return true;
      case 'object':   return Object.keys(value).every(key => isSerializable(value[key]));
      default:         return false;
    }
  }
}

function pluginsAreParallelizable(plugins) {
  const errors = [];
  let isParallelizable = true;

  if (Array.isArray(plugins)) {
    for (let i = 0; i < plugins.length; i++) {
      let plugin = plugins[i];
      if (isSerializable(plugin) === false) {
        isParallelizable = false;
        errors.push(humanizePlugin(plugin));
      }
    }
  }

  return {
    isParallelizable,
    errors
  };
}

function callbacksAreParallelizable(options) {
  const callbacks = Object.keys(options).filter(key => typeof options[key] === 'function');
  let isParallelizable = true;
  const errors = [];

  for (let i = 0; i < callbacks.length; i++) {
    let callback = options[callbacks[i]];
    if (implementsParallelAPI(callback) === false) {
      isParallelizable = false;
      errors.push(humanizePlugin(callback))
    }
  }

  return { isParallelizable, errors };
}

function transformIsParallelizable(options) {
  const plugins = pluginsAreParallelizable(options.plugins);
  const callbacks = callbacksAreParallelizable(options);

  return {
    isParallelizable: isSerializable(options),
    errors: [].concat(plugins.errors, callbacks.errors)
  };
}

function humanizePlugin(plugin) {
  let name, baseDir;

  if (Array.isArray(plugin) && typeof plugin[0] === 'function') {
    plugin = plugin[0];
  }

  if (typeof plugin === 'function' || typeof plugin === 'object' && plugin !== null) {
    if (typeof plugin.name === 'string') {
      name = plugin.name;
    }
    if (typeof plugin.baseDir === 'function') {
      baseDir = plugin.baseDir();
    }
  }
  const output = `name: ${name || 'unknown'}, location: ${baseDir || 'unknown'}`;

  if (!name && typeof plugin === 'function') {
    return output + `,\n↓ function source ↓ \n${plugin.toString().substr(0, 200)}\n \n`;
  } else {
    return output;
  }
}

function buildFromParallelApiInfo(parallelApiInfo, cacheKey) {
  let requiredStuff = require(parallelApiInfo.requireFile);

  if (parallelApiInfo.useMethod) {
    if (requiredStuff[parallelApiInfo.useMethod] === undefined) {
      throw new Error("method '" + parallelApiInfo.useMethod + "' does not exist in file " + parallelApiInfo.requireFile);
    }
    return requiredStuff[parallelApiInfo.useMethod];
  }

  if (parallelApiInfo.buildUsing) {
    if (typeof requiredStuff[parallelApiInfo.buildUsing] !== 'function') {
      throw new Error("'" + parallelApiInfo.buildUsing + "' is not a function in file " + parallelApiInfo.requireFile);
    }
    return requiredStuff[parallelApiInfo.buildUsing](parallelApiInfo.params, cacheKey);
  }

  return requiredStuff;
}

function deserialize(data, cacheKey) {
  if (data.cacheKey) {
    cacheKey = data.cacheKey;
  }
  if (data.babel) {
    data = data.babel;
  }
  if (Array.isArray(data)) {
    const result = [];
    for (let i =0; i < data.length; i++) {
      let member = data[i];
      if (implementsParallelAPI(member)) {
        result[i] = buildFromParallelApiInfo(member._parallelBabel, cacheKey);
      } else {
        result[i] = deserialize(member, cacheKey);
      }
    }

    return result;
  } else if (typeof data === 'object' && data !== null) {
    const result = {};

    Object.keys(data).forEach(key => {
      let value = data[key];

      if (value === undefined || value == null) {

      } else if (implementsParallelAPI(value)) {
        value = buildFromParallelApiInfo(value._parallelBabel, cacheKey);
      } else {
        value = deserialize(value, cacheKey);
      }
      result[key] = value;
    });

    return result;
  }  else {
    return data;
  }
}

const visited = new WeakSet();
// replace callback functions with objects so they can be transferred to the worker processes
function serialize(options) {
  let optionsType = typeof options;

  if ((optionsType === 'object' && options !== null) ||
    optionsType === 'function' ||
    Array.isArray(options)) {

    if (visited.has(options)) {
      // a cycle, so simply reuse
      return options;
    } else {
      if (implementsParallelAPI(options)) {
        options = {
          _parallelBabel: options._parallelBabel
        };
      }
      visited.add(options);
    }

  } else {
    return options;
  }

  const serialized = Array.isArray(options) ? [] : {};

  Object.keys(options).forEach(key => {
    let value = options[key];

    if (value === options) {
      throw new TypeError('Babel Plugins contain a cycle');
    }

    if (isSerializable(value)) {
      value = serialize(value);
    } else {
      // leave as is
    }
    serialized[key] = value;
  });

  return serialized;
}

function transformString(string, options, buildOptions) {
  const isParallelizable = transformIsParallelizable(options.babel).isParallelizable;
  if (JOBS > 1 && isParallelizable) {
    let pool = getWorkerPool();
    _logger.info('transformString is parallelizable');
    let serializedObj = { babel : serialize(options.babel), 'cacheKey': options.cacheKey };
    return pool.exec('transform', [string, serializedObj]);
  } else {
    if (JOBS <= 1) {
      _logger.info('JOBS <= 1, skipping worker, using main thread');
    } else {
      _logger.info('transformString is NOT parallelizable');
    }

    return new Promise(resolve => {
      resolve(transpiler.transform(string, deserialize(options)));
    });
  }
}

module.exports = {
  jobs: JOBS,
  getBabelVersion,
  implementsParallelAPI,
  isSerializable,
  pluginsAreParallelizable,
  callbacksAreParallelizable,
  transformIsParallelizable,
  deserialize,
  serialize,
  buildFromParallelApiInfo,
  transformString,
  humanizePlugin,
  getWorkerPool
};
