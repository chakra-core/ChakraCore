var validate = require('..')
  , fs = require('fs')
  , path = require('path')
  , assert = require('assert')
  , uglify = require('uglify-js')
  , each = require('lodash.foreach')
  , libDir = path.join(__dirname, 'fixtures', 'integration')
  , tests = {}
  , fixtures;

fixtures = fs.readdirSync(libDir);

each(fixtures, function (fixture) {
  if(fixture.charAt(0) == '.')
    return;

  tests['Uglified ' + fixture + ' should not throw'] = function () {
    var raw = fs.readFileSync(path.join(libDir, fixture)).toString()
      , result = uglify.minify(raw, {
          outSourceMap: fixture + '.min.map'
        , fromString: true
        });

    assert.doesNotThrow(function () {
      validate(result.code, result.map, {'?': raw});
    }, 'Valid ' + fixture + ' sourcemap should not throw');
  };
});

module.exports = tests;
