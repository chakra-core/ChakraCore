'use strict';

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.createWindowsInstaller = undefined;

var _bluebird = require('bluebird');

let createWindowsInstaller = exports.createWindowsInstaller = (() => {
  var _ref = (0, _bluebird.coroutine)(function* (options) {
    let useMono = false;

    const monoExe = 'mono';
    const wineExe = 'wine';

    if (process.platform !== 'win32') {
      useMono = true;
      if (!wineExe || !monoExe) {
        throw new Error('You must install both Mono and Wine on non-Windows');
      }

      log(`Using Mono: '${monoExe}'`);
      log(`Using Wine: '${wineExe}'`);
    }

    let appDirectory = options.appDirectory,
        outputDirectory = options.outputDirectory,
        loadingGif = options.loadingGif;

    outputDirectory = _path2.default.resolve(outputDirectory || 'installer');

    const vendorPath = _path2.default.join(__dirname, '..', 'vendor');
    const vendorUpdate = _path2.default.join(vendorPath, 'Update.exe');
    const appUpdate = _path2.default.join(appDirectory, 'Update.exe');

    yield fsUtils.copy(vendorUpdate, appUpdate);
    if (options.setupIcon && options.skipUpdateIcon !== true) {
      let cmd = _path2.default.join(vendorPath, 'rcedit.exe');
      let args = [appUpdate, '--set-icon', options.setupIcon];

      if (useMono) {
        args.unshift(cmd);
        cmd = wineExe;
      }

      yield (0, _spawnPromise2.default)(cmd, args);
    }

    const defaultLoadingGif = _path2.default.join(__dirname, '..', 'resources', 'install-spinner.gif');
    loadingGif = loadingGif ? _path2.default.resolve(loadingGif) : defaultLoadingGif;

    let certificateFile = options.certificateFile,
        certificatePassword = options.certificatePassword,
        remoteReleases = options.remoteReleases,
        signWithParams = options.signWithParams,
        remoteToken = options.remoteToken;


    const metadata = {
      description: '',
      iconUrl: 'https://raw.githubusercontent.com/atom/electron/master/atom/browser/resources/win/atom.ico'
    };

    if (options.usePackageJson !== false) {
      const appResources = _path2.default.join(appDirectory, 'resources');
      const asarFile = _path2.default.join(appResources, 'app.asar');
      let appMetadata;

      if (yield fsUtils.fileExists(asarFile)) {
        appMetadata = JSON.parse(_asar2.default.extractFile(asarFile, 'package.json'));
      } else {
        appMetadata = JSON.parse((yield fsUtils.readFile(_path2.default.join(appResources, 'app', 'package.json'), 'utf8')));
      }

      Object.assign(metadata, {
        exe: `${appMetadata.name}.exe`,
        title: appMetadata.productName || appMetadata.name
      }, appMetadata);
    }

    Object.assign(metadata, options);

    if (!metadata.authors) {
      if (typeof metadata.author === 'string') {
        metadata.authors = metadata.author;
      } else {
        metadata.authors = (metadata.author || {}).name || '';
      }
    }

    metadata.owners = metadata.owners || metadata.authors;
    metadata.version = convertVersion(metadata.version);
    metadata.copyright = metadata.copyright || `Copyright © ${new Date().getFullYear()} ${metadata.authors || metadata.owners}`;

    let templateData = yield fsUtils.readFile(_path2.default.join(__dirname, '..', 'template.nuspectemplate'), 'utf8');
    if (_path2.default.sep === '/') {
      templateData = templateData.replace(/\\/g, '/');
    }
    const nuspecContent = (0, _lodash2.default)(templateData)(metadata);

    log(`Created NuSpec file:\n${nuspecContent}`);

    const nugetOutput = yield fsUtils.createTempDir('si-');
    const targetNuspecPath = _path2.default.join(nugetOutput, metadata.name + '.nuspec');

    yield fsUtils.writeFile(targetNuspecPath, nuspecContent);

    let cmd = _path2.default.join(vendorPath, 'nuget.exe');
    let args = ['pack', targetNuspecPath, '-BasePath', appDirectory, '-OutputDirectory', nugetOutput, '-NoDefaultExcludes'];

    if (useMono) {
      args.unshift(cmd);
      cmd = monoExe;
    }

    // Call NuGet to create our package
    log((yield (0, _spawnPromise2.default)(cmd, args)));
    const nupkgPath = _path2.default.join(nugetOutput, `${metadata.name}.${metadata.version}.nupkg`);

    if (remoteReleases) {
      cmd = _path2.default.join(vendorPath, 'SyncReleases.exe');
      args = ['-u', remoteReleases, '-r', outputDirectory];

      if (useMono) {
        args.unshift(cmd);
        cmd = monoExe;
      }

      if (remoteToken) {
        args.push('-t', remoteToken);
      }

      log((yield (0, _spawnPromise2.default)(cmd, args)));
    }

    cmd = _path2.default.join(vendorPath, 'Update.com');
    args = ['--releasify', nupkgPath, '--releaseDir', outputDirectory, '--loadingGif', loadingGif];

    if (useMono) {
      args.unshift(_path2.default.join(vendorPath, 'Update-Mono.exe'));
      cmd = monoExe;
    }

    if (signWithParams) {
      args.push('--signWithParams');
      args.push(signWithParams);
    } else if (certificateFile && certificatePassword) {
      args.push('--signWithParams');
      args.push(`/a /f "${_path2.default.resolve(certificateFile)}" /p "${certificatePassword}"`);
    }

    if (options.setupIcon) {
      args.push('--setupIcon');
      args.push(_path2.default.resolve(options.setupIcon));
    }

    if (options.noMsi) {
      args.push('--no-msi');
    }

    if (options.noDelta) {
      args.push('--no-delta');
    }

    log((yield (0, _spawnPromise2.default)(cmd, args)));

    if (options.fixUpPaths !== false) {
      log('Fixing up paths');

      if (metadata.productName || options.setupExe) {
        const setupPath = _path2.default.join(outputDirectory, options.setupExe || `${metadata.productName}Setup.exe`);
        const unfixedSetupPath = _path2.default.join(outputDirectory, 'Setup.exe');
        log(`Renaming ${unfixedSetupPath} => ${setupPath}`);
        yield fsUtils.rename(unfixedSetupPath, setupPath);
      }

      if (metadata.productName || options.setupMsi) {
        const msiPath = _path2.default.join(outputDirectory, options.setupMsi || `${metadata.productName}Setup.msi`);
        const unfixedMsiPath = _path2.default.join(outputDirectory, 'Setup.msi');
        if (yield fsUtils.fileExists(unfixedMsiPath)) {
          log(`Renaming ${unfixedMsiPath} => ${msiPath}`);
          yield fsUtils.rename(unfixedMsiPath, msiPath);
        }
      }
    }
  });

  return function createWindowsInstaller(_x) {
    return _ref.apply(this, arguments);
  };
})();

exports.convertVersion = convertVersion;

var _lodash = require('lodash.template');

var _lodash2 = _interopRequireDefault(_lodash);

var _spawnPromise = require('./spawn-promise');

var _spawnPromise2 = _interopRequireDefault(_spawnPromise);

var _asar = require('asar');

var _asar2 = _interopRequireDefault(_asar);

var _path = require('path');

var _path2 = _interopRequireDefault(_path);

var _fsUtils = require('./fs-utils');

var fsUtils = _interopRequireWildcard(_fsUtils);

function _interopRequireWildcard(obj) { if (obj && obj.__esModule) { return obj; } else { var newObj = {}; if (obj != null) { for (var key in obj) { if (Object.prototype.hasOwnProperty.call(obj, key)) newObj[key] = obj[key]; } } newObj.default = obj; return newObj; } }

function _interopRequireDefault(obj) { return obj && obj.__esModule ? obj : { default: obj }; }

const log = require('debug')('electron-windows-installer:main');

function convertVersion(version) {
  const parts = version.split('-');
  const mainVersion = parts.shift();

  if (parts.length > 0) {
    return [mainVersion, parts.join('-').replace(/\./g, '')].join('-');
  } else {
    return mainVersion;
  }
}