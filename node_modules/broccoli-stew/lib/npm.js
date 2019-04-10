'use strict';

const path = require('path');
const Funnel = require('broccoli-funnel');
const resolve = require('./utils/require-resolve');

/**
 * Return tree containing the file referenced in package.json.
 * Optionally, you can rename the file in the returned tree.
 *
 * given a module called foo with the following package.json
 * {
 *   name: "foo",
 *   main: "lib/index.js"
 * }
 *
 * npm.main('foo', undefined, __dirname) => [ 'index.js' ]
 * npm.main('foo', 'foo.js', __dirname) => [ 'foo.js' ]
 * npm.main('foo', 'foo/index.js', __dirname) => [ 'foo/index.js' ]
 *
 * @param moduleName of module to get script from
 * @param newName relative path to rename the retrieved file
 * @param basedir String relative which directory should we require.resolve
 * @returns {*}
 */
module.exports.main = function main(packageName, outputFileName, basedir) {
  let mainPath = resolve(packageName, basedir || process.cwd());

  let inputPath = path.dirname(mainPath);
  let fileName = path.basename(mainPath);

  return new Funnel(inputPath, {
    files: [fileName],
    getDestinationPath() {
      return outputFileName || fileName;
    }
  });
};
