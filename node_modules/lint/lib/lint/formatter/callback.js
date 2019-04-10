/*jslint nodejs: true, indent:4 */
/**
 * Import
 */
var util = require(process.binding('natives').util ? 'util' : 'sys');
var runScript = process.binding('natives').vm ? require('vm').runInNewContext : function (code, sandbox, filename) {
    var scriptObj = new process.binding('evals').Script(code, filename);

    return scriptObj.runInNewContext(sandbox);
};
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
    this._callback = null;
    this._callbackName = 'format';

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
        options = options || {};
        Formatter.super_.prototype.configure.call(this, options);

        if (options.callback !== undefined) {
            if (typeof(options.callback) === 'string') {
                var scriptObj, env;

                env  = {};
                runScript(options.callback, env, __filename);
                options.callback = env[this._callbackName];
            }

            if (typeof(options.callback) === 'function') {
                this._callback = options.callback;
            } else {
                throw new Error('options.callback must be a Function');
            }
        }
    }
    return this;
};

/**
 *
 * @return {string}
 */
Formatter.prototype._formatNormal = function (report) {
    return this._callback(report);
};

/**
 * Exports
 */
exports.Formatter = Formatter;
