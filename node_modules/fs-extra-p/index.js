"use strict"

const fsExtra = require("fs-extra-tf")
const Promise = require('bluebird')

// returns new name
function rename(oldPath, newPath) {
  return new Promise((resolve, reject) => {
    fsExtra.rename(oldPath, newPath, error => error == null ? resolve(newPath) : reject(error))
  })
}

// returns copied name
function copy(src, dest, options) {
  return new Promise((resolve, reject) => {
    fsExtra.copy(src, dest, options, error => error == null ? resolve(dest) : reject(error))
  })
}

function deleteFile(path, ignoreIfNotExists) {
  return new Promise((resolve, reject) => {
    fsExtra.unlink(path, it => it == null || (ignoreIfNotExists && it.code === "ENOENT") ? resolve(null) : reject(it))
  })
}

function makeFs(Promise) {
  const fs = {
    rename: rename,
    copy: copy,
    deleteFile: deleteFile,
  }


  for (let methodName of Object.keys(fsExtra)) {
    const method = fsExtra[methodName]
    if (methodName === "createFile" || methodName === "copy" || methodName === "rename" || methodName === "mkdirp") {
      continue
    }

    if (typeof method !== 'function' ||
        methodName.endsWith("Sync") ||
        methodName.endsWith("Stream") ||
        methodName.match(/^[A-Z]/) ||
        ["exists", "watch", "watchFile", "unwatchFile"].indexOf(methodName) != -1) {
      fs[methodName] = method
    }
    else {
      fs[methodName] = Promise.promisify(method)
    }
  }

  fs.createFile = fs.ensureFile
  fs.mkdirp = fs.mkdirs
  return fs
}

module.exports = makeFs(Promise)