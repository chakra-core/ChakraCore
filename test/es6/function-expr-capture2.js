eval(
    '(function f() {' +
    '     with({}) {' +
    '         (function () {' +
    '             return f;' +
    '         })();' +
    '     }' +
    ' }());'
);

WScript.Echo('pass');
