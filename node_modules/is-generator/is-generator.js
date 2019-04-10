/**
 * Export generator function checks.
 */
module.exports = isGenerator
module.exports.fn = isGeneratorFunction

/**
 * Check whether an object is a generator.
 *
 * @param  {Object}  obj
 * @return {Boolean}
 */
function isGenerator (obj) {
  return obj &&
    typeof obj.next === 'function' &&
    typeof obj.throw === 'function'
}

/**
 * Check whether a function is generator.
 *
 * @param  {Function} fn
 * @return {Boolean}
 */
function isGeneratorFunction (fn) {
  return typeof fn === 'function' &&
    fn.constructor &&
    fn.constructor.name === 'GeneratorFunction'
}
