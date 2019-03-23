'use strict';

const TapConsumer = require('./tap_consumer');

module.exports = class BrowserTapConsumer {
  constructor(socket, tapConsumer) {
    tapConsumer = tapConsumer || new TapConsumer();
    let stream = tapConsumer.stream;
    socket.on('tap', msg => {
      if (!stream.writable) {
        return;
      }
      stream.write(msg + '\n');
      if (msg.match(/^#\s+ok\s*$/) ||
        msg.match(/^#\s+fail\s+[0-9]+\s*$/)) {
        stream.end();
      }
    });
    return tapConsumer;
  }
};
