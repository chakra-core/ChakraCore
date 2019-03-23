'use strict';
Object.defineProperty(exports, "__esModule", { value: true });
var path = require("path");
var chai = require("chai");
var resolvePackagePath = require("../lib/resolve-package-path");
var Cache = require("../lib/cache");
var CacheGroup = require("../lib/cache-group");
var fs = require("fs");
var Project = require("fixturify-project");
var fixturesPath = path.join(__dirname, 'fixtures');
var assert = chai.assert;
var _cache = new Cache();
describe('resolvePackagePath', function () {
    var project, foo, fooBar, dedupped, linked, linkedBar, unlinked;
    beforeEach(function () {
        project = new Project('example', '0.0.0', function (project) {
            dedupped = project.addDependency('dedupped', '1.0.0', function (dedupped) {
                dedupped.addDependency('dedupped-child', '1.0.0');
            });
            foo = project.addDependency('foo', '1.0.0', function (foo) {
                fooBar = foo.addDependency('bar', '1.0.0');
            });
            unlinked = project.addDependency('linked', '1.0.0', function (linked) {
                linkedBar = linked.addDependency('bar', '1.0.0');
            });
        });
        project.writeSync();
        fs.symlinkSync('../../linked', foo.baseDir + "/node_modules/linked");
        linked = {
            baseDir: foo.baseDir + "/node_modules/linked"
        };
    });
    afterEach(function () {
        project.dispose();
    });
    describe('._findPackagePath', function () {
        // NOTE: _findPackagePath requires that 'dir' must be non-empty and valid.
        it('finds linked package.json', function () {
            var result = resolvePackagePath._findPackagePath(_cache, 'linked/node_modules/bar/package.json', fooBar.baseDir);
            assert.equal(result, path.join(linkedBar.baseDir, 'package.json'), 'should resolve link to real path package.json');
        });
        it('does not find invalid package.json file name', function () {
            var result = resolvePackagePath._findPackagePath(_cache, 'dedupped/package2.json', fooBar.baseDir);
            assert.isNull(result, 'invalid package.json should return null');
        });
        it('does not find invalid package file name', function () {
            var result = resolvePackagePath._findPackagePath(_cache, 'dedupped2/package.json', fooBar.baseDir);
            assert.isNull(result, 'invalid package filename should return null');
        });
        it('finds child package.json', function () {
            var result = resolvePackagePath._findPackagePath(_cache, 'bar/package.json', foo.baseDir);
            assert.equal(result, path.join(fooBar.baseDir, 'package.json'), '"child" package.json should resolve correctly');
        });
        it('finds parent package.json', function () {
            var result = resolvePackagePath._findPackagePath(_cache, 'foo/package.json', fooBar.baseDir);
            assert.equal(result, path.join(foo.baseDir, 'package.json'), '"parent" package.json should resolve correctly');
        });
        // Note: we do not need to provide a 'find self package.json' because this private function is only called
        // during resolvePackagePath when the path does not start with './'.
        it('finds uncle package.json', function () {
            var result = resolvePackagePath._findPackagePath(_cache, 'dedupped/package.json', fooBar.baseDir);
            assert.equal(result, path.join(dedupped.baseDir, 'package.json'), '"uncle" package.json should resolve correctly');
        });
    });
    describe('._getRealDirectoryPath', function () {
        var cache;
        var unlinkedDirPath = path.join(fixturesPath, 'node_modules', 'linked');
        beforeEach(function () {
            cache = new Cache();
        });
        it('resolves linked dir through link to unlinked dir', function () {
            var result = resolvePackagePath._getRealDirectoryPath(cache, linked.baseDir);
            assert.equal(result, unlinked.baseDir, 'link should resolve to real path package.json');
            assert.equal(cache.size, 1, 'cache should contain 1 entry');
            assert.equal(cache.get(linked.baseDir), unlinked.baseDir, 'cached entry from linked path should be to unlinked path');
        });
        it('resolves unlinked dir to itself', function () {
            var result1 = resolvePackagePath._getRealDirectoryPath(cache, linked.baseDir); // repeat just to load an entry
            var result2 = resolvePackagePath._getRealDirectoryPath(cache, unlinked.baseDir);
            assert.equal(cache.size, 2, 'cache should now contain 2 entries');
            assert.equal(result1, result2, '2 cache entries should both have the same value (different keys)');
            assert.equal(result2, unlinked.baseDir, 'resolving the unlinked path should not change the value');
            assert.equal(cache.get(unlinked.baseDir), unlinked.baseDir, 'the cached value for the unlinked path should be to itself');
        });
        it('resolves an existing file as null (it is not a directory)', function () {
            var filePath = path.join(unlinkedDirPath, 'index.js');
            var result = resolvePackagePath._getRealDirectoryPath(cache, filePath);
            assert.isNull(result, 'reference to a file should return null');
            assert.isNull(cache.get(filePath), 'cache reference to file should return null');
        });
        it('resolves a non-existent path as null', function () {
            var result = resolvePackagePath._getRealDirectoryPath(cache, unlinkedDirPath + '2');
            assert.isNull(result, 'reference to a non-existent path correctly returns null');
        });
    });
    describe('._getRealFilePath', function () {
        // create a temporary cache to make sure that we're actually caching things.
        var cache;
        beforeEach(function () {
            cache = new Cache();
        });
        it('resolves linked package.json through link to unlinked', function () {
            var result = resolvePackagePath._getRealFilePath(cache, path.join(linked.baseDir, 'package.json'));
            assert.equal(result, path.join(unlinked.baseDir, 'package.json'), 'link should resolve to real path package.json');
            assert.equal(cache.size, 1, 'cache should contain 1 entry');
            assert.equal(cache.get(path.join(linked.baseDir, 'package.json')), path.join(unlinked.baseDir, 'package.json'), 'cached entry from linked path should be to unlinked path');
        });
        it('resolves unlinked package.json to itself', function () {
            var result1 = resolvePackagePath._getRealFilePath(cache, path.join(linked.baseDir, 'package.json')); // repeat just to load an entry
            var result2 = resolvePackagePath._getRealFilePath(cache, path.join(unlinked.baseDir, 'package.json'));
            assert.equal(cache.size, 2, 'cache should now contain 2 entries');
            assert.equal(result1, result2, '2 cache entries should both have the same value (different keys)');
            assert.equal(result2, path.join(unlinked.baseDir, 'package.json'), 'resolving the unlinked path should not change the value');
            assert.equal(cache.get(path.join(unlinked.baseDir, 'package.json')), path.join(unlinked.baseDir, 'package.json'), 'the cached value for the unlinked path should be to itself');
        });
        it('resolves an existing directory as null (it is not a file)', function () {
            var result = resolvePackagePath._getRealFilePath(cache, project.root);
            assert.isNull(result, 'reference to a directory should return null');
            assert.isNull(cache.get(project.root), 'cache reference to directory should return null');
        });
        it('resolves a non-existent path as null', function () {
            var result = resolvePackagePath._getRealFilePath(cache, project.root + '2');
            assert.isNull(result, 'reference to a non-existent path correctly returns null');
        });
    });
    describe('resolvePackagePath', function () {
        var caches;
        beforeEach(function () {
            caches = new CacheGroup();
        });
        it('no module name', function () {
            assert.throws(function () { return resolvePackagePath(caches, undefined, '/'); }, TypeError);
        });
        it('baseDir were the end of the path does not exist, but the start does and if resolution proceeds would be a hit', function () {
            assert.equal(resolvePackagePath(caches, 'bar', path.join(foo.baseDir, 'node_modules', 'does-not-exist')), path.join(foo.baseDir, 'node_modules', 'bar', 'package.json'));
        });
        it('invalid basedir', function () {
            assert.equal(resolvePackagePath(caches, 'omg', 'asf'), null);
        });
        it('linked directory as name', function () {
            var result = resolvePackagePath(caches, linked.baseDir, undefined);
            assert.equal(path.join(unlinked.baseDir, 'package.json'), result, 'should resolve to unlinked "linked/package.json"');
        });
        it('through linked directory as dir to node_modules package', function () {
            var result = resolvePackagePath(caches, 'bar', linked.baseDir);
            assert.equal(path.join(unlinked.baseDir, 'node_modules', 'bar', 'package.json'), result, 'should resolve to unlinked "linked/node_ odules/bar/package.json"');
        });
        it('.. relative path through "linked" directory', function () {
            var result = resolvePackagePath(caches, '../linked', fooBar.baseDir);
            assert.equal(path.join(unlinked.baseDir, 'package.json'), result, 'should resolve to unlinked "linked/package.json"');
        });
        it('. relative path ', function () {
            var result = resolvePackagePath(caches, '..', path.join(foo.baseDir, 'node_modules'));
            assert.equal(path.join(foo.baseDir, 'package.json'), result, 'should resolve to "foo/package.json"');
        });
        it('. relative empty path ', function () {
            var result = resolvePackagePath(caches, '.', foo.baseDir);
            assert.equal(path.join(foo.baseDir, 'package.json'), result, 'should resolve to "foo/package.json"');
        });
    });
});
