var tmp = require('tmp');

module.exports = function testCanSymlink (options) {
  options = options || {};
  var fs = options.fs || require('fs');

  var canLinkSrc  = tmp.tmpNameSync();
  var canLinkDest = tmp.tmpNameSync();

  try {
    fs.writeFileSync(canLinkSrc, '');
  } catch (e) {
    return false
  }

  try {
    fs.symlinkSync(canLinkSrc, canLinkDest)
  } catch (e) {
    fs.unlinkSync(canLinkSrc)
    return false
  }

  fs.unlinkSync(canLinkSrc)
  fs.unlinkSync(canLinkDest)

  return true
}
