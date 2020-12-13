//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function f0(a, start, end) {
    for (var i = start, j = start; i < end; i++, j++) {
        j = j + 1
        a[i] = 1
    }
    a[j]
    print(j)

}

function f1(a, start, end) {
    for (var i = start, j = start; i < end; i++, j++) {
        j = j + 2
        a[i] = 1
    }
    a[j]
    print(j)

}

function f2(a, start, end) {
    for (var i = start, j = start; i >= end; i--, j--) {
        j = j - 1
        a[i] = 1
    }
    a[j]
    print(j)
}

function f3(a, start, end) {
    for (var i = start, j = start; i >= end; i--, j--) {
        j = j - 2
        a[i] = 1
    }
    a[j]
    print(j)
}

function f4(a, start, end) {
    for (var i = start, j = start; i < end; i++, j--) {
        j = j + 1
        a[i] = 1
    }
    a[j]
    print(j)
}

function f5(a, start, end) {
    for (var i = start, j = start; i < end; i++, j--) {
        j = j + 2
        a[i] = 1
    }
    a[j]
    print(j)
}

var a = new Float64Array(0xfff);
f0(a, 0, 100)
f0(a, 0x7fffffff, 0x80000000)
f0(a, 10, 20)

f1(a, 0, 100)
f1(a, 0x7fffffff, 0x80000000)
f1(a, 10, 20)

f2(a, 0, 100)
f2(a, 0x7fffffff, 0x80000000)
f2(a, 10, 20)

f3(a, 0, 100)
f3(a, 0x7fffffff, 0x80000000)
f3(a, 10, 20)

f4(a, 0, 100)
f4(a, 0x7fffffff, 0x80000000)
f4(a, 10, 20)

f5(a, 0, 100)
f5(a, 0x7fffffff, 0x80000000)
f5(a, 10, 20)
