'use strict';

module.exports = function sha1_file(str_filename) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/sha1_file/
  // original by: Kevin van Zonneveld (http://kvz.io)
  //      note 1: Relies on file_get_contents which does not work in the browser, so Node only.
  //      note 2: Keep in mind that in accordance with PHP, the whole file is buffered and then
  //      note 2: hashed. We'd recommend Node's native crypto modules for faster and more
  //      note 2: efficient hashing
  //   example 1: sha1_file('test/never-change.txt')
  //   returns 1: '0ea65a1f4b4d69712affc58240932f3eb8a2af66'

  var fileGetContents = require('../filesystem/file_get_contents');
  var sha1 = require('../strings/sha1');
  var buf = fileGetContents(str_filename);

  if (buf === false) {
    return false;
  }

  return sha1(buf);
};
//# sourceMappingURL=sha1_file.js.map