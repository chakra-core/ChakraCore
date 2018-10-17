//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// https://github.com/Microsoft/ChakraCore/issues/5492

const blah = {"\0":"hi","\0\0":"hello"};
for (var i in new Proxy(blah, {})) console.log(blah[i]);
