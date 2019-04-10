/*jslint nodejs: true, indent:4 */
/**
 * Import
 */
var util = require(process.binding('natives').util ? 'util' : 'sys');
var path = require('path');
var formatter = require('../formatter');

/**
 * Formatter constructor
 *
 * @constructor
 * @extends lint.formatter.Base
 * @param {Object} options
 */
function Formatter(options) {
    options =  options || {};

    formatter.Base.call(this, options);
    this.pretty = false;

    this.configure(options);
}
util.inherits(Formatter, formatter.Base);

/**
 * Configure the Formatter
 *
 * @param {Object} options
 * @return this
 */
Formatter.prototype.configure = function (options) {
    if (options) {
        Formatter.super_.prototype.configure.call(this, options);

        if (options.pretty !== undefined) {
            this.pretty = options.pretty;
        }

        this.__tab = (this.pretty) ? this.tab  : '';
        this.__eol = (this.pretty) ? this.eol  : '';
    }
    return this;
};

/**
 *
 * @return {string}
 */
Formatter.prototype._formatNormal = function (report) {
    var output, thisFormatter;

    thisFormatter = this;
    output = '';
    output += thisFormatter._line(0, '<?xml version="1.0" encoding="' + thisFormatter.encoding.toUpperCase() + '" ?>');
    output += thisFormatter._line(0, '<jslint>');
    report.forEach(function (fileReport) {
        var errors, file;

        file = fileReport.file;
        errors = fileReport.errors;

        output += thisFormatter._line(1, '<file name="' + file + '">');

        errors.forEach(function (error) {
            output += thisFormatter._line(2, '<issue' +
                    ' ' + thisFormatter._attribute('line', error.line) +
                    ' ' + thisFormatter._attribute('char', error.character) +
                    ' ' + thisFormatter._attribute('evidence', error.evidence) +
                    ' ' + thisFormatter._attribute('reason', error.reason) +
                    ' />'
            );
        });

        output += thisFormatter._line(1, '</file>');

    });
    output += thisFormatter._line(0, '</jslint>');
    return output;
};

Formatter.prototype._line = function (indentation, content) {
    if (!content || content === '') {
        return this.__eol;
    }
    return this._indent(indentation, content + this.__eol);
};

Formatter.prototype._indent = function (count, content) {
    if (this.__tab === '') {
        return content;
    }

    var output = '';
    while (count > 0) {
        output += this.__tab;
        count -= 1;
    }
    output += content;
    return output;
};

Formatter.prototype._attribute = function (name, value) {
    return name + '="' + this._escape(value || '') + '"';
};

Formatter.prototype._escape = function (string) {
    return (string) ? ('' + string).replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;') : '';
};

/**
 * Exports
 */
exports.Formatter = Formatter;