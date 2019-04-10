var fs = require('fs')
var path = require('path')
var mktemp = require('mktemp')
var rimraf = require('rimraf')
var underscoreString = require('underscore.string')

exports.makeOrRemake = makeOrRemake
function makeOrRemake(obj, prop, className) {
  if (obj[prop] != null) {
    remake(obj, prop)
    return obj[prop]
  }
  return obj[prop] = makeTmpDir(obj, prop, className)
}

exports.makeOrReuse = makeOrReuse
function makeOrReuse(obj, prop, className) {
  if (obj[prop] != null) {
    return obj[prop]
  }
  return obj[prop] = makeTmpDir(obj, prop, className)
}

exports.remake = remake
function remake(obj, prop) {
  var fullpath = obj[prop];
  if (fullpath != null) {
    rimraf.sync(fullpath);
    fs.mkdirSync(fullpath);
  }
}

exports.remove = remove
function remove(obj, prop) {
  if (obj[prop] != null) {
    rimraf.sync(obj[prop])
  }
  obj[prop] = null
}


function makeTmpDir(obj, prop, className) {
  if (className == null) className = obj.constructor && obj.constructor.name
  var tmpDirName = prettyTmpDirName(className, prop)
  return mktemp.createDirSync(path.join(findBaseDir(), tmpDirName))
}

function currentTmp() {
  return path.join(process.cwd(), 'tmp')
}

function findBaseDir () {
  var tmp = currentTmp();
  try {
    if (fs.statSync(tmp).isDirectory()) {
      return tmp;
    }
  } catch (err) {
    if (err.code !== 'ENOENT') throw err
    // We could try other directories, but for now we just create ./tmp if
    // it doesn't exist
    fs.mkdirSync(tmp);
    return tmp;
  }
}

function cleanString (s) {
  return underscoreString.underscored(s || '')
    .replace(/[^a-z_]/g, '')
    .replace(/^_+/, '')
}

function prettyTmpDirName (className, prop) {
  var cleanClassName = cleanString(className)
  if (cleanClassName === 'object') cleanClassName = ''
  if (cleanClassName) cleanClassName += '-'
  var cleanPropertyName = cleanString(prop)
  return cleanClassName + cleanPropertyName + '-XXXXXXXX.tmp'
}
