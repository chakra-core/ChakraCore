/*jslint nodejs: true, indent:4 */
/**
 * Imports
 */

/*******************************************************************************
 * Report class
 *
 * Usage:
 *
 * <pre>
 * var report = new Report();
 * report.addFile('foo.js', [
 *      //... some errors
 * ]);
 * report.addFile('bar.js', [
 *      //... some other errors
 * ]);
 * </pre>
 ******************************************************************************/
/**
 * Report constructor
 *
 * @constructor
 * @return
 */
function Report() {
    this._files = {};
    this._count = 0;
}

/**
 * Add file report
 *
 * @param {string} filePath
 * @param {Array} errors
 * @return
 */
Report.prototype.addFile = function (filePath, errors) {
    errors = errors || [];

    this._files[filePath] = {};
    this._files[filePath].file = filePath;
    this._files[filePath].errors = errors;
    this._files[filePath].isValid = errors.length === 0;

    this._count += errors.length;
    return this;
};

/**
 * Return the file report
 *
 * @param {string} file
 * @return {Object}
 */
Report.prototype.getFile = function (file) {
    return this._files[file];
};

/**
 *
 * @param {string} file
 * @return {int}
 */
Report.prototype.countFile = function (file) {
    var fileIterator, count;

    count = 0;
    for (fileIterator in this._files) {
        if (this._files.hasOwnProperty(fileIterator)) {
            count += 1;
        }
    }
    return count;
};

/**
 *
 * @param {string} file
 * @return {int}
 */
Report.prototype.countErrors = function (file) {
    if (file) {
        return this._files[file].errors.length;
    }
    return this._count;
};

/**
 * Iterate over the file reports
 *
 * @param {Function} iterator
 * @return this
 */
Report.prototype.forEach = function (iterator) {
    var file;

    for (file in this._files) {
        if (this._files.hasOwnProperty(file)) {
            iterator(this._files[file], file);
        }
    }
    return this;
};

/*******************************************************************************
 * Formatter class
 *
 * Usage:
 *
 * <pre>
 * var formatter = new Formatter({type: 'json'});
 *
 * formatter.formater( report)//report is instance of Report
 * </pre>
 ******************************************************************************/
/**
 * Formatter constructor
 *
 * @constructor
 */
function Formatter(options) {
    this._adapterType = 'cli';
    this._adapter = null;

    this.configure(options);
}

Formatter.SIMPLE = 'simple';
Formatter.NORMAL = 'normal';
Formatter.FULL = 'full';

/**
 * Return the adapter from configuration
 *
 * @return {Base}
 */
Formatter.prototype._getAdapter = function () {
    if (!this._adapter) {
        var formatterModule, FormatterClass, options;

        options = {
            type: this._adapterType,
            mode: Formatter.NORMAL
        };

        try {
            formatterModule = require('./formatter/' + options.type);
        } catch (e) {
            throw new Error('Formatter "' + options.type + '" not found');
        }
        FormatterClass = formatterModule.Formatter;

        if (FormatterClass === undefined) {
            throw new Error('Formatter class not found in module "' + options.type + '"');
        }

        this._adapter = new FormatterClass(options);
    }
    return this._adapter;
};

/**
 * Configure the formatter
 *
 * @return this
 */
Formatter.prototype.configure = function (options) {
    options = options || {};
    if (options.type !== undefined) {
        this._adapterType = options.type;
        this._adapter = null;
    }

    this._getAdapter().configure(options);
    return this;
};

/**
 * Return a formatted text
 *
 * @return {string}
 */
Formatter.prototype.format = function (report) {
    return this._getAdapter().format(report);
};

/*******************************************************************************
 * Base class
 *
 * Usage:
 *
 * > inherits from this class to create a new formatter
 ******************************************************************************/
/**
 * Base constructor
 *
 * @constructor
 * @param {Object} options
 */
function Base(options) {
    options = options || {};

    this._type = options.type;
    this.mode = Formatter.NORMAL;
    this.encoding = 'utf-8';
    this.tab = "\t";
    this.eol = this._guessEol();
    this.limit = null;

    this.configure(options);
}

/**
 * Configure the formatter
 *
 * @param {Object} options
 * - mode: quantity of information displayed (= simple|normal|full)
 * - encoding: character encoding utf-8
 * - tab: tabulation character used (default: "\t")
 * - eol: end of line character (default: "\n")
 * - limit: limit the error count (default: null)
 *
 * @return this
 */
Base.prototype.configure = function (options) {
    if (options) {
        if (options.mode !== undefined) {
            if (
                    options.mode !== Formatter.SIMPLE &&
                    options.mode !== Formatter.NORMAL &&
                    options.mode !== Formatter.FULL
            ) {
                throw new Error('mode "' + options.mode + '" is not recognized.');
            }

            this.mode = options.mode;
        }
        if (options.encoding !== undefined) {
            this.encoding = options.encoding;
        }

        if (options.tab !== undefined) {
            this.tab = options.tab;
        }
        if (options.eol !== undefined) {
            this.eol = options.eol;
        }

        if (options.limit !== undefined) {
            this.limit = options.limit;
        }
    }
    return this;
};

/**
 * Return formatted data
 *
 * @param {Object} data
 * @param {string} mode
 * @return {string}
 */
Base.prototype.format = function (report, mode) {
    if (report === undefined) {
        throw new Error('`report` should not be undefined');
    }

    if (!(report instanceof Report)) {
        throw new Error('`report` should be an instance of Report.');
    }

    mode = mode || this.mode;
    report = report || {};

    switch (mode) {
    case Formatter.SIMPLE:
        return this._formatSimple(report);
    case Formatter.NORMAL:
        return this._formatNormal(report);
    case Formatter.FULL:
        return this._formatFull(report);
    default:
        throw new Error('type "' + mode + '" does not exist');
    }
};

/**
 * Return simple formatted content (can be overriden)
 *
 * @return {string}
 */
Base.prototype._formatSimple = function (report) {
    return this._formatNormal(report);
};

/**
 * Return normal formatted content (can be overriden)
 *
 * @return {string}
 */
Base.prototype._formatNormal = function (report) {
    throw new Error('_formatNormal() not implemented.');
};

/**
 * Return full formatted content (can be overriden)
 *
 * @return {string}
 */
Base.prototype._formatFull = function (report) {
    return this._formatNormal(report);
};

Base.prototype._guessEol = function () {
    switch (process.platform) {
    case 'win':
    case 'windows'://TODO: check this if supported
        return "\r\n";
    default:
        return "\n";
    }

};





/**
 * Exports
 */
exports.Formatter = Formatter;
exports.Base = Base;
exports.Report = Report;