
/*---------------------------------------------------------
* Copyright (C) Microsoft Corporation. All rights reserved.
*--------------------------------------------------------*/
'use strict';

var path = require('path');
var fs = require('fs');
var plistParser = require('./plistParser');
var request = require('request');

function convertGrammar(location, extensionConfig) {
    extensionConfig.languageId = '';
    extensionConfig.languageName = '';
    extensionConfig.languageScopeName = '';
    extensionConfig.languageExtensions = [];

    if (!location) {
        extensionConfig.languageContent = '';
        return Promise.resolve();
    }

    if (location.match(/\w*:\/\//)) {
        // load from url
        return new Promise(function (resolve, reject) {
            request(location, function (error, response, body) {
                if (!error && response.statusCode == 200) {
                    var contentDisposition = response.headers['content-disposition'];
                    var fileName = '';
                    if (contentDisposition) {
                        var fileNameMatch = contentDisposition.match(/filename="([^"]*)/);
                        if (fileNameMatch) {
                            fileName = fileNameMatch[1];
                        }
                    }
                    processContent(extensionConfig, fileName, body);
                } else {
                    return Promise.reject("Problems loading language definition file: " + error);
                }
                resolve();
            });
        });
    } else {
        // load from disk
        var body = null;
        // trim the spaces of the location path
        location = location.trim()
        try {
            body = fs.readFileSync(location);
        } catch (error) {
            return Promise.reject("Problems loading language definition file: " + error.message);
        }
        if (body) {
            processContent(extensionConfig, path.basename(location), body.toString());
            return Promise.resolve();
        } else {
            return Promise.reject("Problems loading language definition file: Not found");
        }
    }
}

function processContent(extensionConfig, fileName, body) {
    if (body.indexOf('<!DOCTYPE plist') === -1) {
        return Promise.reject("Language definition file does not contain 'DOCTYPE plist'. Make sure the file content is really plist-XML.");
    }
    var result = plistParser.parse(body);
    if (result.value) {
        var languageInfo = result.value;

        extensionConfig.languageName = languageInfo.name || '';

        // evaluate language id
        var languageId = '';
        var languageScopeName;

        if (languageInfo.scopeName) {
            languageScopeName = languageInfo.scopeName;

            var lastIndexOfDot = languageInfo.scopeName.lastIndexOf('.');
            if (lastIndexOfDot) {
                languageId = languageInfo.scopeName.substring(lastIndexOfDot + 1);
            }
        }
        if (!languageId && fileName) {
            var lastIndexOfDot2 = fileName.lastIndexOf('.');
            if (lastIndexOfDot2 && fileName.substring(lastIndexOfDot2 + 1) == 'tmLanguage') {
                languageId = fileName.substring(0, lastIndexOfDot2);
            }
        }
        if (!languageId && languageInfo.name) {
            languageId = languageInfo.name.toLowerCase().replace(/[^\w-_]/, '');
        }
        if (!fileName) {
            fileName = languageId + '.tmLanguage';
        }

        extensionConfig.languageFileName = fileName;
        extensionConfig.languageId = languageId;
        extensionConfig.name = languageId;
        extensionConfig.languageScopeName = languageScopeName;

        // evaluate file extensions
        if (Array.isArray(languageInfo.fileTypes)) {
            extensionConfig.languageExtensions = languageInfo.fileTypes.map(function (ft) { return '.' + ft; });
        } else {
            extensionConfig.languageExtensions = languageId ? ['.' + languageId] : [];
        }
    }
    extensionConfig.languageContent = body;
};

exports.convertGrammar = convertGrammar;
