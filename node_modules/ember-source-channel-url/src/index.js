'use strict';
const got = require('got');

module.exports = function(channelType) {
  let HOST = process.env.EMBER_SOURCE_CHANNEL_URL_HOST || 'https://s3.amazonaws.com';
  let PATH = 'builds.emberjs.com';

  return got(`${HOST}/${PATH}/${channelType}.json`, { json: true }).then(
    result => `${HOST}/${PATH}${result.body.assetPath}`
  );
};
