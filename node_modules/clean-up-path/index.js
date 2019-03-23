"use strict";

let path = require("path");

// Instruct Win32 to suspend path parsing by prefixing the path with a \\?\.
// Fix for https://github.com/broccolijs/broccoli-merge-trees/issues/42
let WINDOWS_PREFIX = "\\\\?\\";

// The path must start with x: or // or \\.
function cleanUpResolvedPathWin32(p) {
  if (p[0] !== "\\" || p[1] !== "\\") {
    p = WINDOWS_PREFIX + p;
  }
  return p;
}

function cleanUpResolvedPathPosix(p) {
  return p;
}

let cleanUpResolvedPath =
  process.platform === "win32"
    ? cleanUpResolvedPathWin32
    : cleanUpResolvedPathPosix;

function isResolvedWin32(p) {
  return (
    path.win32.isAbsolute(p) &&
    // Not drive-relative
    (p[0] !== path.sep || p[1] === path.sep) &&
    // Path separators are normalized. This lets us prefix LFS paths like c:\foo
    // with \\?\ to allow for long paths; see
    // https://github.com/broccolijs/broccoli-merge-trees/issues/42
    p.indexOf("/") === -1
  );
}

function isResolvedPosix(p) {
  return (
    path.posix.isAbsolute(p) &&
    // No duplicate or trailing `/`. This causes issues on Windows Subsystem
    // Linux (WSL); see https://github.com/ember-cli/ember-cli/issues/6338 and
    // https://github.com/Microsoft/BashOnWindows/issues/1421
    p.indexOf("//") === -1 &&
    (p[p.length - 1] !== "/" || p === "/")
  );
}

let isResolved =
  process.platform === "win32" ? isResolvedWin32 : isResolvedPosix;

function cleanUpPath(p) {
  if (!isResolved(p)) p = path.resolve(p);
  p = cleanUpResolvedPath(p);
  return p;
}

module.exports = cleanUpPath;
module.exports.isResolved = isResolved;
module.exports.isResolvedPosix = isResolvedPosix;
module.exports.isResolvedWin32 = isResolvedWin32;
module.exports.cleanUpResolvedPath = cleanUpResolvedPath;
module.exports.cleanUpResolvedPathPosix = cleanUpResolvedPathPosix;
module.exports.cleanUpResolvedPathWin32 = cleanUpResolvedPathWin32;
