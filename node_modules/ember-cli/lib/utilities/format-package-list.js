'use strict';

module.exports = function formatPackageList(packages) {
  if (Array.isArray(packages)) {
    if (packages.length === 1) {
      return packages[0];
    } else if (packages.length === 2) {
      return `${packages[0]} and ${packages[1]}`;
    } else if (packages.length === 3) {
      return `${packages[0]}, ${packages[1]} and ${packages[2]}`;
    } else if (packages.length > 3) {
      return `${packages[0]}, ${packages[1]} and ${packages.length - 2} other packages`;
    }
  }

  return 'dependencies';
};
