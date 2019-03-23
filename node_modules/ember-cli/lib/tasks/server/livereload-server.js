'use strict';

const SilentError = require('silent-error');
const walkSync = require('walk-sync');
const path = require('path');
const FSTree = require('fs-tree-diff');
const logger = require('heimdalljs-logger')('ember-cli:live-reload:');
const fs = require('fs');
const Promise = require('rsvp').Promise;
const cleanBaseUrl = require('clean-base-url');
const isLiveReloadRequest = require('../../utilities/is-live-reload-request');

function isNotRemoved(entryTuple) {
  let operation = entryTuple[0];
  return operation !== 'unlink' && operation !== 'rmdir';
}

function isNotDirectory(entryTuple) {
  let entry = entryTuple[2];
  return entry && !entry.isDirectory();
}

function relativePath(patch) {
  return patch[1];
}

function isNotSourceMapFile(file) {
  return !(/\.map$/.test(file));
}

function readSSLandCert(sslKey, sslCert) {
  if (!fs.existsSync(sslKey)) {
    throw new TypeError(`SSL key couldn't be found in "${sslKey}", ` +
      `please provide a path to an existing ssl key file with --ssl-key`);
  }

  if (!fs.existsSync(sslCert)) {
    throw new TypeError(`SSL certificate couldn't be found in "${sslCert}", ` +
      `please provide a path to an existing ssl certificate file with --ssl-cert`);
  }

  let key = fs.readFileSync(sslKey);
  let cert = fs.readFileSync(sslCert);
  return { key, cert };
}

const DEFAULT_PREFIX = '/';

module.exports = class LiveReloadServer {
  constructor({ app, watcher, ui, project, analytics, httpServer }) {
    this.app = app;
    this.watcher = watcher;
    this.ui = ui;
    this.project = project;
    this.analytics = analytics;
    this.httpServer = httpServer;
    this.liveReloadPrefix = DEFAULT_PREFIX;
  }

  setupMiddleware(options) {
    const tinylr = require('tiny-lr');
    const Server = tinylr.Server;

    if (options.liveReloadPrefix) {
      this.liveReloadPrefix = cleanBaseUrl(options.liveReloadPrefix);
    }

    if (options.liveReload) {
      if (options.liveReloadPort && options.port !== options.liveReloadPort) {

        return this.createServerforCustomPort(options, Server)
          .catch(error => {
            if (error !== null && typeof error === 'object' && error.code === 'EADDRINUSE') {
              let url = `http${options.ssl ? 's' : ''}://${this.displayHost(options.liveReloadHost)}:${options.liveReloadPort}`;
              throw new SilentError(`${error.message}\nLivereload failed on '${url}', It may be in use.`);
            } else {
              throw error;
            }
          });
      } else {
        this.liveReloadServer = this.createServer(options, Server);
      }
      this.start();
    } else {
      this.ui.writeWarnLine('Livereload server manually disabled.');
    }
    return Promise.resolve();
  }

  start() {
    this.tree = new FSTree.fromEntries([]);
    // Reload on file changes
    this.watcher.on('change', function() {
      try {
        this.didChange.apply(this, arguments);
      } catch (e) {
        this.ui.writeError(e);
      }
    }.bind(this));

    this.watcher.on('error', this.didChange.bind(this));

    // Reload on express server restarts
    this.app.on('restart', this.didRestart.bind(this));
    this.httpServer.on('upgrade', (req, socket, head) => {
      if (isLiveReloadRequest(req.url, this.liveReloadPrefix)) {
        this.liveReloadServer.websocketify(req, socket, head);
      }
    });
    this.httpServer.on('error', this.liveReloadServer.error.bind(this.liveReloadServer));
    this.httpServer.on('close', this.liveReloadServer.close.bind(this.liveReloadServer));
    this.app.use(this.liveReloadPrefix, this.liveReloadServer.handler.bind(this.liveReloadServer));
  }

  createServer(options, Server) {
    let serverOptions = {
      app: this.app,
      dashboard: 'false',
      prefix: DEFAULT_PREFIX,
      port: options.port,
    };
    if (options.ssl) {
      let { key, cert } = readSSLandCert(options.sslKey, options.sslCert);
      serverOptions.key = key;
      serverOptions.cert = cert;
    }
    let lrServer = new Server(serverOptions);

    // this is required to prevent tiny-lr from triggering an error
    // when checking this.server._handle during its close handler
    // here: https://git.io/fA7Tr
    lrServer.server = this.httpServer;

    return lrServer;
  }

  createServerforCustomPort(options, Server) {
    let instance;
    Server.prototype.error = function() {
      instance.error.apply(instance, arguments);
    };
    let serverOptions = {
      dashboard: 'false',
      prefix: options.liveReloadPrefix,
      port: options.liveReloadPort,
      host: options.liveReloadHost,
    };

    if (options.ssl) {
      let { key, cert } = readSSLandCert(options.sslKey, options.sslCert);
      serverOptions.key = key;
      serverOptions.cert = cert;
    }

    instance = new Server(serverOptions);
    this.liveReloadServer = instance;
    this.start();
    return new Promise((resolve, reject) => {
      this.liveReloadServer.error = reject;
      this.liveReloadServer.listen(options.liveReloadPort, options.liveReloadHost, resolve);
    });
  }

  displayHost(specifiedHost) {
    return specifiedHost || 'localhost';
  }


  writeSkipBanner(filePath) {
    this.ui.writeLine(`Skipping livereload for: ${filePath}`);
  }

  getDirectoryEntries(directory) {
    return walkSync.entries(directory);
  }

  shouldTriggerReload(options) {
    let result = true;

    if (this.project.liveReloadFilterPatterns.length > 0) {
      let filePath = path.relative(this.project.root, options.filePath || '');

      result = this.project.liveReloadFilterPatterns
        .every(pattern => pattern.test(filePath) === false);

      if (result === false) {
        this.writeSkipBanner(filePath);
      }
    }

    return result;
  }

  didChange(results) {
    let previousTree = this.tree;
    let files;

    if (results.stack) {
      this._hasCompileError = true;
      files = ['LiveReload due to compile error'];
    } else if (this._hasCompileError) {
      this._hasCompileError = false;
      files = ['LiveReload due to resolved compile error'];
    } else if (results.directory) {
      this.tree = new FSTree.fromEntries(this.getDirectoryEntries(results.directory), { sortAndExpand: true });
      files = previousTree.calculatePatch(this.tree)
        .filter(isNotRemoved)
        .filter(isNotDirectory)
        .map(relativePath)
        .filter(isNotSourceMapFile);

    } else {
      files = ['LiveReload files'];
    }

    logger.info('files %o', files);

    if (this.shouldTriggerReload(results)) {
      this.liveReloadServer.changed({
        body: {
          files,
        },
      });

      this.analytics.track({
        name: 'broccoli watcher',
        message: 'live-reload',
      });
    }
  }

  didRestart() {
    this.liveReloadServer.changed({
      body: {
        files: ['LiveReload files'],
      },
    });

    this.analytics.track({
      name: 'express server',
      message: 'live-reload',
    });
  }
};
