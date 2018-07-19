//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let shouldBailout = false;
let desc = {
    get: function () {
        return 3;
    }
};

let arr = [42, 123];
function test0() {
    for (let a in arr) {
        arr[a] =
            (
                (shouldBailout
                    ? this.p = ((() => 1)()) // should attempt to assign 1 to this.p (which will fail, if this.p has been defined because there is no setter)
                    : this.p)
                !== this.p // checks that the result of `get this.p` is not the same as `set this.p` (which is true if this.p has been defined with a getter and no setter)
            ) ? 8000 : 700
                +
                (
                    shouldBailout
                        ? (Object.defineProperty(this, 'p', desc), 60)
                        : 5
                );
    }
    if (shouldBailout && arr[1] !== 8000) {
        throw new Error("Unexpected result");
    }
}

// this sequence necessary
test0();
shouldBailout = true;
test0();

console.log("pass");
