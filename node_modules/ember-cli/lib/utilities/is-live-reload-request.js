'use strict';

const cleanBaseUrl = require('clean-base-url');
const deprecate = require('./deprecate');

module.exports = function isLiveReloadRequest(url, liveReloadPrefix) {
  let regex = /\/livereload$/gi;
  if (url === `${cleanBaseUrl(liveReloadPrefix)}livereload`) {
    return true;
  } else if (regex.test(url)) {
    //version needs to be updated according to the this PR (https://github.com/ember-cli/ember-cli-inject-live-reload/pull/55)
    //in master of ember-cli-inject-live-reload.
    deprecate(`Upgrade ember-cli-inject-live-reload version to 1.10.0 or above`, true);
    return true;
  }
  return false;
};
