/* jshint node:true */
'use strict';
/**
 * @fileOverview
 * Global proxy settings.
 */
var globalTunnel = exports;
exports.constructor = function() {};

var http = require('http');
var https = require('https');
var urlParse = require('url').parse;
var urlStringify = require('url').format;

var pick = require('lodash/pick');
var assign = require('lodash/assign');
var clone = require('lodash/clone');
var tunnel = require('tunnel');
var npmConfig = require('npm-conf');
var encodeUrl = require('encodeurl');

var agents = require('./lib/agents');
exports.agents = agents;

var ENV_VAR_PROXY_SEARCH_ORDER = [
  'https_proxy',
  'HTTPS_PROXY',
  'http_proxy',
  'HTTP_PROXY'
];
var NPM_CONFIG_PROXY_SEARCH_ORDER = ['https-proxy', 'http-proxy', 'proxy'];

// Save the original settings for restoration later.
var ORIGINALS = {
  http: pick(http, 'globalAgent', ['request', 'get']),
  https: pick(https, 'globalAgent', ['request', 'get']),
  env: pick(process.env, ENV_VAR_PROXY_SEARCH_ORDER)
};

var loggingEnabled =
  process &&
  process.env &&
  process.env.DEBUG &&
  process.env.DEBUG.toLowerCase().indexOf('global-tunnel') !== -1 &&
  console &&
  typeof console.log === 'function';

function log(message) {
  if (loggingEnabled) {
    console.log('DEBUG global-tunnel: ' + message);
  }
}

function resetGlobals() {
  assign(http, ORIGINALS.http);
  assign(https, ORIGINALS.https);
  var val;
  for (var key in ORIGINALS.env) {
    if (Object.prototype.hasOwnProperty.call(ORIGINALS.env, key)) {
      val = ORIGINALS.env[key];
      if (val !== null && val !== undefined) {
        process.env[key] = val;
      }
    }
  }
}

/**
 * Parses the de facto `http_proxy` environment.
 */
function tryParse(url) {
  if (!url) {
    return null;
  }

  var parsed = urlParse(url);

  return {
    protocol: parsed.protocol,
    host: parsed.hostname,
    port: parseInt(parsed.port, 10),
    proxyAuth: parsed.auth
  };
}

// Stringifies the normalized parsed config
function stringifyProxy(conf) {
  return encodeUrl(
    urlStringify({
      protocol: conf.protocol,
      hostname: conf.host,
      port: conf.port,
      auth: conf.proxyAuth
    })
  );
}

globalTunnel.isProxying = false;
globalTunnel.proxyUrl = null;
globalTunnel.proxyConfig = null;

function findEnvVarProxy() {
  var i;
  var key;
  var val;
  var result;
  for (i = 0; i < ENV_VAR_PROXY_SEARCH_ORDER.length; i++) {
    key = ENV_VAR_PROXY_SEARCH_ORDER[i];
    val = process.env[key];
    if (val !== null && val !== undefined) {
      // Get the first non-empty
      result = result || val;
      // Delete all
      // NB: we do it here to prevent double proxy handling (and for example path change)
      // by us and the `request` module or other sub-dependencies
      delete process.env[key];
      log('Found proxy in environment variable ' + ENV_VAR_PROXY_SEARCH_ORDER[i]);
    }
  }

  if (!result) {
    // __GLOBAL_TUNNEL_DEPENDENCY_NPMCONF__ is a hook to override the npm-conf module
    var config =
      (global.__GLOBAL_TUNNEL_DEPENDENCY_NPMCONF__ &&
        global.__GLOBAL_TUNNEL_DEPENDENCY_NPMCONF__()) ||
      npmConfig();

    for (i = 0; i < NPM_CONFIG_PROXY_SEARCH_ORDER.length && !val; i++) {
      val = config.get(NPM_CONFIG_PROXY_SEARCH_ORDER[i]);
    }

    if (val) {
      log('Found proxy in npm config ' + NPM_CONFIG_PROXY_SEARCH_ORDER[i]);
      result = val;
    }
  }

  return result;
}

/**
 * Overrides the node http/https `globalAgent`s to use the configured proxy.
 *
 * If the config is empty, the `http_proxy` environment variable is checked.
 * If that's not present, the NPM `http-proxy` configuration is checked.
 * If neither are present no proxying will be enabled.
 *
 * @param {object} conf - Options
 * @param {string} conf.host - Hostname or IP of the HTTP proxy to use
 * @param {int} conf.port - TCP port of the proxy
 * @param {string} [conf.protocol='http'] - The protocol of the proxy, 'http' or 'https'
 * @param {string} [conf.proxyAuth] - Credentials for the proxy in the form userId:password
 * @param {string} [conf.connect='https'] - Which protocols will use the CONNECT method 'neither', 'https' or 'both'
 * @param {int} [conf.sockets=5] Maximum number of TCP sockets to use in each pool. There are two different pools for HTTP and HTTPS
 * @param {object} [conf.httpsOptions] - HTTPS options
 */
globalTunnel.initialize = function(conf) {
  // Don't do anything if already proxying.
  // To change the settings `.end()` should be called first.
  if (globalTunnel.isProxying) {
    log('Already proxying');
    return;
  }

  try {
    // This has an effect of also removing the proxy config
    // from the global env to prevent other modules (like request) doing
    // double handling
    var envVarProxy = findEnvVarProxy();

    if (conf && typeof conf === 'string') {
      // Passed string - parse it as a URL
      conf = tryParse(conf);
    } else if (conf) {
      // Passed object - take it but clone for future mutations
      conf = clone(conf);
    } else if (envVarProxy) {
      // Nothing passed - parse from the env
      conf = tryParse(envVarProxy);
    } else {
      log('No configuration found, not proxying');
      // No config - do nothing
      return;
    }

    log('Proxy configuration to be used is ' + JSON.stringify(conf, null, 2));

    if (!conf.host) {
      throw new Error('upstream proxy host is required');
    }
    if (!conf.port) {
      throw new Error('upstream proxy port is required');
    }

    if (conf.protocol === undefined) {
      conf.protocol = 'http:'; // Default to proxy speaking http
    }
    if (!/:$/.test(conf.protocol)) {
      conf.protocol += ':';
    }

    if (!conf.connect) {
      conf.connect = 'https'; // Just HTTPS by default
    }

    if (['both', 'neither', 'https'].indexOf(conf.connect) < 0) {
      throw new Error('valid connect options are "neither", "https", or "both"');
    }

    var connectHttp = conf.connect === 'both';
    var connectHttps = conf.connect !== 'neither';

    if (conf.httpsOptions) {
      conf.innerHttpsOpts = conf.httpsOptions;
      conf.outerHttpsOpts = conf.innerHttpsOpts;
    }

    http.globalAgent = globalTunnel._makeAgent(conf, 'http', connectHttp);
    https.globalAgent = globalTunnel._makeAgent(conf, 'https', connectHttps);

    http.request = globalTunnel._makeHttp('request', http, 'http');
    https.request = globalTunnel._makeHttp('request', https, 'https');
    http.get = globalTunnel._makeHttp('get', http, 'http');
    https.get = globalTunnel._makeHttp('get', https, 'https');

    globalTunnel.isProxying = true;
    globalTunnel.proxyUrl = stringifyProxy(conf);
    globalTunnel.proxyConfig = clone(conf);
  } catch (e) {
    resetGlobals();
    throw e;
  }
};

var _makeAgent = function(conf, innerProtocol, useCONNECT) {
  log('Creating proxying agent');
  var outerProtocol = conf.protocol;
  innerProtocol += ':';

  var opts = {
    proxy: pick(conf, 'host', 'port', 'protocol', 'localAddress', 'proxyAuth'),
    maxSockets: conf.sockets
  };
  opts.proxy.innerProtocol = innerProtocol;

  if (useCONNECT) {
    if (conf.proxyHttpsOptions) {
      assign(opts.proxy, conf.proxyHttpsOptions);
    }
    if (conf.originHttpsOptions) {
      assign(opts, conf.originHttpsOptions);
    }

    if (outerProtocol === 'https:') {
      if (innerProtocol === 'https:') {
        return tunnel.httpsOverHttps(opts);
      }
      return tunnel.httpOverHttps(opts);
    }
    if (innerProtocol === 'https:') {
      return tunnel.httpsOverHttp(opts);
    }
    return tunnel.httpOverHttp(opts);
  }
  if (conf.originHttpsOptions) {
    throw new Error('originHttpsOptions must be combined with a tunnel:true option');
  }
  if (conf.proxyHttpsOptions) {
    // NB: not opts.
    assign(opts, conf.proxyHttpsOptions);
  }

  if (outerProtocol === 'https:') {
    return new agents.OuterHttpsAgent(opts);
  }
  return new agents.OuterHttpAgent(opts);
};

/**
 * Construct an agent based on:
 * - is the connection to the proxy secure?
 * - is the connection to the origin secure?
 * - the address of the proxy
 */
globalTunnel._makeAgent = function(conf, innerProtocol, useCONNECT) {
  var agent = _makeAgent(conf, innerProtocol, useCONNECT);
  // Set the protocol to match that of the target request type
  agent.protocol = innerProtocol + ':';
  return agent;
};

/**
 * Override for http/https, makes sure to default the agent
 * to the global agent. Due to how node implements it in lib/http.js, the
 * globalAgent we define won't get used (node uses a module-scoped variable,
 * not the exports field).
 * @param {string} method 'request' or 'get', http/https methods
 * @param {string|object} options http/https request url or options
 * @param {function} [cb]
 * @private
 */
globalTunnel._makeHttp = function(method, httpOrHttps, protocol) {
  return function(options, callback) {
    if (typeof options === 'string') {
      options = urlParse(options);
    } else {
      options = clone(options);
    }

    // Respect the default agent provided by node's lib/https.js
    if (
      (options.agent === null || options.agent === undefined) &&
      typeof options.createConnection !== 'function' &&
      (options.host || options.hostname)
    ) {
      options.agent = options._defaultAgent || httpOrHttps.globalAgent;
    }

    // Set the default port ourselves to prevent Node doing it based on the proxy agent protocol
    if (options.protocol === 'https:' || (!options.protocol && protocol === 'https')) {
      options.port = options.port || 443;
    }
    if (options.protocol === 'http:' || (!options.protocol && protocol === 'http')) {
      options.port = options.port || 80;
    }

    log(
      'Requesting to ' +
        (options.protocol || protocol) +
        '//' +
        (options.host || options.hostname) +
        ':' +
        options.port
    );

    return ORIGINALS[protocol][method].call(httpOrHttps, options, callback);
  };
};

/**
 * Restores global http/https agents.
 */
globalTunnel.end = function() {
  resetGlobals();
  globalTunnel.isProxying = false;
  globalTunnel.proxyUrl = null;
  globalTunnel.proxyConfig = null;
};
