//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// OS 13988805: Assert Failure in JS:ParseableFunctionInfo

try {
    f()
} catch (
    e
) {}

function f() {
    "use asm";
    Number.prototype.toString.apply(
        DataView.prototype.getUint16.apply(
            Object.freeze(
                URIError.prototype.name.apply(
                    RegExp.prototype.global.apply(
                        (
                            function() {
                                return (
                                    (
                                        new Intl.DateTimeFormat(
                                            (
                                                function(
                                                    [] =
                                                    (
                                                        Date.prototype.setSeconds.apply(
                                                            (
                                                                i6 = (
                                                                    i1 =>
                                                                    String.prototype.sub.apply()
                                                                )
                                                            )
                                                        )
                                                    )
                                                ) {}
                                            )
                                        )
                                    )
                                );
                            }
                        )
                    )
                )
            )
        )
    )
}

WScript.Echo("PASS");