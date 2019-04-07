/* jshint node:true */
'use strict';

var util = require('util');
var http = require('http');
var HttpAgent = http.Agent;
var https = require('https');
var HttpsAgent = https.Agent;

var pick = require('lodash/pick');

/**
 * Proxy some traffic over HTTP.
 */
function OuterHttpAgent(opts) {
  HttpAgent.call(this, opts);
  mixinProxying(this, opts.proxy);
}
util.inherits(OuterHttpAgent, HttpAgent);
exports.OuterHttpAgent = OuterHttpAgent;

/**
 * Proxy some traffic over HTTPS.
 */
function OuterHttpsAgent(opts) {
  HttpsAgent.call(this, opts);
  mixinProxying(this, opts.proxy);
}
util.inherits(OuterHttpsAgent, HttpsAgent);
exports.OuterHttpsAgent = OuterHttpsAgent;

/**
 * Override createConnection and addRequest methods on the supplied agent.
 * http.Agent and https.Agent will set up createConnection in the constructor.
 */
function mixinProxying(agent, proxyOpts) {
  agent.proxy = proxyOpts;

  var orig = pick(agent, 'createConnection', 'addRequest');

  // Make the tcp or tls connection go to the proxy, ignoring the
  // destination host:port arguments.
  agent.createConnection = function(port, host, options) {
    return orig.createConnection.call(this, this.proxy.port, this.proxy.host, options);
  };

  agent.addRequest = function(req, options) {
    req.path =
      this.proxy.innerProtocol + '//' + options.host + ':' + options.port + req.path;
    return orig.addRequest.call(this, req, options);
  };
}
