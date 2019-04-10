/*!
 * src-stream <https://github.com/doowb/src-stream>
 *
 * Copyright (c) 2015, Brian Woodward.
 * Licensed under the MIT License.
 */

'use strict';

var ms = require('merge-stream');
var through = require('through2');
var duplexify = require('duplexify');

/**
 * Wrap a source stream to passthrough any data
 * that's being written to it.
 *
 * ```js
 * var src = require('src-stream');
 *
 * // wrap something that returns a readable stream
 * var stream = src(plugin());
 *
 * fs.createReadStream('./package.json')
 *   .pipe(stream)
 *   .on('data', console.log)
 *   .on('end', function () {
 *     console.log();
 *     console.log('Finished');
 *     console.log();
 *   });
 * ```
 *
 * @param  {Stream} `stream` Readable stream to be wrapped.
 * @return {Stream} Duplex stream to handle reading and writing.
 * @api public
 */

function srcStream(stream) {
  var pass = through.obj();
  pass.setMaxListeners(0);

  var outputstream = duplexify.obj(pass, ms(stream, pass));
  outputstream.setMaxListeners(0);

  var isReading = false;
  outputstream.on('pipe', function (src) {
    isReading = true;
    src.on('end', function () {
      isReading = false;
      outputstream.end();
    });
  });

  stream.on('end', function () {
    if (!isReading) {
      outputstream.end();
    }
  });

  return outputstream;
}

/**
 * Expose `srcStream`
 */

module.exports = srcStream;
