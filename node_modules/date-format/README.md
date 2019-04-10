date-format
===========

node.js formatting of Date objects as strings. Probably exactly the same as some other library out there.

```sh
npm install date-format
```

usage
=====

Formatting dates as strings
----

```javascript
var format = require('date-format');
format.asString(); //defaults to ISO8601 format and current date.
format.asString(new Date()); //defaults to ISO8601 format
format.asString('hh:mm:ss.SSS', new Date()); //just the time
```

or

```javascript
var format = require('date-format');
format(); //defaults to ISO8601 format and current date.
format(new Date());
format('hh:mm:ss.SSS', new Date());
```

Format string can be anything, but the following letters will be replaced (and leading zeroes added if necessary):
* dd - `date.getDate()`
* MM - `date.getMonth() + 1`
* yy - `date.getFullYear().toString().substring(2, 4)`
* yyyy - `date.getFullYear()`
* hh - `date.getHours()`
* mm - `date.getMinutes()`
* ss - `date.getSeconds()`
* SSS - `date.getMilliseconds()`
* O - timezone offset in +hm format (note that time will be in UTC if displaying offset)

Built-in formats:
* `format.ISO8601_FORMAT` - `2017-03-14T14:10:20.391` (local time used)
* `format.ISO8601_WITH_TZ_OFFSET_FORMAT` - `2017-03-14T03:10:20.391+1100` (UTC + TZ used)
* `format.DATETIME_FORMAT` - `14 03 2017 14:10:20.391` (local time used)
* `format.ABSOLUTETIME_FORMAT` - `14:10:20.391` (local time used)

Parsing strings as dates
----
The date format library has limited ability to parse strings into dates. It can convert strings created using date format patterns (as above), but if you're looking for anything more sophisticated than that you should probably look for a better library ([momentjs](https://momentjs.com) does pretty much everything).

```javascript
var format = require('date-format');
// pass in the format of the string as first argument
format.parse(format.ISO8601_FORMAT, '2017-03-14T14:10:20.391');
// returns Date
```
