require('should');

const _ = require('lodash');
const path = require('path');
const zlib = require('zlib');
const async = require('async');
const stream = require('stream');
const fs = require('fs-extra');
const proxyquire = require('proxyquire').noPreserveCache();

let fakeNow = new Date(2012, 8, 12, 10, 37, 11);
const mockNow = () => fakeNow;
const RollingFileWriteStream = proxyquire('../lib/RollingFileWriteStream', {
  './now': mockNow
});
let fakedFsDate = fakeNow;
const mockFs = require('fs-extra');
const oldStatSync = mockFs.statSync
mockFs.statSync = fd => {
  const result = oldStatSync(fd);
  result.birthtime = fakedFsDate.valueOf();
  return result;
}

function generateTestFile(fileName) {
  const dirName = path.join(__dirname, 'tmp_' + Math.floor(Math.random() * new Date()));
  fileName = fileName || 'ignored.log';
  const fileNameObj = path.parse(fileName);
  return {
    dir: dirName,
    base: fileNameObj.base,
    name: fileNameObj.name,
    ext: fileNameObj.ext,
    path: path.join(dirName, fileName)
  };
}

function resetTime() {
  fakeNow = new Date(2012, 8, 12, 10, 37, 11);
  fakedFsDate = fakeNow;
}

describe('RollingFileWriteStream', () => {

  beforeEach(() => {
    resetTime();
  });

  after(() => {
    fs
      .readdirSync(__dirname)
      .filter(f => f.startsWith('tmp_'))
      .forEach(f => fs.removeSync(path.join(__dirname, f)));
  });

  describe('with no arguments', () => {
    it('should throw an error', () => {
      (() => new RollingFileWriteStream()).should.throw(
        /(the )?"?path"? (argument )?must be (a|of type) string\. received (type )?undefined/i);
    });
  });

  describe('with invalid options', () => {
    after(done => {
      fs.remove('filename', done);
    });

    it('should complain about a negative maxSize', () => {
      (() => { new RollingFileWriteStream('filename', { maxSize: -3 }) }).should.throw('options.maxSize (-3) should be > 0');
      (() => { new RollingFileWriteStream('filename', { maxSize: 0 }) }).should.throw('options.maxSize (0) should be > 0');
    });

    it('should complain about a negative numToKeep', () => {
      (() => { new RollingFileWriteStream('filename', { numToKeep: -3 }) }).should.throw('options.numToKeep (-3) should be > 0');
      (() => { new RollingFileWriteStream('filename', { numToKeep: 0 }) }).should.throw('options.numToKeep (0) should be > 0');
    });
  });

  describe('with default arguments', () => {
    const fileObj = generateTestFile();
    let s;

    before(() => {
      s = new RollingFileWriteStream(fileObj.path);
    });

    after(() => {
      s.end();
      fs.removeSync(fileObj.dir);
    });

    it('should take a filename and options, return Writable', () => {
      s.should.be.an.instanceOf(stream.Writable);
      s.currentFileStream.path.should.eql(fileObj.path);
      s.currentFileStream.mode.should.eql(420);
      s.currentFileStream.flags.should.eql('a');
    });

    it('should apply default options', () => {
      s.options.maxSize.should.eql(Number.MAX_SAFE_INTEGER);
      s.options.encoding.should.eql('utf8');
      s.options.mode.should.eql(420);
      s.options.flags.should.eql('a');
      s.options.compress.should.eql(false);
      s.options.keepFileExt.should.eql(false);
    });
  });

  describe('with 5 maxSize, rotating daily', () => {
    const fileObj = generateTestFile('noExtension');
    let s;

    before(done => {
      fakeNow = new Date(2012, 8, 12, 10, 37, 11);
      s = new RollingFileWriteStream(fileObj.path, { pattern: 'yyyy-MM-dd', maxSize: 5 });
      const flows = Array.from(Array(38).keys()).map(i => cb => {
        fakeNow = new Date(2012, 8, 12 + parseInt(i / 5, 10), 10, 37, 11);
        s.write(i.toString(), 'utf8', cb);
      });
      async.waterfall(flows, () => done());
    });

    after(done => {
      fs.removeSync(fileObj.dir);
      done();
    });

    it('should rotate using filename with no extension', () => {
      const files = fs.readdirSync(fileObj.dir);
      const expectedFileList = [
        fileObj.base, //353637
        fileObj.base + '.2012-09-12.1', // 01234
        fileObj.base + '.2012-09-13.1', // 56789
        fileObj.base + '.2012-09-14.2', // 101112
        fileObj.base + '.2012-09-14.1', // 1314
        fileObj.base + '.2012-09-15.2', // 151617
        fileObj.base + '.2012-09-15.1', // 1819
        fileObj.base + '.2012-09-16.2', // 202122
        fileObj.base + '.2012-09-16.1', // 2324
        fileObj.base + '.2012-09-17.2', // 252627
        fileObj.base + '.2012-09-17.1', // 2829
        fileObj.base + '.2012-09-18.2', // 303132
        fileObj.base + '.2012-09-18.1'  // 3334
      ];
      files.length.should.equal(expectedFileList.length);
      files.should.containDeep(expectedFileList);
      fs.readFileSync(path.format(fileObj)).toString().should.equal('353637');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-12.1',
      }))).toString().should.equal('01234');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-13.1',
      }))).toString().should.equal('56789');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-14.2',
      }))).toString().should.equal('101112');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-14.1',
      }))).toString().should.equal('1314');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-15.2',
      }))).toString().should.equal('151617');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-15.1',
      }))).toString().should.equal('1819');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-16.2',
      }))).toString().should.equal('202122');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-16.1',
      }))).toString().should.equal('2324');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-17.2',
      }))).toString().should.equal('252627');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-17.1',
      }))).toString().should.equal('2829');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-18.2',
      }))).toString().should.equal('303132');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-18.1',
      }))).toString().should.equal('3334');
    });
  });

  describe('with default arguments and recreated in the same day', () => {
    const fileObj = generateTestFile();
    let s;

    before(done => {
      const flows = Array.from(Array(3).keys()).map(() => cb => {
        s = new RollingFileWriteStream(fileObj.path);
        s.write('abc', 'utf8', cb);
        s.end();
      })
      async.waterfall(flows, () => done());
    });

    after(() => {
      fs.removeSync(fileObj.dir);
    });

    it('should have only 1 file', () => {
      const files = fs.readdirSync(fileObj.dir);
      const expectedFileList = [fileObj.base];
      files.length.should.equal(expectedFileList.length);
      files.should.containDeep(expectedFileList);
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base,
      }))).toString().should.equal('abcabcabc');
    });

  });

  describe('with 5 maxSize, using filename with extension', () => {
    const fileObj = generateTestFile("withExtension.log");
    let s;

    before(done => {
      fakeNow = new Date(2012, 8, 12, 10, 37, 11);
      s = new RollingFileWriteStream(fileObj.path, { pattern: 'yyyy-MM-dd', maxSize: 5 });
      const flows = Array.from(Array(38).keys()).map(i => cb => {
        fakeNow = new Date(2012, 8, 12 + parseInt(i / 10, 10), 10, 37, 11);
        s.write(i.toString(), 'utf8', cb);
      });
      async.waterfall(flows, () => done());
    });

    after(done => {
      fs.removeSync(fileObj.dir);
      done();
    });

    it('should rotate files within the day, and when the day changes', () => {
      const files = fs.readdirSync(fileObj.dir);
      const expectedFileList = [
        fileObj.base, //3637
        fileObj.base + '.2012-09-12.2', //01234
        fileObj.base + '.2012-09-12.1', //56789
        fileObj.base + '.2012-09-13.4', //101112
        fileObj.base + '.2012-09-13.3', //131415
        fileObj.base + '.2012-09-13.2', //161718
        fileObj.base + '.2012-09-13.1', //19
        fileObj.base + '.2012-09-14.4', //202122
        fileObj.base + '.2012-09-14.3', //232425
        fileObj.base + '.2012-09-14.2', //262728
        fileObj.base + '.2012-09-14.1', //29
        fileObj.base + '.2012-09-15.2', //303132
        fileObj.base + '.2012-09-15.1', //333435
      ];
      files.length.should.equal(expectedFileList.length);
      files.should.containDeep(expectedFileList);
      fs.readFileSync(path.format(fileObj)).toString().should.equal('3637');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-12.2',
      }))).toString().should.equal('01234');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-12.1',
      }))).toString().should.equal('56789');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-13.4',
      }))).toString().should.equal('101112');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-13.3',
      }))).toString().should.equal('131415');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-13.2',
      }))).toString().should.equal('161718');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-13.1',
      }))).toString().should.equal('19');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-14.4',
      }))).toString().should.equal('202122');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-14.3',
      }))).toString().should.equal('232425');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-14.2',
      }))).toString().should.equal('262728');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-14.1',
      }))).toString().should.equal('29');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-15.2',
      }))).toString().should.equal('303132');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-15.1',
      }))).toString().should.equal('333435');
    });
  });

  describe('with 5 maxSize and 3 files limit', () => {
    const fileObj = generateTestFile();
    let s;

    before(done => {
      fakeNow = new Date(2012, 8, 12, 10, 37, 11);
      s = new RollingFileWriteStream(fileObj.path, {
        maxSize: 5,
        numToKeep: 3
      });
      const flows = Array.from(Array(38).keys()).map(i => cb => {
        fakeNow = new Date(2012, 8, 12 + parseInt(i / 5), 10, 37, 11);
        s.write(i.toString(), 'utf8', cb);
      });
      async.waterfall(flows, () => done());
    });

    after(() => {
      s.end();
      fs.removeSync(fileObj.dir);
    });

    it('should rotate with at most 3 backup files not including the hot one', () => {
      const files = fs.readdirSync(fileObj.dir);
      const expectedFileList = [
        fileObj.base,
        fileObj.base + '.1',
        fileObj.base + '.2',
        fileObj.base + '.3'
      ];
      files.length.should.equal(expectedFileList.length);
      files.should.containDeep(expectedFileList);
      fs.readFileSync(path.format(fileObj)).toString().should.equal('37');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.1'
      }))).toString().should.equal('343536');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2'
      }))).toString().should.equal('313233');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.3'
      }))).toString().should.equal('282930');
    });
  });

  describe('with 5 maxSize and 3 files limit, rotating daily', () => {
    const fileObj = generateTestFile();
    let s;

    before(done => {
      fakeNow = new Date(2012, 8, 12, 10, 37, 11);
      s = new RollingFileWriteStream(fileObj.path, {
        maxSize: 5,
        pattern: 'yyyy-MM-dd',
        numToKeep: 3
      });
      const flows = Array.from(Array(38).keys()).map(i => cb => {
        fakeNow = new Date(2012, 8, 12 + parseInt(i / 10), 10, 37, 11);
        s.write(i.toString(), 'utf8', cb);
      });
      async.waterfall(flows, () => done());
    });

    after(() => {
      s.end();
      fs.removeSync(fileObj.dir);
    });

    it('should rotate with at most 3 backup files not including the hot one', () => {
      const files = fs.readdirSync(fileObj.dir);
      const expectedFileList = [
        fileObj.base, //3637
        fileObj.base + '.2012-09-14.1', //29
        fileObj.base + '.2012-09-15.2', //303132
        fileObj.base + '.2012-09-15.1', //333435
      ];
      files.length.should.equal(expectedFileList.length);
      files.should.containDeep(expectedFileList);
      fs.readFileSync(path.format(fileObj)).toString().should.equal('3637');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-15.1',
      }))).toString().should.equal('333435');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-15.2',
      }))).toString().should.equal('303132');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-14.1',
      }))).toString().should.equal('29');
    });
  });

  describe('with date pattern dd-MM-yyyy', () => {
    const fileObj = generateTestFile();
    let s;

    before(done => {
      fakeNow = new Date(2012, 8, 12, 10, 37, 11);
      s = new RollingFileWriteStream(fileObj.path, {
        maxSize: 5,
        pattern: 'dd-MM-yyyy'
      });
      const flows = Array.from(Array(8).keys()).map(i => cb => {
        fakeNow = new Date(2012, 8, 12 + parseInt(i / 5, 10), 10, 37, 11);
        s.write(i.toString(), 'utf8', cb);
      });
      async.waterfall(flows, () => done());
    });

    after((done) => {
      s.end(() => {
        fs.remove(fileObj.dir, done);
      });
    });

    it('should rotate with date pattern dd-MM-yyyy in the file name', () => {
      const files = fs.readdirSync(fileObj.dir);
      const expectedFileList = [
        fileObj.base,
        fileObj.base + '.12-09-2012.1'
      ];
      files.length.should.equal(expectedFileList.length);
      files.should.containDeep(expectedFileList);
      fs.readFileSync(path.format(fileObj)).toString().should.equal('567');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.12-09-2012.1'
      }))).toString().should.equal('01234');
    });
  });

  describe('with compress true', () => {
    const fileObj = generateTestFile();
    let s;

    before(done => {
      fakeNow = new Date(2012, 8, 12, 10, 37, 11);
      s = new RollingFileWriteStream(fileObj.path, {
        maxSize: 5,
        pattern: 'yyyy-MM-dd',
        compress: true
      });
      const flows = Array.from(Array(8).keys()).map(i => cb => {
        fakeNow = new Date(2012, 8, 12 + parseInt(i / 5, 10), 10, 37, 11);
        s.write(i.toString(), 'utf8', cb);
      });
      async.waterfall(flows, () => done());
    });

    after(() => {
      s.end();
      fs.removeSync(fileObj.dir);
    });

    it('should rotate with gunzip', () => {
      const files = fs.readdirSync(fileObj.dir);
      const expectedFileList = [
        fileObj.base,
        fileObj.base + '.2012-09-12.1.gz'
      ];
      files.length.should.equal(expectedFileList.length);
      files.should.containDeep(expectedFileList);

      fs.readFileSync(path.format(fileObj)).toString().should.equal('567');
      const content = fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.base + '.2012-09-12.1.gz'
      })));
      zlib.gunzipSync(content).toString().should.equal('01234');
    });
  });

  describe('with keepFileExt', () => {
    const fileObj = generateTestFile('keepFileExt.log');
    let s;

    before(done => {
      fakeNow = new Date(2012, 8, 12, 10, 37, 11);
      s = new RollingFileWriteStream(fileObj.path, {
        pattern: 'yyyy-MM-dd',
        maxSize: 5,
        keepFileExt: true
      });
      const flows = Array.from(Array(8).keys()).map(i => cb => {
        fakeNow = new Date(2012, 8, 12 + parseInt(i / 5, 10), 10, 37, 11);
        s.write(i.toString(), 'utf8', cb);
      });
      async.waterfall(flows, () => done());
    });

    after(done => {
      s.end();
      fs.removeSync(fileObj.dir);
      done();
    });

    it('should rotate with the same extension', () => {
      const files = fs.readdirSync(fileObj.dir);
      const expectedFileList = [
        fileObj.base,
        fileObj.name + '.2012-09-12.1.log'
      ];
      files.length.should.equal(expectedFileList.length);
      files.should.containDeep(expectedFileList);

      fs.readFileSync(path.format(fileObj)).toString().should.equal('567');
      fs.readFileSync(path.format({
        dir: fileObj.dir,
        base: fileObj.name + '.2012-09-12.1' + fileObj.ext
      })).toString().should.equal('01234');
    });
  });

  describe('with keepFileExt and compress', () => {
    const fileObj = generateTestFile('keepFileExt.log');
    let s;

    before(done => {
      fakeNow = new Date(2012, 8, 12, 10, 37, 11);
      s = new RollingFileWriteStream(fileObj.path, {
        maxSize: 5,
        pattern: 'yyyy-MM-dd',
        keepFileExt: true,
        compress: true
      });
      const flows = Array.from(Array(8).keys()).map(i => cb => {
        fakeNow = new Date(2012, 8, 12 + parseInt(i / 5, 10), 10, 37, 11);
        s.write(i.toString(), 'utf8', cb);
      });
      async.waterfall(flows, () => done());
    });

    after(done => {
      s.end();
      fs.removeSync(fileObj.dir);
      done();
    });

    it('should rotate with the same extension', () => {
      const files = fs.readdirSync(fileObj.dir);
      const expectedFileList = [
        fileObj.base,
        fileObj.name + '.2012-09-12.1.log.gz'
      ];
      files.length.should.equal(expectedFileList.length);
      files.should.containDeep(expectedFileList);

      fs.readFileSync(path.format(fileObj)).toString().should.equal('567');
      const content = fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.name + '.2012-09-12.1.log.gz'
      })));
      zlib.gunzipSync(content).toString().should.equal('01234');
    });
  });

  describe('with alwaysIncludePattern and keepFileExt', () => {
    const fileObj = generateTestFile('keepFileExt.log');
    let s;

    before(done => {
      fakeNow = new Date(2012, 8, 12, 10, 37, 11);
      s = new RollingFileWriteStream(fileObj.path, {
        maxSize: 5,
        pattern: 'yyyy-MM-dd',
        keepFileExt: true,
        alwaysIncludePattern: true
      });
      const flows = Array.from(Array(8).keys()).map(i => cb => {
        fakeNow = new Date(2012, 8, 12 + parseInt(i / 5, 10), 10, 37, 11);
        s.write(i.toString(), 'utf8', cb);
      });
      async.waterfall(flows, () => done());
    });

    after(done => {
      s.end();
      fs.removeSync(fileObj.dir);
      done();
    });

    it('should rotate with the same extension and keep date in the filename', () => {
      const files = fs.readdirSync(fileObj.dir);
      const expectedFileList = [
        fileObj.name + '.2012-09-12.1.log',
        fileObj.name + '.2012-09-13.log'
      ];
      files.length.should.equal(expectedFileList.length);
      files.should.containDeep(expectedFileList);
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.name + '.2012-09-13.log'
      }))).toString().should.equal('567');
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.name + '.2012-09-12.1.log'
      }))).toString().should.equal('01234');
    });
  });

  describe('with 5 maxSize, compress, keepFileExt and alwaysIncludePattern', () => {
    const fileObj = generateTestFile('keepFileExt.log');
    let s;

    before(done => {
      fakeNow = new Date(2012, 8, 12, 10, 37, 11);
      s = new RollingFileWriteStream(fileObj.path, {
        maxSize: 5,
        compress: true,
        keepFileExt: true,
        alwaysIncludePattern: true,
        pattern: 'yyyy-MM-dd'
      });
      const flows = Array.from(Array(38).keys()).map(i => cb => {
        fakeNow = new Date(2012, 8, 12 + parseInt(i / 5, 10), 10, 37, 11);
        s.write(i.toString(), 'utf8', cb);
      });
      async.waterfall(flows, () => done());
    });

    after(done => {
      s.end();
      fs.removeSync(fileObj.dir);
      done();
    });

    it('should rotate every day', () => {
      const files = fs.readdirSync(fileObj.dir);
      const expectedFileList = [
        fileObj.name + '.2012-09-12.1.log.gz', //01234
        fileObj.name + '.2012-09-13.1.log.gz', //56789
        fileObj.name + '.2012-09-14.2.log.gz', //101112
        fileObj.name + '.2012-09-14.1.log.gz', //1314
        fileObj.name + '.2012-09-15.2.log.gz', //151617
        fileObj.name + '.2012-09-15.1.log.gz', //1819
        fileObj.name + '.2012-09-16.2.log.gz', //202122
        fileObj.name + '.2012-09-16.1.log.gz', //2324
        fileObj.name + '.2012-09-17.2.log.gz', //252627
        fileObj.name + '.2012-09-17.1.log.gz', //2829
        fileObj.name + '.2012-09-18.2.log.gz', //303132
        fileObj.name + '.2012-09-18.1.log.gz', //3334
        fileObj.name + '.2012-09-19.log' //353637
      ];
      files.length.should.equal(expectedFileList.length);
      files.should.containDeep(expectedFileList);
      fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.name + '.2012-09-19.log',
      }))).toString().should.equal('353637');
      zlib.gunzipSync(fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.name + '.2012-09-18.1.log.gz',
      })))).toString().should.equal('3334');
      zlib.gunzipSync(fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.name + '.2012-09-18.2.log.gz',
      })))).toString().should.equal('303132');
      zlib.gunzipSync(fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.name + '.2012-09-17.1.log.gz',
      })))).toString().should.equal('2829');
      zlib.gunzipSync(fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.name + '.2012-09-17.2.log.gz',
      })))).toString().should.equal('252627');
      zlib.gunzipSync(fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.name + '.2012-09-16.1.log.gz',
      })))).toString().should.equal('2324');
      zlib.gunzipSync(fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.name + '.2012-09-16.2.log.gz',
      })))).toString().should.equal('202122');
      zlib.gunzipSync(fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.name + '.2012-09-15.1.log.gz',
      })))).toString().should.equal('1819');
      zlib.gunzipSync(fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.name + '.2012-09-15.2.log.gz',
      })))).toString().should.equal('151617');
      zlib.gunzipSync(fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.name + '.2012-09-14.1.log.gz',
      })))).toString().should.equal('1314');
      zlib.gunzipSync(fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.name + '.2012-09-14.2.log.gz',
      })))).toString().should.equal('101112');
      zlib.gunzipSync(fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.name + '.2012-09-13.1.log.gz',
      })))).toString().should.equal('56789');
      zlib.gunzipSync(fs.readFileSync(path.format(_.assign({}, fileObj, {
        base: fileObj.name + '.2012-09-12.1.log.gz',
      })))).toString().should.equal('01234');
    });
  });

  describe('when old files exist', () => {
    const fileObj = generateTestFile();
    let s;

    before(done => {
      fakeNow = new Date(2012, 8, 12, 10, 37, 11);
      fs.ensureFileSync(fileObj.path);
      fs.writeFileSync(fileObj.path, 'exist');
      s = new RollingFileWriteStream(fileObj.path);
      s.write('now', 'utf8', done);
    });

    after(() => {
      s.end();
      fs.removeSync(fileObj.dir);
    });

    it('should use write in the old file if not reach the maxSize limit', () => {
      const files = fs.readdirSync(fileObj.dir);
      const expectedFileList = [fileObj.base];
      files.length.should.equal(expectedFileList.length);
      files.should.containDeep(expectedFileList);

      fs.readFileSync(path.format(fileObj)).toString().should.equal('existnow');
    });
  });

  describe('when old files exist with contents', () => {
    const fileObj = generateTestFile();
    let s;

    before(done => {
      fakeNow = new Date(2012, 8, 12, 10, 37, 11);
      fs.ensureFileSync(fileObj.path);
      fs.writeFileSync(fileObj.path, 'This is exactly 30 bytes long\n');
      s = new RollingFileWriteStream(fileObj.path, { maxSize: 35 });
      s.write('one\n', 'utf8'); //34
      s.write('two\n', 'utf8'); //38 - file should be rotated next time
      s.write('three\n', 'utf8', done); // this should be in a new file.
    });

    after(() => {
      s.end();
      fs.removeSync(fileObj.dir);
    });

    it('should respect the existing file size', () => {
      const files = fs.readdirSync(fileObj.dir);
      const expectedFileList = [fileObj.base, fileObj.base + '.1'];
      files.length.should.equal(expectedFileList.length);
      files.should.containDeep(expectedFileList);

      fs.readFileSync(path.format(fileObj)).toString().should.equal('three\n');
      fs.readFileSync(path.join(fileObj.dir, fileObj.base + '.1')).toString().should.equal('This is exactly 30 bytes long\none\ntwo\n');

    });
  });

  describe('when old files exist with contents and stream created with overwrite flag', () => {
    const fileObj = generateTestFile();
    let s;

    before(done => {
      fakeNow = new Date(2012, 8, 12, 10, 37, 11);
      fs.ensureFileSync(fileObj.path);
      fs.writeFileSync(fileObj.path, 'This is exactly 30 bytes long\n');
      s = new RollingFileWriteStream(fileObj.path, { maxSize: 35, flags: 'w' });
      s.write('there should only be this\n', 'utf8', done);
    });

    after(() => {
      s.end();
      fs.removeSync(fileObj.dir);
    });

    it('should ignore the existing file size', () => {
      const files = fs.readdirSync(fileObj.dir);
      const expectedFileList = [fileObj.base];
      files.length.should.equal(expectedFileList.length);
      files.should.containDeep(expectedFileList);

      s.state.currentSize.should.equal(26);

      fs.readFileSync(path.format(fileObj)).toString().should.equal('there should only be this\n');
    });
  });

  describe('when dir does not exist', () => {
    const fileObj = generateTestFile();
    let s;

    before(done => {
      fs.removeSync(fileObj.dir);
      fakeNow = new Date(2012, 8, 12, 10, 37, 11);
      s = new RollingFileWriteStream(fileObj.path);
      s.write('test', 'utf8', done);
    });

    after(() => {
      s.end();
      fs.removeSync(fileObj.dir);
    });

    it('should create the dir', () => {
      const files = fs.readdirSync(fileObj.dir);
      const expectedFileList = [fileObj.base];
      files.length.should.equal(expectedFileList.length);
      files.should.containDeep(expectedFileList);

      fs.readFileSync(path.format(fileObj)).toString().should.equal('test');
    });
  });

  describe('when given just a base filename with no dir', () => {
    let s;
    before(done => {
      s = new RollingFileWriteStream('test.log');
      s.write('this should not cause any problems', 'utf8', done);
    });

    after(() => {
      s.end();
      fs.removeSync('test.log');
    });

    it('should use process.cwd() as the dir', () => {
      const files = fs.readdirSync(process.cwd());
      files.should.containDeep(['test.log']);

      fs.readFileSync(
        path.join(process.cwd(), 'test.log')
      ).toString().should.equal('this should not cause any problems');
    });
  });

  describe('with no callback to write', () => {
    let s;
    before(done => {
      s = new RollingFileWriteStream('no-callback.log');
      s.write('this is all very nice', 'utf8', done);
    });

    after(done => {
      fs.remove('no-callback.log', done);
    });

    it('should not complain', done => {
        s.write('I am not bothered if this succeeds or not');
        s.end(done);
    });
  });

  describe('events', () => {
    let s;
    before(done => {
      s = new RollingFileWriteStream('test-events.log');
      s.write('this should not cause any problems', 'utf8', done);
    });

    after(() => {
      s.end();
      fs.removeSync('test-events.log');
    });

    it('should emit the error event of the underlying stream', (done) => {
      s.on('error', (e) => { e.message.should.equal('oh no'); done(); });
      s.currentFileStream.emit('error', new Error('oh no'));
    });

  });

});
