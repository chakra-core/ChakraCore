//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var ary = Array(1);
ary.prop = "pass";
Object.prototype.prop = "Got object prototype : Failed";
Array.prototype.prop = "Got array prototype. Failed";
Object.defineProperty(Object.prototype, 0, {
  get: function () {
    print(this.prop);
    return 3;
  }
});
ary.slice();

