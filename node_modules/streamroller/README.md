streamroller
============

node.js file streams that roll over when they reach a maximum size, or a date/time.

        npm install streamroller

## usage

        var rollers = require('streamroller');
        var stream = new rollers.RollingFileStream('myfile', 1024, 3);
        stream.write("stuff");
        stream.end();

The streams behave the same as standard node.js streams, except that when certain conditions are met they will rename the current file to a backup and start writing to a new file.

### new RollingFileStream(filename [, maxSize, numBackups, options])
* `filename` (String)
* `maxSize` - the size in bytes to trigger a rollover, if not provided this defaults to MAX_SAFE_INTEGER and the stream will not roll.
* `numBackups` - the number of old files to keep
* `options` - Object
  * `encoding` - defaults to 'utf8'
  * `mode` - defaults to 0644
  * `flags` - defaults to 'a' (see [fs.open](https://nodejs.org/dist/latest-v8.x/docs/api/fs.html#fs_fs_open_path_flags_mode_callback) for more details)
  * `compress` - (boolean) defaults to `false` - compress the backup files using gzip (files will have `.gz` extension).
  * `keepFileExt` - (boolean) defaults to `false` - keep the file original extension. e.g.: `abc.log -> abc.1.log`.

This returns a `WritableStream`. When the current file being written to (given by `filename`) gets up to or larger than `maxSize`, then the current file will be renamed to `filename.1` and a new file will start being written to. Up to `numBackups` of old files are maintained, so if `numBackups` is 3 then there will be 4 files:
<pre>
     filename
     filename.1
     filename.2
     filename.3
</pre>
When filename size >= maxSize then:
<pre>
     filename -> filename.1
     filename.1 -> filename.2
     filename.2 -> filename.3
     filename.3 gets overwritten
     filename is a new file
</pre>

### new DateRollingFileStream(filename, pattern, options)
* `filename` (String)
* `pattern` (String) - the date pattern to trigger rolling (see below)
* `options` - Object
	* `encoding` - defaults to 'utf8'
	* `mode` defaults to 0644
	* `flags` defaults to 'a' (see [fs.open](https://nodejs.org/dist/latest-v8.x/docs/api/fs.html#fs_fs_open_path_flags_mode_callback) for more details)
    * `compress` - (boolean) compress the backup files, defaults to false
    * `keepFileExt` - (boolean) defaults to `false` - keep the file original extension. e.g.: `abc.log -> abc.2013-08-30.log`.
	* `alwaysIncludePattern` - (boolean) extend the initial file with the pattern, defaults to false
  * `daysToKeep` - (integer) if this is greater than 0, then files older than `daysToKeep` days will be deleted during file rolling.


This returns a `WritableStream`. When the current time, formatted as `pattern`, changes then the current file will be renamed to `filename.formattedDate` where `formattedDate` is the result of processing the date through the pattern, and a new file will begin to be written. Streamroller uses [date-format](http://github.com/nomiddlename/date-format) to format dates, and the `pattern` should use the date-format format. e.g. with a `pattern` of `".yyyy-MM-dd"`, and assuming today is August 29, 2013 then writing to the stream today will just write to `filename`. At midnight (or more precisely, at the next file write after midnight), `filename` will be renamed to `filename.2013-08-29` and a new `filename` will be created. If `options.alwaysIncludePattern` is true, then the initial file will be `filename.2013-08-29` and no renaming will occur at midnight, but a new file will be written to with the name `filename.2013-08-30`.
