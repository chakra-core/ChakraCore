'use strict';

require('should');
var dateFormat = require('../lib');

describe('dateFormat.parse', function() {
  it('should require a pattern', function() {
    (function() { dateFormat.parse() }).should.throw(/pattern must be supplied/);
    (function() { dateFormat.parse(null) }).should.throw(/pattern must be supplied/);
    (function() { dateFormat.parse('') }).should.throw(/pattern must be supplied/);
  });

  describe('with a pattern that has no replacements', function() {
    it('should return a new date when the string matches', function() {
      dateFormat.parse('cheese', 'cheese').should.be.a.Date()
    });

    it('should throw if the string does not match', function() {
      (function() {
        dateFormat.parse('cheese', 'biscuits');
      }).should.throw(/String 'biscuits' could not be parsed as 'cheese'/);
    });
  });

  describe('with a full pattern', function() {
    var pattern = 'yyyy-MM-dd hh:mm:ss.SSSO';

    it('should return the correct date if the string matches', function() {
      var testDate = new Date();
      testDate.setFullYear(2018);
      testDate.setMonth(8);
      testDate.setDate(13);
      testDate.setHours(18);
      testDate.setMinutes(10);
      testDate.setSeconds(12);
      testDate.setMilliseconds(392);
      testDate.getTimezoneOffset = function() { return 600; };

      dateFormat.parse(pattern, '2018-09-13 08:10:12.392+1000').getTime().should.eql(testDate.getTime());
    });

    it('should throw if the string does not match', function() {
      (function() {
        dateFormat.parse(pattern, 'biscuits')
      }).should.throw(/String 'biscuits' could not be parsed as 'yyyy-MM-dd hh:mm:ss.SSSO'/);
    });
  });

  describe('with a partial pattern', function() {
    var testDate = new Date();
    dateFormat.now = function() { return testDate; };

    function verifyDate(actual, expected) {
      actual.getFullYear().should.eql(expected.year || testDate.getFullYear());
      actual.getMonth().should.eql(expected.month || testDate.getMonth());
      actual.getDate().should.eql(expected.day || testDate.getDate());
      actual.getHours().should.eql(expected.hours || testDate.getHours());
      actual.getMinutes().should.eql(expected.minutes || testDate.getMinutes());
      actual.getSeconds().should.eql(expected.seconds || testDate.getSeconds());
      actual.getMilliseconds().should.eql(expected.milliseconds || testDate.getMilliseconds());
    }

    it('should return a date with missing values defaulting to current time', function() {
      var date = dateFormat.parse('yyyy-MM', '2015-09');
      verifyDate(date, { year: 2015, month: 8 });
    });

    it('should handle variations on the same pattern', function() {
      var date = dateFormat.parse('MM-yyyy', '09-2015');
      verifyDate(date, { year: 2015, month: 8 });

      date = dateFormat.parse('yyyy MM', '2015 09');
      verifyDate(date, { year: 2015, month: 8 });

      date = dateFormat.parse('MM, yyyy.', '09, 2015.');
      verifyDate(date, { year: 2015, month: 8 });
    });

    it('should match all the date parts', function() {
      var date = dateFormat.parse('dd', '21');
      verifyDate(date, { day: 21 });

      date = dateFormat.parse('hh', '12');
      verifyDate(date, { hours: 12 });

      date = dateFormat.parse('mm', '34');
      verifyDate(date,  { minutes: 34 });

      date = dateFormat.parse('ss', '59');
      verifyDate(date, { seconds: 59 });

      date = dateFormat.parse('ss.SSS', '23.452');
      verifyDate(date, { seconds: 23, milliseconds: 452 });

      date = dateFormat.parse('hh:mm O', '05:23 +1000');
      verifyDate(date, { hours: 15, minutes: 23 });

      date = dateFormat.parse('hh:mm O', '05:23 -200');
      verifyDate(date, { hours: 3, minutes: 23 });

      date = dateFormat.parse('hh:mm O', '05:23 +0930');
      verifyDate(date, { hours: 14, minutes: 53 });
    });
  });

  describe('with a date formatted by this library', function() {
    var testDate = new Date();
    testDate.setUTCFullYear(2018);
    testDate.setUTCMonth(8);
    testDate.setUTCDate(13);
    testDate.setUTCHours(18);
    testDate.setUTCMinutes(10);
    testDate.setUTCSeconds(12);
    testDate.setUTCMilliseconds(392);

    it('should format and then parse back to the same date', function() {
      dateFormat.parse(
        dateFormat.ISO8601_WITH_TZ_OFFSET_FORMAT,
        dateFormat(dateFormat.ISO8601_WITH_TZ_OFFSET_FORMAT, testDate)
      ).should.eql(testDate);

      dateFormat.parse(
        dateFormat.ISO8601_FORMAT,
        dateFormat(dateFormat.ISO8601_FORMAT, testDate)
      ).should.eql(testDate);

      dateFormat.parse(
        dateFormat.DATETIME_FORMAT,
        dateFormat(dateFormat.DATETIME_FORMAT, testDate)
      ).should.eql(testDate);

      dateFormat.parse(
        dateFormat.ABSOLUTETIME_FORMAT,
        dateFormat(dateFormat.ABSOLUTETIME_FORMAT, testDate)
      ).should.eql(testDate);
    });

  });
});
