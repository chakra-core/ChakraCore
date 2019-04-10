'use strict';

const url = require('url');
const path = require('path');

const errorHandler = require('./utils/error-handler');

module.exports = function watcherMiddleware(watcher, options) {
  options = options || {};

  if (!options.hasOwnProperty('autoIndex')) {
    // set autoIndex to be true if not provided
    options.autoIndex = true;
  }

  return (request, response, next) => {
    watcher.then((hash) => {
      const outputPath = path.normalize(hash.directory);

      // set the x-broccoli header containing per request info used by the broccoli-middleware
      const urlToBeServed = request.url;
      const urlObj = url.parse(urlToBeServed);
      const filename = path.join(outputPath, decodeURIComponent(urlObj.pathname));
      const broccoliInfo = {
        url: urlToBeServed,
        outputPath: outputPath,
        filename: filename,
        autoIndex: options.autoIndex
      };
      request.headers['x-broccoli'] = broccoliInfo;

      // set the default response headers that are independent of the asset
      response.setHeader('Cache-Control', 'private, max-age=0, must-revalidate');

      next();
    }, (buildError) => {
      errorHandler(response, {
        'buildError': buildError,
        'liveReloadPath': options.liveReloadPath
      });
    })
    .catch((err) => {
      console.log(err);
    })
  }
}
