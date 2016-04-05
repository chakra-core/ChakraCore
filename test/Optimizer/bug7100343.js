//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo(i1)
{
  var d2 = 16777217;
  return (!i1 >>> ((d2 % ~~295147905179352830000) + 4268133759)) | 0;  
}
print(foo());
print(foo());
print(foo());