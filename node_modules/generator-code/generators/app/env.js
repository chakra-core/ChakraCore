/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
'use strict';
var request = require('request');

var fallbackVersion = '^1.29.0';
var promise = new Promise(function(resolve, reject) {
    request.get('https://vscode-update.azurewebsites.net/api/releases/stable', { headers: { "X-API-Version": "2" } }, function(error, response, body) {
        if (!error && response.statusCode === 200) {
            try {
                var tagsAndCommits = JSON.parse(body);
                if (Array.isArray(tagsAndCommits) && tagsAndCommits.length > 0) {
                    var segments = tagsAndCommits[0].version.split('.');
                    if (segments.length === 3) {
                        resolve('^' + segments[0] + '.' + segments[1] + '.0');
                        return;
                    }
                }
            } catch (e) {
                console.log('Problem parsing version: ' + body, e);
            }
        } else {
            console.log('Unable to fetch latest vscode version: ' + (error || ('Status code: ' + response.statusCode + ', ' + body)));
        }
        resolve(fallbackVersion);
    });
});

module.exports.getLatestVSCodeVersion = function() { return promise; };