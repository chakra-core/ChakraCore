/*!
 * base-task <https://github.com/node-base/base-task>
 *
 * Copyright (c) 2015, 2017, Jon Schlinkert.
 * Released under the MIT License.
 */

'use strict';


module.exports = function(name, fn) {
  var isValid = require('is-valid-app');
  var composer = require('composer');

  if (typeof name === 'function') {
    fn = name;
    name = undefined;
  }

  return function baseTask(app) {
    if (!isValid(app, 'base-task')) return;

    // original constructor reference
    var ctor = this.constructor;
    composer.call(this, name);
    this.visit('define', composer.prototype);
    var self = this;
    var emit = this.emit;

    // fix build events
    this.on('starting', function(app, build) {
      build.status = 'starting';
      self.emit('build', build);
    });
    this.on('finished', function(app, build) {
      build.status = 'finished';
      self.emit('build', build);
    });

    // fix task events
    this.emit = function(key, val, task) {
      if (key === 'task' && typeof val === 'undefined' && task) {
        task.status = 'register';
        self.emit('task', task);
        return;
      }
      return emit.apply(self, arguments);
    };
    this.on('task:starting', function(app, task) {
      task.status = 'starting';
      self.emit('task', task);
    });
    this.on('task:finished', function(app, task) {
      task.status = 'finished';
      self.emit('task', task);
    });

    // restore original constructor
    this.constructor = ctor;
    return baseTask;
  };
};

