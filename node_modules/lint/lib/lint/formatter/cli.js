/*jslint nodejs: true, indent:4 */
/**
 * Import
 */
var util = require(process.binding('natives').util ? 'util' : 'sys');
var formatter = require('../formatter');

/**
 * @link http://en.wikipedia.org/wiki/ANSI_escape_code
 */
var ESC = String.fromCharCode(27);
var CONSOLE_STYLES = {

    'font:normal': null,
    'font:alternate-1': [11, 10],
    'font:alternate-2': [12, 10],
    'font:alternate-3': [13, 10],
    'font:alternate-4': [14, 10],
    'font:alternate-5': [15, 10],
    'font:alternate-6': [16, 10],
    'font:alternate-7': [17, 10],
    'font:alternate-8': [18, 10],
    'font:alternate-9': [19, 10],

    'font-weight:normal' : null,
    'font-weight:bold' : [1, 22],

    'font-style:normal' : null,
    'font-style:italic' : [3, 23],

    'text-decoration:underline' : [4, 24],
    'text-decoration:blink' : [5, 25],
    'text-decoration:reverse' : [7, 27],
    'text-decoration:invisible' : [8, 28],

    'color:default' : null,
    'color:black' : [30, 39],
    'color:blue': [34, 39],
    'color:blue-hi': [94, 39],
    'color:cyan' : [36, 39],
    'color:green' : [32, 39],
    'color:green-hi' : [92, 39],
    'color:grey' : [90, 39],
    'color:magenta' : [35, 39],
    'color:red' : [31, 39],
    'color:red-hi' : [95, 39],
    'color:white' : [37, 39],
    'color:yellow' : [33, 39],

    'background-color:transparent': null,
    'background-color:black': [40, 49],
    'background-color:red': [41, 49],
    'background-color:green': [42, 49],
    'background-color:yellow': [43, 49],
    'background-color:blue': [44, 49],
    'background-color:magenta': [45, 49],
    'background-color:cyan': [46, 49],
    'background-color:white': [47, 49]
};

var stylize = function (str, style) {
    var styleDefinition, escape;

    styleDefinition = CONSOLE_STYLES[style];

    if (styleDefinition === undefined) {
        throw new Error('Undefined style "' + style + '"');
    }

    if (styleDefinition === null) {
        return str;
    }

    return ESC + '[' + styleDefinition[0] + 'm' + str + ESC + '[' + styleDefinition[1] + 'm';
};

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

    // customize the error reporting -- the following colours the text red
    this.colors = false;
    this.tab = '  ';
    this.styles = {
        '*': {
            'background-color' : 'transparent',
            'font-style' : 'normal',
            'font-weight' : 'normal',
            'color' :  'default',
            'text-decoration' :  []
        },
        'file': {
            'font': null,
            'font-weight' : 'bold',
            'text-decoration' : ['underline']
        },
        'file-error-count': {
            'font-weight' : 'bold'
        },
        'summary-result': {
            'font-weight' : 'bold'
        },
        'summary-result-valid': {
            'color' : 'green'
        },
        'summary-result-invalid': {
            'color' : 'red'
        },
        'summary-file-count': {
            'font-weight' : 'bold'
        },
        'summary-error-count': {
            'font-weight' : 'bold'
        },

        'error-line': {
            'font-weight' : 'bold'
        },
        'error-character': {
            'font-weight' : 'bold'
        },
        'error-position': {
            'color' : 'grey'
        },

        'error-evidence': {
            'font-weight' : 'bold',
            'color' : 'blue'
        },
        'error-reason': {
            'font-style' : 'italic'
        }
    };
    this._styleCache = {};

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
        var styleClass, styleClassAttributes, styleClassAttribute;

        Formatter.super_.prototype.configure.call(this, options);

        if (options.colors !== undefined) {
            this.colors = options.colors;
        }

        if (options.styles) {
            for (styleClass in options.styles) {
                if (options.style.hasOwnProperty(styleClass)) {
                    styleClassAttributes = options.styles[styleClass];

                    this.styles[styleClass] = this.styles[styleClass] || {};

                    for (styleClassAttribute in styleClassAttributes) {
                        if (styleClassAttributes.hasOwnProperty(styleClassAttribute)) {
                            this.styles[styleClass][styleClassAttribute] = styleClassAttributes[styleClassAttribute];
                        }
                    }
                }
            }
            this._styleCache = {};
        }
    }
    return this;
};

/**
 *
 * @return {string}
 */
Formatter.prototype._formatSimple = function (report) {
    var errorCount, fileCount, hasError, output, files;

    errorCount = report.countErrors();
    hasError = errorCount === 0;

    //Index by files
    fileCount = report.countFile();

    output = '';
    output += this._stylize(this._stylize(hasError ? '✓ Valid' : '✗ Invalid', 'summary-result'), hasError ? 'summary-result-valid' : 'summary-result-invalid') + ' » ';
    output += this._stylize(fileCount, 'summary-file-count') + ' file' + ((fileCount <= 1) ? '' : 's');
    output += ' ∙ ';
    output += this._stylize(errorCount, 'summary-error-count') + ' error' + ((errorCount <= 1) ? '' : 's');

    return this._line(0, output);
};

/**
 *
 * @return {string}
 */
Formatter.prototype._formatNormal = function (report) {
    var thisFormatter, error_regexp, output, errorCount;

    thisFormatter = this;
    error_regexp = /^\s*(\S*(\s+\S+)*)\s*$/;
    errorCount = report.countErrors();


    output = '';

    report.forEach(function (fileReport) {

        if (!fileReport.isValid) {
            output += thisFormatter._line(0, '♢ ' + thisFormatter._stylize(fileReport.file, 'file'));

            fileReport.errors.forEach(function (error, errorIndex) {
                output += thisFormatter._line(1,
                    thisFormatter._stylize((errorIndex + 1) + ')', 'error-index') + ' ' +
                    thisFormatter._stylize((error.evidence || '').replace(error_regexp, "$1"), 'error-evidence') + ' ' +

                    thisFormatter._stylize(' //' +
                        'line ' + thisFormatter._stylize(error.line, 'error-line'),
                    'error-position')
                );
                output += thisFormatter._line(2,  thisFormatter._stylize('» ' + error.reason, 'error-reason'));
            });

            output += thisFormatter._line(0);
        }
    });


    //sum up
    output += this._formatSimple(report);
    return output;
};

/**
 * @return {string}
 */
Formatter.prototype._formatFull = function (report) {
    var thisFormatter, error_regexp, output, errorCount;

    thisFormatter = this;
    error_regexp = /^\s*(\S*(\s+\S+)*)\s*$/;
    errorCount = report.countErrors();


    output = '';
    report.forEach(function (fileReport) {

        if (!fileReport.isValid) {
            output += thisFormatter._line(0,
                    '♢ ' + thisFormatter._stylize(fileReport.file, 'file') +
                    ' (' + thisFormatter._stylize(fileReport.errors.length, 'file-error-count') + ' errors)'
            );

            fileReport.errors.forEach(function (error, errorIndex) {
                output += thisFormatter._line(1,
                    thisFormatter._stylize((errorIndex + 1) + ')', 'error-index') + ' ' +
                    thisFormatter._stylize((error.evidence || '').replace(error_regexp, "$1"), 'error-evidence') + ' ' +

                    thisFormatter._stylize(' //' +
                        'line ' + thisFormatter._stylize(error.line, 'error-line') +
                        ', ' +
                        'character ' + thisFormatter._stylize(error.character, 'error-character'),
                    'error-position')
                );
                output += thisFormatter._line(2,  thisFormatter._stylize('» ' + error.reason, 'error-reason'));
            });

            output += thisFormatter._line(0);
        }
    });


    //sum up
    output += this._formatSimple(report);

    return output;
};

Formatter.prototype._line = function (indentation, content) {
    if (!content || content === '') {
        return this.eol;
    }
    return this._indent(indentation, content + this.eol);
};

Formatter.prototype._indent = function (count, content) {
    if (this.tab === '') {
        return content;
    }

    var output = '';
    while (count > 0) {
        output += this.tab;
        count -= 1;
    }
    output += content;
    return output;
};

Formatter.prototype._stylize = function (str, klass) {
    if (!this.colors) {
        return str;
    }
    var output, style, styleRoot, styleClass, styleAttribute;

    style = this._styleCache[klass];
    if (!style) {
        this._styleCache[klass] = {};

        style = this._styleCache[klass];
        styleRoot = this.styles['*'];
        styleClass = this.styles[klass];

        if (styleRoot) {
            for (styleAttribute in styleRoot) {
                if (styleRoot.hasOwnProperty(styleAttribute)) {
                    style[styleAttribute] = styleRoot[styleAttribute];
                }
            }
        }

        if (styleClass) {
            for (styleAttribute in styleClass) {
                if (styleClass.hasOwnProperty(styleAttribute)) {
                    style[styleAttribute] = styleClass[styleAttribute];
                }
            }
        }
    }

    output = str;
    output = stylize(output, 'font-weight:' + style['font-weight']);
    output = stylize(output, 'font-style:' + style['font-style']);
    output = stylize(output, 'color:' + style.color);
    output = stylize(output, 'background-color:' + style['background-color']);

    //Normalize as array
    if (!Array.isArray(style['text-decoration'])) {
        style['text-decoration'] = [style['text-decoration']];
    }

    //do all decoration
    style['text-decoration'].forEach(function (textdecoration) {
        output = stylize(output, 'text-decoration:' + textdecoration);
    });

    return output;
};

/**
 * Exports
 */
exports.Formatter = Formatter;
