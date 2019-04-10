module.exports = function (grunt) {
    "use strict";

    function getLocalConfig() {
        var config,
            copy,
            fileSystem = require('fs');

        copy = {};
        fileSystem.readFile('./config.json', 'utf8', function (err, data) {
            if (err) {
                return;
            }

            config = JSON.parse(data);
            if (config.copyTo) {
                config.copyTo.forEach(function (element) {
                    copy[element.name] = {
                        'files': [
                            {
                                'src': ['index*.js'],
                                'cwd': '.',
                                'dest': element.path + '/node_modules/js-object-pretty-print/',
                                'expand': true,
                                'filter': 'isFile'
                            }
                        ]
                    };
                });
            }
        });

        return copy;
    }

    grunt.initConfig({
        'jslint': {
            'all': {
                'src': [
                    'index.js',
                    'Gruntfile.js',
                    '*.json',
                    'test/*.js'
                ],
                'directives': {
                    'node': true,
                    'indent': 4,
                    'maxlen': 300,
                    'predef': [
                    ]
                }
            }
        },
        'mochaTest': {
            'test': {
                'options': {
                    'reporter': 'spec'
                },
                'src': ['test/**/*.js']
            }
        },
        'uglify': {
            'options': {
                // the banner is inserted at the top of the output
                'banner': '/*! js-object-pretty-print.js <%= grunt.template.today("dd-mm-yyyy") %> */\n'
            },
            'dist': {
                'files': {
                    'index-min.js': ['index.js']
                }
            }
        },

        'copy': getLocalConfig()
    });

    grunt.loadNpmTasks('grunt-jslint');
    grunt.loadNpmTasks('grunt-mocha-test');
    grunt.loadNpmTasks('grunt-contrib-uglify');
    grunt.loadNpmTasks('grunt-contrib-copy');

    grunt.registerTask('default', [
        'jslint',
        'uglify',
        'mochaTest'
    ]);

    grunt.registerTask('local', [
        'jslint',
        'uglify',
        'mochaTest',
        'copy'
    ]);
};
