'use strict';

const jsBeautify = require('js-beautify').js_beautify;
const cssbeautify = require('cssbeautify');
const html = require('html').prettyPrint;

let files = [];

let clean = data => {
    if (~['"', "'"].indexOf(data[0]) &&
        ~['"', "'"].indexOf(data[data.length - 1]) &&
        data[0] === data[data.length - 1]
    ) {
        return data.substring(1, data.length - 1);
    }

    return data;
};

let beautify = (data, o) => {
    if (!data || !data.length) return '';

    data = clean(data.trim());

    switch (o.format) {
        case 'css':
            return cssbeautify(data, {
                indent: '    ',
                autosemicolon: true
            });
        case 'json':
        case 'js':
            return jsBeautify(data, {
                indent_size: 4
            });
        case 'html':
        case 'xml':
            return html(data);
    }
};

module.exports = beautify;