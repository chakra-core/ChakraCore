'use strict';

const log = require('npmlog');
const Config = require('./config');
const EventEmitter = require('events').EventEmitter;
const fs = require('fs');
const App = require('./app');
const Server = require('./server');

/*
  CLI-level options:

  file:                 [String]  configuration file (testem.json, .testem.json, testem.yml, .testem.yml)
  host:                 [String]  server host to use (localhost)
  port:                 [Number]  server port to use (7357)
  launch:               [Array]   list of launchers to use for current runs (defaults to current mode)
  skip:                 [Array]   list of launchers to skip
  debug:                [Boolean] debug mode (false)
  test_page:            [String|Array]  path (or list of paths) to the page (or pages) to use to run tests
  growl:                [Boolean] enables growl / native notifications (false)

  Config-level options:

  launchers:            [Object]  List of custom launchers
  launch_in_dev:        [Array]   list of launchers to use for dev runs
  launch_in_ci:         [Array]   list of launchers to use for CI runs
  timeout:              [Number]  timeout for a browser
  framework:            [String]  test framework to use
  url:                  [String]  url server runs at (http://{host}:{port}/)
  src_files:            [Array]   list of files or file patterns to use
  src_files_ignore:     [Array]   list of files or file patterns to exclude from usage
  serve_files:          [Array]   list of files or file patterns to inject into test playground (defaults to src_files)
  watch_files:          [Array]   list of files or file patterns to watch changes of (defaults to src_files)
  css_files:            [Array]   additional stylesheets to include
  cwd:                  [Path]    directory to use as root
  config_dir:           [Path]    directory to use as root for resolving configs, if different than cwd
  query_params:         [Object]  Object map defining query params to add to the test page. Can be query string also.
  parallel:             [Number]  max number of parallel runners (1)
  routes:               [Object]  overrides for assets paths
  fail_on_zero_tests:   [Boolean] whether process should exit with error status when no tests found
  unsafe_file_serving:  [Boolean] allow serving directories that are not in your CWD (false)

  Available hooks:

  on_start:             Runs on suite startup
  before_tests:         Runs before every run of tests
  after_tests:          Runs after every run of tests
  on_exit:              Runs before suite exits
*/

class Api {
  setup(mode, finalizer) {
    this.config = new Config(mode, this.options);
    this.config.setDefaultOptions(this.defaultOptions);
    this.configureLogging();
    log.info('Test\'em starting..');
    this.config.read(() => {
      this.app = new App(this.config, finalizer);
      this.app.start();
    });
  }

  configureLogging() {
    let debug = this.config.get('debug');
    if (debug) {
      log.stream = fs.createWriteStream(debug);
    } else {
      let fakeStream = new EventEmitter();
      fakeStream.write = () => {};
      log.stream = fakeStream;
    }
  }

  startDev(options, finalizer) {
    this.options = options;
    this.setup('dev', finalizer);
  }

  restart() {
    this.app.triggerRun('Api: restart');
  }

  startCI(options, finalizer) {
    this.options = options;
    this.setup('ci', finalizer);
  }

  startServer(options) {
    this.options = options;
    this.config = new Config('server', this.options);
    this.config.setDefaultOptions(this.defaultOptions);

    this.config.read(() => {
      let server = new Server(this.config);
      server.start();
      server.on('server-start', () => {
        console.log('Open ' + this.config.get('url') + ' in a browser to connect.');
      });
    });
  }

  setDefaultOptions(defaultOptions) {
    this.defaultOptions = defaultOptions;
  }
}

module.exports = Api;
