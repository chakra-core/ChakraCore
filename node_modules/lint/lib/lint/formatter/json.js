/*jslint nodejs: true, indent:4 */
/**
 * Import
 */
var util = require(process.binding('natives').util ? 'util' : 'sys');
var formatter = require('../formatter');

/**
 * Formatter constructor
 * 
 * @constructor
 * @extends lint.formatter.Base
 * @param {Object} options
 */
function Formatter(options) {
    formatter.Base.call(this, options);
}
util.inherits(Formatter, formatter.Base);

/**
 * 
 * @return {string}
 */
Formatter.prototype._formatNormal = function (report) {
    var data = [];
    report.forEach(function (fileReport) {
        data.push(fileReport);
    });
    
    return JSON.stringify(data);
};

/**
 * Exports
 */
exports.Formatter = Formatter;
