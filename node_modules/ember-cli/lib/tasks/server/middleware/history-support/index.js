'use strict';

const path = require('path');
const fs = require('fs');

const cleanBaseURL = require('clean-base-url');

class HistorySupportAddon {
  /**
   * This addon is used to serve the `index.html` file at every requested
   * URL that begins with `rootURL` and is expecting `text/html` output.
   *
   * @class HistorySupportAddon
   * @constructor
   */
  constructor(project) {
    this.project = project;
    this.name = 'history-support-middleware';
  }

  shouldAddMiddleware(environment) {
    let config = this.project.config(environment);
    let locationType = config.locationType;
    let historySupportMiddleware = config.historySupportMiddleware;

    if (typeof historySupportMiddleware === 'boolean') {
      return historySupportMiddleware;
    }

    return ['auto', 'history'].indexOf(locationType) !== -1;
  }

  serverMiddleware(config) {
    if (this.shouldAddMiddleware(config.options.environment)) {
      this.project.ui.writeWarnLine(
        'Empty `rootURL` is not supported. Disable history support, or use an absolute `rootURL`',
        config.options.rootURL !== '');

      this.addMiddleware(config);
    }
  }

  addMiddleware(config) {
    let app = config.app;
    let options = config.options;
    let watcher = options.watcher;

    let baseURL = options.rootURL === '' ? '/' : cleanBaseURL(options.rootURL || options.baseURL);

    app.use((req, res, next) => {
      watcher.then(results => {
        if (this.shouldHandleRequest(req, options)) {
          let assetPath = req.path.slice(baseURL.length);
          let isFile = false;
          try { isFile = fs.statSync(path.join(results.directory, assetPath)).isFile(); } catch (err) { /* ignore */ }
          if (!isFile) {
            req.serveUrl = `${baseURL}index.html`;
          }
        }
      }).finally(next);
    });
  }

  shouldHandleRequest(req, options) {
    let acceptHeaders = req.headers.accept || [];
    let hasHTMLHeader = acceptHeaders.indexOf('text/html') !== -1;
    if (req.method !== 'GET') {
      return false;
    }
    if (!hasHTMLHeader) {
      return false;
    }
    let baseURL = options.rootURL === '' ? '/' : cleanBaseURL(options.rootURL || options.baseURL);
    let baseURLRegexp = new RegExp(`^${baseURL}`);
    return baseURLRegexp.test(req.path);
  }
}

module.exports = HistorySupportAddon;
