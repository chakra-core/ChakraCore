//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Default debugger cases
// We are validating that 
// 1. halting and stepping at formals
// 2. variables at body not visible at formals
// 2. excercising in multiple storage container, regslot, slotarray or actiation object 

var glob = {}; // This is just a dummy variable for log purpose

glob; /**bp:logJson("Stepping on default params - formals on regslot")**/
(function() {
    function f1(a = 1, b, c = 2) {
        var m = 3;
    }
    f1(); /**bp:resume('step_into');locals();dumpBreak();resume('step_into');locals();dumpBreak();evaluate('a = 11');evaluate('a')**/
})();

glob; /**bp:logJson("Stepping on default params - formals on regslot and slot array")**/
(function() {
    function f1(a = 1, b, c = 2) {
        var m = 3;
        function bar() {
            a;
            m;
        }
    }
    f1(); /**bp:resume('step_into');locals();dumpBreak();resume('step_into');locals();dumpBreak();evaluate('a = 11');evaluate('a')**/
})();

glob; /**bp:logJson("Stepping on default params - formals on activation object")**/
(function() {
    function f1(a = 1, b, c = 2) {
        var m = 3;
        eval('');
    }
    f1(); /**bp:resume('step_into');locals();dumpBreak();resume('step_into');locals();dumpBreak();evaluate('a = 11');evaluate('a')**/
})();

glob; /**bp:logJson("Stepping on default params - has mixed destructuring patterns")**/
(function() {
    function f1({a} = {a:1}, b, [c] = [2]) {
        var m = 3;
    }
    f1(); /**bp:resume('step_into');locals();dumpBreak();resume('step_into');locals();dumpBreak();**/
})();

glob; /**bp:logJson("Stepping on default params - one formal has function as default")**/
(function() {
    function f1(a = 1, b = function () { return 2; }, c = 3) {
        var m = 3;
    }
    f1(); /**bp:resume('step_into');resume('step_into');resume('step_into');locals();dumpBreak();**/
})();

glob; /**bp:logJson("Stepping on default params - one formal has function as default but formals on slot array")**/
(function() {
    function f1(a = 1, b = function () { return 2; }, c = 3) {
        function bar() {
            a;b;c;
        }
    }
    f1(); /**bp:resume('step_into');resume('step_into');resume('step_into');locals();dumpBreak();**/
})();

glob; /**bp:logJson("Stepping on default params - one formal has function as default but formals on activation object")**/
(function() {
    function f1(a = 1, b = function () { return 2; }, c = 3) {
        function bar() {
            a;b;c;
        }
        eval('');
    }
    f1(); /**bp:resume('step_into');resume('step_into');resume('step_into');locals();dumpBreak();**/
})();

glob; /**bp:logJson("Stepping on default params - break at function at formals and do setframe")**/
(function() {
    function f1(a = 1, b = function () { 
        return 2; /**bp:setFrame(1);locals()**/  
    }, c = b()) {
        var m = 3;
    }
    f1();
})();

glob; /**bp:logJson("Stepping on default params - split scope due to function at formals")**/
(function() {
    function f1(a = 1, b = function () { return a; }, c = 3) {
        var m = 3;
    }
    f1(); /**bp:resume('step_into');locals();dumpBreak();resume('step_into');resume('step_into');locals();dumpBreak();resume('step_into');locals();dumpBreak();**/
})();

glob; /**bp:logJson("Stepping on default params - Breakpoint at the function defined in param scope when called from param scope")**/
(function() {
    function f1(a = 1, b = function () { 
        return a; /**bp:setFrame(1);locals()**/  
    }, c = b()) {
        var m = 3;
        m; /**bp:locals()**/
    }
    f1();
})();

glob; /**bp:logJson("Stepping on default params - Breakpoint at the function defined in param scope when called from body scope")**/
(function() {
    function f1(a = 1, b = function () { 
        return a; /**bp:setFrame(1);locals()**/  
    }) {
        var m = 3;
        var c = b();
    }
    f1();
})();

glob; /**bp:logJson("Stepping on default params - Breakpoint at the function defined in param scope to print the default frame")**/
(function() {
    function f1(a = 1, b = function () { 
        return a; /**bp:locals()**/  
    }) {
        var m = 3;
        var c = b();
    }
    f1();
})();

glob; /**bp:logJson("Stepping on default params - Split scope function with zero reg slots in param")**/
(function() {
    function f1(a = 1, b = function () { 
        return a + c(); /**bp:setFrame(1);locals()**/  
    }, c = () => {
        return b.length + a; /**bp:setFrame(1);locals()**/
    }) {
        var m = 3; /**bp:locals()**/
        var n = c();
        var o = b();
        o; /**bp:locals()**/
    }
    f1();
})();

glob; /**bp:logJson("Stepping on default params - Split scope function with zero reg slots in param and eval in both scopes")**/
(function() {
    function f1(a = 1, b = function () { 
        return a + c(); /**bp:setFrame(1);locals()**/  
    }, c = () => {
        return eval("b.length + a"); /**bp:setFrame(1);locals()**/
    }) {
        var m = 3; /**bp:locals()**/
        var n = c();
        var o = eval("b()");
        o; /**bp:locals()**/
    }
    f1();
})();

glob; /**bp:logJson("Stepping on default params - Split scope function with zero reg slots in param and print the body scope")**/
(function() {
    function f1(a = 1, b = function () { 
        return a + c();
    }, c = () => {
        return eval("b.length + a"); /**bp:stack();setFrame(3);locals()**/
    }) {
        var m = eval("b()");
    }
    f1(10);
})();

glob; /**bp:logJson("Stepping on default params - split scope due to function at formals and few var captured in inner function")**/
(function() {
    function f1(a = 1, b = function () { return a; }, c) {
        var m = 3;
        (function () {
            a;
            m;
        })();
    }
    f1(); /**bp:resume('step_into');locals();dumpBreak();resume('step_into');resume('step_into');locals();dumpBreak();**/
})();

glob; /**bp:logJson("Stepping on default params - split scope and body has eval")**/
(function() {
    function f1(a = 1, b = function () { return a; }, c) {
        var m = 3;
        (function () {
        })();
        eval('');
    }
    f1(); /**bp:resume('step_into');locals();dumpBreak();resume('step_into');resume('step_into');locals();dumpBreak();**/
})();

glob; /**bp:logJson("Stepping on default params - split scope and body has eval inside inner function")**/
(function() {
    function f1(a = 1, b = function () { return a; }, c) {
        var m = 3;
        (function () {
            eval('');
        })();
    }
    f1(); /**bp:resume('step_into');locals();dumpBreak();resume('step_into');resume('step_into');locals();dumpBreak();**/
})();

glob; /**bp:logJson("Default params - Split scope with eval in param")**/
(function() {
    function f1(a = 1, b = function () { 
        return eval("a"); /**bp:setFrame(1);locals()**/  
    }, c = b()) {
        var m = b();
        m; /**bp:locals()**/
    }
    f1();
})();

glob; /**bp:logJson("Default params - Arguments usage in the param scope")**/
(function() {
    function f1(a = 10,
                    b = arguments /**bp:locals(1)**/) {
        var c = b; /**bp:locals(1)**/
    }
    f1(1);
})();

glob; /**bp:logJson("Default params - Arguments usage in the param scope of a split scoped function")**/
(function() {
    function f1(a = 10, b = arguments,
                                    c = () => a /**bp:locals(1)**/) {
        var d = b; /**bp:locals(1)**/
    }
    f1(1);
})();

glob; /**bp:logJson("Default params - Arguments usage in the body")**/
(function() {
    function f1(a = 10,
                    b = 11/**bp:locals(1)**/) {
        var c = arguments; /**bp:locals(1)**/
    }
    f1(1);
})();

glob; /**bp:logJson("Default params - Arguments usage in the body of a split scoped function")**/
(function() {
    function f1(a = 10,
                    b = () => a/**bp:locals(1)**/) {
        var c = arguments; /**bp:locals(1)**/
    }
    f1(1);
})();

glob; /**bp:logJson("Default params - Arguments usage in the body when one of the formal is in slot")**/
(function() {
    function f1(a = 10, b = 11,
                            c = arguments /**bp:locals(1)**/) {
        function arguments() {
            return c;
        }
        var d = c;
        d; /**bp:locals(1)**/
    }
    f1(1);
})();

glob; /**bp:logJson("Default params - Arguments symbol is overwritten")**/
(function() {
    function f1( a = 10,
                    b = arguments /**bp:locals(1)**/) {
        function arguments() {
            return 100;
        }
        var c = b;
        c; /**bp:locals(1)**/
    }
    f1(1);
})();

glob; /**bp:logJson("Default params - Arguments symbol is overwritten in a split scoped function")**/
(function() {
    function f1( a = 10, b = arguments,
                                c = ()=> a /**bp:locals(1);evaluate('arguments', 1);**/) {
        function arguments() {
            return 100;
        }
        var d = b;
        b; /**bp:locals(1);evaluate('arguments', 1);**/
    }
    f1(1);
})();

glob; /**bp:logJson("Default params - One of the formal is duplicated in the body")**/
(function() {
    function f1(a = 10, b = () => a,
                            c = 11 /**bp:locals(1)**/) {
        var d = a; /**bp:locals(1)**/
        var c = () => b;
        a; /**bp:locals(1)**/ 
    }
    f1(1);
})();

glob; /**bp:logJson("Default params - One of the formal is duplicated in the body by a function definition")**/
(function() {
    function f1(a = 10, b = () => a,
                            c = 11 /**bp:locals(1)**/) {
        var d = a; /**bp:locals(1)**/
        function c() {
            return b;
        };
        a; /**bp:locals(1)**/ 
    }
    f1(1);
})();

glob; /**bp:logJson("Default params - Evaluate one of the formal which is in regslot is duplicated in the body")**/
(function() {
    function f1(a = 10, b = () => a, c = 11 /**bp:evaluate('c');**/) {
        function c() {
            return 100;
        }
        var d = a;
        a; /**bp:evaluate('c', 1);**/
    }
    f1(1);
})();

print("Pass");
