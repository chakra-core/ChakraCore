'use strict';

var escapeRegExp = require('escape-string-regexp');

var SIMPLE_PLACEHOLDER_RX = /^\[(id|name|file|query|filebase)\]/i;
var HASH_PLACEHOLDER_RX = /^\[((?:chunk)?hash)(?::(\d+))?\]/i;

var templateCache = Object.create(null);

module.exports = function createTemplate(str) {
  if (!templateCache[str]) {
    templateCache[str] = new PathTemplate(str);
  }

  return templateCache[str];
};

function PathTemplate(template) {
  this.template = template;
  this.fields = parseTemplate(template);
  this.matcher = createTemplateMatcher(this.fields);
}

PathTemplate.prototype = {

  constructor: PathTemplate,

  /**
   * Returns whether the given path matches this template.
   *
   * @param String data
   */
  matches: function matches(path) {
    return this.matcher.test(path);
  },

  /**
   * Applies data to this template and outputs a filename.
   *
   * @param Object data
   */
  resolve: function resolve(data) {
    return this.fields.reduce(function (output, field) {
      var replacement = '';
      var placeholder = field.placeholder;
      var width = field.width;

      if (field.prefix) {
        output += field.prefix;
      }
      if (placeholder) {
        replacement = data[placeholder] || '';
        if (width && (placeholder === 'hash' || placeholder === 'chunkhash')) {
          replacement = replacement.slice(0, width);
        }
        output += replacement;
      }

      return output;
    }, '');
  }

  /**
   * Loop over the template string and return an array of objects in the form:
   * {
   *     prefix: 'literal text',
   *     placeholder: 'replacement field name'
   *     [, width: maximum hash length for hash & chunkhash placeholders]
   * }
   *
   * The values in the object conceptually represent a span of literal text followed by a single replacement field.
   * If there is no literal text (which can happen if two replacement fields occur consecutively),
   * then prefix will be an empty string.
   * If there is no replacement field, then the value of placeholder will be null.
   * If the value of placeholder is either 'hash' or 'chunkhash', then width will be a positive integer.
   * Otherwise it will be left undefined.
   */
};function parseTemplate(str) {
  var fields = [];
  var char = '';
  var pos = 0;
  var prefix = '';
  var match = null;
  var input;

  while (true) {
    // eslint-disable-line no-constant-condition
    char = str[pos];

    if (!char) {
      fields.push({
        prefix: prefix,
        placeholder: null
      });
      break;
    } else if (char === '[') {
      input = str.slice(pos);
      match = SIMPLE_PLACEHOLDER_RX.exec(input);
      if (match) {
        fields.push({
          prefix: prefix,
          placeholder: match[1].toLowerCase()
        });
        pos += match[0].length;
        prefix = '';
        continue;
      }

      match = HASH_PLACEHOLDER_RX.exec(input);
      if (match) {
        fields.push({
          prefix: prefix,
          placeholder: match[1].toLowerCase(),
          width: parseInt(match[2] || 0, 10)
        });
        pos += match[0].length;
        prefix = '';
        continue;
      }
    }
    prefix += char;
    pos++;
  }

  return fields;
}

/**
 * Returns a RegExp which, given the replacement fields returned by parseTemplate(),
 * can match a file path against a path template.
 */
function createTemplateMatcher(fields) {
  var length = fields.length;
  var pattern = fields.reduce(function (pattern, field, i) {
    if (i === 0) {
      pattern = '^';
    }
    if (field.prefix) {
      pattern += '(' + escapeRegExp(field.prefix) + ')';
    }
    if (field.placeholder) {
      switch (field.placeholder) {
        case 'hash':
        case 'chunkhash':
          pattern += '[0-9a-fA-F]';
          pattern += field.width ? '{1,' + field.width + '}' : '+';
          break;
        case 'id':
        case 'name':
        case 'file':
        case 'filebase':
          pattern += '.+?';
          break;
        case 'query':
          pattern += '(?:\\?.+?)?';
          break;
      }
    }
    if (i === length - 1) {
      pattern += '$';
    }

    return pattern;
  }, '');

  return new RegExp(pattern);
}