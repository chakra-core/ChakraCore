/*jslint nodejs: true, indent:4 */

var vows = require('vows');
var assert = require('assert');
var formatter = require('../../../lib/lint/formatter/vim');

var Report = require('../../../lib/lint/formatter').Report;

function createFormatter(options) {
    return new formatter.Formatter(options);
}

function createReportEmpty() {
    var report;
    report = new Report();
    return report;
}

function createReportValid() {
    var report;
    report = new Report();
    report.addFile('foo', []);
    return report;
}

function createReportInvalid() {
    var report;
    report = new Report();
    report.addFile('foo', [{
        file: 'foo',
        line: 12,
        character: 5,
        evidence: 'e',
        reason: 'r'
    }]);
    
    return report;
}

var FormatterTest = vows.describe('VIM Formatter class').addBatch({
    "format()" : {
        topic : function (item) {
            return createFormatter();
        },
        'should format empty report' : function (topic) {
            assert.equal(topic.format(createReportEmpty()), '');
        },
        'should format valid report' : function (topic) {
            assert.equal(topic.format(createReportValid()), '\n');
        },
        'should format invalid report' : function (topic) {
            assert.equal(topic.format(createReportInvalid()), 'foo line 12 column 5 Error: r e\n');
        }
    }
});

exports.FormatterTest = FormatterTest;