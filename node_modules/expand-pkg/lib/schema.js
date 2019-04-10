'use strict';

var expanders = require('./expanders');
var utils = require('./utils');

/**
 * Expand package.json using a schema.
 */

module.exports = function(options) {
  var pkg = new utils.Pkg(options);
  var schema = pkg.schema

    /**
     * Person fields
     */

    .field('owner', 'string', { normalize: expanders.owner })
    .field('author', ['object', 'string'], { normalize: expanders.person })
    .field('authors', 'array', { normalize: expanders.persons })
    .field('maintainers', 'array', { normalize: expanders.persons })
    .field('contributors', 'array', { normalize: expanders.persons })
    .field('collaborators', 'array', { normalize: expanders.persons })

    /**
     * Bugs, repo and license
     */

    .field('git', ['object', 'string'], {
      normalize: expanders.git
    });

  // Add fields defined on `options.fields`
  schema.addFields(options);
  return schema;
};
