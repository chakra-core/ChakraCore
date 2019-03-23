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
    options = options || {};
    formatter.Base.call(this, options);
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
 * @return {string}
 */
Formatter.prototype._formatNormal = function (report) {
    var error_regexp = /^\s*(\S*(\s+\S+)*)\s*$/, output, errorCount, thisFormatter;

    thisFormatter = this;
    errorCount = report.countErrors();
    output = '';

    //if (length > 0 && length < 2) {
    report.forEach(function (fileReport) {
        var file, errors;

        file = fileReport.file;
        file = file.substring(file.lastIndexOf('/') + 1, file.length);
        errors = fileReport.errors;

        errors.forEach(function (error) {
            output += file + ': line ' + error.line;
            output += ', character ' + error.character;
            output += ', ' + error.reason + thisFormatter.eol;
            output += (error.evidence || '').replace(error_regexp, "$1") + thisFormatter.eol + thisFormatter.eol;
        });

    });
    //}
    output += errorCount + ' error' + ((errorCount <= 1) ? '' : 's') + thisFormatter.eol;

    return output;
};

/**
 * @return {string}
 */
Formatter.prototype._formatFull = function (report) {
    var output;

    output = '';
    output += this._line(0, '<html>');
    output += this._head(1);
    output += this._body(1, report);
    output += this._line(0, '</html>');
    return output;
};

Formatter.prototype._head = function (indentation) {
    var output;

    output = '';
    output += this._line(indentation, '<head>');
    output += this._css(indentation + 1);
    output += this._line(indentation, '</head>');
    return output;
};

Formatter.prototype._body = function (indentation, report) {
    var error_regexp = /^\s*(\S*(\s+\S+)*)\s*$/, output, errorCount, thisFormatter;

    thisFormatter = this;
    errorCount = report.countErrors();

    output = '';
    output += this._line(indentation, '<body>');
    output += this._line(indentation + 1, '<h1>' + errorCount + ' Error' + ((errorCount <= 1) ? '' : 's') + '</h1>');
    output += this._line(indentation + 1, '<hr/>');
    output += this._line(indentation + 1, '<ul/>');

    report.forEach(function (fileReport) {
        var file, errors;

        file = fileReport.file;
        errors = fileReport.errors;

        errors.forEach(function (error) {
            output += thisFormatter._line(indentation + 2, '<li>');
            output += thisFormatter._line(indentation + 3, '<a href="txmt://open?url=file://' + file + '&line=' + error.line + '&column=' + error.character + '">');
            output += thisFormatter._line(indentation + 4, '<strong>' + error.reason + '</strong> <em>line ' + error.line + ', character ' + error.character + '</em>');
            output += thisFormatter._line(indentation + 3, '</a>');

            output += thisFormatter._line(indentation + 3, '<pre>');
            output += thisFormatter._line(indentation + 4, '<code>');
            output += thisFormatter._line(indentation + 5, (error.evidence || '').replace(error_regexp, "$1"));
            output += thisFormatter._line(indentation + 4, '</code>');
            output += thisFormatter._line(indentation + 3, '</pre>');

            output += thisFormatter._line(indentation + 2, '</li>');
        });
    });

    output += thisFormatter._line(indentation + 1, '</ul>');
    output += thisFormatter._line(indentation, '</body>');
    return output;
};

Formatter.prototype._css = function (indentation) {
    var output, rules, ruleSelector, ruleAttributes, ruleAttribute, ruleValue;


    //Define rules
    rules = {
        'body': {
            'font-size' : '14px'
        },
        'pre': {
            'background-color': '#eee',
            'color': '#400',
            'margin': '3px 0'
        },
        'h1, h2': {
            'font-family': '"Arial, Helvetica"',
            'margin': '0 0 5px'
        },
        'h1': {
            'font-size': '20px'
        },
        'h2': {
            'font-size': '16px'
        },
        'a': {
            'font-family': '"Arial, Helvetica"'
        },
        'ul': {
            'margin': '10px 0 0 20px',
            'padding': '0',
            'list-style': 'none'
        },
        'li': {
            'margin': '0 0 10px'
        }
    };


    //Format all
    output = '';
    output += this._line(indentation, '<style type="text/css">');
    for (ruleSelector in rules) {
        if (rules.hasOwnProperty(ruleSelector)) {
            ruleAttributes = rules[ruleSelector];

            output += this._line(indentation + 1, ruleSelector + ' {');
            for (ruleAttribute in ruleAttributes) {
                if (ruleAttributes.hasOwnProperty(ruleAttribute)) {
                    ruleValue = ruleAttributes[ruleAttribute];
                    output += this._line(indentation + 2, ruleAttribute + ': ' + ruleValue + ';');
                }
            }
            output += this._line(indentation + 1, '}');
        }
    }
    output += this._line(indentation, '</style>');
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

/**
 * Exports
 */
exports.Formatter = Formatter;
