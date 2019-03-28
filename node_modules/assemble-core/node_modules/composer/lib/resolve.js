'use strict';

var utils = require('./utils');

/**
 * Resolve the arguments by looking up tasks and their dependencies.
 * This creates an array of composed functions to be run together.
 *
 * ```js
 * // bind the composer to the resolve call
 * var tasks = resolve.call(this, arr);
 * ```
 *
 * @param  {Array} `arr` flattened array of strings and functions to resolve.
 * @return {Array} Return array of composed functions to run.
 */

module.exports = function(arr) {
  var len = arr.length, i = 0;
  var fns = [];
  var options = {};
  if (len && utils.isobject(arr[len - 1])) {
    options = arr.pop();
    len--;
  }

  while (len--) {
    var str = arr[i++];
    if (isGlob(str)) {
      var obj = utils.mm.matchKeys(this.tasks, str);
      var keys = Object.keys(obj);
      if (!keys.length) {
        throw new Error('glob pattern "' + str + '" did not match any tasks');
      }
      for (var j = 0; j < keys.length; j++) {
        fns.push(getTaskFn(this.tasks, keys[j], options));
      }
      continue;
    }
    if (typeof str === 'string') {
      str = getTaskFn(this.tasks, str, options);
    }
    fns.push(str);
  }
  return fns;
};

var re = /\[anonymous \(\d*\)\]/;
function isGlob(str) {
  return utils.isGlob(str) && (!re.test(str));
}

function getTaskFn(tasks, key, options) {
  var task = tasks[key];
  if (!task) {
    throw new Error('task "' + key + '" is not registered');
  }
  task.options = utils.extend({}, task.options, options);
  return task.run.bind(task);
}
