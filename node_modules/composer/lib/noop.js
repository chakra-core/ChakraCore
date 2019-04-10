'use strict';

/**
 * Default noop task to use when tasks only contain compositions.
 *
 * @param  {Function} `done` callback that does nothing.
 * @return {*}
 */

module.exports = function(done) {
  return done();
};
