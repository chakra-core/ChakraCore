'use strict'

var fs = require('fs')
var path = require('path')
var quickTemp = require('quick-temp')
var mapSeries = require('promise-map-series')
var rimraf = require('rimraf')
var symlinkOrCopy = require('symlink-or-copy')
var symlinkOrCopySync = symlinkOrCopy.sync

// Mimic how a Broccoli builder would call a plugin, using quickTemp to create
// directories
module.exports = ReadCompat
function ReadCompat(plugin) {
  this.pluginInterface = plugin.__broccoliGetInfo__()

  quickTemp.makeOrReuse(this, 'outputPath', this.pluginInterface.name)

  if (this.pluginInterface.needsCache) {
    quickTemp.makeOrReuse(this, 'cachePath', this.pluginInterface.name)
  } else {
    this.cachePath = undefined
  }

  quickTemp.makeOrReuse(this, 'inputBasePath', this.pluginInterface.name)

  this.inputPaths = []
  this._priorBuildInputNodeOutputPaths = [];

  if (this.pluginInterface.inputNodes.length === 1) {
    this.inputPaths.push(this.inputBasePath)
    this._priorBuildInputNodeOutputPaths.push(this.inputBasePath);
  } else {
    for (var i = 0; i < this.pluginInterface.inputNodes.length; i++) {
      this.inputPaths.push(path.join(this.inputBasePath, i + ''))
    }
  }

  this.pluginInterface.setup(null, {
    inputPaths: this.inputPaths,
    outputPath: this.outputPath,
    cachePath: this.cachePath
  })

  this.callbackObject = this.pluginInterface.getCallbackObject()

  if (plugin.description == null) {
    plugin.description = this.pluginInterface.name
    if (this.pluginInterface.annotation != null) {
      plugin.description += ': ' + this.pluginInterface.annotation
    }
  }
}

ReadCompat.prototype.read = function(readTree) {
  var self = this

  if (!this.pluginInterface.persistentOutput) {
    rimraf.sync(this.outputPath)
    fs.mkdirSync(this.outputPath)
  }

  return mapSeries(this.pluginInterface.inputNodes, readTree)
    .then(function(outputPaths) {
      var priorBuildInputNodeOutputPaths = self._priorBuildInputNodeOutputPaths;
      // In old .read-based Broccoli, the inputNodes's outputPaths can change
      // on each rebuild. But the new API requires that our plugin sees fixed
      // input paths. Therefore, we symlink the inputNodes' outputPaths to our
      // (fixed) inputPaths on each .read.
      for (var i = 0; i < outputPaths.length; i++) {
        var priorPath = priorBuildInputNodeOutputPaths[i]
        var currentPath = outputPaths[i]

        // if this output path is different from last builds or
        // if we cannot symlink then clear and symlink/copy manually
        var hasDifferentPath = priorPath !== currentPath
        var forceReSymlinking = !symlinkOrCopy.canSymlink || hasDifferentPath

        if (forceReSymlinking) {

          // avoid `rimraf.sync` for initial build
          if (priorPath) {
            rimraf.sync(self.inputPaths[i])
          }

          symlinkOrCopySync(currentPath, self.inputPaths[i])
        }
      }

      // save for next builds comparison
      self._priorBuildInputNodeOutputPaths = outputPaths;

      return self.callbackObject.build()
    })
    .then(function() {
      return self.outputPath
    })
}

ReadCompat.prototype.cleanup = function() {
  quickTemp.remove(this, 'outputPath')
  quickTemp.remove(this, 'cachePath')
  quickTemp.remove(this, 'inputBasePath')
}
