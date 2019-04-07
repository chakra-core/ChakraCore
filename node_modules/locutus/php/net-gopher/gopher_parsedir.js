'use strict';

module.exports = function gopher_parsedir(dirent) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/gopher_parsedir/
  // original by: Brett Zamir (http://brett-zamir.me)
  //   example 1: var entry = gopher_parsedir('0All about my gopher site.\t/allabout.txt\tgopher.example.com\t70\u000d\u000a')
  //   example 1: entry.title
  //   returns 1: 'All about my gopher site.'

  /* Types
   * 0 = plain text file
   * 1 = directory menu listing
   * 2 = CSO search query
   * 3 = error message
   * 4 = BinHex encoded text file
   * 5 = binary archive file
   * 6 = UUEncoded text file
   * 7 = search engine query
   * 8 = telnet session pointer
   * 9 = binary file
   * g = Graphics file format, primarily a GIF file
   * h = HTML file
   * i = informational message
   * s = Audio file format, primarily a WAV file
   */

  var entryPattern = /^(.)(.*?)\t(.*?)\t(.*?)\t(.*?)\u000d\u000a$/;
  var entry = dirent.match(entryPattern);

  if (entry === null) {
    throw new Error('Could not parse the directory entry');
    // return false;
  }

  var type = entry[1];
  switch (type) {
    case 'i':
      // GOPHER_INFO
      type = 255;
      break;
    case '1':
      // GOPHER_DIRECTORY
      type = 1;
      break;
    case '0':
      // GOPHER_DOCUMENT
      type = 0;
      break;
    case '4':
      // GOPHER_BINHEX
      type = 4;
      break;
    case '5':
      // GOPHER_DOSBINARY
      type = 5;
      break;
    case '6':
      // GOPHER_UUENCODED
      type = 6;
      break;
    case '9':
      // GOPHER_BINARY
      type = 9;
      break;
    case 'h':
      // GOPHER_HTTP
      type = 254;
      break;
    default:
      return {
        type: -1,
        data: dirent
      }; // GOPHER_UNKNOWN
  }
  return {
    type: type,
    title: entry[2],
    path: entry[3],
    host: entry[4],
    port: entry[5]
  };
};
//# sourceMappingURL=gopher_parsedir.js.map