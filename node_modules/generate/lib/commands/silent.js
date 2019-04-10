'use strict';

module.exports = function(app) {
  return function(val, key, config, next) {
    app.enable('silent');
    next();
  };
};
