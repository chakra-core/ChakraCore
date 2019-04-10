'use strict';

module.exports = function wrapPrimitiveErrors(err) {
  if (err !== null && typeof err === 'object') {
    return err;
  } else {
    // We could augment the message with " [string exception]" to indicate
    // that the stack trace is not useful, or even set the .stack to null.
    return new Error(err + '');
  }
};
