"use strict";

const Promise = require("bluebird");
const express = require("express");
const app = express();
const path = require("path");
const uiConfig = require("electrode-ui-config");
const setStaticPaths = function() {
  app.use(express.static(path.join(__dirname, "../../dist")));
};

const xrequire = require;

const logger = {
  log() {
    console.log.apply(console, arguments); // eslint-disable-line
  },
  error() {
    console.error.apply(console, arguments); // eslint-disable-line
  }
};

const setDevMiddleware = config => {
  try {
    const devSetup = xrequire("electrode-archetype-react-app-dev/lib/webpack-dev-express");
    devSetup(app, "http", config.port);
  } catch (err) {
    if (process.env.NODE_ENV !== "production") {
      logger.error("setup dev middleware failed", err);
    }
  }
};

const setRouteHandler = config =>
  new Promise((resolve, reject) => {
    const webapp = p => (p.startsWith(".") ? path.resolve(p) : p);
    uiConfig.ui = {
      demo: config.ui.demo
    };
    const registerRoutes = xrequire(webapp(config.webapp.module));
    app.config = config;
    return registerRoutes(app, config.webapp.options, err => {
      if (err) {
        logger.error(err);
        reject(err);
      } else {
        resolve();
      }
    });
  });

const startServer = config =>
  new Promise((resolve, reject) => {
    app.listen(config.port, err => {
      if (err) {
        reject(err);
      } else {
        logger.log(`\nApp listening on port: ${config.port}`);
        resolve();
      }
    });
  });

module.exports = function electrodeServer(userConfig, callback) {
  const promise = Promise.resolve(userConfig)
    .tap(setDevMiddleware)
    .tap(setStaticPaths)
    .tap(setRouteHandler)
    .tap(startServer);

  return callback ? promise.nodeify(callback) : promise;
};
