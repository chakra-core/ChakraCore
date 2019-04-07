/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
var nameRegex = /^[a-z0-9][a-z0-9\-]*$/i;

module.exports.validatePublisher = function(publisher) {
    if (!publisher) {
        return "Missing publisher name";
    }

    if (!nameRegex.test(publisher)) {
        return "Invalid publisher name";
    }

    return true;
}

module.exports.validateExtensionId = function(id) {
    if (!id) {
        return "Missing extension identifier";
    }

    if (!nameRegex.test(id)) {
        return "Invalid extension identifier";
    }

    return true;
}

module.exports.validateNonEmpty = function(name) {
    return name && name.length > 0;
}