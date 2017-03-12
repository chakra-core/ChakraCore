//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Triggering invertloop codepath and ensuring the copying of nodes happens correctly.

function foo() {
	for (var a = 0; a < 1; ++a) {
		for (var b = 0; b < 11; ++b) {
			(true());
		}
	};

};

try {
    foo();
} catch(e) {
    print(e.message === 'Function expected' ? 'pass' : 'fail');
}
