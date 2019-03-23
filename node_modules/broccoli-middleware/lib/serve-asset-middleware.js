'use strict';
const serveAsset = require('./utils/serve-asset');
const setResponseHeaders = require('./utils/response-header').setResponseHeaders;

module.exports = function serveAssetsMiddleware(request, response, next) {
  const broccoliHeader = request.headers['x-broccoli'];

  if (broccoliHeader && broccoliHeader.filename) {
    // set response headers for assets (files) being served from disk
    const updatedFileName = setResponseHeaders(request, response, {
      'url': broccoliHeader.url,
      'filename': broccoliHeader.filename,
      'outputPath': broccoliHeader.outputPath,
      'autoIndex': broccoliHeader.autoIndex
    });

    if (updatedFileName) {
      // serve the file from disk and end the response
      serveAsset(response, {
        'filename': updatedFileName
      });
    } else if (!response.finished) {
      // only if request has not finished processing, call the next middleware (for cases where file is not found)
      next();
    }
  } else {
    // bypass this middleware if the broccoli header or invalid file name is not defined
    next();
  }
}
