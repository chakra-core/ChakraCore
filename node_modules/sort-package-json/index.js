#!/usr/bin/env node
const sortObjectKeys = require('sort-object-keys');
const detectIndent = require('detect-indent');

const sortOrder = [
  'name',
  'version',
  'private',
  'description',
  'keywords',
  'homepage',
  'bugs',
  'repository',
  'license',
  'author',
  'contributors',
  'files',
  'sideEffects',
  'main',
  'umd:main',
  'unpkg',
  'module',
  'source',
  'jsnext:main',
  'browser',
  'types',
  'typings',
  'style',
  'example',
  'examplestyle',
  'assets',
  'bin',
  'man',
  'directories',
  'workspaces',
  'scripts',
  'betterScripts',
  'husky',
  'pre-commit',
  'commitlint',
  'lint-staged',
  'config',
  'nodemonConfig',
  'browserify',
  'babel',
  'browserslist',
  'xo',
  'prettier',
  'eslintConfig',
  'eslintIgnore',
  'stylelint',
  'jest',
  'dependencies',
  'devDependencies',
  'peerDependencies',
  'bundledDependencies',
  'bundleDependencies',
  'optionalDependencies',
  'flat',
  'resolutions',
  'engines',
  'engineStrict',
  'os',
  'cpu',
  'preferGlobal',
  'publishConfig'
];
// See https://docs.npmjs.com/misc/scripts
const defaultNpmScripts = [
  'install',
  'pack',
  'prepare',
  'publish',
  'restart',
  'shrinkwrap',
  'start',
  'stop',
  'test',
  'uninstall',
  'version'
];

function sortPackageJson(packageJson, options = {}) {
  const determinedSortOrder = options.sortOrder || sortOrder;
  let wasString = false;
  let hasWindowsNewlines = false;
  let endCharacters = '';
  let indentLevel = 2;
  if (typeof packageJson === 'string') {
    wasString = true;
    indentLevel = detectIndent(packageJson).indent;
    if (packageJson.substr(-1) === '\n') {
      endCharacters = '\n';
    }
    const newlineMatch = packageJson.match(/(\r?\n)/);
    hasWindowsNewlines = (newlineMatch && newlineMatch[0]) === '\r\n';
    packageJson = JSON.parse(packageJson);
  }

  const prefixedScriptRegex = /^(pre|post)(.)/;
  const prefixableScripts = defaultNpmScripts.slice();
  if (typeof packageJson.scripts === 'object') {
    Object.keys(packageJson.scripts).forEach(script => {
      const prefixOmitted = script.replace(prefixedScriptRegex, '$2');
      if (
        packageJson.scripts[prefixOmitted] &&
        !prefixableScripts.includes(prefixOmitted)
      ) {
        prefixableScripts.push(prefixOmitted);
      }
    });
  }

  function sortSubKey(key, sortList, unique) {
    if (Array.isArray(packageJson[key])) {
      packageJson[key] = packageJson[key].sort();
      if (unique) {
        packageJson[key] = array_unique(packageJson[key]);
      }
      return;
    }
    if (typeof packageJson[key] === 'object') {
      packageJson[key] = sortObjectKeys(packageJson[key], sortList);
    }
  }
  function toSortKey(script) {
    const prefixOmitted = script.replace(prefixedScriptRegex, '$2');
    if (prefixableScripts.includes(prefixOmitted)) {
      return prefixOmitted;
    }
    return script;
  }
  /*             b
   *       pre | * | post
   *   pre  0  | - |  -
   * a  *   +  | 0 |  -
   *   post +  | + |  0
   */
  function compareScriptKeys(a, b) {
    if (a === b) return 0;
    const aScript = toSortKey(a);
    const bScript = toSortKey(b);
    if (aScript === bScript) {
      // pre* is always smaller; post* is always bigger
      // Covers: pre* vs. *; pre* vs. post*; * vs. post*
      if (a === `pre${aScript}` || b === `post${bScript}`) return -1;
      // The rest is bigger: * vs. *pre; *post vs. *pre; *post vs. *
      return 1;
    }
    return aScript < bScript ? -1 : 1;
  }
  function array_unique(array) {
    return array.filter((el, index, arr) => index == arr.indexOf(el));
  }
  sortSubKey('keywords', null, true);
  sortSubKey('homepage');
  sortSubKey('bugs', ['url', 'email']);
  sortSubKey('license', ['type', 'url']);
  sortSubKey('author', ['name', 'email', 'url']);
  sortSubKey('bin');
  sortSubKey('man');
  sortSubKey('directories', ['lib', 'bin', 'man', 'doc', 'example']);
  sortSubKey('repository', ['type', 'url']);
  sortSubKey('scripts', compareScriptKeys);
  sortSubKey('betterScripts', compareScriptKeys);
  sortSubKey('commitlint');
  sortSubKey('lint-staged');
  sortSubKey('config');
  sortSubKey('nodemonConfig');
  sortSubKey('browserify');
  sortSubKey('babel');
  sortSubKey('eslintConfig');
  sortSubKey('jest');
  sortSubKey('xo');
  sortSubKey('prettier');
  sortSubKey('dependencies');
  sortSubKey('devDependencies');
  sortSubKey('peerDependencies');
  sortSubKey('bundledDependencies');
  sortSubKey('bundleDependencies');
  sortSubKey('optionalDependencies');
  sortSubKey('resolutions');
  sortSubKey('engines');
  sortSubKey('engineStrict');
  sortSubKey('os');
  sortSubKey('cpu');
  sortSubKey('preferGlobal');
  sortSubKey('private');
  sortSubKey('publishConfig');
  packageJson = sortObjectKeys(packageJson, determinedSortOrder);
  if (wasString) {
    let result = JSON.stringify(packageJson, null, indentLevel) + endCharacters;
    if (hasWindowsNewlines) {
      result = result.replace(/\n/g, '\r\n');
    }
    return result;
  }
  return packageJson;
}
module.exports = sortPackageJson;
module.exports.sortPackageJson = sortPackageJson;
module.exports.sortOrder = sortOrder;

if (require.main === module) {
  const fs = require('fs');

  const filesToProcess = process.argv[2]
    ? process.argv.slice(2)
    : [`${process.cwd()}/package.json`];

  filesToProcess.forEach(filePath => {
    const packageJson = fs.readFileSync(filePath, 'utf8');
    const sorted = sortPackageJson(packageJson);
    if (sorted !== packageJson) {
      fs.writeFileSync(filePath, sorted, 'utf8');
      console.log(`${filePath} is sorted!`);
    }
  });
}
