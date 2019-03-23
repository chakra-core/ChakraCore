'use strict';

const which = require('which');

module.exports = which.sync('yarn', { nothrow: true });
