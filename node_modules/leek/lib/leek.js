'use strict';

var RSVP        = require('rsvp'),
    Promise     = RSVP.Promise,
    provider    = require('./provider'),
    getVersions = require('./get-versions'),
    md5         = require('./md5'),
    extend      = require('lodash.assign'),
    debug       = require('debug')('leek'),
    https;

function Leek(options) {
  if (!options) {
    throw new Error('You need to specify the options.');
  }

  if (!options.trackingCode) {
    throw new Error('You need to specify the tracking code.');
  }

  if (!options.globalName) {
    throw new Error('You need to specify the global name.');
  }

  this.adapterUrls = options.adapterUrls || null;
  this.adapterServers = options.adapterServers || null;
  this.trackingCode = options.trackingCode;
  this.name         = options.name;

  if (this.name === undefined) {
    throw new Error('You need to specify name, this should be a unique identifier for the current client');
  }

  this.globalName   = options.globalName;
  this.clientId     = this.globalName + md5(this.name);
  this.version      = options.version || '';
  this.silent       = options.silent || process.env.DISABLE_LEEK;

  debug('initialize %o', {
    trackingCode: this.trackingCode,
    name: this.name,
    globalName: this.globalName,
    clientId: this.clientId,
    version: this.version,
    silent: this.silent
  });
}

Leek.prototype.setName = function(value) {
  this.name     = value;
  this.clientId = this.globalName + md5(this.name);
};

Leek.prototype._enqueue = function(eventType, meta) {
  debug('enqueue eventType:%s silent:%o payload:%o', eventType, this.silent, meta);
  if (this.silent) {
    return Promise.resolve();
  }

  if (https === undefined) {
    https = RSVP.denodeify(require('https'));
  }

  var url = provider.call(
    this,
    eventType,
    extend(meta, getVersions()),
    Date.now(),
    (this.adapterServers && this.adapterServers[eventType])
  );

  if (this.adapterUrls) {
    url = this.adapterUrls[eventType];
  }

  debug('http %o', url);

  // don't attach default `on('error')` handler and swallow errors
  return https.get(url).on('error', function(/*error*/) {});
};

Leek.prototype._getConfigObject = function() {
  return {
    name:         this.name,
    version:      this.version,
    trackingCode: this.trackingCode
  };
};

Leek.prototype.track = function(meta) {
  return this._enqueue('appview', {
    name:    meta.name,
    message: meta.message
  });
};

Leek.prototype.trackError = function(meta) {
  return this._enqueue('exception', {
    description: meta.description,
    fatal:       meta.isFatal
  });
};

Leek.prototype.trackTiming = function(meta) {
  return this._enqueue('timing', {
    category: meta.category,
    variable: meta.variable,
    value:    meta.value,
    label:    meta.label
  });
};

Leek.prototype.trackEvent = function(meta) {
  return this._enqueue('event', {
    name:     meta.name,
    category: meta.category,
    label:    meta.label,
    value:    meta.value
  });
};

module.exports = Leek;
