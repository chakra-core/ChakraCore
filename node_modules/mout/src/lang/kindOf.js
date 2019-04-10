define(function () {
    /**
     * Gets the "kind" of value. (e.g. "String", "Number", etc)
     */
    function kindOf(val) {
        return Object.prototype.toString.call(val).slice(8, -1);
    }
    return kindOf;
});
