//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var obj = {
    mSloppy() {
        try {
            super[
                __v_12000] = 55;
        } catch (__v_12004) {
        }
        try {
        } catch (__v_12005) {
        }
    }
};
// There shouldn't be any SyntaxError
print("PASSED");