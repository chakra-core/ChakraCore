(function func(arg = function () {
                         return func;
                     }()) {
    return func;
    function func() {}
})();

WScript.Echo('pass');
