//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function forInKeysToArray(obj) {
    var s = [];
    
    for (key in obj) { 
        s.push(key);
    }
    
    return s;
}

var proto1 = { x: 1 };
var child1 = Object.create(proto1, { x: { value: 2, enumerable: false} });

var proto2 = { a: 1, b: 2, c: 3, d: 4, e: 5 };
var child2 = Object.create(proto2, { b: { value: 20, enumerable: false} });
Object.defineProperty(child2, 'c', { enumerable: false, value: 30 });
child2['d'] = 4;

var o1 = [0,1,2];
o1[4] = 4;
Object.defineProperty(o1, 3, { enumerable: false, value: '3' })

var proto3 = Object.create(null, { x: { value: 1, enumerable: false} });
var child3 = Object.create(proto3, { x: { value: 2, enumerable: true} });

var proto4 = Object.create(null, { x: { value: 1, enumerable: false} });
var child4 = Object.create(proto4, { x: { value: 2, enumerable: false} });

var proto5 = Object.create(null, { 
    a: { value: 1, enumerable: false},
    b: { value: 1, enumerable: true},
    c: { value: 1, enumerable: false},
    d: { value: 1, enumerable: false},
    w: { value: 1, enumerable: true},
    x: { value: 1, enumerable: false},
    y: { value: 1, enumerable: true},
    z: { value: 1, enumerable: true},
    });
var child5 = Object.create(proto5, { 
    a: { value: 2, enumerable: false},
    b: { value: 2, enumerable: false},
    c: { value: 2, enumerable: true},
    d: { value: 2, enumerable: false},
    w: { value: 2, enumerable: true},
    x: { value: 2, enumerable: true},
    y: { value: 2, enumerable: false},
    z: { value: 2, enumerable: true},
    });
var childchild5 = Object.create(child5, { 
    a: { value: 3, enumerable: false},
    b: { value: 3, enumerable: false},
    c: { value: 3, enumerable: false},
    d: { value: 3, enumerable: true},
    w: { value: 3, enumerable: false},
    x: { value: 3, enumerable: true},
    y: { value: 3, enumerable: true},
    z: { value: 3, enumerable: true},
    });


WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog(`${JSON.stringify(forInKeysToArray(child1))}`, true); //[]
    telemetryLog(`${JSON.stringify(forInKeysToArray(child2))}`, true); //['d','a','e']
    telemetryLog(`${JSON.stringify(forInKeysToArray(o1))}`, true); //['0','1','2','4']
    telemetryLog(`${JSON.stringify(forInKeysToArray(child3))}`, true); //['x']
    telemetryLog(`${JSON.stringify(forInKeysToArray(child4))}`, true); //[]

    telemetryLog(`${JSON.stringify(forInKeysToArray(childchild5))}`, true); //['d','x','y','z']
    telemetryLog(`${JSON.stringify(forInKeysToArray(child5))}`, true); //['c','w','x','z']
    telemetryLog(`${JSON.stringify(forInKeysToArray(proto5))}`, true); //['b','w','y','z']

    emitTTDLog(ttdLogURI);
}
