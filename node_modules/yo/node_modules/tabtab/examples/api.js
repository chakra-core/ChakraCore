var tab = require('tabtab');

tab.on('complete', function(err, data, done) {
  // General handler
  done(null, ['foo', 'bar']);
});

// yourbin command completion
//
// Ex. yourbin list
tab.on('list', function(err, data, done) {
  done(null, ['file.js', 'file2.js']);
});
