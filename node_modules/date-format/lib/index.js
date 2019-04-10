'use strict';

function padWithZeros(vNumber, width) {
    var numAsString = vNumber.toString();
    while (numAsString.length < width) {
        numAsString = '0' + numAsString;
    }
    return numAsString;
}

function addZero(vNumber) {
    return padWithZeros(vNumber, 2);
}

/**
 * Formats the TimeOffset
 * Thanks to http://www.svendtofte.com/code/date_format/
 * @private
 */
function offset(timezoneOffset) {
    var os = Math.abs(timezoneOffset);
    var h = String(Math.floor(os / 60));
    var m = String(os % 60);
    if (h.length === 1) {
        h = '0' + h;
    }
    if (m.length === 1) {
        m = '0' + m;
    }
    return timezoneOffset < 0 ? '+' + h + m : '-' + h + m;
}

function datePart(date, displayUTC, part) {
  return displayUTC ? date['getUTC' + part]() : date['get' + part]();
}

function asString(format, date) {
    if (typeof format !== 'string') {
        date = format;
        format = module.exports.ISO8601_FORMAT;
    }
    if (!date) {
        date =  module.exports.now();
    }

    var displayUTC = format.indexOf('O') > -1;

    var vDay = addZero(datePart(date, displayUTC, 'Date'));
    var vMonth = addZero(datePart(date, displayUTC, 'Month') + 1);
    var vYearLong = addZero(datePart(date, displayUTC, 'FullYear'));
    var vYearShort = addZero(vYearLong.substring(2, 4));
    var vYear = (format.indexOf('yyyy') > -1 ? vYearLong : vYearShort);
    var vHour = addZero(datePart(date, displayUTC, 'Hours'));
    var vMinute = addZero(datePart(date, displayUTC, 'Minutes'));
    var vSecond = addZero(datePart(date, displayUTC, 'Seconds'));
    var vMillisecond = padWithZeros(datePart(date, displayUTC, 'Milliseconds'), 3);
    var vTimeZone = offset(date.getTimezoneOffset());
    var formatted = format
        .replace(/dd/g, vDay)
        .replace(/MM/g, vMonth)
        .replace(/y{1,4}/g, vYear)
        .replace(/hh/g, vHour)
        .replace(/mm/g, vMinute)
        .replace(/ss/g, vSecond)
        .replace(/SSS/g, vMillisecond)
        .replace(/O/g, vTimeZone);
    return formatted;
}

function extractDateParts(pattern, str) {
  var matchers = [
    { pattern: /y{1,4}/, regexp: "\\d{1,4}", fn: function(date, value) { date.setFullYear(value); } },
    { pattern: /MM/, regexp: "\\d{1,2}", fn: function(date, value) { date.setMonth(value -1); } },
    { pattern: /dd/, regexp: "\\d{1,2}", fn: function(date, value) { date.setDate(value); } },
    { pattern: /hh/, regexp: "\\d{1,2}", fn: function(date, value) { date.setHours(value); } },
    { pattern: /mm/, regexp: "\\d\\d", fn: function(date, value) { date.setMinutes(value); } },
    { pattern: /ss/, regexp: "\\d\\d", fn: function(date, value) { date.setSeconds(value); } },
    { pattern: /SSS/, regexp: "\\d\\d\\d", fn: function(date, value) { date.setMilliseconds(value); } },
    { pattern: /O/, regexp: "[+-]\\d{3,4}|Z", fn: function(date, value) {
      if (value === 'Z') {
        value = 0;
      }
      var offset = Math.abs(value);
      var minutes = (offset % 100) + (Math.floor(offset / 100) * 60);
      date.setMinutes(date.getMinutes() + (value > 0 ? minutes : -minutes));
    } }
  ];

  var parsedPattern = matchers.reduce(function(p, m) {
    if (m.pattern.test(p.regexp)) {
      m.index = p.regexp.match(m.pattern).index;
      p.regexp = p.regexp.replace(m.pattern, "(" + m.regexp + ")");
    } else {
      m.index = -1;
    }
    return p;
  }, { regexp: pattern, index: [] });

  var dateFns = matchers.filter(function(m) {
    return m.index > -1;
  });
  dateFns.sort(function(a, b) { return a.index - b.index; });

  var matcher = new RegExp(parsedPattern.regexp);
  var matches = matcher.exec(str);
  if (matches) {
    var date = module.exports.now();
    dateFns.forEach(function(f, i) {
      f.fn(date, matches[i+1]);
    });
    return date;
  }

  throw new Error('String \'' + str + '\' could not be parsed as \'' + pattern + '\'');
}

function parse(pattern, str) {
  if (!pattern) {
    throw new Error('pattern must be supplied');
  }

  return extractDateParts(pattern, str);
}

/**
 * Used for testing - replace this function with a fixed date.
 */
function now() {
  return new Date();
}

module.exports = asString;
module.exports.asString = asString;
module.exports.parse = parse;
module.exports.now = now;
module.exports.ISO8601_FORMAT = 'yyyy-MM-ddThh:mm:ss.SSS';
module.exports.ISO8601_WITH_TZ_OFFSET_FORMAT = 'yyyy-MM-ddThh:mm:ss.SSSO';
module.exports.DATETIME_FORMAT = 'dd MM yyyy hh:mm:ss.SSS';
module.exports.ABSOLUTETIME_FORMAT = 'hh:mm:ss.SSS';
