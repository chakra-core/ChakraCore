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
 * @return {string}
 */
Formatter.prototype._formatNormal = function (report) {
    var error_regexp, output, thisFormatter;

    thisFormatter = this;
    error_regexp = /^\s*(\S*(\s+\S+)*)\s*$/;

    output = '';
    report.forEach(function (fileReport) {
        fileReport.errors.forEach(function (error) {
            output += fileReport.file;
            output += ' line ' + error.line;
            output += ' column ' + error.character;
            output += ' Error: ' + error.reason + ' ' + (error.evidence || '').replace(error_regexp, "$1");
        });
        output += thisFormatter.eol;
    });


    return output;
};

/**
 * Exports
 */
exports.Formatter = Formatter;