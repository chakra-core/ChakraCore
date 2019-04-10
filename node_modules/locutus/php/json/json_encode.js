'use strict';

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

module.exports = function json_encode(mixedVal) {
  // eslint-disable-line camelcase
  //       discuss at: http://phpjs.org/functions/json_encode/
  //      original by: Public Domain (http://www.json.org/json2.js)
  // reimplemented by: Kevin van Zonneveld (http://kevin.vanzonneveld.net)
  //      improved by: Michael White
  //         input by: felix
  //      bugfixed by: Brett Zamir (http://brett-zamir.me)
  //        example 1: json_encode('Kevin')
  //        returns 1: '"Kevin"'

  /*
    http://www.JSON.org/json2.js
    2008-11-19
    Public Domain.
    NO WARRANTY EXPRESSED OR IMPLIED. USE AT YOUR OWN RISK.
    See http://www.JSON.org/js.html
  */

  var $global = typeof window !== 'undefined' ? window : global;
  $global.$locutus = $global.$locutus || {};
  var $locutus = $global.$locutus;
  $locutus.php = $locutus.php || {};

  var json = $global.JSON;
  var retVal;
  try {
    if ((typeof json === 'undefined' ? 'undefined' : _typeof(json)) === 'object' && typeof json.stringify === 'function') {
      // Errors will not be caught here if our own equivalent to resource
      retVal = json.stringify(mixedVal);
      if (retVal === undefined) {
        throw new SyntaxError('json_encode');
      }
      return retVal;
    }

    var value = mixedVal;

    var quote = function quote(string) {
      var escapeChars = ['\0-\x1F', '\x7F-\x9F', '\xAD', '\u0600-\u0604', '\u070F', '\u17B4', '\u17B5', '\u200C-\u200F', '\u2028-\u202F', '\u2060-\u206F', '\uFEFF', '\uFFF0-\uFFFF'].join('');
      var escapable = new RegExp('[\\"' + escapeChars + ']', 'g');
      var meta = {
        // table of character substitutions
        '\b': '\\b',
        '\t': '\\t',
        '\n': '\\n',
        '\f': '\\f',
        '\r': '\\r',
        '"': '\\"',
        '\\': '\\\\'
      };

      escapable.lastIndex = 0;
      return escapable.test(string) ? '"' + string.replace(escapable, function (a) {
        var c = meta[a];
        return typeof c === 'string' ? c : '\\u' + ('0000' + a.charCodeAt(0).toString(16)).slice(-4);
      }) + '"' : '"' + string + '"';
    };

    var _str = function _str(key, holder) {
      var gap = '';
      var indent = '    ';
      // The loop counter.
      var i = 0;
      // The member key.
      var k = '';
      // The member value.
      var v = '';
      var length = 0;
      var mind = gap;
      var partial = [];
      var value = holder[key];

      // If the value has a toJSON method, call it to obtain a replacement value.
      if (value && (typeof value === 'undefined' ? 'undefined' : _typeof(value)) === 'object' && typeof value.toJSON === 'function') {
        value = value.toJSON(key);
      }

      // What happens next depends on the value's type.
      switch (typeof value === 'undefined' ? 'undefined' : _typeof(value)) {
        case 'string':
          return quote(value);

        case 'number':
          // JSON numbers must be finite. Encode non-finite numbers as null.
          return isFinite(value) ? String(value) : 'null';

        case 'boolean':
        case 'null':
          // If the value is a boolean or null, convert it to a string. Note:
          // typeof null does not produce 'null'. The case is included here in
          // the remote chance that this gets fixed someday.
          return String(value);

        case 'object':
          // If the type is 'object', we might be dealing with an object or an array or
          // null.
          // Due to a specification blunder in ECMAScript, typeof null is 'object',
          // so watch out for that case.
          if (!value) {
            return 'null';
          }

          // Make an array to hold the partial results of stringifying this object value.
          gap += indent;
          partial = [];

          // Is the value an array?
          if (Object.prototype.toString.apply(value) === '[object Array]') {
            // The value is an array. Stringify every element. Use null as a placeholder
            // for non-JSON values.
            length = value.length;
            for (i = 0; i < length; i += 1) {
              partial[i] = _str(i, value) || 'null';
            }

            // Join all of the elements together, separated with commas, and wrap them in
            // brackets.
            v = partial.length === 0 ? '[]' : gap ? '[\n' + gap + partial.join(',\n' + gap) + '\n' + mind + ']' : '[' + partial.join(',') + ']';
            gap = mind;
            return v;
          }

          // Iterate through all of the keys in the object.
          for (k in value) {
            if (Object.hasOwnProperty.call(value, k)) {
              v = _str(k, value);
              if (v) {
                partial.push(quote(k) + (gap ? ': ' : ':') + v);
              }
            }
          }

          // Join all of the member texts together, separated with commas,
          // and wrap them in braces.
          v = partial.length === 0 ? '{}' : gap ? '{\n' + gap + partial.join(',\n' + gap) + '\n' + mind + '}' : '{' + partial.join(',') + '}';
          gap = mind;
          return v;
        case 'undefined':
        case 'function':
        default:
          throw new SyntaxError('json_encode');
      }
    };

    // Make a fake root object containing our value under the key of ''.
    // Return the result of stringifying the value.
    return _str('', {
      '': value
    });
  } catch (err) {
    // @todo: ensure error handling above throws a SyntaxError in all cases where it could
    // (i.e., when the JSON global is not available and there is an error)
    if (!(err instanceof SyntaxError)) {
      throw new Error('Unexpected error type in json_encode()');
    }
    // usable by json_last_error()
    $locutus.php.last_error_json = 4;
    return null;
  }
};
//# sourceMappingURL=json_encode.js.map