'use strict';

const cleanBaseURL = require('clean-base-url');
const logger = require('heimdalljs-logger')('ember-cli:broccoli-watcher');

class WatcherAddon {
  /**
   * This addon is used to set the default response headers for the assets that will be
   * served by the next middleware. It waits for the watcher promise to resolve before
   * setting the response headers.
   *
   * @class WatcherAddon
   * @constructor
   */
  constructor(project) {
    this.project = project;
    this.name = 'broccoli-watcher';
  }

  serverMiddleware(options) {
    let app = options.app;
    options = options.options;

    let broccoliMiddleware = options.middleware || require('broccoli-middleware').watcherMiddleware;
    let middleware = broccoliMiddleware(options.watcher, {
      liveReloadPath: '/ember-cli-live-reload.js',
      autoIndex: false, // disable directory listings
    });

    let baseURL = options.rootURL === '' ? '/' : cleanBaseURL(options.rootURL || options.baseURL);

    logger.info('serverMiddleware: baseURL: %s', baseURL);

    app.use((req, res, next) => {
      let oldURL = req.url;
      let url = req.serveUrl || req.url;
      logger.info('serving: %s', url);

      let actualPrefix = req.url.slice(0, baseURL.length - 1); // Don't care
      let expectedPrefix = baseURL.slice(0, baseURL.length - 1); // about last slash

      if (actualPrefix === expectedPrefix) {
        let urlToBeServed = url.slice(actualPrefix.length); // Remove baseURL prefix
        req.url = urlToBeServed;
        logger.info('serving: (prefix stripped) %s, was: %s', urlToBeServed, url);

        // Find asset and set response headers, if no file has been found, reset url for proxy stuff
        // that comes afterwards
        middleware(req, res, err => {
          req.url = oldURL;
          if (err) {
            logger.error('err', err);
          }
          next(err);
        });
      } else {
        logger.info('prefixes didn\'t match, passing control on: (actual:%s expected:%s)', actualPrefix, expectedPrefix);
        next();
      }
    });
  }
}

module.exports = WatcherAddon;
