'use strict'

// This list of [feature, augmentationFunction] pairs is used to maintain
// backwards compatibility with older broccoli-plugin versions.
//
// If a plugin doesn't support `feature`, then `augmentationFunction` is
// called on its node info (as returned by node.__broccoliGetInfo__())
// in order to bring the interface up-to-date. If a plugin is missing several
// features, each `augmentationFunction` is applied in succession.
//
// Add new features to the bottom of the list.
var augmenters = [
  [
    'persistentOutputFlag', function(nodeInfo) {
      nodeInfo.persistentOutput = false
    }
  ], [
    'sourceDirectories', function(nodeInfo) {
      nodeInfo.nodeType = 'transform'
    }
  ], [
    'needsCacheFlag', function(nodeInfo) {
      if (nodeInfo.nodeType === 'transform') {
        nodeInfo.needsCache = true
      }
    }
  ]
]

var features = {}
for (var i = 0; i < augmenters.length; i++) {
  features[augmenters[i][0]] = true
}
exports.features = features


exports.getNodeInfo = getNodeInfo
function getNodeInfo(node) {
  // Check that `node` is in fact a Broccoli node
  if (node == null || !node.__broccoliGetInfo__) {
    if (typeof node === 'string') {
      throw new InvalidNodeError('"' + node + '": String nodes are not supported. Use the WatchedDir class provided by the broccoli-source package instead.')
    } else if (node != null && (typeof node.read === 'function' || typeof node.rebuild === 'function')) {
      var legacyNodeDescription = node.description ||
        node.constructor && node.constructor.name ||
        ('' + node)
      throw new InvalidNodeError(
        legacyNodeDescription +
        ': The .read/.rebuild API is no longer supported as of Broccoli 1.0. ' +
        'Plugins must now derive from broccoli-plugin. ' +
        'https://github.com/broccolijs/broccoli/blob/master/docs/broccoli-1-0-plugin-api.md')
    } else {
      throw new InvalidNodeError(node + ' is not a Broccoli node')
    }
  }

  // Call __broccoliGetInfo__. Note that we're passing the builder's full
  // feature set (the `features` variable) rather than the smaller feature set
  // we'll be mimicking to interact with this plugin. This is a fairly
  // arbitrary choice, but it's easier to implement, and it usually won't make
  // a difference because the Plugin class won't care about features it
  // doesn't know about.
  var originalNodeInfo = node.__broccoliGetInfo__(features)

  // Now, for backward compatibility, deal with plugins that don't implement
  // our full feature set:

  // 1. Make a shallow copy of the nodeInfo hash so we can modify it
  //
  // We don't to use prototypal inheritance (Object.create) because some test
  // code can get confused if hasOwnProperty isn't true.
  var nodeInfo = {}
  for (var key in originalNodeInfo) {
    nodeInfo[key] = originalNodeInfo[key]
  }

  // 2. Discover features we have in common
  for (var i = 0; i < augmenters.length; i++) {
    var feature = augmenters[i][0]
    if (!node.__broccoliFeatures__[feature]) {
      break
    }
  }

  // 3. Augment the interface with the new features that the plugin doesn't support
  for (; i < augmenters.length; i++) {
    var fn = augmenters[i][1]
    fn(nodeInfo)
  }

  // We generally trust the nodeInfo to be valid, but unexpected
  // nodeTypes could break our code paths really badly, and some of those
  // paths call rimraf, so we check this to be safe.
  if (nodeInfo.nodeType !== 'transform' && nodeInfo.nodeType !== 'source') {
    throw new InvalidNodeError('Unexpected nodeType: ' + nodeInfo.nodeType)
  }

  return nodeInfo
}


exports.InvalidNodeError = InvalidNodeError
InvalidNodeError.prototype = Object.create(Error.prototype)
InvalidNodeError.prototype.constructor = InvalidNodeError
function InvalidNodeError(message) {
  // Subclassing Error in ES5 is non-trivial because reasons, so we need this
  // extra constructor logic from http://stackoverflow.com/a/17891099/525872.
  var temp = Error.apply(this, arguments)
  // Need to assign temp.name for correct error class in .stack and .message
  temp.name = this.name = this.constructor.name
  this.stack = temp.stack
  this.message = temp.message
}
