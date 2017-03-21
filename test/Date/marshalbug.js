//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const crossSiteDate =
    WScript.LoadScript("var x = new Date()", "samethread").x;

// Run every Date method on the cross-site instance
Object.getOwnPropertyNames(Date.prototype)
    .filter(name => !name.match(/^set/))
    .forEach(name => {
        print(name);
        try {
            print(Date.prototype[name].call(crossSiteDate));
        } catch(e) {
            // Ignore. Just to catch assertions on debug build.
        }
    });
