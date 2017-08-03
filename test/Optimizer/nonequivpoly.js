//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var protoObj1 = {
method0: function () {
}};
var v3 = {
v4: function () {
  return function bar() {
    this.method0.apply({});
  };
}
};
protoObj1.v6 = v3.v4();
protoObj1.v6.prototype = {
method0: function () {
}
};
protoObj1.v48 = v3.v4();
protoObj1.v48.prototype = {
method0: function () {
  new protoObj1.v6();
}
};
var v67 = new protoObj1.v48();
var v68 = protoObj1.v48();

print("Pass");