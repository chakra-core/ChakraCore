'use strict';

/**
 * Checks if the string starts with a number.
 *
 * @method startsWithNumber
 * @return {Boolean}
*/
function startsWithNumber(name) {
  return (/^\d.*$/).test(name);
}

/**
 * Checks if project name is valid.
 *
 * Invalid names are some of the internal constants that Ember CLI uses, such as
 * `app`, `ember`, `ember-cli`, `test`, and `vendor`. Names that start with
 * numbers are considered invalid as well.
 *
 * @method validProjectName
 * @param {String} name The name of Ember CLI project
 * @return {Boolean}
*/
module.exports = function(name) {
  let invalidProjectNames = [
    'test',
    'ember',
    'ember-cli',
    'vendor',
    'public',
    'app',
  ];
  name = name.toLowerCase();

  if (invalidProjectNames.indexOf(name) !== -1 || name.indexOf('.') !== -1 || startsWithNumber(name)) {
    return false;
  }

  return true;
};
