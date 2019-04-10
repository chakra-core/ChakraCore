'use strict';

const CancelationError = require('./errors/cancelation');

module.exports = class CancelationRequest {
  constructor(pendingWork) {
    this._pendingWork = pendingWork; // all
    this._canceling = null;
  }

  get isCanceled() {
    return !!this._canceling;
  }

  throwIfRequested() {
    if (this.isCanceled) {
      throw new CancelationError('Build Canceled');
    }
  }

  then() {
    return this._pendingWork.then(...arguments);
  }

  cancel() {
    if (this._canceling) {
      return this._canceling;
    }

    this._canceling = this._pendingWork.catch(e => {
      if (CancelationError.isCancelationError(e)) {
        return;
      } else {
        throw e;
      }
    });
    return this._canceling;
  }
};
