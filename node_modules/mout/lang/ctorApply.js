

    var bind = Function.prototype.bind;

    /**
     * Do fn.apply on a constructor.
     */
    function ctorApply(ctor, args) {
        var Bound = bind.bind(ctor, undefined).apply(undefined, args);
        return new Bound();
    }

    module.exports = ctorApply;


