'use strict';

module.exports = function cleanExit(code) {
  // Workaround for this node core bug <https://github.com/joyent/node/issues/3584>
  // Instead of using `process.exit(?code)`, use this instead.
  //
  let draining = 0;
  function exit() {
    if (!(draining--)) {
      process.exit(code);
    }
  }

  let streams = [process.stdout, process.stderr];
  streams.forEach(stream => {
    // submit empty write request and wait for completion
    draining += 1;
    stream.write('', exit);
  });
  exit();
};
