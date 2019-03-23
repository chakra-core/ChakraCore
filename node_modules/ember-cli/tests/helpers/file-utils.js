'use strict';

const fs = require('fs-extra');

function touch(path, obj) {
  if (!fs.existsSync(path)) {
    fs.createFileSync(path);
    fs.writeJsonSync(path, obj || {});
  }
}

function replaceFile(path, findString, replaceString) {
  if (fs.existsSync(path)) {
    let newFile;
    let file = fs.readFileSync(path, 'utf-8');
    let find = new RegExp(findString);
    let match = new RegExp(replaceString);
    if (!match.test(file)) {
      newFile = file.replace(find, replaceString);
      fs.writeFileSync(path, newFile, 'utf-8');
    }
  }
}

module.exports = {
  touch,
  replaceFile,
};
