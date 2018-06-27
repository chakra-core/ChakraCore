//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/**exception(firstchance):stack();**/


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

function handledPromiseRejection8() {
    var p = Promise.resolve(0).then(() => {
        p.catch(() => { }); // lazily added catch on the currently executing promise
        throw new Error('error for handledPromiseRejection8');
    });
}
handledPromiseRejection8();

function noRejection9() {
    let p = Promise.resolve(true)
        .then(() => {
            try {
                throw new Error('error for noRejection9');
            } catch (err) {
            }
        });
}
noRejection9();
