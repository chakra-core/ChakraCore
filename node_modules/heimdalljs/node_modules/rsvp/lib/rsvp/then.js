import { config } from './config';
import instrument from './instrument';
import {
  noop,
  subscribe,
  FULFILLED,
  REJECTED,
  invokeCallback
} from './-internal';

export default function then(onFulfillment, onRejection, label) {
  var parent = this;
  var state = parent._state;

  if (state === FULFILLED && !onFulfillment || state === REJECTED && !onRejection) {
    config.instrument && instrument('chained', parent, parent);
    return parent;
  }

  parent._onError = null;

  var child = new parent.constructor(noop, label);
  var result = parent._result;

  config.instrument && instrument('chained', parent, child);

  if (state) {
    var callback = arguments[state - 1];
    config.async(function(){
      invokeCallback(state, child, callback, result);
    });
  } else {
    subscribe(parent, child, onFulfillment, onRejection);
  }

  return child;
}
