'use strict'

const getDocsUrl = require('./lib/get-docs-url')

const errorMessage = 'Avoid callbacks. Prefer Async/Await.'

module.exports = {
  meta: {
    docs: {
      url: getDocsUrl('prefer-await-to-callbacks')
    }
  },
  create(context) {
    function checkLastParamsForCallback(node) {
      const lastParam = node.params[node.params.length - 1] || {}
      if (lastParam.name === 'callback' || lastParam.name === 'cb') {
        context.report({ node: lastParam, message: errorMessage })
      }
    }
    function isInsideYieldOrAwait() {
      return context.getAncestors().some(parent => {
        return (
          parent.type === 'AwaitExpression' || parent.type === 'YieldExpression'
        )
      })
    }
    return {
      CallExpression(node) {
        // Callbacks aren't allowed.
        if (node.callee.name === 'cb' || node.callee.name === 'callback') {
          context.report({ node, message: errorMessage })
          return
        }

        // Then-ables aren't allowed either.
        const args = node.arguments
        const lastArgIndex = args.length - 1
        const arg = lastArgIndex > -1 && node.arguments[lastArgIndex]
        if (
          (arg && arg.type === 'FunctionExpression') ||
          arg.type === 'ArrowFunctionExpression'
        ) {
          // Ignore event listener callbacks.
          if (
            node.callee.property &&
            (node.callee.property.name === 'on' ||
              node.callee.property.name === 'once')
          ) {
            return
          }
          if (arg.params && arg.params[0] && arg.params[0].name === 'err') {
            if (!isInsideYieldOrAwait()) {
              context.report({ node: arg, message: errorMessage })
            }
          }
        }
      },
      FunctionDeclaration: checkLastParamsForCallback,
      FunctionExpression: checkLastParamsForCallback,
      ArrowFunctionExpression: checkLastParamsForCallback
    }
  }
}
