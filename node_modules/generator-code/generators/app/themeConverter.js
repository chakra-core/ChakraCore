/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
'use strict';

var path = require('path');
var fs = require('fs');
var plistParser = require('./plistParser');
var request = require('request');

function convertTheme(location, extensionConfig, inline, generator) {
    if (!location) {
        extensionConfig.tmThemeFileName = '';
        extensionConfig.tmThemeContent = '';
    } else if (location.match(/\w*:\/\//)) {
        // load from url
        return new Promise(function (resolve, reject) {
            request(location, function (error, response, body) {
                if (!error && response.statusCode == 200) {
                    var tmThemeFileName = null;
                    if (!inline) {
                        var contentDisposition = response.headers['content-disposition'];
                        if (contentDisposition) {
                            var fileNameMatch = contentDisposition.match(/filename="([^"]*)/);
                            if (fileNameMatch) {
                                tmThemeFileName = fileNameMatch[1];
                            }
                        }
                        if (!tmThemeFileName) {
                            var lastSlash = location.lastIndexOf('/');
                            if (lastSlash) {
                                tmThemeFileName = location.substr(lastSlash + 1);
                            } else {
                                tmThemeFileName = 'theme.tmTheme';
                            }
                        }
                    }
                    processContent(extensionConfig, tmThemeFileName, body, generator);
                    resolve();
                } else {
                    if (error) {
                        reject("Problems loading theme: " + error);
                    } else {
                        reject("Problems loading theme: HTTP status " + response.statusCode);
                    }
                }
            });
        });
    } else {
        // load from disk
        var body = null;
        try {
            body = fs.readFileSync(location);
        } catch (error) {
            return Promise.reject("Problems loading theme: " + error.message);
        }
        if (body) {
            var fileName = null;
            if (!inline) {
                fileName = path.basename(location);
            }
            processContent(extensionConfig, fileName, body.toString(), generator);
        } else {
            return Promise.reject("Problems loading theme: Not found");
        }
    }
    return Promise.resolve();
}

function processContent(extensionConfig, tmThemeFileName, body, generator) {
    var themeNameMatch = body.match(/<key>name<\/key>\s*<string>([^<]*)/);
    var themeName = themeNameMatch ? themeNameMatch[1] : '';

    extensionConfig.themeContent = migrate(body, tmThemeFileName, generator);
    if (tmThemeFileName) {
        if (tmThemeFileName.indexOf('.tmTheme') === -1) {
            tmThemeFileName = tmThemeFileName + '.tmTheme';
        }
        extensionConfig.tmThemeFileName = tmThemeFileName;
        extensionConfig.tmThemeContent = body;
    }
    extensionConfig.themeName = themeName;
    extensionConfig.displayName = themeName;
};

// mapping from old tmTheme setting to new workbench color ids
var mappings = {
    "background": ["editor.background"],
    "foreground": ["editor.foreground"],
    "hoverHighlight": ["editor.hoverHighlightBackground"],
    "linkForeground": ["editorLink.foreground"],
    "selection": ["editor.selectionBackground"],
    "inactiveSelection": ["editor.inactiveSelectionBackground"],
    "selectionHighlightColor": ["editor.selectionHighlightBackground"],
    "wordHighlight": ["editor.wordHighlightBackground"],
    "wordHighlightStrong": ["editor.wordHighlightStrongBackground"],
    "findMatchHighlight": ["editor.findMatchHighlightBackground", "peekViewResult.matchHighlightBackground"],
    "currentFindMatchHighlight": ["editor.findMatchBackground"],
    "findRangeHighlight": ["editor.findRangeHighlightBackground"],
    "referenceHighlight": ["peekViewEditor.matchHighlightBackground"],
    "lineHighlight": ["editor.lineHighlightBackground"],
    "rangeHighlight": ["editor.rangeHighlightBackground"],
    "caret": ["editorCursor.foreground"],
    "invisibles": ["editorWhitespace.foreground"],
    "guide": ["editorIndentGuide.background"],
    "ansiBlack": ["terminal.ansiBlack"], "ansiRed": ["terminal.ansiRed"], "ansiGreen": ["terminal.ansiGreen"], "ansiYellow": ["terminal.ansiYellow"],
    "ansiBlue": ["terminal.ansiBlue"], "ansiMagenta": ["terminal.ansiMagenta"], "ansiCyan": ["terminal.ansiCyan"], "ansiWhite": ["terminal.ansiWhite"],
    "ansiBrightBlack": ["terminal.ansiBrightBlack"], "ansiBrightRed": ["terminal.ansiBrightRed"], "ansiBrightGreen": ["terminal.ansiBrightGreen"],
    "ansiBrightYellow": ["terminal.ansiBrightYellow"], "ansiBrightBlue": ["terminal.ansiBrightBlue"], "ansiBrightMagenta": ["terminal.ansiBrightMagenta"],
    "ansiBrightCyan": ["terminal.ansiBrightCyan"], "ansiBrightWhite": ["terminal.ansiBrightWhite"]
};

function migrate(content, tmThemeFileName, generator) {
    try {
        let result = {};
        let theme = plistParser.parse(content).value;
        let settings = theme.settings;
        if (Array.isArray(settings)) {
            let colorMap = {};
            for (let entry of settings) {
                let scope = entry.scope;
                if (scope) {
                    let parts = scope.split(',').map(p => p.trim());
                    if (parts.length > 1) {
                        entry.scope = parts;
                    }
                } else {
                    var entrySettings = entry.settings;
                    let notSupported = [];
                    for (let entry in entrySettings) {
                        let mapping = mappings[entry];
                        if (mapping) {
                            for (let newKey of mapping) {
                                colorMap[newKey] = entrySettings[entry];
                            }
                            if (entry !== 'foreground' && entry !== 'background') {
                                delete entrySettings[entry];
                            }
                        } else {
                            notSupported.push(entry);
                        }
                    }
                    if (notSupported.length > 0) {
                        generator.log('Note: the following theming properties are not supported by VSCode and will be ignored: ' + notSupported.join(', '))
                    }
                }
            }
            if (!tmThemeFileName) {
                result.tokenColors = settings;
            } else {
                result.tokenColors = './' + tmThemeFileName;
            }
            result.colors = colorMap;
        }
        return result
    } catch (e) {
        console.log(e);
    }
};


exports.convertTheme = convertTheme;