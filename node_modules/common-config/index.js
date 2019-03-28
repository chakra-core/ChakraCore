'use strict';

module.exports = global.COMMON_CONFIG || (global.COMMON_CONFIG = require('data-store')('common-config'));
