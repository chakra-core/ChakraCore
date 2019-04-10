'use strict';

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.fileExists = exports.rename = exports.writeFile = exports.unlink = exports.readDir = exports.readFile = exports.createTempDir = exports.copy = undefined;

var _bluebird = require('bluebird');

let fileExists = exports.fileExists = (() => {
  var _ref = (0, _bluebird.coroutine)(function* (file) {
    let stats;

    try {
      stats = yield inspect(file);
      return stats.isFile();
    } catch (err) {
      log(err);
    }

    return false;
  });

  return function fileExists(_x) {
    return _ref.apply(this, arguments);
  };
})();

var _fsExtra = require('fs-extra');

var _temp = require('temp');

var _temp2 = _interopRequireDefault(_temp);

var _fs = require('fs');

var _fs2 = _interopRequireDefault(_fs);

function _interopRequireDefault(obj) { return obj && obj.__esModule ? obj : { default: obj }; }

const log = require('debug')('electron-windows-installer:fs-utils');

_temp2.default.track();

const copy = exports.copy = _bluebird.Promise.promisify(_fsExtra.copy);
const createTempDir = exports.createTempDir = _bluebird.Promise.promisify(_temp2.default.mkdir);
const readFile = exports.readFile = _bluebird.Promise.promisify(_fs2.default.readFile);
const readDir = exports.readDir = _bluebird.Promise.promisify(_fs2.default.readdir);
const unlink = exports.unlink = _bluebird.Promise.promisify(_fs2.default.unlink);
const writeFile = exports.writeFile = _bluebird.Promise.promisify(_fs2.default.writeFile);
const rename = exports.rename = _bluebird.Promise.promisify(_fs2.default.rename);

const inspect = _bluebird.Promise.promisify(_fs2.default.stat);