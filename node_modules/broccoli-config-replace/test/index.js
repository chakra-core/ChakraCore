var assert = require('assert');
var broccoli = require('broccoli');
var ConfigReplace = require('..');
var join = require('path').join;
var fs = require('fs');
var tmp = require('tmp-sync');
var expect = require('chai').expect;

afterEach(function() {
  if (this.builder) {
    this.builder.cleanup();
  }
});

function writeExample(options) {
  var root = tmp.in(join(process.cwd(), 'tmp'));

  fs.writeFileSync(join(root, 'config.json'), options.config);
  fs.writeFileSync(join(root, 'index.html'), options.index);

  return root;
}

function read(fullPath) {
  return fs.readFileSync(fullPath, 'UTF8');
}

function makeConfigReplace(root, patterns) {
  return new ConfigReplace(
    // In these tests, config.json and index.html live in the same
    // directory, so we just pass the root twice.
    root,
    root, {
      files: ['index.html'],
      configPath: 'config.json',
      patterns: patterns
    }
  );
}

function makeBuilder(root, patterns) {
  var configReplace = makeConfigReplace(root, patterns);
  return new broccoli.Builder(configReplace);
}

function expectEquals(expected) {
  return function(results) {
    var resultsPath = join(results.directory, 'index.html'),
        contents = fs.readFileSync(resultsPath, { encoding: 'utf8' });

    assert.equal(contents.trim(), expected);
    return results;
  };
}

describe('config-replace', function() {
  it('replaces with text from config.json', function() {
    var root = writeExample({
      config: '{"color":"red"}',
      index: '{{color}}'
    });

    return makeBuilder(root, [{
      match: /\{\{color\}\}/g,
      replacement: function(config) { return config.color; }
    }]).build().then(
      expectEquals('red')
    );
  });

  it('replaces with string passed in via options', function() {
    var root = writeExample({
      config: '{}',
      index: '{{name}}'
    });

    return makeBuilder(root, [{
      match: /\{\{name\}\}/g,
      replacement: 'hari'
    }]).build().then(
      expectEquals('hari')
    );
  });

  it('rebuilds if the config file changes', function() {
    var root = writeExample({
      config: '{"pokemon":"diglet"}',
      index: '{{pokemon}}'
    });

    var builder = makeBuilder(root, [{
      match: /\{\{pokemon\}\}/g,
      replacement: function(config) { return config.pokemon; }
    }]);

    return builder.build().then(
      expectEquals('diglet')
    ).then(function() {
      fs.writeFileSync(join(root, 'config.json'), '{"pokemon":"jigglypuff"}');
      return builder.build();
    }).then(
      expectEquals('jigglypuff')
    );
  });

  it('caches the result', function() {
    var root, configReplace, builder, key, entry;

    root = writeExample({
      config: '{"city":"nyc"}',
      index: '{{city}}'
    });

    configReplace = makeConfigReplace(root, [{
      match: /\{\{city\}\}/g,
      replacement: function(config) { return config.city; }
    }]);

    builder = new broccoli.Builder(configReplace);

    var indexStat;
    return builder.build().then(
      expectEquals('nyc')
    ).then(function() {
      indexStat = fs.statSync(join(root, 'index.html'));

      return builder.build();
    }).then(function() {
      var nextStat = fs.statSync(join(root, 'index.html'));
      assert.deepEqual(indexStat, nextStat);
    });
  });

  it('evicts after change', function() {
    var root, configReplace, builder, key, entry;

    root = writeExample({
      config: '{"city":"nyc"}',
      index: '{{city}}'
    });

    configReplace = makeConfigReplace(root, [{
      match: /\{\{city\}\}/g,
      replacement: function(config) { return config.city; }
    }]);

    builder = new broccoli.Builder(configReplace);

    var oldContent;

    return builder.build().then(function(results) {
      oldContent = read(results.directory + '/index.html');

      fs.writeFileSync(root + '/index.html', 'foo');
      return builder.build();
    }).then(function(results) {

      var newContent = read(results.directory + '/index.html');
      expect(newContent).to.not.equal(oldContent);
    });
  });

  it('handle nested paths', function() {
    var root, configReplace, builder;

    root = tmp.in(join(process.cwd(), 'tmp'));

    fs.writeFileSync(join(root, 'config.json'), '{"color":"red"}');
    fs.mkdirSync(join(root, 'dir1'));
    fs.mkdirSync(join(root, 'dir1', 'dir2'));
    fs.writeFileSync(join(root, 'dir1', 'dir2', 'index.html'), "{{color}}");

    configReplace = new ConfigReplace(
      root,
      root, {
        files: ['dir1/dir2/index.html'],
        configPath: 'config.json',
        patterns: [{
          match: /\{\{color\}\}/g,
          replacement: 'red'
        }]
      }
    );

    builder = new broccoli.Builder(configReplace);

    return builder.build().then(function(results) {
      var resultsPath = join(results.directory, 'dir1', 'dir2', 'index.html'),
          contents = fs.readFileSync(resultsPath, { encoding: 'utf8' });

      assert.equal(contents.trim(), 'red');
    })
  });
});
