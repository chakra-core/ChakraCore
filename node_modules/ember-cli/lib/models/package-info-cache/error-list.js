'use strict';

const ErrorEntry = require('./error-entry');


/*
 * Small utility class to store a list of errors during loading of
 * a package into the PackageInfoCache.
 *
 * @protected
 * @class ErrorList
 */
module.exports = class ErrorList {
  constructor() {
    this.errors = [];
  }

  /*
   * Add an error. The errorData object is optional, and can be anything.
   * We do this so we don't really need to create a series of error
   * classes.
   *
   * @public
   * @param {String} errorType one of the Errors.ERROR_* constants.
   * @param {Object} errorData any error data relevant to the type of error
   * being created. See showErrors().
   */
  addError(errorType, errorData) {
    this.errors.push(new ErrorEntry(errorType, errorData));
  }

  getErrors() {
    return this.errors;
  }

  hasErrors() {
    return this.errors.length > 0;
  }
};
