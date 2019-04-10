'use strict';
function includes(array, entry) {
    var result = false;
    for (var i = 0; i < array.length; i++) {
        if (array[i] === entry) {
            return true;
        }
    }
    return false;
}
module.exports = function (process) {
    return !!process.env.NODE_PRESERVE_SYMLINKS || includes(process.execArgv, '--preserve-symlinks');
};
