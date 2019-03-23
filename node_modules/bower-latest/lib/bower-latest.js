/*
 * bower-latest
 * https://github.com/kwakayama/bower-latest
 *
 * Copyright (c) 2014 Kentaro Wakayama
 * Licensed under the MIT license.
 */

'use strict';

var util = require('util'),
  https = require('https'),
  url = require('url'),
  _ = require('lodash');

var API_URL = 'https://bower-component-list.herokuapp.com';
var GITHUB_URL = 'https://api.github.com/repos/%s/%s/tags';

function findBowerPackage (packageName, cb) {

  var options = {
    hostname: url.parse(API_URL).host,
    path: url.parse(API_URL).pathname
  };

  https.get(options, function(res) {
    var chunks = '';
    res.on('data', function(chunk) {
      chunks += chunk;
    });

    res.on('end', function() {
      if (chunks) {
        var name = packageName.toLowerCase();
        var list = JSON.parse(chunks);
        var exact = _.findIndex(list, function(item) {
          return item.name.toLowerCase() === name;
        });
        if (exact !== -1) {
          return cb(list[exact]);
        } else {
          return cb();
        }
      }
      return cb();
    });

  }).on('error', function(e) {
    console.error(e);
  });
}

module.exports = function bowerLatest (packageName, options, cb) {

  if (typeof options === 'function') {
    cb = options;
  }

  // Default request options
  var defaults = {
    headers: {
      'User-Agent': 'npm-search-v2'
    }
  };

  // Merge options with defaults
  options = _.merge(defaults, options);

  findBowerPackage(packageName, function (bowerPackage) {
    if (!bowerPackage) {
      return cb();
    }
    var gitUrl = bowerPackage.website;
    var data = gitUrl.split('/').slice(3,5);
    var user = data[0];
    var repo = data[1].replace('.git','');
    var repoUrl = util.format(GITHUB_URL, user, repo);

    var options = {
      hostname: url.parse(repoUrl).host,
      path: url.parse(repoUrl).pathname,
      headers: { 'User-Agent': 'npm-search-v2' }
    };

    https.get(options, function(res) {
      var chunks = '';
      res.on('data', function(chunk) {
        chunks += chunk;
      });

      res.on('end', function() {
        var tags = JSON.parse(chunks);
        var compontent;
        if (tags.length > 0) {
          compontent = {
            name: packageName,
            version: tags[0].name
          };
        }
        cb(compontent);

      });

    }).on('error', function(e) {
      console.error(e);
    });
  });

};
