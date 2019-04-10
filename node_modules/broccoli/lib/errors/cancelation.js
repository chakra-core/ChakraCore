'use strict';

// Base class for builder errors
module.exports = class Cancelation extends Error {
  static isCancelationError(e) {
    return typeof e === 'object' && e !== null && e.isCancelation === true;
  }

  constructor(a, b, c) {
    // Subclassing Error in ES5 is non-trivial because reasons, so we need this
    // extra constructor logic from http://stackoverflow.com/a/17891099/525872.
    // Note that ES5 subclasses of BuilderError don't in turn need any special
    // code.
    let temp = super(a, b, c);
    // Need to assign temp.name for correct error class in .stack and .message
    temp.name = this.name = this.constructor.name;
    this.stack = temp.stack;
    this.message = temp.message;

    this.isCancelation = true;
    this.isSilent = true;
  }
};
