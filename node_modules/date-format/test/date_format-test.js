'use strict';

require('should');

var dateFormat = require('../lib');

function createFixedDate() {
    return new Date(2010, 0, 11, 14, 31, 30, 5);
}

describe('date_format', function() {
    var date = createFixedDate();

    it('should default to now when a date is not provided', function() {
        dateFormat.asString(dateFormat.DATETIME_FORMAT).should.not.be.empty();
    });

    it('should be usable directly without calling asString', function() {
        dateFormat(dateFormat.DATETIME_FORMAT, date).should.eql('11 01 2010 14:31:30.005');
    });

    it('should format a date as string using a pattern', function() {
        dateFormat.asString(dateFormat.DATETIME_FORMAT, date).should.eql('11 01 2010 14:31:30.005');
    });

    it('should default to the ISO8601 format', function() {
        dateFormat.asString(date).should.eql('2010-01-11T14:31:30.005');
    });

    it('should provide a ISO8601 with timezone offset format', function() {
        var tzDate = createFixedDate();
        tzDate.setMinutes(tzDate.getMinutes() - tzDate.getTimezoneOffset() - 660);
        tzDate.getTimezoneOffset = function () {
            return -660;
        };

        // when tz offset is in the pattern, the date should be in UTC
        dateFormat.asString(dateFormat.ISO8601_WITH_TZ_OFFSET_FORMAT, tzDate)
            .should.eql('2010-01-11T03:31:30.005+1100');

        tzDate = createFixedDate();
        tzDate.setMinutes((tzDate.getMinutes() - tzDate.getTimezoneOffset()) + 120);
        tzDate.getTimezoneOffset = function () {
            return 120;
        };

        dateFormat.asString(dateFormat.ISO8601_WITH_TZ_OFFSET_FORMAT, tzDate)
            .should.eql('2010-01-11T16:31:30.005-0200');
    });

    it('should provide a just-the-time format', function() {
        dateFormat.asString(dateFormat.ABSOLUTETIME_FORMAT, date).should.eql('14:31:30.005');
    });

    it('should provide a custom format', function() {
        var customDate = createFixedDate();
        customDate.setMinutes((customDate.getMinutes() - customDate.getTimezoneOffset()) + 120);
        customDate.getTimezoneOffset = function () {
            return 120;
        };

        dateFormat.asString('O.SSS.ss.mm.hh.dd.MM.yy', customDate).should.eql('-0200.005.30.31.16.11.01.10');
    });
});
