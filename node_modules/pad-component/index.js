
/**
 * Expose `pad()`.
 */

exports = module.exports = pad;

/**
 * Pad `str` to `len` with optional `c` char,
 * favoring the left when unbalanced.
 *
 * @param {String} str
 * @param {Number} len
 * @param {String} c
 * @return {String}
 * @api public
 */

function pad(str, len, c) {
  c = c || ' ';
  if (str.length >= len) return str;
  len = len - str.length;
  var left = Array(Math.ceil(len / 2) + 1).join(c);
  var right = Array(Math.floor(len / 2) + 1).join(c);
  return left + str + right;
}

/**
 * Pad `str` left to `len` with optional `c` char.
 *
 * @param {String} str
 * @param {Number} len
 * @param {String} c
 * @return {String}
 * @api public
 */

exports.left = function(str, len, c){
  c = c || ' ';
  if (str.length >= len) return str;
  return Array(len - str.length + 1).join(c) + str;
};

/**
 * Pad `str` right to `len` with optional `c` char.
 *
 * @param {String} str
 * @param {Number} len
 * @param {String} c
 * @return {String}
 * @api public
 */

exports.right = function(str, len, c){
  c = c || ' ';
  if (str.length >= len) return str;
  return str + Array(len - str.length + 1).join(c);
};