'use strict';

const path = require('path');
const url = require('url');

const setResponseHeaders = require('./utils/response-header').setResponseHeaders;
const serveAsset = require('./utils/serve-asset');
const errorHandler = require('./utils/error-handler');

// You must call watcher.watch() before you call `getMiddleware`
//
// This middleware is for development use only. It hasn't been reviewed
// carefully enough to run on a production server.
//
// Supported options:
//   autoIndex (default: true) - set to false to disable directory listings
//   liveReloadPath - LiveReload script URL for error pages
module.exports = function getMiddleware(watcher, options) {
  options = options || {};

  if (!options.hasOwnProperty('autoIndex')) {
    // set autoIndex to be true if not provided
    options.autoIndex = true;
  }

  return (request, response, next) => {
    watcher.then((hash) => {
      const outputPath = path.normalize(hash.directory);
      const incomingUrl = request.url;
      const urlObj = url.parse(incomingUrl);
      const filename = path.join(outputPath, decodeURIComponent(urlObj.pathname));

      const updatedFileName = setResponseHeaders(request, response, {
        'url': incomingUrl,
        'filename': filename,
        'outputPath': outputPath,
        'autoIndex': options.autoIndex
      });

      if (updatedFileName) {
        serveAsset(response, {
          'filename': updatedFileName
        });
      } else {
        // bypassing serving assets and call the next middleware
        next();
      }
    }, (buildError) => {
      errorHandler(response, {
        'buildError': buildError,
        'liveReloadPath': options.liveReloadPath
      });
    })
    .catch((err) => {
      console.log(err.stack);
    });
  };
};
