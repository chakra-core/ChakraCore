"use strict";
var path = require("path");
module.exports = function ensurePosix(filepath) {
    if (path.sep !== '/') {
        return filepath.split(path.sep).join('/');
    }
    return filepath;
};
