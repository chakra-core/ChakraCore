'use strict';

const path = require('path');
const EventEmitter = require('events').EventEmitter;
const chalk = require('chalk');
const fs = require('fs');
const debounce = require('ember-cli-lodash-subset').debounce;
const mapSeries = require('promise-map-series');
const Promise = require('rsvp').Promise;
const Task = require('../../models/task');
const SilentError = require('silent-error');
const LiveReloadServer = require('./livereload-server');

class ExpressServerTask extends Task {
  constructor(options) {
    super(options);
    this.emitter = new EventEmitter();
    this.express = this.express || require('express');
    this.http = this.http || require('http');
    this.https = this.https || require('https');

    let serverRestartDelayTime = this.serverRestartDelayTime || 100;
    this.scheduleServerRestart = debounce(function() {
      this.restartHttpServer();
    }, serverRestartDelayTime);
  }

  on() {
    this.emitter.on.apply(this.emitter, arguments);
  }

  off() {
    this.emitter.off.apply(this.emitter, arguments);
  }

  emit() {
    this.emitter.emit.apply(this.emitter, arguments);
  }

  displayHost(specifiedHost) {
    return specifiedHost || 'localhost';
  }

  setupHttpServer() {
    if (this.startOptions.ssl) {
      this.httpServer = this.createHttpsServer();
    } else {
      this.httpServer = this.http.createServer(this.app);
    }

    // We have to keep track of sockets so that we can close them
    // when we need to restart.
    this.sockets = {};
    this.nextSocketId = 0;
    this.httpServer.on('connection', socket => {
      let socketId = this.nextSocketId++;
      this.sockets[socketId] = socket;

      socket.on('close', () => {
        delete this.sockets[socketId];
      });
    });
  }

  createHttpsServer() {
    if (!fs.existsSync(this.startOptions.sslKey)) {
      throw new TypeError(`SSL key couldn't be found in "${this.startOptions.sslKey}", ` +
        `please provide a path to an existing ssl key file with --ssl-key`);
    }

    if (!fs.existsSync(this.startOptions.sslCert)) {
      throw new TypeError(`SSL certificate couldn't be found in "${this.startOptions.sslCert}", ` +
        `please provide a path to an existing ssl certificate file with --ssl-cert`);
    }

    let options = {
      key: fs.readFileSync(this.startOptions.sslKey),
      cert: fs.readFileSync(this.startOptions.sslCert),
    };

    return this.https.createServer(options, this.app);
  }

  listen(port, host) {
    let server = this.httpServer;

    return new Promise((resolve, reject) => {
      server.listen(port, host);
      server.on('listening', () => {
        resolve();
        this.emit('listening');
      });
      server.on('error', reject);
    });
  }

  processAddonMiddlewares(options) {
    this.project.initializeAddons();

    return mapSeries(this.project.addons, function(addon) {
      if (addon.serverMiddleware) {
        return addon.serverMiddleware({
          app: this.app,
          options,
        });
      }
    }, this);
  }

  processAppMiddlewares(options) {
    if (this.project.has(this.serverRoot)) {
      let server = this.project.require(this.serverRoot);

      if (typeof server !== 'function') {
        throw new TypeError('ember-cli expected ./server/index.js to be the entry for your mock or proxy server');
      }

      if (server.length === 3) {
        // express app is function of form req, res, next
        return this.app.use(server);
      }

      return server(this.app, options);
    }
  }

  start(options) {
    options.project = this.project;
    options.watcher = this.watcher;
    options.serverWatcher = this.serverWatcher;
    options.analytics = this.analytics;
    options.ui = this.ui;

    this.startOptions = options;

    if (this.serverWatcher) {
      this.serverWatcher.on('change', this.serverWatcherDidChange.bind(this));
      this.serverWatcher.on('add', this.serverWatcherDidChange.bind(this));
      this.serverWatcher.on('delete', this.serverWatcherDidChange.bind(this));
    }

    return this.startHttpServer();
  }

  serverWatcherDidChange() {
    this.scheduleServerRestart();
  }

  restartHttpServer() {
    if (!this.serverRestartPromise) {
      this.serverRestartPromise = this.stopHttpServer().then(() => {
        this.invalidateCache(this.serverRoot);
        return this.startHttpServer();
      }).then(() => {
        this.emit('restart');
        this.ui.writeLine('');
        this.ui.writeLine(chalk.green('Server restarted.'));
        this.ui.writeLine('');
      }).catch(err => {
        this.ui.writeError(err);
      }).finally(() => {
        this.serverRestartPromise = null;
      });

      return this.serverRestartPromise;
    } else {
      return this.serverRestartPromise
        .then(() => this.restartHttpServer());
    }
  }

  stopHttpServer() {
    return new Promise((resolve, reject) => {
      if (!this.httpServer) {
        return resolve();
      }
      this.httpServer.close(err => {
        if (err) {
          reject(err);
          return;
        }
        this.httpServer = null;
        resolve();
      });

      // We have to force close all sockets in order to get a fast restart
      let sockets = this.sockets;
      for (let socketId in sockets) {
        sockets[socketId].destroy();
      }
    });
  }

  startHttpServer() {
    this.app = this.express();
    const compression = require('compression');
    this.app.use(compression({
      filter(req, res) {
        let type = res.getHeader('Content-Type');

        if (res.getHeader('x-no-compression')) {
          // don't compress responses with this response header
          return false;
        } else if (type && type.indexOf('text/event-stream') > -1) {
          // don't attempt to compress server sent events
          return false;
        } else {
          return compression.filter(req, res);
        }
      },
    }));

    this.setupHttpServer();

    let options = this.startOptions;
    options.httpServer = this.httpServer;
    let liveReloadServer;

    if (options.path) {
      liveReloadServer = {
        setupMiddleware() {},
      };
    } else {
      liveReloadServer = new LiveReloadServer({
        app: this.app,
        ui: options.ui,
        watcher: options.watcher,
        analytics: options.analytics,
        project: options.project,
        httpServer: options.httpServer,
      });
    }

    const config = this.project.config(options.environment);
    const middlewareOptions = Object.assign({}, options, {
      rootURL: config.rootURL,
      baseURL: config.baseURL || '/',
    });

    return Promise.resolve()
      .then(() => liveReloadServer.setupMiddleware(this.startOptions))
      .then(() => this.processAppMiddlewares(middlewareOptions))
      .then(() => this.processAddonMiddlewares(middlewareOptions))
      .then(() => this.listen(options.port, options.host)
        .catch(() => {
          throw new SilentError(`Could not serve on http://${this.displayHost(options.host)}:${options.port}. ` +
            `It is either in use or you do not have permission.`);
        }));
  }

  invalidateCache(serverRoot) {
    let absoluteServerRoot = path.resolve(serverRoot);
    if (absoluteServerRoot[absoluteServerRoot.length - 1] !== path.sep) {
      absoluteServerRoot += path.sep;
    }

    let allKeys = Object.keys(require.cache);
    for (let i = 0; i < allKeys.length; i++) {
      if (allKeys[i].indexOf(absoluteServerRoot) === 0) {
        delete require.cache[allKeys[i]];
      }
    }
  }
}

module.exports = ExpressServerTask;
