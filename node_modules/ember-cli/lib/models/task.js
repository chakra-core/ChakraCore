'use strict';

const CoreObject = require('core-object');

class Task extends CoreObject {
  run(/*options*/) {
    throw new Error('Task needs to have run() defined.');
  }

  /**
   * Interrupt comamd with an exit code
   * Called when the process is interrupted from outside, e.g. CTRL+C or `process.kill()`
   *
   * @private
   * @method onInterrupt
   */
  onInterrupt() {
    // eslint-disable-next-line no-process-exit
    process.exit(1);
  }
}

module.exports = Task;
