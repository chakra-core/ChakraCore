'use strict';

import resolvePackagePath = require('../index');
import Project = require('fixturify-project');
import fs = require('fs-extra');
import chai = require('chai');
import path = require('path');
import semver = require('semver');

const expect = chai.expect;
const FIXTURE_ROOT = `${__dirname}/tmp/fixtures/`

describe('resolve-package-path', function() {
  beforeEach(function() {
    fs.removeSync(FIXTURE_ROOT);
  });

  afterEach(function() {
    fs.removeSync(FIXTURE_ROOT);
  });

  it('exposes its cache', function() {
    expect(resolvePackagePath._CACHE).to.be.ok;
    expect(resolvePackagePath._resetCache).to.be.a('function');
  });

  it('exposes helper methods', function() {
    expect(resolvePackagePath.getRealFilePath).to.be.a('function');
    expect(resolvePackagePath.getRealDirectoryPath).to.be.a('function');

    // smoke tests, real tests are unit tests of the underlying utilities
    expect(resolvePackagePath.getRealFilePath(__filename)).to.eql(__filename);
    expect(resolvePackagePath.getRealDirectoryPath(__dirname)).to.eql(__dirname);
  });

  it('appears to reset cache', function() {
    resolvePackagePath._CACHE.PATH.set('hi', 1);
    expect(resolvePackagePath._CACHE.PATH.has('hi')).eql(true);
    resolvePackagePath._resetCache();
    expect(resolvePackagePath._CACHE.PATH.has('hi')).eql(false);
  });

  describe('npm usage', function() {
    let app: Project, rsvp: Project, a: Project, orange: Project, apple: Project;

    beforeEach(function() {
      app = new Project('app', '3.1.1',  app => {
        rsvp = app.addDependency('rsvp', '3.2.2', rsvp => {
          a = rsvp.addDependency('a', '1.1.1');
        });
        orange = app.addDependency('orange', '1.0.0');
        apple = app.addDependency('apple', '1.0.0');
      });
    });

    it('smoke test', function() {
      app.writeSync();

      expect(resolvePackagePath('app',    app.root)).    to.eql(null);
      expect(resolvePackagePath('rsvp',   app.baseDir)). to.eql(path.normalize(`${app.root}/app/node_modules/rsvp/package.json`));
      expect(resolvePackagePath('orange', app.baseDir)). to.eql(path.normalize(`${app.root}/app/node_modules/orange/package.json`));
      expect(resolvePackagePath('apple',  app.baseDir)). to.eql(path.normalize(`${app.root}/app/node_modules/apple/package.json`));
      expect(resolvePackagePath('a',      app.baseDir)). to.eql(null);
      expect(resolvePackagePath('a',      rsvp.baseDir)).to.eql(path.normalize(`${rsvp.baseDir}/node_modules/a/package.json`));
      expect(resolvePackagePath('rsvp',   a.baseDir)).   to.eql(path.normalize(`${rsvp.baseDir}/package.json`));
      expect(resolvePackagePath('orange', a.baseDir)).   to.eql(path.normalize(`${orange.baseDir}/package.json`));
      expect(resolvePackagePath('apple',  a.baseDir)).   to.eql(path.normalize(`${apple.baseDir}/package.json`));
      expect(resolvePackagePath('app',    a.baseDir)).   to.eql(null);
    });
  });

  if (semver.gte(process.versions.node, '8.0.0')) {
    describe('yarn pnp usage', function() {
      this.timeout(30000); // in-case the network IO is slow
      let app: Project;
      const execa = require('execa');

      beforeEach(function() {
        app = new Project('dummy', '1.0.0', app => {
          app.pkg.private = true;
          app.pkg.name
          app.pkg.scripts = {
            test: "node -r ./.pnp.js ./test.js"
          };
          app.pkg.installConfig = {
            pnp: true
          };
          app.addDependency('ember-source-channel-url', '1.1.0');
          app.addDependency('resolve-package-path', 'link:' + path.join(__dirname, '..'));
          app.files = {
            'test.js': 'require("resolve-package-path")(process.argv[2], __dirname); console.log("success!");'
          };
        });

        app.writeSync();
        execa.sync('yarn', {
          cwd: app.baseDir
        })
      });

      afterEach(function() {
        app.dispose();
      });

      it('handles yarn pnp usage - package exists', function() {
        let result = execa.sync('yarn', ['test', 'ember-source-channel-url'], {
          cwd: app.baseDir
        });

        expect(result.stdout.toString()).includes('success!');
      });

      it('handles yarn pnp usage - package missing', function() {
        let result = execa.sync('yarn', ['test', 'some-non-existent-package'], {
          cwd: app.baseDir
        });

        expect(result.stdout.toString()).includes('success!');
      });
    });
  }
});
