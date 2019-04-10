#!/usr/bin/env node

'use strict';

const app = require('commander');
const cfg = require('../package.json');
const beautify = require('../index');
const path = require('path');
const fs = require('fs');

app.version(cfg.version)
    .option('-f, --format <format>', 'input format.(optional)', /^(css|json|js|html|xml)$/, 'json')
    .option('-o, --output <file>', 'output file or folder')
    .description('prettify js/json, html/xml or css')

app.parse(process.argv);

let write = (p, data) => new Promise((res, rej) => {
    console.log(path.resolve(p));
    fs.writeFile(path.resolve(p), data, 'utf8', err => {
        if (err) return rej(err);

        return res();
    });
});

let err = err => console.log(err);
let output = o => {
    if (!app.output) {
        console.log(o);
    } else {
        fs.lstat(path.resolve(app.output), (err, stat) => {
            if(!err) {
                if (stat.isDirectory()) {
                    Promise.all(app.args.map(f => write(app.output + '/' + path.basename(f), o)))
                           .catch(err);
                } else {
                    // TODO this one still not very clear
                    write(app.output, o).catch(err);
                }
            } else {
                // if we have many files it's evident 
                // that user wants to create folder and not rewrite one file
                // with output
                if (app.args.length > 1) {
                    fs.mkdir(path.resolve(app.output), err => {
                        if (!err) {
                            Promise.all(app.args.map(f => write(app.output + '/' + path.basename(f), o)))
                                   .catch(err);
                        } else {
                            console.log(err);
                        }
                    });
                } else {
                    write(app.output, o).catch(err);
                }
            }
        });
    }
};

if(app.args.length) {
    fs.readFile(app.args[0], 'utf8', (err, data) => {
        if (err) throw err;

        let b = beautify(data, {
            format: (app.format||path.extname(app.args[0]).substring(1)).toLowerCase()
        });

        output(b);
    });
} else {
    let data = '';

    process.stdin.resume();
    process.stdin.setEncoding('utf8');

    process.stdin.on('data', function(chunk) {
        data += chunk;
    });

    process.stdin.on('end', function() {
        let b = beautify(data, {
            format: app.format.toLowerCase()
        });

        output(b);
    });
}
