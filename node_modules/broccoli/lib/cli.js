'use strict';

const promiseFinally = require('promise.prototype.finally');
const TreeSync = require('tree-sync');
const childProcess = require('child_process');
const fs = require('fs');
const WatchDetector = require('watch-detector');
const path = require('path');

const broccoli = require('./index');
const messages = require('./messages');
const CliError = require('./errors/cli');

module.exports = function broccoliCLI(args) {
  // always require a fresh commander, as it keeps state at module scope
  delete require.cache[require.resolve('commander')];
  const program = require('commander');
  let actionPromise;

  program.version(require('../package.json').version).usage('[options] <command> [<args ...>]');

  program
    .command('serve')
    .alias('s')
    .description('start a broccoli server')
    .option('--port <port>', 'the port to bind to [4200]', 4200)
    .option('--host <host>', 'the host to bind to [localhost]', 'localhost')
    .option('--brocfile-path <path>', 'the path to brocfile')
    .option('--output-path <path>', 'the path to target output folder')
    .option('--cwd <path>', 'the path to working folder')
    .option('--no-watch', 'turn off the watcher')
    .option('--watcher <watcher>', 'select sane watcher mode')
    .option('-e, --environment <environment>', 'build environment [development]', 'development')
    .option('--prod', 'alias for --environment=production')
    .option('--dev', 'alias for --environment=development')
    .action(options => {
      if (options.prod) {
        options.environment = 'production';
      } else if (options.dev) {
        options.environment = 'development';
      }

      const builder = getBuilder(options);
      const Watcher = getWatcher(options);
      const outputDir = options.outputPath;
      const watcher = new Watcher(builder, buildWatcherOptions(options));

      if (outputDir) {
        try {
          guardOutputDir(outputDir);
        } catch (e) {
          if (e instanceof CliError) {
            console.error(e.message);
            return process.exit(1);
          }

          throw e;
        }

        const outputTree = new TreeSync(builder.outputPath, outputDir);

        watcher.on('buildSuccess', () => outputTree.sync());
      }

      const server = broccoli.server.serve(watcher, options.host, parseInt(options.port, 10));
      actionPromise = (server && server.closingPromise) || Promise.resolve();
    });

  program
    .command('build [target]')
    .alias('b')
    .description('output files to target directory')
    .option('--brocfile-path <path>', 'the path to brocfile')
    .option('--output-path <path>', 'the path to target output folder')
    .option('--cwd <path>', 'the path to working folder')
    .option('--watch', 'turn on the watcher')
    .option('--watcher <watcher>', 'select sane watcher mode')
    .option('-e, --environment <environment>', 'build environment [development]', 'development')
    .option('--prod', 'alias for --environment=production')
    .option('--dev', 'alias for --environment=development')
    .action((outputDir, options) => {
      if (outputDir && options.outputPath) {
        console.error('option --output-path and [target] cannot be passed at same time');
        return process.exit(1);
      }

      if (options.outputPath) {
        outputDir = options.outputPath;
      }

      if (!outputDir) {
        outputDir = 'dist';
      }

      if (options.prod) {
        options.environment = 'production';
      } else if (options.dev) {
        options.environment = 'development';
      }

      try {
        guardOutputDir(outputDir);
      } catch (e) {
        if (e instanceof CliError) {
          console.error(e.message);
          return process.exit(1);
        }

        throw e;
      }

      const builder = getBuilder(options);
      const Watcher = getWatcher(options);
      const outputTree = new TreeSync(builder.outputPath, outputDir);
      const watcher = new Watcher(builder, buildWatcherOptions(options));

      watcher.on('buildSuccess', () => {
        outputTree.sync();
        messages.onBuildSuccess(builder);

        if (!options.watch) {
          watcher.quit();
        }
      });
      watcher.on('buildFailure', messages.onBuildFailure);

      function cleanupAndExit() {
        return watcher.quit();
      }

      process.on('SIGINT', cleanupAndExit);
      process.on('SIGTERM', cleanupAndExit);

      actionPromise = promiseFinally(
        watcher.start().catch(err => console.log((err && err.stack) || err)),
        () => {
          builder.cleanup();
          process.exit(0);
        }
      ).catch(err => {
        console.log('Cleanup error:');
        console.log((err && err.stack) || err);
        process.exit(1);
      });
    });

  program.parse(args || process.argv);

  if (!actionPromise) {
    program.outputHelp();
    return process.exit(1);
  }

  return actionPromise || Promise.resolve();
};

function getBuilder(options) {
  const brocfile = broccoli.loadBrocfile(options);
  return new broccoli.Builder(brocfile(buildBrocfileOptions(options)));
}

function getWatcher(options) {
  return options.watch ? broccoli.Watcher : require('./dummy-watcher');
}

function buildWatcherOptions(options) {
  if (!options) {
    options = {};
  }

  const detector = new WatchDetector({
    ui: { writeLine: console.log },
    childProcess,
    fs,
    watchmanSupportsPlatform: /^win/.test(process.platform),
    root: process.cwd(),
  });

  const watchPreference = detector.findBestWatcherOption({
    watcher: options.watcher,
  });
  const watcher = watchPreference.watcher;

  return {
    saneOptions: {
      poll: watcher === 'polling',
      watchman: watcher === 'watchman',
      node: watcher === 'node' || !watcher,
    },
  };
}

function buildBrocfileOptions(options) {
  return {
    env: options.environment,
  };
}

function guardOutputDir(outputDir) {
  if (isParentDirectory(outputDir)) {
    throw new CliError(
      `build directory can not be the current or direct parent directory: ${outputDir}`
    );
  }
}

function isParentDirectory(outputPath) {
  if (!fs.existsSync(outputPath)) {
    return false;
  }

  outputPath = fs.realpathSync(outputPath);

  const rootPath = process.cwd();
  const rootPathParents = [rootPath];
  let dir = path.dirname(rootPath);
  rootPathParents.push(dir);

  while (dir !== path.dirname(dir)) {
    dir = path.dirname(dir);
    rootPathParents.push(dir);
  }

  return rootPathParents.indexOf(outputPath) !== -1;
}
