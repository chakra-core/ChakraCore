/*global describe, it*/
"use strict";

var assert = require('chai').assert,
    pretty = require('../index').pretty,
    prettyMin = require('../index-min').pretty,
    address,
    value,
    undefinedValue;

function onAnother(foo, bar) {
    var lola;

    lola = foo + bar;
    return lola;
}

address = { 'street': 'Callejon de las ranas 128', 'city': 'Falfurrias', 'state': 'Texas', 'zip': '88888-9999' };

value = {
    'name': 'Damaso Infanzon Manzo',
    'address': address,
    'longString': "one\ntwo\n\"three\"",
    'favorites': { 'music': ['Mozart', 'Beethoven', 'The Beatles'],  'authors': ['John Grisham', 'Isaac Asimov', 'P.L. Travers'], 'books': [ 'Pelican Brief', 'I, Robot', 'Mary Poppins' ] },
    'dates': [ new Date(), new Date("05/25/1954") ],
    'numbers': [ 10, 883, 521 ],
    'boolean': [ true, false, false, false ],
    'isObject': true,
    'isDuck': false,
    'onWhatever': function (foo, bar) {
        var lola;

        lola = foo + bar;
        return lola;
    },
    'onAnother': onAnother,
    'foo': undefinedValue,
    'thisShouldBeNull': null,
    'err': new Error('This is an error')
};

address.value = value;

describe('Object serialized for print', function () {
    var serialized, index;
    serialized = pretty(value, 4, 'print');
    console.log(serialized);
    it('Serialized object', function () {
        assert.isNotNull(serialized, 'This should never be null');
        assert.equal(typeof serialized, 'string');
        assert.notEqual(serialized, 'Error: no Javascript object provided');
    });

    index = 0;
    it('Testing indentation', function () {
        index = serialized.indexOf('    name: "Damaso Infanzon Manzo"', index);
        assert.notEqual(index, -1);

        index = serialized.indexOf('    address:', index);
        assert.notEqual(index, -1);

        index = serialized.indexOf('        street:', index);
        assert.notEqual(index, -1);

        index = serialized.indexOf('        city:', index);
        assert.notEqual(index, -1);

        index = serialized.indexOf('        state:', index);
        assert.notEqual(index, -1);

        index = serialized.indexOf('        zip:', index);
        assert.notEqual(index, -1);

        index = serialized.indexOf('        true,', index);
        assert.notEqual(index, -1);

        index = serialized.indexOf('        false,', index);
        assert.notEqual(index, -1);

        index = serialized.indexOf('        false', index);
        assert.notEqual(index, -1);
    });

    it('Dates created', function () {
        assert.notEqual(serialized.indexOf('Tue May 25 1954 00:00:00'), -1);
    });

    it('Functions found', function () {
        assert.notEqual(serialized.indexOf('onWhatever: "function (foo, bar)"'), -1);
        assert.notEqual(serialized.indexOf('onAnother: "function onAnother(foo, bar)"'), -1);
    });

    it('renders strings with escapes', function () {
        assert.notEqual(serialized.indexOf('longString: "one\\ntwo\\n\\"three\\""'), -1);
    });
});

describe('Object serialized with full function expansion', function () {
    var serialized,
        functionLiteral,
        row,
        index;

    functionLiteral = [
        "onWhatever: function (foo, bar) {",
        "var lola;",
        "lola = foo + bar;",
        "return lola;",
        "},",
        "onAnother: function onAnother(foo, bar) {",
        "var lola;",
        "lola = foo + bar;",
        "return lola;",
        "}"
    ];

    serialized = pretty(value, 4, 'print', true);
    console.log(serialized);
    it('Full function expand', function () {
        index = 0;
        for (row = 0; row < functionLiteral.length; row += 1) {
            index = serialized.indexOf(functionLiteral[row], index);
            assert.notEqual(index, -1);
        }
    });
});

describe('Object serialized for JSON', function () {
    var serialized = pretty(value, 4, 'JSON');
    it('Serialized object', function () {
        assert.isNotNull(serialized, 'This should never be null');
        assert.equal(typeof serialized, 'string');
        assert.notEqual(serialized, 'Error: no Javascript object provided');
    });

    it('JSON Serialized object', function () {
        assert.notEqual(serialized.indexOf('    "name": "Damaso Infanzon Manzo"'), -1);
        assert.notEqual(serialized.indexOf('    "address":'), -1);
        assert.notEqual(serialized.indexOf('        "street":'), -1);
        assert.notEqual(serialized.indexOf('        "city":'), -1);
        assert.notEqual(serialized.indexOf('        "state":'), -1);
        assert.notEqual(serialized.indexOf('        "zip":'), -1);
        assert.notEqual(serialized.indexOf('    "foo": undefined'), -1);
        assert.notEqual(serialized.indexOf('    "thisShouldBeNull": null'), -1);
    });
});

describe('Object serialized for HTML', function () {
    var serialized = pretty(value, 4, 'HTML');
    it('Serialized object', function () {
        assert.isNotNull(serialized, 'This should never be null');
        assert.equal(typeof serialized, 'string');
        assert.notEqual(serialized, 'Error: no Javascript object provided');
    });

    it('Indentation for HTML', function () {
        assert.notEqual(serialized.indexOf('&nbsp;&nbsp;&nbsp;&nbsp;"name": "Damaso Infanzon Manzo"'), -1);
        assert.notEqual(serialized.indexOf('&nbsp;&nbsp;&nbsp;&nbsp;"address":'), -1);
        assert.notEqual(serialized.indexOf('&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"street":'), -1);
        assert.notEqual(serialized.indexOf('&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"city":'), -1);
        assert.notEqual(serialized.indexOf('&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"state":'), -1);
        assert.notEqual(serialized.indexOf('&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"zip":'), -1);
        assert.notEqual(serialized.indexOf('&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;true,'), -1);
        assert.notEqual(serialized.indexOf('&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;false'), -1);
    });

    it('Line breaks for HTML', function () {
        assert.notEqual(serialized.indexOf('<br/>'), -1);
    });
});

describe('Using minified version', function () {
    var serialized = prettyMin(value, 4, 'print');
    it('Serialized object', function () {
        assert.isNotNull(serialized, 'This should never be null');
        assert.equal(typeof serialized, 'string');
        assert.notEqual(serialized, 'Error: no Javascript object provided');
    });

    it('Testing indentation', function () {
        assert.notEqual(serialized.indexOf('    name: "Damaso Infanzon Manzo"', 0), -1);
        assert.notEqual(serialized.indexOf('    address:'), -1);
        assert.notEqual(serialized.indexOf('        street:'), -1);
        assert.notEqual(serialized.indexOf('        city:'), -1);
        assert.notEqual(serialized.indexOf('        state:'), -1);
        assert.notEqual(serialized.indexOf('        zip:'), -1);
        assert.notEqual(serialized.indexOf('        true,'), -1);
        assert.notEqual(serialized.indexOf('        false'), -1);
    });

    it('Dates created', function () {
        assert.notEqual(serialized.indexOf('Tue May 25 1954 00:00:00'), -1);
    });
});

describe('Object serialized with default arguments', function () {
    var serialized = pretty(value);
    it('Serialized object', function () {
        assert.isNotNull(serialized, 'This should never be null');
        assert.equal(typeof serialized, 'string');
        assert.notEqual(serialized, 'Error: no Javascript object provided');
    });

    it('Testing indentation', function () {
        assert.notEqual(serialized.indexOf('    name: "Damaso Infanzon Manzo"', 0), -1);
        assert.notEqual(serialized.indexOf('    address:'), -1);
        assert.notEqual(serialized.indexOf('        street:'), -1);
        assert.notEqual(serialized.indexOf('        city:'), -1);
        assert.notEqual(serialized.indexOf('        state:'), -1);
        assert.notEqual(serialized.indexOf('        zip:'), -1);
        assert.notEqual(serialized.indexOf('        true,'), -1);
        assert.notEqual(serialized.indexOf('        false'), -1);
    });

    it('Dates created', function () {
        assert.notEqual(serialized.indexOf('Tue May 25 1954 00:00:00'), -1);
    });
});
