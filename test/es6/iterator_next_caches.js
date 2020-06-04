//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const iter = {
	[Symbol.iterator]() {
		return {
			i : 0,
			next() {
				this.next = function () { throw new Error("next should be cached so this should not be called")}
				return {
					value : this.i++,
					done : this.i > 3
				}
			}
		}
	}
}


var tests = [
    {
		name : "for...of cache's next method",
		body : function () {
			let i = 0;
			for (const bar of iter)
			{
				++i;
			}
			assert.areEqual(3, i, "Loop should run 3 times");
		}
	},
	{
		name : "yield* cache's next method",
		body : function () {
			function* genFun() { yield * iter; }
			const gen = genFun();
			assert.areEqual(0, gen.next().value);
			assert.areEqual(1, gen.next().value);
			assert.areEqual(2, gen.next().value);
		}
	},
	{
		name : "Spread operator cache's next method",
		body : function () {
			assert.areEqual([0, 1, 2], [...iter], )
		}
	},
	{
		name : "Destructuring cache's next method",
		body : function () {
			const [a, b, c] = iter;
			assert.areEqual(0, a);
			assert.areEqual(1, b);
			assert.areEqual(2, c);
		}
	}
];

testRunner.runTests(tests, {
	verbose : WScript.Arguments[0] != "summary"
});
