'use strict';

/**
 * This function just wraps some common prototype methods that
 * are exposed on collection, list and app.
 */

module.exports = function(Base, Ctor, CtorName) {

  /**
   * Inherit `Base`
   */

  Base.extend(Ctor);
  Base.bubble(Ctor, ['preInit', 'Init']);

  /**
   * Decorate static methods
   */

  require('./is')(Ctor);

  /**
   * Mixin prototype methods
   */

  require('./layout')(Ctor.prototype);
  require('./render')(Ctor.prototype);
  require('./errors')(Ctor.prototype, CtorName);
};
