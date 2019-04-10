module.exports = function installedPodRegex() {
  'use strict';
  // https://github.com/CocoaPods/CocoaPods/blob/0.39.0/lib/cocoapods/installer.rb#L301
  return /^Installing ([^\.\s\/][^\s\/]*) (\d\S+)(?: \(was (\d\S+)\))?$/gm;
};
