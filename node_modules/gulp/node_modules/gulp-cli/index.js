'use strict';

var fs = require('fs');
var path = require('path');
var log = require('gulplog');
var yargs = require('yargs');
var Liftoff = require('liftoff');
var interpret = require('interpret');
var v8flags = require('v8flags');
var findRange = require('semver-greatest-satisfied-range');
var ansi = require('./lib/shared/ansi');
var exit = require('./lib/shared/exit');
var tildify = require('./lib/shared/tildify');
var makeTitle = require('./lib/shared/make-title');
var cliOptions = require('./lib/shared/cli-options');
var completion = require('./lib/shared/completion');
var verifyDeps = require('./lib/shared/verify-dependencies');
var cliVersion = require('./package.json').version;
var getBlacklist = require('./lib/shared/get-blacklist');
var toConsole = require('./lib/shared/log/to-console');

var loadConfigFiles = require('./lib/shared/config/load-files');
var mergeConfigToCliFlags = require('./lib/shared/config/cli-flags');
var mergeConfigToEnvFlags = require('./lib/shared/config/env-flags');

// Logging functions
var logVerify = require('./lib/shared/log/verify');
var logBlacklistError = require('./lib/shared/log/blacklist-error');

// Get supported ranges
var ranges = fs.readdirSync(__dirname + '/lib/versioned/');

// Set env var for ORIGINAL cwd
// before anything touches it
process.env.INIT_CWD = process.cwd();

var cli = new Liftoff({
  name: 'gulp',
  processTitle: makeTitle('gulp', process.argv.slice(2)),
  completions: completion,
  extensions: interpret.jsVariants,
  v8flags: v8flags,
  configFiles: {
    '.gulp': {
      home: {
        path: '~',
        extensions: interpret.extensions,
      },
      cwd: {
        path: '.',
        extensions: interpret.extensions,
      },
    },
  },
});

var usage =
  '\n' + ansi.bold('Usage:') +
  ' gulp ' + ansi.blue('[options]') + ' tasks';

var parser = yargs.usage(usage, cliOptions);
var opts = parser.argv;

// Set up event listeners for logging temporarily.
toConsole(log, opts);

cli.on('require', function(name) {
  log.info('Requiring external module', ansi.magenta(name));
});

cli.on('requireFail', function(name) {
  log.error(ansi.red('Failed to load external module'), ansi.magenta(name));
});

cli.on('respawn', function(flags, child) {
  var nodeFlags = ansi.magenta(flags.join(', '));
  var pid = ansi.magenta(child.pid);
  log.info('Node flags detected:', nodeFlags);
  log.info('Respawned to PID:', pid);
});

function run() {
  cli.launch({
    cwd: opts.cwd,
    configPath: opts.gulpfile,
    require: opts.require,
    completion: opts.completion,
  }, handleArguments);
}

module.exports = run;

// The actual logic
function handleArguments(env) {
  var cfgLoadOrder = ['home', 'cwd'];
  var cfg = loadConfigFiles(env.configFiles['.gulp'], cfgLoadOrder);
  opts = mergeConfigToCliFlags(opts, cfg);
  env = mergeConfigToEnvFlags(env, cfg);

  // This translates the --continue flag in gulp
  // To the settle env variable for undertaker
  // We use the process.env so the user's gulpfile
  // Can know about the flag
  if (opts.continue) {
    process.env.UNDERTAKER_SETTLE = 'true';
  }

  // Set up event listeners for logging again after configuring.
  toConsole(log, opts);

  if (opts.help) {
    parser.showHelp(console.log);
    exit(0);
  }

  if (opts.version) {
    log.info('CLI version', cliVersion);
    if (env.modulePackage && typeof env.modulePackage.version !== 'undefined') {
      log.info('Local version', env.modulePackage.version);
    }
    exit(0);
  }

  if (opts.verify) {
    var pkgPath = opts.verify !== true ? opts.verify : 'package.json';
    if (path.resolve(pkgPath) !== path.normalize(pkgPath)) {
      pkgPath = path.join(env.cwd, pkgPath);
    }
    log.info('Verifying plugins in ' + pkgPath);
    return getBlacklist(function(err, blacklist) {
      if (err) {
        return logBlacklistError(err);
      }

      var blacklisted = verifyDeps(require(pkgPath), blacklist);

      logVerify(blacklisted);
    });
  }

  if (!env.modulePath) {
    log.error(
      ansi.red('Local gulp not found in'),
      ansi.magenta(tildify(env.cwd))
    );
    log.error(ansi.red('Try running: npm install gulp'));
    exit(1);
  }

  if (!env.configPath) {
    log.error(ansi.red('No gulpfile found'));
    exit(1);
  }

  // Chdir before requiring gulpfile to make sure
  // we let them chdir as needed
  if (process.cwd() !== env.cwd) {
    process.chdir(env.cwd);
    log.info(
      'Working directory changed to',
      ansi.magenta(tildify(env.cwd))
    );
  }

  // Find the correct CLI version to run
  var range = findRange(env.modulePackage.version, ranges);

  if (!range) {
    return log.error(
      ansi.red('Unsupported gulp version', env.modulePackage.version)
    );
  }

  // Load and execute the CLI version
  require(path.join(__dirname, '/lib/versioned/', range, '/'))(opts, env, cfg);
}
