var fs   = require('fs') 
var path = require('path')
var resolvers = require('./resolvers')
var NestedError = require('nested-error-stacks')

'use strict';

module.exports = requireg

function requireg(module) {
  try {
    return require(resolve(module))
  } catch (e) {
    throw new NestedError("Could not require module '"+ module +"'", e)
  }
}

requireg.resolve = resolve

requireg.globalize = function () {
  global.requireg = requireg
}

function resolve(module, dirname) {
  var i, resolver, modulePath

  for (i = 0, l = resolvers.length; i < l; i += 1) {
    resolver = resolvers[i]
    if (modulePath = resolver(module, dirname)) {
      break
    }
  }

  return modulePath
}
