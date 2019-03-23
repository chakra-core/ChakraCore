/*jslint nodejs: true, indent:4 */
/**
 * Imports
 */
try {
	var vows = require('vows');
} catch (e) {
	console.info("vows cannot be found, extension is disabled");
	return;
}
var assert = require('assert');
var fs = require('fs');

require('../assert/extension');

/*******************************************************************************
 * LINT / Vows helpers
 *
 * Usage:
 * <pre>
 * require('lint').createTest()
 *
 * </pre>
 ******************************************************************************/
/**
 * Return a new Batch object for vows
 *
 * @param {Array} files
 * @param {Object} options passed to the JSLINT parser
 * @return {Object}
 */
function createBatch(files, options) {
    options = options || {};
    files = Array.isArray(files) ? files : [files];
    var batch = {},
        fileCount = files.length,
        filePath,
        fileModule,
        createSection;

    //Section generator
    createSection = function (filePath) {
        return function (topic) {
            assert.validateLintFile(filePath);
        };
    };

    //Create for all files
    while (fileCount > 0) {
        fileCount -= 1;
        filePath = files[fileCount];
        filePath = fs.realpathSync(filePath);
        batch[filePath] = createSection(filePath);
    }

    return {
        'JSLINT results' : batch
    };
}

/**
 * Create a default test Object. If files is specified then it creates a batch
 * with files and options and add it to the test object
 *
 * @param {Array} files
 * @param {Object} options
 * @return
 */
function createTest(files, options) {
    var test = vows.describe('JSLINT test');
    if (files) {
        test.addBatch(createBatch(files, options));
    }
    return test;
}

/**
 * Exports
 */
exports.createBatch = createBatch;
exports.createTest = createTest;