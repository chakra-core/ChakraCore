"use strict";

var address,
    value,
    pretty;

pretty = require('../js-object-pretty-print').pretty;

function onAnother(foo, bar) {
    var lola;

    lola = foo + bar;
    return lola;
}

address = { 'street': 'Callejon de las ranas 128', 'city': 'Falfurrias', 'state': 'Texas', 'zip': '88888-9999' };

value = {
    'name': 'Damaso Infanzon Manzo',
    'address': address,
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
    'foo': undefined,
    err: new Error('This is a bad error')
};
process.stdout.write(pretty(value));
