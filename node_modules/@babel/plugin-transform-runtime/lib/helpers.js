"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.hasMinVersion = hasMinVersion;

function _semver() {
  const data = _interopRequireDefault(require("semver"));

  _semver = function () {
    return data;
  };

  return data;
}

function _interopRequireDefault(obj) { return obj && obj.__esModule ? obj : { default: obj }; }

function hasMinVersion(minVersion, runtimeVersion) {
  if (!runtimeVersion) return true;
  if (_semver().default.valid(runtimeVersion)) runtimeVersion = `^${runtimeVersion}`;
  return !_semver().default.intersects(`<${minVersion}`, runtimeVersion) && !_semver().default.intersects(`>=8.0.0`, runtimeVersion);
}