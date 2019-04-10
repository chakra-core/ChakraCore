'use strict';

const cleanBaseURL = require('clean-base-url');
const logger = require('heimdalljs-logger')('ember-cli:testem-url-rewriter');

class TestemUrlRewriterAddon {
  constructor(project) {
    this.name = 'testem-url-rewriter';
    this.project = project;
  }

  testemMiddleware(app) {
    let env = process.env.EMBER_ENV;
    logger.info('Reading config for environment "%s"', env);

    let config = this.project.config(env);
    logger.info('config.rootURL = %s', config.rootURL);

    this.project.ui.writeDeprecateLine(
      'Using the `baseURL` setting is deprecated, use `rootURL` instead.',
      !(!('rootURL' in config) && config.baseURL));

    this.project.ui.writeWarnLine(
      'The `baseURL` and `rootURL` settings should not be used at the same time.',
      !(('rootURL' in config) && config.baseURL));

    let rootURL = cleanBaseURL(config.rootURL) || '/';
    logger.info('rootURL = %s', rootURL);

    app.use((req, res, next) => {
      let oldUrl = req.url;
      if (rootURL !== '/' && oldUrl.indexOf(rootURL) === 0) {
        req.url = `/${oldUrl.slice(rootURL.length)}`;
        logger.info('Rewriting %s %s -> %s', req.method, oldUrl, req.url);
      } else {
        logger.info('Ignoring %s %s', req.method, req.url);
      }

      next();
    });
  }
}

module.exports = TestemUrlRewriterAddon;
