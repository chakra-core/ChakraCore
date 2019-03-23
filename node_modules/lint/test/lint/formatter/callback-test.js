/*jslint nodejs: true, indent:4 */

var vows = require('vows');
var assert = require('assert');
var formatter = require('../../../lib/lint/formatter/callback');

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

var FormatterTest = vows.describe('Callback Formatter class').addBatch({
    "format() / with function as callback" : {
        topic : function (item) {
            return createFormatter({callback: function (report) {
                return report;
            }});
        },
        'should format empty report' : function (topic) {
            
            assert.deepEqual(topic.format(createReportEmpty()), createReportEmpty());
        },
        'should format valid report' : function (topic) {
            assert.deepEqual(topic.format(createReportValid()), createReportValid());
        },
        'should format invalid report' : function (topic) {
            assert.deepEqual(topic.format(createReportInvalid()), createReportInvalid());
        }
    },
    "format() / with raw javascript as callback" : {
        topic : function (item) {
            return createFormatter({callback: "function format(report) {return report};"});
        },
        'should format empty report' : function (topic) {
            
            assert.deepEqual(topic.format(createReportEmpty()), createReportEmpty());
        },
        'should format valid report' : function (topic) {
            assert.deepEqual(topic.format(createReportValid()), createReportValid());
        },
        'should format invalid report' : function (topic) {
            assert.deepEqual(topic.format(createReportInvalid()), createReportInvalid());
        }
    }
});

exports.FormatterTest = FormatterTest;