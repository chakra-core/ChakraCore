//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/**exception(uncaught):stack();**/

function unhandledPromiseRejection1() {
    Promise.resolve(true)
        .then(() => {
            throw new Error('error for unhandledPromiseRejection1')
        });
}
unhandledPromiseRejection1();

function unhandledPromiseRejection2() {
    Promise.resolve(true)
        .then(() => {
            throw new Error('error for unhandledPromiseRejection2');
        })
        .then(() => {
            // no catch
        });
}
unhandledPromiseRejection2();

function unhandledPromiseRejection3() {
    let p = Promise.resolve(true)
        .then(() => {
            throw new Error('error for unhandledPromiseRejection3');
        })
        .then(() => 0);
    p.then(() => 0).then(() => 1); // this path is not caught
    p.then(() => 2, (err) => { }); // this path is caught

}
unhandledPromiseRejection3();

function unhandledPromiseRejection4() {
    let p = Promise.resolve(true)
        .then(() => {
            throw new Error('error for unhandledPromiseRejection3');
        })
        .catch((err) => {
            throw err;
        });
}
unhandledPromiseRejection4();

function handledPromiseRejection5() {
    Promise.resolve(true)
        .then(() => {
            throw new Error('error for handledPromiseRejection5')
        }).catch(() => { });
}
handledPromiseRejection5();

function handledPromiseRejection6() {
    Promise.resolve(true)
        .then(() => {
            throw new Error('error for handledPromiseRejection6');
        })
        .then(() => { }, () => { });
}
handledPromiseRejection6()

function handledPromiseRejection7() {
    let p = Promise.resolve(true)
        .then(() => {
            throw new Error('error for handledPromiseRejection7');
        })
        .then(() => 0);
    p.then(() => 0).then(() => 1).catch(() => { }); // this path is  caught
    p.then(() => 2, (err) => { }); // this path is caught

}
handledPromiseRejection7();

//
//  validate that when we have a handler from one script context 
// and a promise from another script context, we'll break appropriately
//
function unhandledPromiseRejectionCrossContext() { 
    var external = WScript.LoadScriptFile("JsDiagExceptionsInPromises_BreakOnFirstChanceExceptions.crosscontext.js", "samethread");
    let p = Promise.prototype.then.call(
        external.externalContextPromise.promise, () => { 
    });
    external.externalContextPromise.resolvePromise();
}
unhandledPromiseRejectionCrossContext();

//
//  validate that when we have a handler from one script context 
// and a promise from another script context, we'll not break if a rejection handler is available
//
function handledPromiseRejectionCrossContext() { 
    var external = WScript.LoadScriptFile("JsDiagExceptionsInPromises_BreakOnFirstChanceExceptions.crosscontext.js", "samethread");
    let p = Promise.prototype.then.call(
        external.externalContextPromise.promise, () => {}, () => {}); 
    external.externalContextPromise.resolvePromise();
}
handledPromiseRejectionCrossContext();

// 
// This one below is an edge case where we will break on uncaught exceptions
// even though the rejection is handled.  What's happening here is before
// we execute the function onResolve, there is no handler attached
// so we, then as part of executing onResolve, the catch handler is  
// attached.  We'll break in the function below.
//
// I don't think it is worth fixing this since it seems like a relatively
// rare case. 
//
function handledPromiseRejection8_bugbug() {
    var p = Promise.resolve(0).then(function onResolve() {
        p.catch(() => { }); // lazily added catch on the currently executing promise
        throw new Error('error for handledPromiseRejection8_bugbug');
    });
}
handledPromiseRejection8_bugbug();

// 
// In the case below, we're resolving a promise with a promise. 
// Ultimately, the rejections is handled, but according to 
// ES standard, the resolve of promiseA with promiseB gets
// pushed on the task queue, therefore, at the time the exception
// is raised, promiseB hasn't been "then'd".
//
// There are two ways to address this:
//   1.  Change the ResolveThenable task to run immediately vs runing in task queue (this would be in violation of the spec) 
//   2.  Keep a list of the pending resolve-thenable tasks.
//
function handledPromiseRejection9_bugbug() {
    function f1() {
        let promiseA =  new Promise((resolveA, rejectA) => {
            let promiseB = Promise.resolve(true).then(() => {
                throw new Error('error for handledPromiseRejection9_bugbug');
            });
            resolveA(promiseB);
        });
        return promiseA;
    }
        
    f1().catch((e) => {
    });
}
handledPromiseRejection9_bugbug();


function noRejection10() {
    let p = Promise.resolve(true)
        .then(() => {
            try {
                throw new Error('error for noRejection10');
            } catch (err) {
            }
        });
}
noRejection10();
