'use strict';

function _defineProperty(obj, key, value) { if (key in obj) { Object.defineProperty(obj, key, { value: value, enumerable: true, configurable: true, writable: true }); } else { obj[key] = value; } return obj; }

function _toConsumableArray(arr) { if (Array.isArray(arr)) { for (var i = 0, arr2 = Array(arr.length); i < arr.length; i++) { arr2[i] = arr[i]; } return arr2; } else { return Array.from(arr); } }

var mkdirp = require('mkdirp');
var path = require('path');
var fs = require('fs');
var _ = require('lodash');

var error = require('../utils/error');

function orderAssets(assets, options) {
  return options.manifestFirst ? Object.assign.apply(Object, [{}].concat(_toConsumableArray(Object.keys(assets).map(function (i) {
    return _defineProperty({}, i, assets[i]);
  }).sort(function (a, b) {
    if (a.manifest) {
      return -1;
    }

    if (b.manifest) {
      return 1;
    }

    return 0;
  })))) : assets;
}

module.exports = function (options) {
  var update = options.update;
  var firstRun = true;

  options.processOutput = options.processOutput || function (assets) {
    return JSON.stringify(assets, null, options.prettyPrint ? 2 : null);
  };

  return function writeOutput(fileStream, newAssets, next) {
    // if options.update is false and we're on the first pass of a (possibly) multicompiler
    var overwrite = !update && firstRun;
    var localFs = options.keepInMemory ? fileStream : fs;

    function mkdirpCallback(err) {
      if (err) {
        return next(error('Could not create output folder ' + options.path, err));
      }

      var outputPath = options.keepInMemory ? localFs.join(options.path, options.filename) : path.join(options.path, options.filename);

      localFs.readFile(outputPath, 'utf8', function (err, data) {
        // if file does not exist, just write data to it
        if (err && err.code !== 'ENOENT') {
          return next(error('Could not read output file ' + outputPath, err));
        }
        // if options.update is false and we're on first run,
        // start with empty data
        data = overwrite ? '{}' : data || '{}';

        var oldAssets;
        try {
          oldAssets = JSON.parse(data);
        } catch (err) {
          oldAssets = {};
        }

        var assets = orderAssets(_.merge({}, oldAssets, newAssets), options);
        var output = options.processOutput(assets);
        if (output !== data) {
          localFs.writeFile(outputPath, output, function (err) {
            if (err) {
              return next(error('Unable to write to ' + outputPath, err));
            }
            firstRun = false;
            next();
          });
        } else {
          next();
        }
      });
    }

    options.keepInMemory ? localFs.mkdirp(options.path, mkdirpCallback) : mkdirp(options.path, mkdirpCallback);
  };
};