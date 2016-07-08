//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
function Shape() {
  this.color = "white"; 
}

function Circle() {
  this.diameter = 1;
}

Circle.prototype = new Shape();
