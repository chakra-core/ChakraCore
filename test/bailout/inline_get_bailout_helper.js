//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function Bar() {
  this.foo = 5;
}

Object.defineProperty(Bar.prototype, "data", {
  get: function () {
      return this.foo;
  },
  set: function (v) {
      this.foo = v;
  },
  configurable: true
});
