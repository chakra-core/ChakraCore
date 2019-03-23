/* eslint no-underscore-dangle:0 */

'use strict';

const debug = require('debug')('log4js:logger');
const LoggingEvent = require('./LoggingEvent');
const levels = require('./levels');
const clustering = require('./clustering');
const categories = require('./categories');
const configuration = require('./configuration');

/**
 * Logger to log messages.
 * use {@see log4js#getLogger(String)} to get an instance.
 *
 * @name Logger
 * @namespace Log4js
 * @param name name of category to log to
 * @param level - the loglevel for the category
 * @param dispatch - the function which will receive the logevents
 *
 * @author Stephan Strittmatter
 */
class Logger {
  constructor(name) {
    if (!name) {
      throw new Error('No category provided.');
    }
    this.category = name;
    this.context = {};
    debug(`Logger created (${this.category}, ${this.level})`);
  }

  get level() {
    return levels.getLevel(categories.getLevelForCategory(this.category), levels.TRACE);
  }

  set level(level) {
    categories.setLevelForCategory(this.category, levels.getLevel(level, this.level));
  }

  log(level, ...args) {
    const logLevel = levels.getLevel(level, levels.INFO);
    if (this.isLevelEnabled(logLevel)) {
      this._log(logLevel, args);
    }
  }

  isLevelEnabled(otherLevel) {
    return this.level.isLessThanOrEqualTo(otherLevel);
  }

  _log(level, data) {
    debug(`sending log data (${level}) to appenders`);
    const loggingEvent = new LoggingEvent(this.category, level, data, this.context);
    clustering.send(loggingEvent);
  }

  addContext(key, value) {
    this.context[key] = value;
  }

  removeContext(key) {
    delete this.context[key];
  }

  clearContext() {
    this.context = {};
  }
}

function addLevelMethods(target) {
  const level = levels.getLevel(target);

  const levelStrLower = level.toString().toLowerCase();
  const levelMethod = levelStrLower.replace(/_([a-z])/g, g => g[1].toUpperCase());
  const isLevelMethod = levelMethod[0].toUpperCase() + levelMethod.slice(1);

  Logger.prototype[`is${isLevelMethod}Enabled`] = function () {
    return this.isLevelEnabled(level);
  };

  Logger.prototype[levelMethod] = function (...args) {
    this.log(level, ...args);
  };
}

levels.levels.forEach(addLevelMethods);

configuration.addListener(() => {
  levels.levels.forEach(addLevelMethods);
});

module.exports = Logger;
