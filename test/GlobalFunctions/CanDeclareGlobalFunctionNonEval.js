//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// #sec-candeclareglobalfunction states that redeclaration of global functions
// that are not configurable, not writable, and not enumerable should fail.
// #sec-globaldeclarationinstantiation states that if #sec-candeclareglobalfunction
// fails, a TypeError should be thrown. The global property "undefined" is not
// configurable, not writable, and not enumerable. More tests can be found in
// CanDeclareGlobalFunction.js

function undefined() { }