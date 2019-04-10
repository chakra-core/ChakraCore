'use strict';

var commands = require('./commands/');

module.exports = function(app, options) {
  for (var key in commands) {
    if (commands.hasOwnProperty(key)) {
      app.cli.map(key, commands[key](app, options));
    }
  }
};
