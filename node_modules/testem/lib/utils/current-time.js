'use strict';

function currentTimeAsLocaleTimeString() {
  let dateTime = new Date();

  return dateTime.toLocaleTimeString();
}

module.exports = {
  asLocaleTimeString: currentTimeAsLocaleTimeString
};
