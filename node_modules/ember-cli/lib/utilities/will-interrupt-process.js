'use strict';

// Allows to setup process interruption handlers.
// The process can be interrupted when ges SIGINT, SIGTERM signal,
// something called process.exit(exitCode) or CTRL+C was pressed.
//
// Node.js doesn't allow to perform async tasks when the process is exiting.
// Also there are some work arounds for exit in the node.js ecosystem.
//
// In order to supply reliable process exit phase, `will-interrupt-process`
// is tightly integrated with `capture-exit` which allows us to perform async cleanup
// on `process.exit()` and control the final exit code.

const captureExit = require('capture-exit');
const EventEmitter = require('events');

const handlers = [];

let _process,
    windowsCtrlCTrap,
    originalIsRaw;

module.exports = {
  capture(outerProcess) {
    if (_process) {
      throw new Error('process already captured');
    }

    if (outerProcess instanceof EventEmitter === false) {
      throw new Error('attempt to capture bad process instance');
    }

    _process = outerProcess;

    // ember-cli and user apps have many dependencies, many of which require
    // process.addListener('exit', ....) for cleanup, by default this limit for
    // such listeners is 10, recently users have been increasing this and not to
    // their fault, rather they are including large and more diverse sets of
    // node_modules.
    //
    // https://github.com/babel/ember-cli-babel/issues/76
    _process.setMaxListeners(1000);

    // work around misbehaving libraries, so we can correctly cleanup before actually exiting.
    captureExit.captureExit();
  },

  /**
   * Drops all the interruption handlers and disables an ability to add new one
   *
   * Note: We don't call `captureExit.releaseExit() here.
   * In some rare scenarios it can lead to the hard to debug issues.
   * see: https://github.com/ember-cli/ember-cli/issues/6779#issuecomment-280940358
   *
   * We can more or less feel comfortable with a captured exit because it behaves very
   * similar to the original `exit` except of cases when we need to do cleanup before exit.
   *
   * @private
   * @method release
   */
  release() {
    while (handlers.length > 0) {
      this.removeHandler(handlers[0]);
    }

    _process = null;
  },

  /**
   * Add process interruption handler
   *
   * When the first handler is added then automatically
   * sets up process interruption signals listeners
   *
   * @private
   * @method addHandler
   * @param {function} cb   Callback to be called when process interruption fired
   */
  addHandler(cb) {
    if (!_process) {
      throw new Error('process is not captured');
    }

    let index = handlers.indexOf(cb);
    if (index > -1) { return; }

    if (handlers.length === 0) {
      setupSignalsTrap();
    }

    handlers.push(cb);
    captureExit.onExit(cb);
  },

  /**
   * Remove process interruption handler
   *
   * If there are no remaining handlers after removal
   * then clean up all the process interruption signal listeners
   *
   * @private
   * @method removeHandler
   * @param {function} cb   Callback to be removed
   */
  removeHandler(cb) {
    let index = handlers.indexOf(cb);
    if (index < 0) { return; }

    handlers.splice(index, 1);
    captureExit.offExit(cb);

    if (handlers.length === 0) {
      teardownSignalsTrap();
    }
  },
};

/**
 * Sets up listeners for interruption signals
 *
 * When one of these signals is caught than raise process.exit()
 * which enforces `capture-exit` to run registered interruption handlers
 *
 * @method setupSignalsTrap
 */
function setupSignalsTrap() {
  _process.on('SIGINT', exit);
  _process.on('SIGTERM', exit);
  _process.on('message', onMessage);

  if (isWindowsTTY(_process)) {
    trapWindowsSignals(_process);
  }
}

/**
 * Removes interruption signal listeners and tears down capture-exit
 *
 * @method teardownSignalsTrap
 */
function teardownSignalsTrap() {
  _process.removeListener('SIGINT', exit);
  _process.removeListener('SIGTERM', exit);
  _process.removeListener('message', onMessage);

  if (isWindowsTTY(_process)) {
    cleanupWindowsSignals(_process);
  }
}

/**
 * Suppresses "Terminate batch job (Y/N)" confirmation on Windows
 *
 * @method trapWindowsSignals
 */
function trapWindowsSignals(_process) {
  const stdin = _process.stdin;

  originalIsRaw = stdin.isRaw;

  // This is required to capture Ctrl + C on Windows
  stdin.setRawMode(true);

  windowsCtrlCTrap = function(data) {
    if (data.length === 1 && data[0] === 0x03) {
      _process.emit('SIGINT');
    }
  };
  stdin.on('data', windowsCtrlCTrap);
}

function cleanupWindowsSignals(_process) {
  const stdin = _process.stdin;

  stdin.setRawMode(originalIsRaw);

  stdin.removeListener('data', windowsCtrlCTrap);
}

function isWindowsTTY(_process) {
  return (/^win/).test(_process.platform) && _process.stdin && _process.stdin.isTTY;
}

function exit() {
  _process.exit();
}

function onMessage(message) {
  if (message.kill) {
    exit();
  }
}
