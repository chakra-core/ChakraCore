"use strict";

function func(a, b, c) {
    a[0] = 1.2;
    b[0] = c;
    a[1] = 2.2;
    a[0] = 2.3023e-320;
}

function main() {
    var a = [1.1, 2.2];
    var b = new Uint32Array(100);

    // force to optimize
    for (var i = 0; i < 0x10000; i++)
        func(a, b, i);

    func(a, b, {
        valueOf: function () {
            a[0] = {};

            return 0;
        }
    });

    a[0].toString();
}

main();

WScript.Echo('pass');