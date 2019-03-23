'use strict';
var Cache = require("./cache");
module.exports = /** @class */ (function () {
    function CacheGroup() {
        this.MODULE_ENTRY = new Cache();
        this.PATH = new Cache();
        this.REAL_FILE_PATH = new Cache();
        this.REAL_DIRECTORY_PATH = new Cache();
        Object.freeze(this);
    }
    return CacheGroup;
}());
