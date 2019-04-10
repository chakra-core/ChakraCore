'use strict';

const stringUtil = require('ember-cli-string-utils');

module.exports = {
  description: 'Generates an ES6 module shim for global libraries.',

  locals(options) {
    let entity = options.entity;
    let rawName = entity.name;
    let name = stringUtil.dasherize(rawName);

    return {
      name,
    };
  },
};
