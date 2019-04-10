'use strict';

const logger = require('heimdalljs-logger')('ember-cli:broccoli-serve-files');

class ServeFilesAddon {
  /**
   * This addon is used to serve the requested assets and set the required response
   * headers. It runs after broccoli-watcher addon.
   *
   * @class ServeFilesAddon
   * @constructor
   */
  constructor(project) {
    this.project = project;
    this.name = 'broccoli-serve-files';
  }

  serverMiddleware(options) {
    let app = options.app;
    options = options.options;

    let serveAssetMiddleware = require('broccoli-middleware').serveAssetMiddleware;

    app.use((req, res, next) => {
      logger.info('serving asset: %s', req.url);

      // serve the asset and close the response.
      serveAssetMiddleware(req, res, next);
    });
  }
}

module.exports = ServeFilesAddon;
