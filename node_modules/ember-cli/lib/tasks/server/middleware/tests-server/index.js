'use strict';

const cleanBaseURL = require('clean-base-url');
const path = require('path');
const fs = require('fs');
const logger = require('heimdalljs-logger')('ember-cli:test-server');

class TestsServerAddon {
  /**
   * This addon is used to serve the QUnit or Mocha test runner
   * at `baseURL + '/tests'`.
   *
   * @class TestsServerAddon
   * @constructor
   */
  constructor(project) {
    this.project = project;
    this.name = 'tests-server-middleware';
  }

  serverMiddleware(config) {
    let app = config.app;
    let options = config.options;
    let watcher = options.watcher;

    let baseURL = options.rootURL === '' ? '/' : cleanBaseURL(options.rootURL || options.baseURL);
    let testsPath = `${baseURL}tests`;

    app.use((req, res, next) => {
      watcher.then(results => {
        let acceptHeaders = req.headers.accept || [];
        let hasHTMLHeader = acceptHeaders.indexOf('text/html') !== -1;
        let hasWildcardHeader = acceptHeaders.indexOf('*/*') !== -1;

        let isForTests = req.path.indexOf(testsPath) === 0;

        logger.info('isForTests: %o', isForTests);

        if (isForTests && (hasHTMLHeader || hasWildcardHeader) && req.method === 'GET') {
          let assetPath = req.path.slice(baseURL.length);
          let filePath = path.join(results.directory, assetPath);

          if (!fs.existsSync(filePath) || !fs.statSync(filePath).isFile()) {
            // N.B., `baseURL` will end with a slash as it went through `cleanBaseURL`
            let newURL = `${baseURL}tests/index.html`;

            logger.info('url: %s resolved to path: %s which is not a file. Assuming %s instead', req.path, filePath, newURL);
            req.url = newURL;
          }
        }

      }).finally(next).finally(() => {
        if (config.finally) {
          config.finally();
        }
      });
    });
  }
}

module.exports = TestsServerAddon;
