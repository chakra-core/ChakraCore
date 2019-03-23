/*

server.js
=========

Testem's server. Serves up the HTML, JS, and CSS required for
running the tests in a browser.

*/
'use strict';

var express = require('express');
var socketIO = require('socket.io');
var fs = require('fs');
var path = require('path');
var log = require('npmlog');
var EventEmitter = require('events').EventEmitter;
var Mustache = require('consolidate').mustache;
var http = require('http');
var https = require('https');
var httpProxy = require('http-proxy');
var Bluebird = require('bluebird');

var readFileAsync = Bluebird.promisify(fs.readFile);

class Server extends EventEmitter {
  constructor(config) {
    super();

    this.config = config;
    this.ieCompatMode = null;

    // Maintain a hash of all connected sockets to close them manually
    // Workaround https://github.com/joyent/node/issues/9066
    this.sockets = {};
    this.nextSocketId = 0;
  }

  start(callback) {
    this.createExpress();

    // Start the server!
    // Create socket.io sockets
    this.server.on('connection', socket => {
      var socketId = this.nextSocketId++;
      this.sockets[socketId] = socket;
      socket.on('close', () => {
        delete this.sockets[socketId];
      });
    });

    return new Bluebird.Promise((resolve, reject) => {
      this.server.on('listening', () => {
        this.config.set('port', this.server.address().port);
        resolve();
        this.emit('server-start');
      });
      this.server.on('error', e => {
        this.stopped = true;
        reject(e);
        this.emit('server-error', e);
      });

      this.server.listen(this.config.get('port'));
    }).asCallback(callback);
  }

  stop(callback) {
    if (this.server && !this.stopped) {
      this.stopped = true;

      return Bluebird.fromCallback(closeCallback => {
        this.server.close(closeCallback);

        // Destroy all open sockets
        for (var socketId in this.sockets) {
          this.sockets[socketId].destroy();
        }
      }).asCallback(callback);
    } else {
      return Bluebird.resolve().asCallback(callback);
    }
  }

  createExpress() {
    var app = this.express = express();
    var serveStaticFile = (req, res) => {
      this.serveStaticFile(req.params[0], req, res);
    };

    if (this.config.get('key') || this.config.get('pfx')) {
      var options = {};
      if (this.config.get('key')) {
        options.key = fs.readFileSync(this.config.get('key'));
        options.cert = fs.readFileSync(this.config.get('cert'));
      } else {
        options.pfx = fs.readFileSync(this.config.get('pfx'));
      }
      this.server = https.createServer(options, this.express);
    } else {
      this.server = http.createServer(this.express);
    }
    this.io = socketIO(this.server);

    let socketHeartbeatTimeout = this.config.get('socket_heartbeat_timeout');
    this.io.set('heartbeat timeout', socketHeartbeatTimeout * 1000);

    this.io.on('connection', this.onClientConnected.bind(this));

    this.configureExpress(app);

    this.injectMiddleware(app);

    this.configureProxy(app);

    app.get('/', (req, res) => {
      res.redirect(`/${String(Math.floor(Math.random() * 10000))}`);
    });

    app.get(/\/(-?[0-9]+)$/, (req, res) => {
      this.serveHomePage(req, res);
    });

    app.get('/testem.js', (req, res) => {
      this.serveTestemClientJs(req, res);
    });

    app.all(/^\/(?:-?[0-9]+)(\/.+)$/, serveStaticFile);
    app.all(/^(.+)$/, serveStaticFile);

    app.use((err, req, res, next) => {
      if (err) {
        log.error(err.message);
        if (err.code === 'ENOENT') {
          res.status(404).send(`Not found: ${req.url}`);
        } else {
          res.status(500).send(err.message);
        }
      } else {
        next();
      }
    });
  }

  configureExpress(app) {
    app.engine('mustache', Mustache);
    app.set('view options', {layout: false});
    app.set('etag', 'strong');
    app.use((req, res, next) => {
      if (this.ieCompatMode) {
        res.setHeader('X-UA-Compatible', `IE=${this.ieCompatMode}`);
      }
      next();
    });
    app.use(express.static(`${__dirname}/../../public`));
  }

  injectMiddleware(app) {
    var middlewares = this.config.get('middleware');
    if (middlewares) {
      middlewares.forEach(middleware => {
        middleware(app);
      });
    }
  }

  shouldProxy(req, opts) {
    var accepts;
    var acceptCheck = [
      'html',
      'css',
      'javascript'
    ];

    //Only apply filtering logic if 'onlyContentTypes' key is present
    if (!('onlyContentTypes' in opts)) {
      return true;
    }

    acceptCheck = acceptCheck.concat(opts.onlyContentTypes);
    acceptCheck.push('text');
    accepts = req.accepts(acceptCheck);
    if (accepts.indexOf(opts.onlyContentTypes) !== -1) {
      return true;
    }
    return false;
  }

  configureProxy(app) {
    var proxies = this.config.get('proxies');
    if (proxies) {
      this.proxy = new httpProxy.createProxyServer({changeOrigin: true});

      this.proxy.on('error', (err, req, res) => {
        res.status(500).json(err);
      });

      Object.keys(proxies).forEach(url => {
        app.all(`${url}*`, (req, res, next) => {
          var opts = proxies[url];
          if (this.shouldProxy(req, opts)) {
            if (opts.host) {
              opts.target = `http://${opts.host}:${opts.port}`;
              delete opts.host;
              delete opts.port;
            }
            this.proxy.web(req, res, opts);
          } else {
            next();
          }
        });
      });
    }
  }

  renderRunnerPage(err, files, footerScripts, res) {
    var config = this.config;
    var framework = config.get('framework') || 'jasmine';
    var css_files = config.get('css_files');
    var templateFile = {
      jasmine: 'jasminerunner',
      jasmine2: 'jasmine2runner',
      qunit: 'qunitrunner',
      mocha: 'mocharunner',
      'mocha+chai': 'mochachairunner',
      buster: 'busterrunner',
      custom: 'customrunner',
      tap: 'taprunner'
    }[framework] + '.mustache';
    res.render(`${__dirname}/../../views/${templateFile}`, {
      scripts: files,
      styles: css_files,
      footer_scripts: footerScripts
    });
  }

  renderDefaultTestPage(req, res) {
    var config = this.config;
    var test_page = config.get('test_page')[0];

    if (test_page) {
      if (test_page[0] === '/') {
        test_page = encodeURIComponent(test_page);
      }
      var base = req.path === '/' ?
        req.path : `${req.path}/`;
      var url = base + test_page;
      res.redirect(url);
    } else {
      config.getServeFiles((err, files) => {
        config.getFooterScripts((err, footerScripts) => {
          this.renderRunnerPage(err, files, footerScripts, res);
        });
      });
    }
  }

  serveHomePage(req, res) {
    var config = this.config;
    var routes = config.get('routes') || config.get('route') || {};
    if (routes['/']) {
      this.serveStaticFile('/', req, res);
    } else {
      this.renderDefaultTestPage(req, res);
    }
  }

  serveTestemClientJs(req, res) {
    res.setHeader('Content-Type', 'text/javascript');

    res.write(';(function(){');
    res.write('\n//============== config ==================\n\n');
    res.write(`var TestemConfig = ${JSON.stringify(this.config.client())};`);

    var files = [
      'decycle.js',
      'jasmine_adapter.js',
      'jasmine2_adapter.js',
      'qunit_adapter.js',
      'mocha_adapter.js',
      'buster_adapter.js',
      'testem_client.js'
    ];
    Bluebird.each(files, file => {
      if (file.indexOf(path.sep) === -1) {
        file = `${__dirname}/../../public/testem/${file}`;
      }
      return readFileAsync(file).then(data => {
        res.write(`\n//============== ${path.basename(file)} ==================\n\n`);
        res.write(data);
      }).catch(err => {
        res.write(`// Error reading ${file}: ${err}`);
      });
    }).then(() => {
      res.write('}());');
      res.end();
    });
  }

  route(uri) {
    var config = this.config;
    var routes = config.get('routes') || config.get('route') || {};
    var bestMatchLength = 0;
    var bestMatch = null;
    var prefixes = Object.keys(routes);
    prefixes.forEach(prefix => {
      if (uri.substring(0, prefix.length) === prefix) {
        if (bestMatchLength < prefix.length) {
          if (routes[prefix] instanceof Array) {
            routes[prefix].some(folder => {
              bestMatch = `${folder}/${uri.substring(prefix.length)}`;
              return fs.existsSync(config.resolvePath(bestMatch));
            });
          } else {
            bestMatch = `${routes[prefix]}/${uri.substring(prefix.length)}`;
          }
          bestMatchLength = prefix.length;
        }
      }
    });
    return {
      routed: !!bestMatch,
      uri: bestMatch || uri.substring(1)
    };
  }

  serveStaticFile(uri, req, res) {
    var config = this.config;
    var routeRes = this.route(uri);
    uri = routeRes.uri;
    var wasRouted = routeRes.routed;
    var allowUnsafeDirs = config.get('unsafe_file_serving');
    var filePath = path.resolve(config.resolvePath(uri));
    var ext = path.extname(filePath);
    var isPathPermitted = filePath.indexOf(path.resolve(config.cwd())) !== -1;
    if (!wasRouted && !allowUnsafeDirs && !isPathPermitted) {
      res.status(403);
      res.write('403 Forbidden');
      res.end();
    } else if (ext === '.mustache') {
      config.getTemplateData((err, data) => {
        res.render(filePath, data);
        this.emit('file-requested', filePath);
      });
    } else {
      fs.stat(filePath, (err, stat) => {
        this.emit('file-requested', filePath);
        if (err) {
          return res.sendFile(filePath);
        }
        if (stat.isDirectory()) {
          fs.readdir(filePath, (err, files) => {
            var dirListingPage = `${__dirname}/../../views/directorylisting.mustache`;
            res.render(dirListingPage, {files: files});
          });
        } else {
          res.sendFile(filePath);
        }
      });
    }
  }

  onClientConnected(client) {
    client.once('browser-login', (browserName, id) => {
      log.info(`New client connected: ${browserName} ${id}`);
      this.emit('browser-login', browserName, id, client);
    });
  }
}

module.exports = Server;
