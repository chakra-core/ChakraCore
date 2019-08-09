//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// force:deferparse

(function v2(a = function v2(){ +v2; }) {
    a();
    console.log('pass');
})();

(function v2(a = function v3(){ function v4(b = (function v4() {v4; console.log('pass');})()){}; v4(); }) {
    a();
    console.log('pass');
})();

(function a() {
    a = function a(a=function(a){}){}
    function a(){
        var a = "a";
    }
    console.log('pass');
})();
