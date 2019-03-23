var validate
  , validateMapping
  , toAscii
  , assert = require('assert')
  , SMConsumer = require('source-map').SourceMapConsumer
  , each = require('lodash.foreach')
  , template = require('lodash.template')
  , jsesc = require('jsesc')
  , fancyArrow = String.fromCharCode(parseInt(2192,16));

// Lifted from UglifyJS
toAscii = function (str, identifier) {
    return str.replace(/[\u0080-\uffff]/g, function(ch) {
        var code = ch.charCodeAt(0).toString(16);
        if (code.length <= 2 && !identifier) {
            while (code.length < 2) code = "0" + code;
            return "\\x" + code;
        } else {
            while (code.length < 4) code = "0" + code;
            return "\\u" + code;
        }
    }).replace(/\x0B/g, "\\x0B");
};

// Performs simple validation of a mapping
validateMapping = function (mapping) {
  var prettyMapping = JSON.stringify(mapping, null, 2);

  assert.ok(mapping.generatedColumn!=null, 'missing generated column, mapping: ' + prettyMapping);
  assert.ok(mapping.generatedLine!=null, 'missing generated line, mapping: ' + prettyMapping);
  assert.ok(mapping.generatedColumn >= 0, 'generated column must be greater or equal to zero, mapping: ' + prettyMapping);
  assert.ok(mapping.generatedLine >= 0, 'generated line must be greater or equal to zero: ' + prettyMapping);

  // If the source is null, the original location data has been explicitly
  // omitted from the map to clear the mapped state of a line.
  if (typeof mapping.source === "string") {
    assert.ok(mapping.originalColumn!=null, 'missing original column, mapping: ' + prettyMapping);
    assert.ok(mapping.originalLine!=null, 'missing original line, mapping: ' + prettyMapping);
    assert.ok(mapping.originalColumn >= 0, 'original column must be greater or equal to zero, mapping: ' + prettyMapping);
    assert.ok(mapping.originalLine >= 0, 'original line must be greater or equal to zero, mapping: ' + prettyMapping);
  }
};

// Validates an entire sourcemap
validate = function (min, map, srcs) {
  var consumer
    , mappingCount = 0
    , splitSrcs = {};

  srcs = srcs || {};

  // If no map was given, try to extract it from min
  if(map == null) {
    try {
      var re = /\s*\/\/(?:@|#) sourceMappingURL=data:application\/json;base64,(\S*)$/m
        , map = min.match(re);

      map = (new Buffer(map[1], 'base64')).toString();
      min = min.replace(re, '');
    }
    catch (e) {
      throw new Error('No map argument provided, and no inline sourcemap found');
    }
  }

  try {
    consumer = new SMConsumer(map);
  }
  catch (e) {
    throw new Error('The map is not valid JSON');
  }

  each(consumer.sources, function (src) {
    var content = consumer.sourceContentFor(src);
    if(content)
      srcs[src] = content;
  });

  each(srcs, function (src, file) {
    return splitSrcs[file] = src.split('\n'); // Split sources by line
  });

  consumer.eachMapping(function (mapping) {
    mappingCount++;

    validateMapping(mapping);

    // These validations can't be performed with just the mapping
    var originalLine
      , errMsg
      , mapRef
      , expected
      , actuals = []
      , success = false;

    if(mapping.name) {
      if(!splitSrcs[mapping.source]) {
        throw new Error(mapping.source + ' not found in ' + Object.keys(splitSrcs).join(', '));
      }

      originalLine = splitSrcs[mapping.source][mapping.originalLine - 1];

      expected = [
        mapping.name
      , '\'' + jsesc(mapping.name) + '\''
      , '\'' + toAscii(mapping.name) + '\''
      , '"' + jsesc(mapping.name, {quotes: 'double'}) + '"'
      , '"' + toAscii(mapping.name) + '"'
      ];

      // An exact match
      for(var i=0, ii=expected.length; i<ii; i++) {
        // It's possible to go out of bounds on some stupid cases
        try {
          var actual = originalLine.split('').splice(mapping.originalColumn, expected[i].length).join('');
        }
        catch (e) {
          return;
        }

        actuals.push(actual);

        if(expected[i] === actual) {
          success = true;
          break;
        }
      };

      if(!success) {
        mapRef = template('<%=generatedLine%>:<%=generatedColumn%>'
                          + fancyArrow
                          + '<%=originalLine%>:<%=originalColumn%> "<%=name%>" in <%=source%>')(mapping);
        errMsg = template(''
          + 'Warning: mismatched names\n'
          + 'Expected: <%=expected%>\n'
          + 'Got: <%=actual%>\n'
          + 'Original Line: <%=original%>\n'
          + 'Mapping: <%=mapRef%>'
          , {
            expected: expected.join(' || ')
          , actual: actuals.join(' || ')
          , original: originalLine
          , mapRef: mapRef
          });

          throw new Error(errMsg);
      }
    }
  });

  assert.ok(JSON.parse(map).sources && JSON.parse(map).sources.length, 'There were no sources in the file');
  assert.ok(mappingCount > 0, 'There were no mappings in the file');
};

module.exports = validate;
