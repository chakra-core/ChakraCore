//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Destructuring debug cases

(function () {
    var {x} = {x:1};
    var [y] = [2];
    x; /**bp:evaluate('x == 1 && y == 2')**/
    ({x} = {x:3});
    [y] = [4];
    y; /**bp:evaluate('x == 3 && y == 4')**/
    ({x, y:[y]} = {x:5, y:[6]});
    y; /**bp:evaluate('x == 5 && y == 6')**/
})();

(function () {
    let [i,j] = [0,0];
    function getName() {
        if (i++ == 0) return 'x';
        else return 'y'
    }
    function getData() {
        if (j++ == 0) return 'this is x';
        else return 'this is y';
    }
    let  {[getName()]:x1, [getName()]:y1} = {x:getData(), y:getData()};
    x1; /**bp:evaluate("x1 == 'this is x' && y1 == 'this is y'")**/
})();

(function () {
    {
        (function ({x}, [y], z) {
            x; /**bp:evaluate('x == 1 && y == 2 && z == 3')**/
        })({x:1}, [2], 3);
    }
    {
        (function (x, [y, {z}]) {
            x; /**bp:evaluate('x == 4 && y == 5 && z == 6')**/
        })(4, [5, {z:6}]);
    }
    {
        (function (x, [y = 10, {z:z = 11}]) {
            x; /**bp:evaluate('x == 1 && y == 10 && z == 11')**/
        })(1, [, {}]);
    }
    {
        (function ([x, ...y]) {
            x; /**bp:evaluate('x == 5 && y[0] == 6 && y[1] == 7 && y[2] == 8')**/
        })([5, 6, 7, 8]);
    }
    
})();

(function () {
    {
    
        let i = 0, data = [20, 30];
        for ( let {x:item} of [{x:20}, {x:30}]) {
            item; /**bp:evaluate('item == data[i++]')**/
        }
        
        function data2() {
            return {x:[{y:[20]}, {y:[30]}]};
        }
        
        i = 0;
        for ({y:[item]} of data2().x) {
            item; /**bp:evaluate('item == data[i++]')**/
        }
    }
})();

(function () {
    try {
        throw {x:10, z:['this is z']};
    }
    catch({x, y, z:[z]}) {
        x; /**bp:evaluate('x == 10 && y == undefined && z == "this is z"')**/
    }
})();

(function () {
    try {
        throw {x:10, z:['this is z']};
    }
    catch({x, y, z:[z]}) {
        (function () {
            x; /**bp:evaluate('x == 10')**/
       })();
       x; /**bp:evaluate('x == 10 && y == undefined && z == "this is z"')**/
    }
})();

(function () {
    try {
        throw {x:10, z:['this is z']};
    }
    catch({x, y = 10, z:[z]}) {
        (function () {
            eval('')
       })();
       x; /**bp:evaluate('x == 10 && y == 10 && z == "this is z"')**/
    }
})();

// stepping

(function () {
    var {x, y:[y]} = {x:1, y:[]}, {x1, y1:[y1]} = {x1:1, y1:[]}; /**bp:dumpBreak();resume('step_into');dumpBreak();resume('step_into');dumpBreak();**/
})();

(function () {
    function foo( {x}, {y}) {
    }
    foo({}, []); /**bp:resume('step_into');dumpBreak();resume('step_into');dumpBreak();**/
})();


(function () {
    function foo( {x} = {x:10}, a, [y = 1] = [2]) {
    }
    foo({}, []); /**bp:resume('step_into');dumpBreak();resume('step_into');dumpBreak();resume('step_into');dumpBreak();**/
})();


(function () {
    try {
        throw {x:10, z:['this is z']};
    }
    catch({x, y = 20, z:[z]}) { /**bp:dumpBreak();resume('step_over');dumpBreak();**/
        x;
    }
})();

(function () {
    var i = 0; /**bp:resume('step_over');dumpBreak();resume('step_over');dumpBreak();resume('step_over');dumpBreak();resume('step_over');dumpBreak();**/
    for (var {item} of [{item:1}]) {
        item;
    }
})();

// editing
(function () {
    function foo( {x = 10} , a, [y = 1] = [2]) {
    }
    foo({}); /**bp:resume('step_into');dumpBreak();evaluate("x");resume('step_into');dumpBreak();evaluate('x == 10');evaluate('x = 20');evaluate('y = 12');evaluate('y');resume('step_into');dumpBreak();evaluate('x==20');evaluate('y == 2')**/
})();


(function () {
    try {
        throw {x:10, z:['this is z']};
    }
    catch({x, y, z:[z]}) /**bp:evaluate('x = 20');resume('step_into');evaluate('x ');**/
    {
        x; 
    }
})();

// Setnext statement
(function () {
    function foo( 
    {x = 10},  /**bp:setnext('thirdparam');**/
    a = 2,
    [y = 1] = [2] ) /**loc(thirdparam)**/
    {
        var m = x + y;
        return m; /**bp:evaluate('m == 12');evaluate('a == 2');**/
    }
    
    foo({})
})();

(function () {
    function foo( 
    {x = 10},  /**loc(firstparam)**/
    a,
    [y = 1] = [2] ) /**bp:setnext('firstparam');removeExpr();**/
    {
        var m = x + y;
        return m; /**bp:evaluate('m == 12');**/
    }
    
    foo({})
})();

(function () {
    function foo( 
    {x = 10},  /**bp:setnext('body');**/
    a,
    [y = 1] = [2] ) /**bp:setnext('body');**/
    {
        var m = x + y; /**loc(body)**/
        return m; /**bp:evaluate('m == 12');**/
    }
    
    foo({})
})();


(function () {
    function foo( 
    {x = 10},  /**loc(first)**/
    a,
    [y = 1] = [2] ) 
    {
        var m = x + y; /**bp:setnext('first');removeExpr();**/
        return m; /**bp:evaluate('m == 12');**/
    }
    
    foo({})
})();

print('pass');