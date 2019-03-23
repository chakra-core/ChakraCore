'use strict'

var broccoliNodeInfo = require('..')
var chai = require('chai'), expect = chai.expect
var multidepRequire = require('multidep')('test/multidep.json')

describe('transform nodes', function() {
  multidepRequire.forEachVersion('broccoli-plugin', function(version, Plugin) {
    describe('broccoli-plugin ' + version, function() {
      NoopPlugin.prototype = Object.create(Plugin.prototype)
      NoopPlugin.prototype.constructor = NoopPlugin
      function NoopPlugin(inputNodes, options) {
        Plugin.call(this, inputNodes || [], options)
      }
      NoopPlugin.prototype.build = function() {
      }

      it('has all properties', function() {
        var nodeInfo = broccoliNodeInfo.getNodeInfo(new NoopPlugin)
        expect(nodeInfo.nodeType).to.equal('transform')
        expect(nodeInfo.name).to.equal('NoopPlugin')
        expect(nodeInfo).to.have.property('annotation', undefined)
        expect(nodeInfo.instantiationStack).to.be.a('string')
        expect(nodeInfo.inputNodes).to.deep.equal([])
        expect(nodeInfo.setup).to.be.a('function')
        expect(nodeInfo.getCallbackObject).to.be.a('function')
        expect(nodeInfo.persistentOutput).to.equal(false)
        expect(nodeInfo.needsCache).to.equal(true)
        // Check that there are no extra keys
        expect(nodeInfo).to.have.keys([
          'nodeType', 'name', 'annotation', 'instantiationStack',
          'inputNodes', 'setup', 'getCallbackObject', 'persistentOutput',
          'needsCache'
        ])
      })
    })
  })
})

describe('source nodes', function() {
  multidepRequire.forEachVersion('broccoli-source', function(version, broccoliSource) {
    describe('broccoli-source ' + version, function() {
      var classNames = { 'WatchedDir': true, 'UnwatchedDir': false }
      Object.keys(classNames).forEach(function(className) {
        var class_ = broccoliSource[className]
        var watched = classNames[className]
        describe(className, function() {
          it('has all properties', function() {
            var nodeInfo = broccoliNodeInfo.getNodeInfo(new class_('some/dir'))
            expect(nodeInfo.nodeType).to.equal('source')
            expect(nodeInfo.name).to.be.a('string')
            expect(nodeInfo).to.have.property('annotation', undefined)
            expect(nodeInfo.instantiationStack).to.be.a('string')
            expect(nodeInfo.sourceDirectory).to.equal('some/dir')
            expect(nodeInfo.watched).to.equal(watched)
            // Check that there are no extra keys
            expect(nodeInfo).to.have.keys(['nodeType', 'name', 'annotation', 'instantiationStack',
              'sourceDirectory', 'watched'])
          })
        })
      })
    })
  })
})

describe('invalid nodes', function() {
  ['read', 'rebuild'].forEach(function(functionName) {
    it('rejects .' + functionName + '-based nodes', function() {
      var node = {}
      node[functionName] = function() { }
      expect(function() {
        broccoliNodeInfo.getNodeInfo(node)
      }).to.throw(broccoliNodeInfo.InvalidNodeError, 'The .read/.rebuild API is no longer supported')
    })
  })

  it('rejects null', function() {
    expect(function() {
      broccoliNodeInfo.getNodeInfo(null)
    }).to.throw(broccoliNodeInfo.InvalidNodeError, 'null is not a Broccoli node')
  })

  it('rejects non-node objects', function() {
    expect(function() {
      broccoliNodeInfo.getNodeInfo({})
    }).to.throw(broccoliNodeInfo.InvalidNodeError, '[object Object] is not a Broccoli node')
  })

  it('rejects string nodes', function() {
    expect(function() {
      broccoliNodeInfo.getNodeInfo('some/dir')
    }).to.throw(broccoliNodeInfo.InvalidNodeError, '"some/dir": String nodes are not supported')
  })

  it('rejects nodes with invalid nodeType', function() {
    expect(function() {
      console.log(broccoliNodeInfo.getNodeInfo({
        __broccoliFeatures__: { persistentOutputFlag: true, sourceDirectories: true },
        __broccoliGetInfo__: function(builderFeatures) {
          return { nodeType: 'foo' }
        }
      }))
    }).to.throw(broccoliNodeInfo.InvalidNodeError, 'Unexpected nodeType: foo')
  })
})
