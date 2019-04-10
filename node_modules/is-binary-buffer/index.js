'use strict';

var isBuffer = require('is-buffer');

module.exports = function isBinary(buf) {
  if (!isBuffer(buf)) {
    return false;
  }

  var size = buf.length;
  var MAX_BYTES = size < 512 ? size : 512;

  if (size === 0) {
    return false;
  }

  var suspicious_bytes = 0;

  // UTF-8 BOM
  if (size >= 3 && buf[0] == 0xef && buf[1] == 0xbb && buf[2] == 0xbf) {
    return false;
  }

  // UTF-32 BOM
  if (size >= 4 && buf[0] === 0x00 && buf[1] === 0x00 && buf[2] == 0xfe && buf[3] == 0xff) {
    return false;
  }

  // UTF-32 LE BOM
  if (size >= 4 && buf[0] == 0xff && buf[1] == 0xfe && buf[2] === 0x00 && buf[3] === 0x00) {
    return false;
  }

  // GB BOM
  if (size >= 4 && buf[0] == 0x84 && buf[1] == 0x31 && buf[2] == 0x95 && buf[3] == 0x33) {
    return false;
  }

  if (MAX_BYTES >= 5 && buf.slice(0, 5) == '%PDF-') {
    /* PDF. This is binary. */
    return true;
  }

  // UTF-16 BE BOM
  if (size >= 2 && buf[0] == 0xfe && buf[1] == 0xff) {
    return false;
  }

  // UTF-16 LE BOM
  if (size >= 2 && buf[0] == 0xff && buf[1] == 0xfe) {
    return false;
  }

  for (var i = 0; i < MAX_BYTES; i++) {
    if (buf[i] === 0) {
      // NULL byte--it's binary!
      return true;
    } else if ((buf[i] < 7 || buf[i] > 14) && (buf[i] < 32 || buf[i] > 127)) {
      // UTF-8 detection
      if (buf[i] > 193 && buf[i] < 224 && i + 1 < MAX_BYTES) {
        i++;
        if (buf[i] > 127 && buf[i] < 192) {
          continue;
        }
      } else if (buf[i] > 223 && buf[i] < 240 && i + 2 < MAX_BYTES) {
        i++;
        if (buf[i] > 127 && buf[i] < 192 && buf[i + 1] > 127 && buf[i + 1] < 192) {
          i++;
          continue;
        }
      }

      suspicious_bytes++;
      // Read at least 32 bytes before making a decision
      if (i > 32 && suspicious_bytes * 100 / MAX_BYTES > 10) {
        return true;
      }
    }
  }

  if (suspicious_bytes * 100 / MAX_BYTES > 10) {
    return true;
  }

  return false;
};
