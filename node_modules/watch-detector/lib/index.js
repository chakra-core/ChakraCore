'use strict';

const quickTemp = require('quick-temp');
const Promise = require('rsvp').Promise;
const WATCHMAN_INFO = 'Visit https://ember-cli.com/user-guide/#watchman for more info.';
const semver = require('semver');
const SilentError = require('silent-error');

let POLLING = 'polling';
let WATCHMAN = 'watchman';
let NODE = 'node';
let EVENTS = 'events';
let POSSIBLE_WATCHERS = [POLLING, WATCHMAN, NODE, EVENTS];

let debug = require('heimdalljs-logger')('ember-cli:watcher');

class WatchPreference {
  constructor(watcher) {
    this.watcher = watcher || null;
    this._watchmanInfo = {
      enabled: false,
      version: null,
      canNestRoots: false,
    };
  }

  watchmanWorks(details) {
    this._watchmanInfo.enabled = true;
    this._watchmanInfo.version = details.version;
    this._watchmanInfo.canNestRoots = semver.satisfies(details.version, '>= 3.7.0');
  }

  get watchmanInfo() {
    return this._watchmanInfo;
  }
}

/*
 * @public
 *
 * A testable class that encapsulates which watch technique to use.
 */
class WatchDetector {
  constructor(options) {
    this.childProcess = options.childProcess;
    this.watchmanSupportsPlatform = options.watchmanSupportsPlatform;
    this.fs = options.fs;
    this.root = options.root;

    this.ui = options.ui;
    this.tmp = undefined;

    // This exists, because during tests repeadelty testing the same directories
    this._doesNodeWorkCache = options.cache || {};
  }

  /*
   * @private
   *
   * implements input options parsing and fallback.
   *
   * @method extractPreferenceFromOptions
   * @returns {WatchDetector)
  */
  extractPreferenceFromOptions(options) {
    if (options.watcher === POLLING) {
      debug.info('skip detecting watchman, "polling" was selected.');
      return new WatchPreference(POLLING);
    } else if (options.watcher === NODE) {
      debug.info('skip detecting watchman, "node" was selected.');
      return new WatchPreference(NODE);
    } else {
      debug.info('no watcher preference set, lets attempt watchman');
      return new WatchPreference(WATCHMAN);
    }
  }

  /*
   * @public
   *
   * Detect if `fs.watch` provided by node is capable of observing a directory
   * within `process.cwd()`. Although `fs.watch` is provided as a part of the
   * node stdlib, their exists platforms it ships to that do not implement the
   * required sub-system. For example, there exists `posix` targets, without both
   * inotify and FSEvents
   *
   * @method testIfNodeWatcherAppearsToWork
   * @returns {Boolean) outcome
  */
  testIfNodeWatcherAppearsToWork() {
    let root = this.root;
    if (root in this._doesNodeWorkCache) {
      return this._doesNodeWorkCache[root];
    }

    this._doesNodeWorkCache[root] = false;
    try {
      // builds a tmp directory at process.cwd() + '/tmp/' + 'something-unique';
      let tmpDir = quickTemp.makeOrRemake(this, 'tmp');

      let watcher = this.fs.watch(tmpDir, { persistent: false, recursive: false });
      watcher.close();
    } catch (e) {
      debug.info('testing if node watcher failed with: %o', e);
      return false;
    } finally {
      try {
        quickTemp.remove(this, 'tmp');
        // cleanup dir
      } catch (e) {
        // not much we can do, lets debug log.
        debug.info('cleaning up dir failed with: %o', e);
      }
    }

    // it seems we where able to at least watch and unwatch, this should catch
    // systems that have a very broken NodeWatcher.
    //
    // NOTE: we could also add a more advance chance, that triggers a change
    // and expects to be informed of the change. This can be added in a future
    // iteration.
    //

    this._doesNodeWorkCache[root] = true;
    return true;
  }

  /*
   * @public
   *
   * Selecting the best watcher is complicated, this method (given a preference)
   * will test and provide the best possible watcher available to the current
   * system and project.
   *
   *
   *  @method findBestWatcherOption
   *  @input {Object} options
   *  @returns {Object} watch preference
  */
  findBestWatcherOption(options) {
    let preference = this.extractPreferenceFromOptions(options);
    let original = options.watcher;

    if (original && POSSIBLE_WATCHERS.indexOf(original) === -1) {
      return Promise.reject(
        new SilentError(
          `Unknown Watcher: \`${original}\` supported watchers: [${POSSIBLE_WATCHERS.join(', ')}]`
        )
      );
    }

    if (preference.watcher === WATCHMAN) {
      preference = this.checkWatchman();
    }

    let bestOption = preference;
    let actual;

    if (bestOption.watcher === NODE) {
      // although up to this point, we may believe Node is the best watcher
      // this may not be true because:
      // * not all platforms that run node, support node.watch
      // * not all file systems support node watch
      //
      let appearsToWork = this.testIfNodeWatcherAppearsToWork();
      actual = new WatchPreference(appearsToWork ? NODE : POLLING);
    } else {
      actual = bestOption;
    }
    debug.info('foundBestWatcherOption, preference was: %o, actual: %o', options, actual);
    if (
      actual /* if no preference was initial set, don't bother informing the user */ &&
      original /* if no original was set, the fallback is expected */ &&
      actual.watcher !== original &&
      // events represents either watcherman or node, but no preference
      !(original === EVENTS && (actual.watcher === WATCHMAN || actual.watcher === NODE))
    ) {
      // if there was an initial preference, but we had to fall back inform the user.
      this.ui.writeLine(`was unable to use: "${original}", fell back to: "${actual.watcher}"`);
    }
    return actual;
  }

  /*
   * @public
   *
   * Although watchman may be selected, it may not work due to:
   *
   *  * invalid version
   *  * it may be broken in detectable ways
   *  * watchman executable may be something unexpected
   *  * ???
   *
   *  @method checkWatchman
   *  @returns {Object} watch preference + _watchmanInfo
  */
  checkWatchman() {
    let result = new WatchPreference(null);

    debug.info('execSync("watchman version")');
    try {
      let output = this.childProcess.execSync('watchman version', { stdio: [] });
      debug.info('watchman version STDOUT: %s', output);
      let version;
      try {
        version = JSON.parse(output).version;
      } catch (e) {
        this.ui.writeLine('Looks like you have a different program called watchman.');
        this.ui.writeLine(WATCHMAN_INFO);

        result.watcher = NODE;
        return result;
      }

      debug.info('detected watchman: %s', version);

      if (semver.satisfies(version, '>= 3.0.0')) {
        debug.info('watchman %s does satisfy: %s', version, '>= 3.0.0');

        result.watcher = WATCHMAN;

        result.watchmanWorks({
          version,
          canNestRoots: semver.satisfies(version, '>= 3.7.0'),
        });
      } else {
        debug.info('watchman %s does NOT satisfy: %s', version, '>= 3.0.0');

        this.ui.writeLine(
          `Invalid watchman found, version: [${version}] did not satisfy [>= 3.0.0].`
        );
        this.ui.writeLine(WATCHMAN_INFO);

        result.watcher = NODE;
      }

      return result;
    } catch (reason) {
      debug.info('detecting watchman failed %o', reason);

      if (this.watchmanSupportsPlatform) {
        // don't bother telling windows users watchman detection failed, that is
        // until watchman is legit on windows.
      } else {
        this.ui.writeLine('Could not start watchman');
        this.ui.writeLine(WATCHMAN_INFO);
      }

      result.watcher = NODE;
      return result;
    }
  }
}

module.exports = WatchDetector;
