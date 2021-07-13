//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

R"====(
var $262 = {
    createRealm: () => WScript.LoadScript('', 'samethread').$262,
    global: this,
    agent: {
        start(src) {
            WScript.LoadScript(`
                $262 = {
                    agent: {
                        receiveBroadcast: WScript.ReceiveBroadcast,
                        report: WScript.Report,
                        leaving: WScript.Leaving,
                        monotonicNow: WScript.monotonicNow
                    }
                };
                ${ src }
            `, 'crossthread');
        },
        broadcast: WScript.Broadcast,
        sleep: WScript.Sleep,
        getReport: WScript.GetReport,
        monotonicNow: WScript.monotonicNow
    }
};
)===="
