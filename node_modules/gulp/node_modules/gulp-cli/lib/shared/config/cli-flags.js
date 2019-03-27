'use strict';

var copyProps = require('copy-props');

var fromTo = {
  'flags.silent': 'silent',
  'flags.continue': 'continue',
  'flags.series': 'series',
  'flags.logLevel': 'logLevel',
  'flags.compactTasks': 'compactTasks',
  'flags.tasksDepth': 'tasksDepth',
  'flags.sortTasks': 'sortTasks',
};

function mergeConfigToCliFlags(opt, config) {
  return copyProps(config, opt, fromTo);
}

module.exports = mergeConfigToCliFlags;
