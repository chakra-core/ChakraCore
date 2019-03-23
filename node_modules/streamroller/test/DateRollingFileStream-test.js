/**
 * This file will be removed.
 */
'use strict';

require('should');

const fs = require('fs')
  , zlib = require('zlib')
  , proxyquire = require('proxyquire').noPreserveCache()
  , util = require('util')
  , async = require('async')
  , streams = require('stream');

let fakeNow = new Date(2012, 8, 12, 10, 37, 11);
const mockNow = () => fakeNow;
const RollingFileWriteStream = proxyquire('../lib/RollingFileWriteStream', {
  './now': mockNow
});
const DateRollingFileStream = proxyquire('../lib/DateRollingFileStream', {
  './RollingFileWriteStream': RollingFileWriteStream
});

function remove(filename, cb) {
  fs.unlink(filename, function () {
    cb();
  });
}

describe('DateRollingFileStream', function () {
  describe('arguments', function () {
    var stream;

    before(function (done) {
      stream = new DateRollingFileStream(
        __dirname + '/test-date-rolling-file-stream-1',
        'yyyy-MM-dd.hh'
      );
      done();
    });

    after(function (done) {
      remove(__dirname + '/test-date-rolling-file-stream-1', done);
    });

    it('should take a filename and a pattern and return a WritableStream', function (done) {
      stream.filename.should.eql(__dirname + '/test-date-rolling-file-stream-1');
      stream.options.pattern.should.eql('yyyy-MM-dd.hh');
      stream.should.be.instanceOf(streams.Writable);
      done();
    });

    it('with default settings for the underlying stream', function (done) {
      stream.currentFileStream.mode.should.eql(420);
      stream.currentFileStream.flags.should.eql('a');
      done();
    });
  });

  describe('default arguments', function () {
    var stream;

    before(function (done) {
      stream = new DateRollingFileStream(__dirname + '/test-date-rolling-file-stream-2');
      done();
    });

    after(function (done) {
      remove(__dirname + '/test-date-rolling-file-stream-2', done);
    });

    it('should have pattern of .yyyy-MM-dd', function (done) {
      stream.options.pattern.should.eql('yyyy-MM-dd');
      done();
    });
  });

  describe('with stream arguments', function () {
    var stream;

    before(function (done) {
      stream = new DateRollingFileStream(
        __dirname + '/test-date-rolling-file-stream-3',
        'yyyy-MM-dd',
        {mode: parseInt('0666', 8)}
      );
      done();
    });

    after(function (done) {
      remove(__dirname + '/test-date-rolling-file-stream-3', done);
    });

    it('should pass them to the underlying stream', function (done) {
      stream.theStream.mode.should.eql(parseInt('0666', 8));
      done();
    });
  });

  describe('with stream arguments but no pattern', function () {
    var stream;

    before(function (done) {
      stream = new DateRollingFileStream(
        __dirname + '/test-date-rolling-file-stream-4',
        {mode: parseInt('0666', 8)}
      );
      done();
    });

    after(function (done) {
      remove(__dirname + '/test-date-rolling-file-stream-4', done);
    });

    it('should pass them to the underlying stream', function (done) {
      stream.theStream.mode.should.eql(parseInt('0666', 8));
      done();
    });

    it('should use default pattern', function (done) {
      stream.options.pattern.should.eql('yyyy-MM-dd');
      done();
    });
  });

  describe('with a pattern of .yyyy-MM-dd', function () {
    var stream;

    before(function (done) {
      stream = new DateRollingFileStream(
        __dirname + '/test-date-rolling-file-stream-5', '.yyyy-MM-dd',
        null
      );
      stream.write('First message\n', 'utf8', done);
    });

    after(function (done) {
      remove(__dirname + '/test-date-rolling-file-stream-5', done);
    });

    it('should create a file with the base name', function (done) {
      fs.readFile(__dirname + '/test-date-rolling-file-stream-5', 'utf8', function (err, contents) {
        contents.should.eql('First message\n');
        done(err);
      });
    });

    describe('when the day changes', function () {

      before(function (done) {
        fakeNow = new Date(2012, 8, 13, 0, 10, 12);
        stream.write('Second message\n', 'utf8', done);
      });

      after(function (done) {
        remove(__dirname + '/test-date-rolling-file-stream-5.2012-09-12', done);
      });

      describe('the number of files', function () {
        var files = [];

        before(function (done) {
          fs.readdir(__dirname, function (err, list) {
            files = list;
            done(err);
          });
        });

        it('should be two', function (done) {
          files.filter(
            function (file) {
              return file.indexOf('test-date-rolling-file-stream-5') > -1;
            }
          ).should.have.length(2);
          done();
        });
      });

      describe('the file without a date', function () {
        it('should contain the second message', function (done) {
          fs.readFile(
            __dirname + '/test-date-rolling-file-stream-5', 'utf8',
            function (err, contents) {
              contents.should.eql('Second message\n');
              done(err);
            }
          );
        });
      });

      describe('the file with the date', function () {
        it('should contain the first message', function (done) {
          fs.readFile(
            __dirname + '/test-date-rolling-file-stream-5.2012-09-12', 'utf8',
            function (err, contents) {
              contents.should.eql('First message\n');
              done(err);
            }
          );
        });
      });
    });
  });

  describe('with alwaysIncludePattern', function () {
    var stream;

    before(function (done) {
      fakeNow = new Date(2012, 8, 12, 11, 10, 12);
      remove(
        __dirname + '/test-date-rolling-file-stream-pattern.2012-09-12-11.log',
        function () {
          stream = new DateRollingFileStream(
            __dirname + '/test-date-rolling-file-stream-pattern',
            '.yyyy-MM-dd-hh.log',
            {alwaysIncludePattern: true}
          );
          setTimeout(function () {
            stream.write('First message\n', 'utf8', done);
          }, 50);
        }
      );
    });

    after(function (done) {
      remove(__dirname + '/test-date-rolling-file-stream-pattern.2012-09-12-11.log', done);
    });

    it('should create a file with the pattern set', function (done) {
      fs.readFile(
        __dirname + '/test-date-rolling-file-stream-pattern.2012-09-12-11.log', 'utf8',
        function (err, contents) {
          contents.should.eql('First message\n');
          done(err);
        }
      );
    });

    describe('when the day changes', function () {
      before(function (done) {
        fakeNow = new Date(2012, 8, 12, 12, 10, 12);
        stream.write('Second message\n', 'utf8', done);
      });

      after(function (done) {
        remove(__dirname + '/test-date-rolling-file-stream-pattern.2012-09-12-12.log', done);
      });

      describe('the number of files', function () {
        it('should be two', function (done) {
          fs.readdir(__dirname, function (err, files) {
            files.filter(
              function (file) {
                return file.indexOf('test-date-rolling-file-stream-pattern') > -1;
              }
            ).should.have.length(2);
            done(err);
          });
        });
      });

      describe('the file with the later date', function () {
        it('should contain the second message', function (done) {
          fs.readFile(
            __dirname + '/test-date-rolling-file-stream-pattern.2012-09-12-12.log', 'utf8',
            function (err, contents) {
              contents.should.eql('Second message\n');
              done(err);
            }
          );
        });
      });

      describe('the file with the date', function () {
        it('should contain the first message', function (done) {
          fs.readFile(
            __dirname + '/test-date-rolling-file-stream-pattern.2012-09-12-11.log', 'utf8',
            function (err, contents) {
              contents.should.eql('First message\n');
              done(err);
            }
          );
        });
      });
    });
  });

  describe('with a pattern that evaluates to digits', function() {
    let stream;
    before(done => {
      fakeNow = new Date(2012, 8, 12, 0, 10, 12);
      stream = new DateRollingFileStream(
        __dirname + '/digits.log',
        '.yyyyMMdd'
      );
      stream.write('First message\n', 'utf8', done);
    });

    describe('when the day changes', function () {
      before(function (done) {
        fakeNow = new Date(2012, 8, 13, 0, 10, 12);
        stream.write('Second message\n', 'utf8', done);
      });

      it('should be two files (it should not get confused by indexes)', function (done) {
        fs.readdir(__dirname, function (err, files) {
          var logFiles = files.filter(
            function (file) {
              return file.indexOf('digits.log') > -1;
            }
          );
          logFiles.should.have.length(2);

          fs.readFile(__dirname + '/digits.log.20120912', 'utf8',
            (err, contents) => {
              contents.should.eql('First message\n');
              fs.readFile(__dirname + '/digits.log', 'utf8', (e, c) => {
                c.should.eql('Second message\n');
                done(err || e);
              });
            }
          );
        });
      });
    });

    after(function (done) {
      remove(
        __dirname + '/digits.log',
        function () {
          remove(__dirname + '/digits.log.20120912', done);
        }
      );
    });

  });

  describe('with compress option', function () {
    var stream;

    before(function (done) {
      fakeNow = new Date(2012, 8, 12, 0, 10, 12);
      stream = new DateRollingFileStream(
        __dirname + '/compressed.log',
        '.yyyy-MM-dd',
        {compress: true}
      );
      stream.write('First message\n', 'utf8', done);
    });

    describe('when the day changes', function () {
      before(function (done) {
        fakeNow = new Date(2012, 8, 13, 0, 10, 12);
        stream.write('Second message\n', 'utf8', done);
      });

      it('should be two files, one compressed', function (done) {
        fs.readdir(__dirname, function (err, files) {
          var logFiles = files.filter(
            function (file) {
              return file.indexOf('compressed.log') > -1;
            }
          );
          logFiles.should.have.length(2);

          zlib.gunzip(
            fs.readFileSync(__dirname + '/compressed.log.2012-09-12.gz'),
            function (err, contents) {
              contents.toString('utf8').should.eql('First message\n');
              fs.readFileSync(__dirname + '/compressed.log', 'utf8').should.eql('Second message\n');
              done(err);
            }
          );
        });
      });
    });

    after(function (done) {
      remove(
        __dirname + '/compressed.log',
        function () {
          remove(__dirname + '/compressed.log.2012-09-12.gz', done);
        }
      );
    });

  });

  describe('with keepFileExt option', function () {
    var stream;

    before(function (done) {
      fakeNow = new Date(2012, 8, 12, 0, 10, 12);
      stream = new DateRollingFileStream(
        __dirname + '/keepFileExt.log',
        '.yyyy-MM-dd',
        {keepFileExt: true}
      );
      stream.write('First message\n', 'utf8', done);
    });

    describe('when the day changes', function () {
      before(function (done) {
        fakeNow = new Date(2012, 8, 13, 0, 10, 12);
        stream.write('Second message\n', 'utf8', done);
      });

      it('should be two files', function (done) {
        fs.readdir(__dirname, function (err, files) {
          var logFiles = files.filter(
            function (file) {
              return file.indexOf('keepFileExt') > -1;
            }
          );
          logFiles.should.have.length(2);
          fs.readFileSync(__dirname + '/keepFileExt.2012-09-12.log', 'utf8')
            .should.eql('First message\n');
          fs.readFileSync(__dirname + '/keepFileExt.log', 'utf8')
            .should.eql('Second message\n');
          done(err);
        });
      });
    });

    after(function (done) {
      remove(
        __dirname + '/keepFileExt.log',
        function () {
          remove(__dirname + '/keepFileExt.2012-09-12.log', done);
        }
      );
    });

  });

  describe('with compress option and keepFileExt option', function () {
    var stream;

    before(function (done) {
      fakeNow = new Date(2012, 8, 12, 0, 10, 12);
      stream = new DateRollingFileStream(
        __dirname + '/compressedAndKeepExt.log',
        '.yyyy-MM-dd',
        {compress: true, keepFileExt: true}
      );
      stream.write('First message\n', 'utf8', done);
    });

    describe('when the day changes', function () {
      before(function (done) {
        fakeNow = new Date(2012, 8, 13, 0, 10, 12);
        stream.write('Second message\n', 'utf8', done);
      });

      it('should be two files, one compressed', function (done) {
        fs.readdir(__dirname, function (err, files) {
          var logFiles = files.filter(
            function (file) {
              return file.indexOf('compressedAndKeepExt') > -1;
            }
          );
          logFiles.should.have.length(2);

          zlib.gunzip(
            fs.readFileSync(__dirname + '/compressedAndKeepExt.2012-09-12.log.gz'),
            function (err, contents) {
              contents.toString('utf8').should.eql('First message\n');
              fs.readFileSync(__dirname + '/compressedAndKeepExt.log', 'utf8')
                .should.eql('Second message\n');
              done(err);
            }
          );
        });
      });
    });

    after(function (done) {
      remove(
        __dirname + '/compressedAndKeepExt.log',
        function () {
          remove(__dirname + '/compressedAndKeepExt.2012-09-12.log.gz', done);
        }
      );
    });

  });

  describe('with daysToKeep option', function () {
    var stream;
    var daysToKeep = 4;
    var numOriginalLogs = 10;

    before(function (done) {
      var day = 0;
      var streams = [];
      async.whilst(
        function () {
          return day < numOriginalLogs;
        },
        function (nextCallback) {
          fakeNow = new Date(2012, 8, 20 - day, 0, 10, 12);
          var currentStream = new DateRollingFileStream(
            __dirname + '/daysToKeep.log',
            '.yyyy-MM-dd',
            {
              alwaysIncludePattern: true,
              daysToKeep: daysToKeep
            }
          );
          async.waterfall([
            function (callback) {
              currentStream.write(util.format('Message on day %d\n', day), 'utf8', callback);
            },
            function (callback) {
              fs.utimes(currentStream.filename, fakeNow, fakeNow, callback);
            }
          ],
          function (err) {
            day++;
            streams.push(currentStream);
            nextCallback(err);
          });
        },
        function (err) {
          stream = streams[0];
          done(err);
        });
      });

      describe('when the day changes', function () {
        before(function (done) {
          fakeNow = new Date(2012, 8, 21, 0, 10, 12);
          stream.write('Second message\n', 'utf8', done);
        });

        it('should be daysToKeep + 1 files left from numOriginalLogs', function (done) {
          fs.readdir(__dirname, function (err, files) {
            var logFiles = files.filter(
              function (file) {
                return file.indexOf('daysToKeep.log') > -1;
              }
            );
            logFiles.should.have.length(daysToKeep + 1);
            done(err);
          });
        });
      });

      after(function (done) {
        fs.readdir(__dirname, function (err, files) {
          var logFiles = files.filter(
            function (file) {
              return file.indexOf('daysToKeep.log') > -1;
            }
          );
          async.each(logFiles, (logFile, nextCallback) => {
            remove(__dirname + '/' + logFile, nextCallback);
          }, done);
        });
      });
    });

  describe('with daysToKeep and compress options', function () {
    var stream;
    var daysToKeep = 4;
    var numOriginalLogs = 10;

    before(function (done) {
      var day = 0;
      var streams = [];
      async.whilst(
        function () {
          return day < numOriginalLogs;
        },
        function (nextCallback) {
          fakeNow = new Date(2012, 8, 20 - day, 0, 10, 12);
          var currentStream = new DateRollingFileStream(
            __dirname + '/compressedDaysToKeep.log',
            '.yyyy-MM-dd',
            {
              alwaysIncludePattern: true,
              compress: true,
              daysToKeep: daysToKeep
            }
          );
          async.waterfall([
            function (callback) {
              currentStream.write(util.format('Message on day %d\n', day), 'utf8', callback);
            },
            function (callback) {
              var filename = currentStream.filename;
              var gzip = zlib.createGzip();
              var inp = fs.createReadStream(filename);
              var out = fs.createWriteStream(filename+'.gz');
              inp.pipe(gzip).pipe(out);

              out.on('finish', function () {
                fs.unlink(filename, callback);
              });
            },
            function (callback) {
              fs.utimes(currentStream.filename + '.gz', fakeNow, fakeNow, callback);
            }
          ],
          function (err) {
            day++;
            streams.push(currentStream);
            nextCallback(err);
          });
        },
        () => {
          stream = streams[0];
          done();
        }
      );
    });

    describe('when the day changes', function () {
      before(function (done) {
        fakeNow = new Date(2012, 8, 21, 0, 10, 12);
        stream.write('New file message\n', 'utf8', done);
      });

      it('should be 4 files left from original 3', function (done) {
        fs.readdir(__dirname, function (err, files) {
          var logFiles = files.filter(
            function (file) {
              return file.indexOf('compressedDaysToKeep.log') > -1;
            }
          );
          logFiles.should.have.length(daysToKeep + 1);
          done(err);
        });
      });
    });

    after(function (done) {
      fs.readdir(__dirname, function (err, files) {
        var logFiles = files.filter(
          function (file) {
            return file.indexOf('compressedDaysToKeep.log') > -1;
          }
        );
        async.each(logFiles, function (logFile, nextCallback) {
          remove(__dirname + '/' + logFile, nextCallback);
        },
        function (err) {
          done(err);
        });
      });
    });
  });
});
