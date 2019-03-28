/*
 * Date Format 1.2.3
 * (c) 2007-2009 Steven Levithan <stevenlevithan.com>
 * MIT license
 *
 * Includes enhancements by Scott Trenda <scott.trenda.net>
 * and Kris Kowal <cixar.com/~kris.kowal/>
 * and Johan Heander <timezynk.com>
 *
 * Accepts a date, a mask, or a date and a mask.
 * Returns a formatted version of the given date.
 * The date defaults to the current date/time.
 * The mask defaults to dateFormat.masks.default.
 */

(function(global) {
    'use strict';

    var DEFAULT_ISO_DATE_FORMAT = 'yyyy-mm-dd';

    var dateFormat = (function() {
        var token = /d{1,4}|m{1,4}|yy(?:yy)?|([HhMsTt])\1?|[LloSZWNVG]|"[^"]*"|'[^']*'/g;

        // Regexes and supporting functions are cached through closure
        return function (date, mask, utc, gmt) {

            // You can't provide utc if you skip other args (use the 'UTC:' mask prefix)
            if (arguments.length === 1 && typeof(date) === 'string' && !/\d/.test(date)) {
                mask = date;
                date = undefined;
            }

            date = date || new Date;

            if(!(date instanceof Date)) {
                date = new Date(date);
            }

            if (isNaN(date)) {
                throw TypeError('Invalid date');
            }

            mask = String(dateFormat.masks[mask] || mask || dateFormat.masks['default']);

            // Shortcut for faster iso date formatting
            if (mask === DEFAULT_ISO_DATE_FORMAT) {
                return getISODateString(date);
            }

            // Allow setting the utc/gmt argument via the mask
            var maskSlice = mask.slice(0, 4);
            if (maskSlice === 'UTC:' || maskSlice === 'GMT:') {
                mask = mask.slice(4);
                utc = true;
                if (maskSlice === 'GMT:') {
                    gmt = true;
                }
            }

            var _ = utc ? 'getUTC' : 'get';
            var d = date[_ + 'Date']();
            var D = date[_ + 'Day']();
            var m = date[_ + 'Month']();
            var y = date[_ + 'FullYear']();
            var H = date[_ + 'Hours']();
            var M = date[_ + 'Minutes']();
            var s = date[_ + 'Seconds']();
            var L = date[_ + 'Milliseconds']();
            var flags = {
                d    : d,
                dd   : pad(d),
                ddd  : dateFormat.i18n.dayNames[D],
                dddd : dateFormat.i18n.dayNames[D + 7],
                m    : m + 1,
                mm   : pad(m + 1),
                mmm  : dateFormat.i18n.monthNames[m],
                mmmm : dateFormat.i18n.monthNames[m + 12],
                yy   : String(y).slice(2),
                yyyy : y,
                h    : H % 12 || 12,
                hh   : pad(H % 12 || 12),
                H    : H,
                HH   : pad(H),
                M    : M,
                MM   : pad(M),
                s    : s,
                ss   : pad(s),
                l    : pad(L, 3),
                L    : pad(Math.round(L / 10)),
                t    : H < 12 ? 'a'  : 'p',
                tt   : H < 12 ? 'am' : 'pm',
                T    : H < 12 ? 'A'  : 'P',
                TT   : H < 12 ? 'AM' : 'PM',
                Z    : getTimezone,
                o    : getTimezoneOffset,
                S    : ['th', 'st', 'nd', 'rd'][d % 10 > 3 ? 0 : (d % 100 - d % 10 != 10) * d % 10],
                W    : getWeek,
                N    : getDayOfWeek,
                V    : getWeek,
                G    : getWeekYear
            };

            return mask.replace(token, function (match) {
                if (match in flags) {
                    var f = flags[match];
                    if (typeof(f) === 'function') {
                        var val = f(date, utc, gmt);
                        flags[match] = val;
                        return val;
                    } else {
                        return f;
                    }
                } else {
                    return match.slice(1, match.length - 1);
                }
            });
        };
    })();

    dateFormat.masks = {
        'default'             : 'ddd mmm dd yyyy HH:MM:ss',
        'shortDate'           : 'm/d/yy',
        'mediumDate'          : 'mmm d, yyyy',
        'longDate'            : 'mmmm d, yyyy',
        'fullDate'            : 'dddd, mmmm d, yyyy',
        'shortTime'           : 'h:MM TT',
        'mediumTime'          : 'h:MM:ss TT',
        'longTime'            : 'h:MM:ss TT Z',
        'isoDate'             : DEFAULT_ISO_DATE_FORMAT,
        'isoTime'             : 'HH:MM:ss',
        'isoDateTime'         : 'yyyy-mm-dd\'T\'HH:MM:sso',
        'isoUtcDateTime'      : 'UTC:yyyy-mm-dd\'T\'HH:MM:ss\'Z\'',
        'expiresHeaderFormat' : 'ddd, dd mmm yyyy HH:MM:ss Z'
    };

    // Internationalization strings
    dateFormat.i18n = {
        dayNames: [
            'Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat',
            'Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday'
        ],
        monthNames: [
            'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec',
            'January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December'
        ]
    };

    function pad(val, len) {
        val = String(val);
        len = len || 2;
        while (val.length < len) {
            val = '0' + val;
        }
        return val;
    }

    /**
     * Get the ISO 8601 week number
     * Based on comments from
     * http://techblog.procurios.nl/k/n618/news/view/33796/14863/Calculate-ISO-8601-week-and-year-in-javascript.html
     *
     * @param  {Object} `date`
     * @return {Number}
     */
    function getWeek(date) {
        // Create a copy of date object
        var target = new Date(date.valueOf());

        // ISO week date weeks start on monday
        // so correct the day number
        var dayNr = (date.getDay() + 6) % 7;

        // ISO 8601 states that week 1 is the week
        // with the first thursday of that year.
        // Set the target date to the thursday in the target week
        target.setDate(target.getDate() - dayNr + 3);

        // Store the millisecond value of the target date
        var firstThursday = target.valueOf();

        // Set the target to the first thursday of the year
        // First set the target to january first
        target.setMonth(0, 1);
        // Not a thursday? Correct the date to the next thursday
        if (target.getDay() != 4) {
            target.setMonth(0, 1 + ((4 - target.getDay()) + 7) % 7);
        }

        // The weeknumber is the number of weeks between the
        // first thursday of the year and the thursday in the target week
        return 1 + Math.ceil((firstThursday - target) / 604800000); // 604800000 = 7 * 24 * 3600 * 1000
    }

    function getWeekYear(date) {
        // Create a new date object for the thursday of date's week
        var target  = new Date(date.valueOf());
        target.setDate(target.getDate() - ((date.getDay() + 6) % 7) + 3);

        return target.getFullYear();
    }

    /**
     * Get ISO-8601 numeric representation of the day of the week
     * 1 (for Monday) through 7 (for Sunday)
     *
     * @param  {Object} `date`
     * @return {Number}
     */
    function getDayOfWeek(date) {
        var dow = date.getDay();
        if(dow === 0) {
            dow = 7;
        }
        return dow;
    }

    var timezone = /\b(?:[PMCEA][SDP]T|(?:Pacific|Mountain|Central|Eastern|Atlantic) (?:Standard|Daylight|Prevailing) Time|(?:GMT|UTC)(?:[-+]\d{4})?)\b/g;
    var timezoneClip = /[^-+\dA-Z]/g;
    function getTimezone(date, utc, gmt) {
        return gmt ? 'GMT' : utc ? 'UTC' : (String(date).match(timezone) || ['']).pop().replace(timezoneClip, '');
    }

    function getTimezoneOffset(date, utc) {
        var o = utc ? 0 : date.getTimezoneOffset();
        return (o > 0 ? '-' : '+') + pad(Math.floor(Math.abs(o) / 60) * 100 + Math.abs(o) % 60, 4);
    }

    function getISODateString(date) {
        var str = String(date.getFullYear()) + '-',
            m = date.getMonth() + 1,
            d = date.getDate();

        if (m < 10) {
            str += '0';
        }
        str += m + '-';

        if (d < 10) {
            str += '0';
        }
        return str + d;
    }

    if (typeof(define) === 'function' && define.amd) {
        define(function () {
            return dateFormat;
        });
    } else if (typeof(exports) === 'object') {
        module.exports = dateFormat;
    } else {
        global.dateFormat = dateFormat;
    }
})(this);
