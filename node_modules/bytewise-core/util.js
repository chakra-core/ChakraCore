var util = exports

//
// buffer compare
//
util.compare = require('typewise-core/collation').bitwise

//
// buffer equality
//
util.equal = function (a, b) {
  if (!Buffer.isBuffer(a) || !Buffer.isBuffer(b))
    return

  if (a === b)
    return true

  if (typeof a.equals === 'function')
    return a.equals(b)

  return util.compare(a, b) === 0
}

var assert = util.assert = function (test, message) {
  if (!test)
    throw new TypeError(message)
}

var FLOAT_LENGTH = 8

util.invertBytes = function (buffer) {
  var bytes = []
  for (var i = 0, end = buffer.length; i < end; ++i) {
    bytes.push(~buffer[i])
  }

  return new Buffer(bytes)
}

util.encodeFloat = function (value) {
  var buffer = new Buffer(FLOAT_LENGTH)
  if (value < 0) {
    //
    // write negative numbers as negated positive values to invert bytes
    //
    buffer.writeDoubleBE(-value.valueOf(), 0)
    return util.invertBytes(buffer)
  }

  //
  // normalize -0 values to 0
  //
  buffer.writeDoubleBE(value.valueOf() || 0, 0)
  return buffer
}

util.decodeFloat = function (buffer, base, negative) {
  assert(buffer.length === FLOAT_LENGTH, 'Invalid float encoding length')

  if (negative)
    buffer = util.invertBytes(buffer)

  var value = buffer.readDoubleBE(0)
  return negative ? -value : value
}

//
// sigil for controlling the escapement functions (TODO: clean this up)
//
var SKIP_HIGH_BYTES = {}

util.escapeFlat = function (buffer, options) {
  //
  // escape high and low bytes 0x00 and 0xff (and by necessity, 0x01 and 0xfe)
  //
  var b, bytes = []
  for (var i = 0, end = buffer.length; i < end; ++i) {
    b = buffer[i]

    //
    // escape low bytes with 0x01 and by adding 1
    //
    if (b === 0x01 || b === 0x00)
      bytes.push(0x01, b + 1)

    //
    // escape high bytes with 0xfe and by subtracting 1
    //
    else if (options !== SKIP_HIGH_BYTES && (b === 0xfe || b === 0xff))
      bytes.push(0xfe, b - 1)

    //
    // no escapement needed
    //
    else
      bytes.push(b)
  }

  return new Buffer(bytes)
}

util.unescapeFlat = function (buffer, options) {
  var b, bytes = []
  //
  // don't escape last byte
  //
  for (var i = 0, end = buffer.length; i < end; ++i) {
    b = buffer[i]

    //
    // if low-byte escape tag use the following byte minus 1
    //
    if (b === 0x01)
      bytes.push(buffer[++i] - 1)

    //
    // if high-byte escape tag use the following byte plus 1
    //
    else if (options !== SKIP_HIGH_BYTES && b === 0xfe)
      bytes.push(buffer[++i] + 1)

    //
    // no unescapement needed
    //
    else
      bytes.push(b)
  }
  return new Buffer(bytes)
}

util.escapeFlatLow = function (buffer) {
  return util.escapeFlat(buffer, SKIP_HIGH_BYTES)
}

util.unescapeFlatLow = function (buffer) {
  return util.unescapeFlat(buffer, SKIP_HIGH_BYTES)
}

util.encodeList = function (source, base) {
  // TODO: cycle detection
  var buffers = []
  var undecodable

  for (var i = 0, end = source.length; i < end; ++i) {
    var buffer = base.encode(source[i], null)

    //
    // bypass assertions for undecodable types (i.e. range bounds)
    //
    undecodable || (undecodable = buffer.undecodable)
    if (undecodable) {
      buffers.push(buffer)
      continue
    }

    var sort = base.getType(buffer[0])
    assert(sort, 'List encoding failure: ' + buffer)

    //
    // escape sorts if it requires it and add closing byte for element
    //
    if (sort.codec && sort.codec.escape)
      buffers.push(sort.codec.escape(buffer), new Buffer([ 0x00 ]))

    else
      buffers.push(buffer)
  }

  //
  // close the list with an end byte
  //
  buffers.push(new Buffer([ 0x00 ]))
  buffer = Buffer.concat(buffers)

  //
  // propagate undecoable bit if set
  //
  undecodable && (buffer.undecodable = undecodable)
  return buffer
}

util.decodeList = function (buffer, base) {
  var result = util.parse(buffer, base)

  assert(result[1] === buffer.length, 'Invalid encoding')
  return result[0]
}

util.encodeHash = function (source, base) {
  //
  // packs hash into an array, e.g. `[ k1, v1, k2, v2, ... ]`
  //
  var list = []
  Object.keys(source).forEach(function(key) {
    list.push(key)
    list.push(source[key])
  })
  return util.encodeList(list, base)
}

util.decodeHash = function (buffer, base) {
  var list = util.decodeList(buffer, base)
  var hash = Object.create(null)

  for (var i = 0, end = list.length; i < end; ++i) {
    hash[list[i]] = list[++i]
  }

  return hash
}

//
// base parser for nested/recursive sorts
//
util.parse = function (buffer, base, sort) {
  //
  // parses and returns the first sort on the buffer and total bytes consumed
  //
  var codec = sort && sort.codec
  var index, end

  //
  // nullary
  //
  if (sort && !codec)
    return [ base.decode(new Buffer([ sort.byte ]), null), 0 ]

  //
  // custom parse implementation provided by sort
  //
  if (codec && codec.parse)
    return codec.parse(buffer, base, sort)

  //
  // fixed length sort, decode fixed bytes
  //
  var length = codec && codec.length
  if (typeof length === 'number')
    return [ codec.decode(buffer.slice(0, length)), length ]

  //
  // escaped sort, seek to end byte and unescape
  //
  if (codec && codec.unescape) {
    for (index = 0, end = buffer.length; index < end; ++index) {
      if (buffer[index] === 0x00)
        break
    }

    assert(index < buffer.length, 'No closing byte found for sequence')
    var unescaped = codec.unescape(buffer.slice(0, index))

    //
    // add 1 to index to account for closing tag byte
    //
    return [ codec.decode(unescaped), index + 1 ]
  }

  //
  // recursive sort, resolve each item iteratively
  //
  index = 0
  var list = []
  var next
  while ((next = buffer[index]) !== 0x00) {
    sort = base.getType(next)
    var result = util.parse(buffer.slice(index + 1), base, sort)
    list.push(result[0])

    //
    // offset current index by bytes consumed (plus a byte for the sort tag)
    //
    index += result[1] + 1
    assert(index < buffer.length, 'No closing byte found for nested sequence')
  }

  //
  // return parsed list and bytes consumed (plus a byte for the closing tag)
  //
  return [ list, index + 1 ]
}

//
// helpers for encoding boundary types
//
function encodeBound(data, base) {
  var prefix = data.prefix
  var buffer = prefix ? base.encode(prefix, null) : new Buffer([ data.byte ])

  if (data.upper)
    buffer = Buffer.concat([ buffer, new Buffer([ 0xff ]) ])

  return util.encodedBound(data, buffer)
}

util.encodeBound = function (data, base) {
  return util.encodedBound(data, encodeBound(data, base))
}

util.encodeBaseBound = function (data, base) {
  return util.encodedBound(data, new Buffer([ data.upper ? 0xff : 0x00 ]))
}

util.encodeListBound = function (data, base) {
  var buffer = encodeBound(data, base)

  if (data.prefix) {
    //
    // trim off end byte if a prefix, and do some hackery if an upper bound
    //
    var endByte = buffer[buffer.length - 1]
    buffer = buffer.slice(0, -1)
    if (data.upper)
      buffer[buffer.length - 1] = endByte
  }

  return util.encodedBound(data, buffer)
}

//
// add some metadata to generated buffer instance
//
util.encodedBound = function (data, buffer) {
  buffer.undecodable = true
  return buffer
}
