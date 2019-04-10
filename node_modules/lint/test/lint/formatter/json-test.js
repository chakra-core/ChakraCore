/*jslint nodejs: true, indent:4 */

var vows = require('vows');
var assert = require('assert');
var formatter = require('../../../lib/lint/formatter/json');

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

var FormatterTest = vows.describe('JSON Formatter class').addBatch({
    "format()" : {
        topic : function (item) {
            return createFormatter();
        },
        'should format empty report' : function (topic) {
            assert.equal(topic.format(createReportEmpty()), '[]');
        },
        'should format valid report' : function (topic) {
            assert.equal(topic.format(createReportValid()), '[{"file":"foo","errors":[],"isValid":true}]');
        },
        'should format invalid report' : function (topic) {
            assert.equal(topic.format(createReportInvalid()), '[{"file":"foo","errors":[{"file":"foo","line":12,"character":5,"evidence":"e","reason":"r"}],"isValid":false}]');
        }
    }
});

exports.FormatterTest = FormatterTest;