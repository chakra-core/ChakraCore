var validate = require('..')
  , fs = require('fs')
  , path = require('path')
  , assert = require('assert')
  , invalidDir = path.join(__dirname, 'fixtures', 'invalid')
  , validDir = path.join(__dirname, 'fixtures', 'valid')
  , tests = {};

tests['Invalid Backbone.js should throw name mismatch'] = function () {
  var bbDir = path.join(invalidDir, 'Backbone')
    , raw = fs.readFileSync(path.join(bbDir, 'backbone.js')).toString()
    , min = fs.readFileSync(path.join(bbDir, 'backbone.min.js')).toString()
    , map = fs.readFileSync(path.join(bbDir, 'backbone.min.map')).toString()
    , expectedError = /^Error: Warning: mismatched names[^\t]*Mapping: 1:2234→211:23 "listenTo" in backbone\.js$/;

  // There is an artifically introduced mismatch on line 2234 of the uncompressed source
  try {
    validate(min, map, {'backbone.js': raw});
  }
  catch (e) {
    assert.equal(expectedError.test(e.toString()), true, 'The validation error is not the expected one');

    // We got an error, and it matched our expected output, so return
    return;
  }

  // We got no error. Wat. This is wrong.
  throw new Error('The invalid Backbone sourcemap should throw');
};

tests['Invalid Jquery should throw name mismatch'] = function () {
  var bbDir = path.join(invalidDir, 'Jquery')
    , raw = fs.readFileSync(path.join(bbDir, 'jquery-1.10.2.js')).toString()
    , min = fs.readFileSync(path.join(bbDir, 'jquery-1.10.2.min.js')).toString()
    , map = fs.readFileSync(path.join(bbDir, 'jquery-1.10.2.min.map')).toString()
    , expectedError = /^Error: Warning: mismatched names[^\t]*Mapping: 4:18→23:1 "readyList" in jquery-1\.10\.2\.js$/;

  // jQuery is hopelessly broken, with incorrect offsets EVERYWHERE.
  try {
    validate(min, map, {'jquery-1.10.2.js': raw});
  }
  catch (e) {
    assert.equal(expectedError.test(e.toString()), true, 'The validation error is not the expected one');

    // We got an error, and it matched our expected output, so return
    return;
  }

  // We got no error. Wat. This is wrong.
  throw new Error('The invalid Jquery sourcemap should throw');
};

tests['Invalid Jquery should throw source missing'] = function () {
  var bbDir = path.join(invalidDir, 'Jquery')
    , raw = fs.readFileSync(path.join(bbDir, 'jquery-1.10.2.js')).toString()
    , min = fs.readFileSync(path.join(bbDir, 'jquery-1.10.2.min.js')).toString()
    , map = fs.readFileSync(path.join(bbDir, 'jquery-1.10.2.min.map')).toString()
    , expectedError = /^Error: jquery-1\.10\.2\.js not found in jquery-notreally\.js$/;

  // The raw source was not declared with the correct key, so this should throw
  try {
    validate(min, map, {'jquery-notreally.js': raw});
  }
  catch (e) {
    assert.equal(expectedError.test(e.toString()), true, 'The validation error is not the expected one');

    // We got an error, and it matched our expected output, so return
    return;
  }

  // We got no error. Wat. This is wrong.
  throw new Error('The invalid Jquery sourcemap should throw');
};

tests['Valid Underscore should not throw'] = function () {
  var usDir = path.join(validDir, 'Underscore')
    , raw = fs.readFileSync(path.join(usDir, 'underscore.js')).toString()
    , min = fs.readFileSync(path.join(usDir, 'underscore.min.js')).toString()
    , map = fs.readFileSync(path.join(usDir, 'underscore.min.map')).toString();

  assert.doesNotThrow(function () {
    validate(min, map, {'underscore.js': raw});
  }, 'Valid Underscore sourcemap should not throw');
};

tests['Valid Minifyified bundle with inline sourcemap should not throw'] = function () {
  var mfDir = path.join(validDir, 'Minifyified')
    , min = fs.readFileSync(path.join(mfDir, 'bundle.min.js')).toString();

  assert.doesNotThrow(function () {
    validate(min);
  }, 'Valid Minifyified inline sourcemap and inline sourceContent should not throw');
};

tests['Valid Babel map should not throw'] = function () {
  var babelDir = path.join(validDir, 'Babel')
    , map = fs.readFileSync(path.join(babelDir, 'router.js.map')).toString();

  assert.doesNotThrow(function () {
    validate(null, map);
  }, 'Valid Minifyified inline sourcemap and inline sourceContent should not throw');
};

module.exports = tests;
