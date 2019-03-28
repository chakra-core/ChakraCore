var deepDiff  = require('deep-diff')
var util      = require('util')

var stripDeps = function (json) {
  if (json === undefined) return undefined;
  return {
    dependencies: json.dependencies || {},
    devDependencies: json.devDependencies || {},
    optionalDependencies: json.optionalDependencies || {}
  }
}

var Diff = module.exports = function Diff(left, right) {
  // left and right are optional arguments
  if (!(this instanceof Diff)) return new Diff(left, right)
  this._files = {
    left: stripDeps(left),
    right: stripDeps(right)
  }

}

Diff.prototype.left = function (left) {
  this._files.left = stripDeps(left)
  return this
}

Diff.prototype.right = function (right) {
  this._files.right = stripDeps(right)
  return this  
}

Diff.prototype.validate = function () {
  if (this._files.left === undefined) {
    throw new Error('Left Hand Package.json has not been provided')
  }
  if (this._files.right === undefined) {
    throw new Error('Right Hand Package.json has not been provided')
  }
  return this
}

Diff.prototype.calculateDiff = function () {
  return deepDiff(this._files.left, this._files.right) || []
}

Diff.prototype.kindToOp = function (kind) {
  switch (kind) {
    case 'E': 
      return 'edit'
    case 'A':
      return 'append'
    case 'N':
      return 'new'
    case 'D':
      return 'delete'
  }
}

Diff.prototype.cmdFromDiffObj = function (diffObj) {
  var operation = diffObj.operation === 'delete' ? 'uninstall' : 'install'
  if (operation === 'uninstall') {
    return 'npm uninstall ' + diffObj.name
  }
  var cmd = util.format('npm %s "%s@%s" --spin=false --color=true --production --loglevel=http', operation, diffObj.name, diffObj.version)
  return cmd
}

Diff.prototype.toCmdList = function () {
  var diffObj = this.validate().toObject()

  var list = []

  Object.keys(diffObj).forEach(function (level) {
    diffObj[level].forEach(function (obj) {
      list.push(obj.cmd)
    }.bind(this))
  }.bind(this))

  return list
}

Diff.prototype.toObject = function () {
  var obj = {
    dependencies: [],
    devDependencies: [],
    optionalDependencies: []
  }
  this.validate().calculateDiff().forEach(function (diffObj) {
    var op = this.kindToOp(diffObj.kind)

    var index = obj[diffObj.path[0]].push ({
      operation: op,
      name: diffObj.path[1]
    }) - 1;

    if (op === 'delete') {
      obj[diffObj.path[0]][index].version = diffObj.lhs
    } else {
      obj[diffObj.path[0]][index].version = diffObj.rhs
    }
    obj[diffObj.path[0]][index].cmd = this.cmdFromDiffObj(obj[diffObj.path[0]][index]);
  }.bind(this))
  return obj
}

Diff.stripDeps = stripDeps