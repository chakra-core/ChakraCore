'use strict';

var path = require('path');

/*
 * Subclass of `Error`.
 *
 * Includes detailed information about the errors from
 * broccoli plugins, such as error type, code frames,
 * broccoli node name and package versions.
 */

module.exports = BroccoliBuildError;
function BroccoliBuildError(originalError, node) {
  var name = node.info.name;
  var message = 'Build Canceled: Broccoli Builder ran into an error with `' + name + '` plugin. 💥\n' + originalError.message;
  var error = Error.call(this, message);
  this.message = error.message;
  this.stack = originalError.stack;
  var broccoliBuilderPackage = require(path.join(__dirname, '../package.json'));
  /*
   * Setting versions to `null` for now as it requires
   * adding an option to broccoli-builder to identify
   * ember cli/ember/ember data versions.
   */
  this.broccoliPayload = {
    versions: {
      'broccoli-builder': broccoliBuilderPackage.version,
      node: process.version
    },
    broccoliNode: {
      nodeName: name,
      nodeAnnotation: node.info.annotation,
      instantiationStack: node.info.instantiationStack || ''
    },
    error: {
      message: originalError.message,
      stack: originalError.stack,
      errorType: originalError.type || 'Build Error',
      codeFrame: originalError.codeFrame || originalError.message,
      location: originalError.location || {}
    }
  };
  this.broccoliPayload.error.location.file = originalError.file;
  this.broccoliPayload.error.location.treeDir = originalError.treeDir;

  var location = originalError.location || originalError.loc;
  if (location) {
    this.broccoliPayload.error.location = location;
  } else {
    this.broccoliPayload.error.location.line = originalError.line;
    this.broccoliPayload.error.location.column = originalError.column;
  }

  // we need to set this flag to make sure builder does re-throw
  // and actually stops
  this.wasCanceled = true;
}

BroccoliBuildError.prototype = Object.create(Error.prototype);
BroccoliBuildError.prototype.constructor = BroccoliBuildError;
