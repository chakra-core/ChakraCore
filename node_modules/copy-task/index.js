/*!
 * copy-task <https://github.com/doowb/copy-task>
 *
 * Copyright (c) 2016, Brian Woodward.
 * Licensed under the MIT License.
 */

'use strict';

/**
 * Copy a task and it's dependencies from one app to another.
 *
 * ```js
 * copy(app1, app2, 'default');
 * ```
 *
 * @param  {Object} `from` app to copy the task from
 * @param  {Object} `to` app to copy the task to
 * @param  {String} `name` name of task to copy
 * @api public
 */

module.exports = function copy(from, to, name) {
  if (typeof from.task !== 'function') {
    throw new Error('expected `from` to use `composer`.');
  }

  if (typeof to.task !== 'function') {
    throw new Error('expected `to` to use `composer`.');
  }

  if (typeof name !== 'string') {
    throw new TypeError('expected `name` to be a string');
  }

  if (!(name in from.tasks)) {
    throw new Error('"' + name + '" not found in tasks');
  }

  var task = from.tasks[name];
  var deps = task.deps || [];
  to.task(name, task.options, task.deps, task.fn);
  deps.forEach(function(dep) {
    copy(from, to, dep);
  });
};
