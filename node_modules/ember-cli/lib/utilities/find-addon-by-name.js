'use strict';

function unscope(name) {
  if (name[0] !== '@') {
    return name;
  }

  return name.slice(name.indexOf('/') + 1);
}

let HAS_FOUND_ADDON_BY_NAME = Object.create(null);
let HAS_FOUND_ADDON_BY_UNSCOPED_NAME = Object.create(null);
/*
  Finds an addon given a specific name. Due to older versions of ember-cli
  not properly supporting scoped packages it was (at one point) common practice
  to have `package.json` include a scoped name but `index.js` having an unscoped
  name.

  Changes to the blueprints and addon model (in ~ 3.4+) have made it much clearer
  that both `package.json` and `index.js` `name`'s should match. At some point
  this will be "forced" and no longer optional.

  This function is attempting to prioritize matching across all of the combinations
  (in this priority order):

  - package.json name matches requested name exactly
  - index.js name matches requested name exactly
  - unscoped (leaf portion) index.js name matches unscoped requested name
*/
module.exports = function findAddonByName(addons, name) {
  let unscopedName = unscope(name);
  let exactMatchFromPkg = addons.find(addon => addon.pkg && addon.pkg.name === name);

  if (exactMatchFromPkg) {
    return exactMatchFromPkg;
  }

  let exactMatchFromIndex = addons.find(addon => addon.name === name);
  if (exactMatchFromIndex) {
    let pkg = exactMatchFromIndex.pkg;

    if (HAS_FOUND_ADDON_BY_NAME[name] !== true) {
      HAS_FOUND_ADDON_BY_NAME[name] = true;
      console.warn(`The addon at \`${exactMatchFromIndex.root}\` has different values in its addon index.js ('${exactMatchFromIndex.name}') and its package.json ('${pkg && pkg.name}').`);
    }

    return exactMatchFromIndex;
  }


  let unscopedMatchFromIndex = addons.find(addon => addon.name && unscope(addon.name) === unscopedName);
  if (unscopedMatchFromIndex) {
    if (HAS_FOUND_ADDON_BY_UNSCOPED_NAME[name] !== true) {
      HAS_FOUND_ADDON_BY_UNSCOPED_NAME[name] = true;
      console.trace(`Finding a scoped addon via its unscoped name is deprecated. You searched for \`${name}\` which we found as \`${unscopedMatchFromIndex.name}\` in '${unscopedMatchFromIndex.root}'`);
    }

    return unscopedMatchFromIndex;
  }

  return null;
};

module.exports._clearCaches = function() {
  HAS_FOUND_ADDON_BY_NAME = Object.create(null);
  HAS_FOUND_ADDON_BY_UNSCOPED_NAME = Object.create(null);
};
