"use strict";

let fs = require("fs");
let path = require("path");
let cleanUpPath = require("clean-up-path");
let rimraf = require("rimraf");
let canSymlink = require("can-symlink");
let heimdall = require("heimdalljs");
let loggerGen = require("heimdalljs-logger");

let cleanUpResolvedPath = cleanUpPath.cleanUpResolvedPath;
let isResolved = cleanUpPath.isResolved;

class DirectoryIndex extends Map {}

class Directory extends String {
  constructor(p, cleanedUp) {
    if (!cleanedUp) p = cleanUpPath(p);
    super(p);
  }

  getIndexSync() {
    if (this._index != null) return this._index;
    let index = new DirectoryIndex();
    let p = this.valueOf();
    let base = p;
    if (base[base.length - 1] !== path.sep && base[base.length - 1] !== "/") {
      base += path.sep;
    }
    for (let entry of fs.readdirSync(p).sort()) {
      index.set(entry, new makeFSObjectCleanedUp(base + entry));
    }
    this._index = index;
    return index;
  }

  inspect() {
    return `[Directory: '${this.valueOf()}']`;
  }
}

class File extends String {
  constructor(p, _cleanedUp, _stats) {
    if (!_cleanedUp) p = cleanUpPath(p);
    super(p);
    if (_stats != null) this._stats = _stats;
  }

  inspect() {
    return `[File: '${this.valueOf()}']`;
  }

  stat() {
    if (this._stats != null) return this._stats;
    return (this._stats = fs.statSync(this.valueOf()));
  }
}

if (!(new File("", true) instanceof File)) {
  throw new Error("The fs-updater package requires Node 6.0.0 or newer.");
}

function makeFSObject(p) {
  return makeFSObjectCleanedUp(cleanUpPath(p));
}

function makeFSObjectCleanedUp(p) {
  let stats = fs.lstatSync(p);
  if (stats.isDirectory()) return new Directory(p, true);
  if (stats.isFile()) return new File(p, true, stats);
  // Return FSObject pointing to target of symbolic link. This is so you can use
  // the returned FSObject to create a symlink without symlink indirection
  // growing out of control.
  if (!stats.isSymbolicLink())
    throw new Error("File has unexpected type: " + p);
  let target = fs.readlinkSync(p);
  if (!isResolved(target)) {
    // We expect most symlinks coming from other plugins to be resolved already,
    // so we optimistically try readlink and only use the slower realpath as a
    // fallback here. Note that we cannot use path.resolve here because it does
    // not handle `symlinked_directory/..` correctly.
    target = fs.realpathSync(p);
  }
  target = cleanUpResolvedPath(target);
  return makeFSObjectCleanedUp(target);
}

class FSUpdater {
  constructor(outputPath, options) {
    this.outputPath = outputPath;
    if (options == null) options = {};
    if (options.canSymlink == null) options.canSymlink = canSymlink();
    if (options.retry == null) options.retry = true;
    this.options = options;
    this._logger = loggerGen("FSUpdater");

    if (fs.existsSync(outputPath)) {
      fs.rmdirSync(outputPath);
    }
    this.state = null;
  }

  update(dir) {
    let instrumentation = heimdall.start("FSUpdater::update");
    this._update(dir, this.options.retry);
    instrumentation.stop();
  }

  _update(dir, retry) {
    if (this.state === ERROR_STATE) {
      rimraf.sync(this.outputPath);
      this.state = null;
    }

    try {
      update(this.outputPath, this.state, dir, this.options);
      this.state = dir;
    } catch (err) {
      this.state = ERROR_STATE;
      if (!retry) throw err;
      this._update(dir, false);
      this._logger.warn(
        "Incremental updating failed, but FSUpdater was able to recover by\n" +
          "rebuilding the output from scratch. This can be caused by\n" +
          "\n" +
          "* a bug in the code calling FSUpdater, or\n" +
          "* a bug in FSUpdater itself.\n" +
          "\n" +
          "Fixing this issue will improve performance.\n" +
          "\n" +
          "Original stack trace:\n" +
          "\n" +
          err.stack
      );
    }
  }
}

const ERROR_STATE = Symbol("error");

function update(outputPath, oldState, newState, options) {
  // Identical objects are presumed to have no changes
  if (oldState === newState) return;
  // Unchanged symlinks do not need updating
  if (
    options.canSymlink &&
    ((oldState instanceof File && newState instanceof File) ||
      (oldState instanceof Directory && newState instanceof Directory)) &&
    oldState.valueOf() === newState.valueOf()
  )
    return;
  // If we cannot symlink, then if we have kept the old file stats around in
  // oldState._stats, we can skip re-copying if the file is unchanged.
  if (
    !options.canSymlink &&
    (oldState instanceof File && newState instanceof File) &&
    oldState.valueOf() === newState.valueOf() &&
    oldState._stats != null &&
    fileStatsEqual(oldState._stats, newState.stat())
  )
    return;

  if (
    !(newState instanceof DirectoryIndex && oldState instanceof DirectoryIndex)
  ) {
    // Delete old state
    if (
      oldState instanceof DirectoryIndex ||
      (!options.canSymlink && oldState instanceof Directory)
    ) {
      rimraf.sync(outputPath);
    } else if (oldState != null) {
      fs.unlinkSync(outputPath);
    }
  }

  if (newState instanceof DirectoryIndex) {
    if (oldState instanceof DirectoryIndex) {
      for (let mapEntry of oldState) {
        let entryName = mapEntry[0];
        let entryState = mapEntry[1];
        if (newState.has(entryName)) continue;
        update(`${outputPath}/${entryName}`, entryState, null, options);
      }
    } else {
      fs.mkdirSync(outputPath);
      oldState = new DirectoryIndex();
    }
    for (let mapEntry of newState) {
      let entryName = mapEntry[0];
      let entryState = mapEntry[1];
      update(
        `${outputPath}${path.sep}${entryName}`,
        oldState.get(entryName),
        entryState,
        options
      );
    }
  } else if (newState instanceof Directory) {
    if (options.canSymlink) {
      fs.symlinkSync(newState.valueOf(), outputPath, "dir");
    } else {
      fs.symlinkSync(newState.valueOf(), outputPath, "junction");
    }
  } else if (newState instanceof File) {
    if (options.canSymlink) {
      fs.symlinkSync(newState.valueOf(), outputPath, "file");
    } else {
      let stats = newState.stat();
      let contents = fs.readFileSync(newState.valueOf());
      fs.writeFileSync(outputPath, contents, {
        flag: "wx",
        mode: stats.mode
      });
      fs.utimesSync(outputPath, stats.atime, stats.mtime);
    }
  } else {
    if (newState != null)
      throw new Error(
        "Expected File, Directory or DirectoryIndex, got " + newState
      );
  }
}

function fileStatsEqual(a, b) {
  // Note that stats.mtimeMs is only available as of Node 9
  return (
    a.ino === b.ino &&
    a.mtime.getTime() === b.mtime.getTime() &&
    a.size === b.size &&
    a.mode === b.mode
  );
}

FSUpdater.DirectoryIndex = DirectoryIndex;
FSUpdater.Directory = Directory;
FSUpdater.File = File;
FSUpdater.makeFSObject = makeFSObject;
FSUpdater.makeFSObjectCleanedUp = makeFSObjectCleanedUp;

module.exports = FSUpdater;
