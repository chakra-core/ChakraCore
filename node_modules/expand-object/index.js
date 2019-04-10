'use strict';

var isNumber = require('is-number');
var set = require('set-value');

/**
 * Expand the given string into an object.
 *
 * @param  {String} `str`
 * @return {Object}
 */

function expand(str, opts) {
  opts = opts || {};

  if (typeof str !== 'string') {
    throw new TypeError('expand-object expects a string.');
  }

  if (!/[.|:=]/.test(str) && /,/.test(str)) {
    return toArray(str);
  }

  var m;
  if ((m = /(\w+[:=]\w+\.)+/.exec(str)) && !/[|,+]/.test(str)) {
    var val = m[0].split(':').join('.');
    str = val + str.slice(m[0].length);
  }

  var arr = splitString(str, '|');
  var len = arr.length, i = -1;
  var res = {};

  if (isArrayLike(str) && arr.length === 1) {
    return expandArrayObj(str, opts);
  }

  while (++i < len) {
    var val = arr[i];
    // test for `https://foo`
    if (/\w:\/\/\w/.test(val)) {
      res[val] = '';
      continue;
    }

    var re = /^((?:\w+)\.(?:\w+))[:.]((?:\w+,)+)+((?:\w+):(?:\w+))/;
    var m = re.exec(val);
    if (m && m[1] && m[2] && m[3]) {
      var arrVal = m[2];
      arrVal = arrVal.replace(/,$/, '');
      var prop = arrVal.split(',');
      prop = prop.concat(toObject(m[3]));
      res = set(res, m[1], prop);
    } else if (!/[.,\|:=]/.test(val)) {
      res[val] = opts.toBoolean ? true : '';
    } else {
      res = expandObject(res, val, opts);
    }
  }
  return res;
}

function setValue(obj, a, b, opts) {
  var val = resolveValue(b, opts);
  if (~String(a).indexOf('.')) {
    return set(obj, a, typeCast(val));
  } else {
    obj[a] = typeCast(val);
  }
  return obj;
}

function resolveValue(val, opts) {
  opts = opts || {};
  if (typeof val === 'undefined') {
    return opts.toBoolean ? true : '';
  }
  if (typeof val === 'string' && ~val.indexOf(',')) {
    val = toArray(val);
  }

  if (Array.isArray(val)) {
    return val.map(function (ele) {
      if (~String(ele).indexOf('.')) {
        return setValue({}, ele, opts.toBoolean ? true : '');
      }
      return ele;
    });
  }
  return val;
}

function expandArray(str, opts) {
  var segs = String(str).split(/[:=]/);
  var key = segs.shift();
  var res = {}, val = [];

  segs.forEach(function (seg) {
    val = val.concat(resolveValue(toArray(seg), opts));
  });

  res[key] = val;
  return res;
}

function toArray(str) {
  return (str || '').split(',').reduce(function (acc, val) {
    if (typeof val !== 'undefined' && val !== '') {
      acc.push(typeCast(val));
    }
    return acc;
  }, []);
}

function expandSiblings(segs, opts) {
  var first = segs.shift();
  var parts = first.split('.');
  var arr = [parts.pop()].concat(segs);
  var key = parts.join('.');
  var siblings = {};

  var val = arr.reduce(function (acc, val) {
    expandObject(acc, val, opts);
    return acc;
  }, {});

  if (!key) return val;
  set(siblings, key, val);
  return siblings;
}

function expandObject(res, str, opts) {
  var segs = splitString(str, '+');
  if (segs.length > 1) {
    return expandSiblings(segs, opts);
  }

  segs = str.split(/[:=]/);
  setValue(res, segs[0], segs[1]);
  return res;
}

function expandArrayObj(str, opts) {
  var m = /\w+:.*?:/.exec(str);
  if (!m) return expandArray(str, opts);

  var i = str.indexOf(':');
  var key = str.slice(0, i);
  var val = str.slice(i + 1);

  if (/\w+,\w+,/.test(val)) {
    var obj = {};
    obj[key] = toArray(val).map(function (ele) {
      return ~ele.indexOf(':')
        ? expandObject({}, ele, opts)
        : ele;
    });
    return obj;
  }

  return toArray(str).map(function (ele) {
    return expandObject({}, ele, opts);
  });
}

function typeCast(val) {
  if (val === 'true') {
    return true;
  }
  if (val === 'false') {
    return false;
  }
  if (isNumber(val)) {
    return +val;
  }
  return val;
}

function isArrayLike(str) {
  return typeof str === 'string' && /^(?:(\w+[:=]\w+[,:])+)+/.exec(str);
}

function toObject(val) {
  var obj = {};
  var segs = String(val).split(/[:=]/);
  obj[segs[0]] = typeCast(segs[1]);
  return obj;
}

function splitString(str, ch) {
  str = String(str);

  var segs = str.split(ch);
  var len = segs.length;
  var res = [];
  var i = -1;

  while (++i < len) {
    var key = segs[i];
    while (key[key.length - 1] === '\\') {
      key = key.slice(0, -1) + ch + segs[++i];
    }
    res.push(key);
  }
  return res;
}

/**
 * Expose `expand`
 */

module.exports = expand;
