'use strict';

var initial = require('lodash.initial');
var last = require('lodash.last');
var asyncSettle = require('async-settle');
var nowAndLater = require('now-and-later');

var helpers = require('./helpers');

function buildSettleSeries() {
  var args = helpers.verifyArguments(arguments);

  var extensions = helpers.getExtensions(last(args));

  if (extensions) {
    args = initial(args);
  }

  function settleSeries(done) {
    var onSettled = helpers.onSettled(done);
    nowAndLater.mapSeries(args, asyncSettle, extensions, onSettled);
  }

  return settleSeries;
}

module.exports = buildSettleSeries;
