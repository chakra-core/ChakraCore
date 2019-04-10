'use strict';

const watcherServerMiddleware = require('./middleware');
const serveAssetMiddleware = require('./serve-asset-middleware');
const watcherMiddleware = require('./watcher-middleware');
const setFileContentResponseHeaders = require('./utils/response-header').setFileContentResponseHeaders;

module.exports = {
  watcherServerMiddleware,
  serveAssetMiddleware,
  watcherMiddleware,
  setFileContentResponseHeaders
}
