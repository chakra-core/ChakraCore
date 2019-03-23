'use strict';

module.exports = function emberCLIBabelConfigKey(emberCLIBabelInstance) {
  // future versions of ember-cli-babel will be moving the location for its
  // own configuration options out of `babel` and will be issuing a deprecation
  // if used in the older way
  //
  // see: https://github.com/babel/ember-cli-babel/pull/105
  let emberCLIBabelConfigKey = (emberCLIBabelInstance && emberCLIBabelInstance.configKey) || 'babel';

  return emberCLIBabelConfigKey;
};
