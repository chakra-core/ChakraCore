'use strict';

require('should');

var async = require('async')
  , fs = require('fs')
  , path = require('path')
  , zlib = require('zlib')
  , streams = require('stream')
  , RollingFileStream = require('../lib').RollingFileStream;

function remove(filename, cb) {
  fs.unlink(filename, function () { cb() });
}

function create(filename, cb) {
  fs.writeFile(filename, 'test file', cb);
}

describe('RollingFileStream', function () {

  describe('arguments', function () {
    var stream;

    before(function (done) {
      remove(__dirname + '/test-rolling-file-stream', function () {
        stream = new RollingFileStream(__dirname + '/test-rolling-file-stream', 1024, 5);
        done();
      });
    });

    after(function (done) {
      remove(__dirname + '/test-rolling-file-stream', done);
    });

    it('should take a filename, file size (bytes), no. backups, return Writable', function () {
      stream.should.be.an.instanceOf(streams.Writable);
      stream.filename.should.eql(__dirname + '/test-rolling-file-stream');
      stream.size.should.eql(1024);
      stream.backups.should.eql(5);
    });

    it('should apply default settings to the underlying stream', function () {
      stream.theStream.mode.should.eql(420);
      stream.theStream.flags.should.eql('a');
    });
  });

  describe('with stream arguments', function () {
    it('should pass them to the underlying stream', function () {
      var stream = new RollingFileStream(
        __dirname + '/test-rolling-file-stream',
        1024,
        5,
        {mode: parseInt('0666', 8)}
      );
      stream.theStream.mode.should.eql(parseInt('0666', 8));
    });

    after(function (done) {
      remove(__dirname + '/test-rolling-file-stream', done);
    });
  });

  describe('without size', function () {
    it('should default to max int size', function () {
      var stream = new RollingFileStream(__dirname + '/test-rolling-file-stream');
      stream.size.should.eql(Number.MAX_SAFE_INTEGER);
    });

    after(function (done) {
      remove(__dirname + '/test-rolling-file-stream', done);
    });
  });

  describe('without number of backups', function () {
    it('should default to 1 backup', function () {
      var stream = new RollingFileStream(__dirname + '/test-rolling-file-stream', 1024);
      stream.backups.should.eql(1);
    });

    after(function (done) {
      remove(__dirname + '/test-rolling-file-stream', done);
    });
  });

  describe('writing less than the file size', function () {

    before(function (done) {
      remove(__dirname + '/test-rolling-file-stream-write-less', function () {
        var stream = new RollingFileStream(
          __dirname + '/test-rolling-file-stream-write-less',
          100
        );
        stream.write('cheese', 'utf8', function () {
          stream.end(done);
        });
      });
    });

    after(function (done) {
      remove(__dirname + '/test-rolling-file-stream-write-less', done);
    });

    it('should write to the file', function (done) {
      fs.readFile(
        __dirname + '/test-rolling-file-stream-write-less', 'utf8',
        function (err, contents) {
          contents.should.eql('cheese');
          done(err);
        }
      );
    });

    it('should write one file', function (done) {
      fs.readdir(__dirname, function (err, files) {
        files.filter(
          function (file) { return file.indexOf('test-rolling-file-stream-write-less') > -1 }
        ).should.have.length(1);
        done(err);
      });
    });
  });

  describe('writing more than the file size', function () {
    before(function (done) {
      async.forEach(
        [
          __dirname + '/test-rolling-file-stream-write-more',
          __dirname + '/test-rolling-file-stream-write-more.1'
        ],
        remove,
        function () {
          var stream = new RollingFileStream(
            __dirname + '/test-rolling-file-stream-write-more',
            45
          );
          async.forEachSeries(
            [0, 1, 2, 3, 4, 5, 6],
            function (i, cb) {
              stream.write(i +'.cheese\n', 'utf8', cb);
            },
            function () {
              stream.end(done);
            }
          );
        }
      );
    });

    after(function (done) {
      async.forEach(
        [
          __dirname + '/test-rolling-file-stream-write-more',
          __dirname + '/test-rolling-file-stream-write-more.1'
        ],
        remove,
        done
      );
    });

    it('should write two files' , function (done) {
      fs.readdir(__dirname, function (err, files) {
        files.filter(
          function (file) {
            return file.indexOf('test-rolling-file-stream-write-more') > -1;
          }
        ).should.have.length(2);
        done(err);
      });
    });

    it('should write the last two log messages to the first file', function (done) {
      fs.readFile(
        __dirname + '/test-rolling-file-stream-write-more', 'utf8',
        function (err, contents) {
          contents.should.eql('5.cheese\n6.cheese\n');
          done(err);
        });
    });

    it('should write the first five log messages to the second file', function (done) {
      fs.readFile(
        __dirname + '/test-rolling-file-stream-write-more.1', 'utf8',
        function (err, contents) {
          contents.should.eql('0.cheese\n1.cheese\n2.cheese\n3.cheese\n4.cheese\n');
          done(err);
        }
      );
    });
  });

  describe('with options.compress = true', function () {
    before(function (done) {
      var stream = new RollingFileStream(
        path.join(__dirname, 'compressed-backups.log'),
        30, //30 bytes max size
        2,  //two backup files to keep
        {compress: true}
      );
      async.forEachSeries(
        [
          'This is the first log message.',
          'This is the second log message.',
          'This is the third log message.',
          'This is the fourth log message.'
        ],
        function (i, cb) {
          stream.write(i + '\n', 'utf8', cb);
        },
        function () {
          stream.end(done);
        }
      );
    });

    it('should produce three files, with the backups compressed', function (done) {
      fs.readdir(__dirname, function (err, files) {
        var testFiles = files.filter(
          function (f) { return f.indexOf('compressed-backups.log') > -1 }
        ).sort();

        testFiles.length.should.eql(3);
        testFiles.should.eql([
          'compressed-backups.log',
          'compressed-backups.log.1.gz',
          'compressed-backups.log.2.gz',
        ]);

        fs.readFile(path.join(__dirname, testFiles[0]), 'utf8', function (err, contents) {
          contents.should.eql('This is the fourth log message.\n');

          zlib.gunzip(fs.readFileSync(path.join(__dirname, testFiles[1])),
            function (err, contents) {
              contents.toString('utf8').should.eql('This is the third log message.\n');
              zlib.gunzip(fs.readFileSync(path.join(__dirname, testFiles[2])),
                function (err, contents) {
                  contents.toString('utf8').should.eql('This is the second log message.\n');
                  done(err);
                }
              );
            }
          );
        });
      });
    });

    after(function (done) {
      async.forEach([
        path.join(__dirname, 'compressed-backups.log'),
        path.join(__dirname, 'compressed-backups.log.1.gz'),
        path.join(__dirname, 'compressed-backups.log.2.gz'),
      ], remove, done);
    });

  });

  describe('with options.keepFileExt = true', function () {
    before(function (done) {
      var stream = new RollingFileStream(
        path.join(__dirname, 'extKept-backups.log'),
        30, //30 bytes max size
        2,  //two backup files to keep
        {keepFileExt: true}
      );
      async.forEachSeries(
        [
          'This is the first log message.',
          'This is the second log message.',
          'This is the third log message.',
          'This is the fourth log message.'
        ],
        function (i, cb) {
          stream.write(i + '\n', 'utf8', cb);
        },
        function () {
          stream.end(done);
        }
      );
    });

    it('should produce three files, with the file-extension kept', function (done) {
      fs.readdir(__dirname, function (err, files) {
        var testFiles = files.filter(
          function (f) { return f.indexOf('extKept-backups') > -1 }
        ).sort();

        testFiles.length.should.eql(3);
        testFiles.should.eql([
          'extKept-backups.1.log',
          'extKept-backups.2.log',
          'extKept-backups.log',
        ]);

        fs.readFile(path.join(__dirname, testFiles[0]), 'utf8', function (err, contents) {
          contents.should.eql('This is the third log message.\n');

          fs.readFile(path.join(__dirname, testFiles[1]), 'utf8',
            function (err, contents) {
              contents.toString('utf8').should.eql('This is the second log message.\n');
              fs.readFile(path.join(__dirname, testFiles[2]), 'utf8',
                function (err, contents) {
                  contents.toString('utf8').should.eql('This is the fourth log message.\n');
                  done(err);
                }
              );
            }
          );
        });
      });
    });

    after(function (done) {
      async.forEach([
        path.join(__dirname, 'extKept-backups.log'),
        path.join(__dirname, 'extKept-backups.1.log'),
        path.join(__dirname, 'extKept-backups.2.log'),
      ], remove, done);
    });

  });

  describe('with options.compress = true and keepFileExt = true', function () {
    before(function (done) {
      var stream = new RollingFileStream(
        path.join(__dirname, 'compressed-backups.log'),
        30, //30 bytes max size
        2,  //two backup files to keep
        {compress: true, keepFileExt: true}
      );
      async.forEachSeries(
        [
          'This is the first log message.',
          'This is the second log message.',
          'This is the third log message.',
          'This is the fourth log message.'
        ],
        function (i, cb) {
          stream.write(i + '\n', 'utf8', cb);
        },
        function () {
          stream.end(done);
        }
      );
    });

    it('should produce three files, with the backups compressed', function (done) {
      fs.readdir(__dirname, function (err, files) {
        var testFiles = files.filter(
          function (f) { return f.indexOf('compressed-backups') > -1 }
        ).sort();

        testFiles.length.should.eql(3);
        testFiles.should.eql([
          'compressed-backups.1.log.gz',
          'compressed-backups.2.log.gz',
          'compressed-backups.log',
        ]);

        fs.readFile(path.join(__dirname, testFiles[2]), 'utf8', function (err, contents) {
          contents.should.eql('This is the fourth log message.\n');

          zlib.gunzip(fs.readFileSync(path.join(__dirname, testFiles[1])),
            function (err, contents) {
              contents.toString('utf8').should.eql('This is the second log message.\n');
              zlib.gunzip(fs.readFileSync(path.join(__dirname, testFiles[0])),
                function (err, contents) {
                  contents.toString('utf8').should.eql('This is the third log message.\n');
                  done(err);
                }
              );
            }
          );
        });
      });
    });

    after(function (done) {
      async.forEach([
        path.join(__dirname, 'compressed-backups.log'),
        path.join(__dirname, 'compressed-backups.1.log.gz'),
        path.join(__dirname, 'compressed-backups.2.log.gz'),
      ], remove, done);
    });

  });

  describe('when many files already exist', function () {
    before(function (done) {
      async.forEach(
        [
          __dirname + '/test-rolling-stream-with-existing-files.11',
          __dirname + '/test-rolling-stream-with-existing-files.20',
          __dirname + '/test-rolling-stream-with-existing-files.-1',
          __dirname + '/test-rolling-stream-with-existing-files.1.1',
          __dirname + '/test-rolling-stream-with-existing-files.1'
        ],
        remove,
        function (err) {
          if (err) done(err);

          async.forEach(
            [
              __dirname + '/test-rolling-stream-with-existing-files.11',
              __dirname + '/test-rolling-stream-with-existing-files.20',
              __dirname + '/test-rolling-stream-with-existing-files.-1',
              __dirname + '/test-rolling-stream-with-existing-files.1.1',
              __dirname + '/test-rolling-stream-with-existing-files.1'
            ],
            create,
            function (err) {
              if (err) done(err);

              var stream = new RollingFileStream(
                __dirname + '/test-rolling-stream-with-existing-files',
                18,
                5
              );

              async.forEachSeries(
                [0, 1, 2, 3, 4, 5, 6],
                function (i, cb) {
                  stream.write(i +'.cheese\n', 'utf8', cb);
                },
                function () {
                  stream.end(done);
                }
              );
            }
          );
        }
      );
    });

    after(function (done) {
      async.forEach([
        __dirname + '/test-rolling-stream-with-existing-files.-1',
        __dirname + '/test-rolling-stream-with-existing-files',
        __dirname + '/test-rolling-stream-with-existing-files.1.1',
        __dirname + '/test-rolling-stream-with-existing-files.0',
        __dirname + '/test-rolling-stream-with-existing-files.1',
        __dirname + '/test-rolling-stream-with-existing-files.2',
        __dirname + '/test-rolling-stream-with-existing-files.3',
        __dirname + '/test-rolling-stream-with-existing-files.4',
        __dirname + '/test-rolling-stream-with-existing-files.5',
        __dirname + '/test-rolling-stream-with-existing-files.6',
        __dirname + '/test-rolling-stream-with-existing-files.11',
        __dirname + '/test-rolling-stream-with-existing-files.20'
      ], remove, done);
    });

    it('should roll the files', function (done) {
      fs.readdir(__dirname, function (err, files) {
        files.should.containEql('test-rolling-stream-with-existing-files');
        files.should.containEql('test-rolling-stream-with-existing-files.1');
        files.should.containEql('test-rolling-stream-with-existing-files.2');
        files.should.containEql('test-rolling-stream-with-existing-files.11');
        files.should.containEql('test-rolling-stream-with-existing-files.20');
        done(err);
      });
    });
  });

  describe('when the directory gets deleted', function () {
    var stream;
    before(function (done) {
      stream = new RollingFileStream(path.join('subdir', 'test-rolling-file-stream'), 5, 5);
      stream.write('initial', 'utf8', done);
    });

    after(function () {
      fs.unlinkSync(path.join('subdir', 'test-rolling-file-stream'));
      fs.rmdirSync('subdir');
    });

    it('handles directory deletion gracefully', function (done) {
      stream.theStream.on('error', done);

      remove(path.join('subdir', 'test-rolling-file-stream'), function () {
        fs.rmdir('subdir', function () {
          stream.write('rollover', 'utf8', function () {
            fs.readFileSync(path.join('subdir', 'test-rolling-file-stream'), 'utf8')
              .should.eql('rollover');
            done();
          });
        });
      });
    });
  });
});
