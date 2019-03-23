var fs = require('fs');
var path = require('path');
var broccoli = require('ember-cli-broccoli');
var rimraf = require('rimraf');
var chai = require('chai'), expect = chai.expect;
var chaiAsPromised = require('chai-as-promised');
var expect = chai.expect;
var Watcher = require('..');
var TestFilter = require('./test_filter');
var RSVP = require('rsvp');
var Promise = RSVP.Promise;
var path    = require('path');
var sane = require('sane');
var heimdall = require('heimdalljs');

chai.use(chaiAsPromised);

describe('broccoli-sane-watcher', function () {
  var watcher;

  beforeEach(function () {
    fs.mkdirSync('tests/fixtures');
    heimdall._reset();
  });

  afterEach(function (done) {
    if (watcher) {
      watcher.close();
      watcher = null;
    }
    rimraf('tests/fixtures', done);
  });

  function eventHandler(cp, defer) {
    return function () {
      try {
        return cp.apply(this, arguments);
      } catch(e) {
        defer.reject(e);
      }
    };
  }

  function errorHandler(defer) {
    return function (error) {
      defer.reject(error);
    };
  }

  it('should pass poll option to sane', function () {
    fs.mkdirSync('tests/fixtures/a');
    var filter = new TestFilter(['tests/fixtures/a'], function () {
      return 'output';
    });
    var builder = new broccoli.Builder(filter);

    watcher = new Watcher(builder, {
      poll: true
    });

    return watcher.sequence.then(function () {
      expect(watcher.watched['tests/fixtures/a'] instanceof sane.PollWatcher).to.equal(true);
    });
  });

  it('should emit change event when file is added', function () {
    var defer = RSVP.defer();
    fs.mkdirSync('tests/fixtures/a');

    var changes = 0;

    var filter = new TestFilter(['tests/fixtures/a'], function () {
      return 'output';
    });

    var builder = new broccoli.Builder(filter);
    watcher = new Watcher(builder);
    watcher.on('change', eventHandler(function (results) {
      expect(results.directory).to.equal('output');

      if (changes++) {
        defer.resolve();
      } else {
        fs.writeFileSync('tests/fixtures/a/file.js');
      }
    }, defer));

    watcher.on('error', errorHandler(defer));

    return defer.promise;
  });

  it('cleans up heimdall nodes after builds and rebuilds', function() {
    function countNodes() {
      var count = 0;

      heimdall.visitPreOrder(function () {
        ++count;
      });

      return count;
    }

    var defer = RSVP.defer();

    expect(countNodes()).to.equal(1, 'there is only the heimdall root node');

    fs.mkdirSync('tests/fixtures/a');
    var filter = new TestFilter(['tests/fixtures/a'], function () {
      return 'output';
    });

    var builder = new broccoli.Builder(filter);
    watcher = new Watcher(builder);

    watcher.on('error', errorHandler(defer));

    var builds = 0;

    watcher.on('change', eventHandler(function () {
      // we build 3 times, but each time we only have the nodes from the current
      // build (not previous builds)
      expect(countNodes()).to.equal(3, 'we have expected number of nodes (root, testfilter, test/fixtures/a)');

      var build = ++builds;
      if (build === 1) {
        fs.writeFileSync('tests/fixtures/a/foo.js');
      } else if (build == 2) {
        fs.writeFileSync('tests/fixtures/a/bar.js');
      } else {
        defer.resolve();
      }
    }, defer));

    return defer.promise;
  });

  it('should emit an error when a filter errors', function () {
    fs.mkdirSync('tests/fixtures/a');
    var filter = new TestFilter(['tests/fixtures/a'], function () {
      throw new Error('filter error');
    });
    var count = 0;
    var builder = new broccoli.Builder(filter);
    watcher = new Watcher(builder);

    var defer = RSVP.defer();

    watcher.on('change', eventHandler(function (results) {
      count++;
      expect(count).to.equal(2, 'only the second build should be here');
      expect(results.directory).to.equal('output');
      defer.resolve();
    }, defer));

    watcher.on('error', eventHandler(function (error) {
      count++;
      expect(count).to.equal(1, 'only the first build should be here');
      if (count !== 1) defer.reject();
      // next result shouldn't fail
      filter.output = function () {
        return 'output';
      };
      // trigger next build
      fs.writeFileSync('tests/fixtures/a/file.js');
    }, defer));

    return defer.promise;
  });

  it('should emit a pleasant error when attempting to watch a missing directory', function () {
    var builder = new broccoli.Builder('test/fixtures/b');
    var watcher = new Watcher(builder);
    return watcher.sequence
      .catch(function(error) {
        var message = error.message;

        expect(message).to.equal('Attempting to watch missing directory: test/fixtures/b');
      });
  });

  it('should include the full file system path in the results hash', function() {
    fs.mkdirSync('tests/fixtures/a');
    var changes = 0;
    var filter = new TestFilter(['tests/fixtures/a'], function () {
      return 'output';
    });
    var builder = new broccoli.Builder(filter);
    var defer = RSVP.defer();
    watcher = new Watcher(builder);

    watcher.on('change', eventHandler(function (results) {
      if (changes++) {
        expect(path.relative(process.cwd(), results.filePath)).to.equal('tests/fixtures/a/file.js');
        defer.resolve();
      } else {
        fs.writeFileSync('tests/fixtures/a/file.js');
      }
    }, defer));

    watcher.on('error', errorHandler(defer));

    return defer.promise;
  });

  it('calls `build` with an annotation explaining why a build is occurring', function() {
    var defer = RSVP.defer();

    fs.mkdirSync('tests/fixtures/a');
    var filter = new TestFilter(['tests/fixtures/a'], function () {
      return 'output';
    });

    var buildArgs = null;
    var builder = new broccoli.Builder(filter);
    builder.build = (function (originalBuild) {
      return function () {
        buildArgs = Array.prototype.slice.call(arguments);
        return originalBuild.apply(builder, buildArgs);
      }
    })(builder.build);
    watcher = new Watcher(builder);

    watcher.on('error', errorHandler(defer));

    var builds = 0;

    watcher.on('change', eventHandler(function () {
      var build = ++builds;
      if (build === 1) {
        fs.writeFileSync('tests/fixtures/a/foo.js');
        fs.writeFileSync('tests/fixtures/a/bar.js');
        fs.writeFileSync('tests/fixtures/a/baz.js');
        fs.mkdirSync('tests/fixtures/a/ohai');
        fs.writeFileSync('tests/fixtures/a/ohai/foo.js');
      } else {
        expect(buildArgs.length).to.eql(2);
        var annotation = buildArgs[1];
        var expectedChangedFiles = [
          path.join(__dirname, 'fixtures/a/bar.js'),
          path.join(__dirname, 'fixtures/a/baz.js'),
          path.join(__dirname, 'fixtures/a/foo.js'),
          path.join(__dirname, 'fixtures/a/ohai'),
          path.join(__dirname, 'fixtures/a/ohai/foo.js'),
        ];

        expect(annotation.reason).to.eql('watcher');
        expect(annotation.type).to.eql('rebuild');
        // These are a bit fuzzy because there's a race and will vary from dev
        // machines & CI &c.; but we expect at least 1 changed file and if we
        // have more than each file twice we'd be very surprised.
        expect(expectedChangedFiles).to.include.members(annotation.changedFiles);
        expect(annotation.changedFiles.length).to.be.within(1, 10);
        expect(expectedChangedFiles).to.include(annotation.primaryFile);

        defer.resolve();
      }
    }, defer));

    return defer.promise;
  });
});
