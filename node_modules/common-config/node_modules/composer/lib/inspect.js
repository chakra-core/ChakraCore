'use strict';

module.exports = function inspect(app, task) {
  if (app.options && app.options.inspectFn === false) {
    return;
  }

  if (app.options && typeof app.options.inspectFn === 'function') {
    task.inspect = function() {
      return app.options.inspectFn.call(app, task);
    };
    return;
  }

  task.inspect = function() {
    var list = '[' + task.options.deps.join(', ') + ']';
    return '<Task "' + task.name + '" deps: ' + list + '>';
  };
};
