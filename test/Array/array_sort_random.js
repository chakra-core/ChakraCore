//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const alphabet = ["a", "b", "c", "d", "e", "f", "g", "h", "i",
                    "j", "k", "l", "m", "n", "o", "p", "q", "r",
                    "s", "t", "u", "v", "w", "x", "y", "z"];

function getRandomObjArray(size)
{
    const arr = [];
    for (let i = 0; i < size; ++i)
    {
        arr[i] = {
            value : Math.random() * 100 | 0,
            index : i
        }
    }
    return arr;
}

function getRandomStringArray(size)
{
    const arr = [];
    for (let i = 0; i < size; ++i)
    {
        let value = "";
        const len = Math.random() * 5 | 0;
        for (let j = 0; j < len; ++j)
        {
            value += alphabet[Math.random() * 26 |0];
        }
        arr[i] = value;
    }
    return arr;
}

function getRandomIntArray(size)
{
    const arr = [];
    for (let i = 0; i < size; ++i)
    {
        arr[i] = Math.random() * 100 | 0;
    }
    return arr;
}

function testRandomIntSort(size)
{
    const unsorted = getRandomIntArray(size);

    // Copy the array and sort it
    const sorted = unsorted.slice();
    sorted.sort(function (a, b){ return 0;});

    for (let i = 0; i < size; ++i)
    {
        if (sorted[i] !== unsorted[i])
        {
            print (`Unsorted: ${unsorted}`);
            print (`Sorted: ${sorted}`);
            throw new Error ("Sort not stable at index " + i);
        }
    }

    sorted.sort();

    for (let i = 1; i < size; ++i)
    {
        if ("" + sorted[i-1] > "" + sorted[i])
        {
            print (`Unsorted: ${unsorted}`);
            print (`Sorted: ${sorted}`);
            throw new Error ("Default sort has not sorted by strings at " + i);
        }
    }

    sorted.sort (function (a, b){ return a - b;});

    // Verify that the array is sorted
    for (let i = 1; i < size; ++i)
    {
        // Sort has not completed correctly
        if (sorted[i] < sorted[i - 1])
        {
            print (`Unsorted: ${unsorted}`);
            print (`Sorted: ${sorted}`);
            throw new Error(`Array is not sorted correctly at index '${i}'`);
        }
    }
}


function testRandomStringSort(size)
{
    const unsorted = getRandomStringArray(size);

    // Copy the array and sort it
    const sorted = unsorted.slice();
    sorted.sort(function (a, b){ return 0;});

    for (let i = 0; i < size; ++i)
    {
        if (sorted[i] !== unsorted[i])
        {
            print (`Unsorted: ${unsorted}`);
            print (`Sorted: ${sorted}`);
            throw new Error ("Sort not stable at index " + i);
        }
    }

    sorted.sort();

    for (let i = 1; i < size; ++i)
    {
        if (sorted[i-1] > sorted[i])
        {
            print (`Unsorted: ${unsorted}`);
            print (`Sorted: ${sorted}`);
            throw new Error ("Default sort has not sorted by strings");
        }
    }
}

function testRandomObjSort(size)
{
    const unsorted = getRandomObjArray(size);
    const sorted = unsorted.slice();
    // stable default sort on objects without custom toString should not re-order
    sorted.sort();
    for (let i = 0; i < size; ++i)
    {
        if (sorted[i].index !== i)
        {
            print (`Unsorted: ${JSON.stringify(unsorted)}`);
            print (`Sorted: ${JSON.stringify(sorted)}`);
            throw new Error ("Sort not stable at index " + i);
        }
    }

    sorted.sort(function (a, b) { return a.value - b.value; });

    for (let i = 1; i < size; ++i)
    {
        if (sorted[i-1].value > sorted[i].value || (sorted[i-1] === sorted[i] && sorted[i-1].index > sorted[i].index))
        {
            print (`Unsorted: ${JSON.stringify(unsorted)}`);
            print (`Sorted: ${JSON.stringify(sorted)}`);
            throw new Error ("Numeric sort of objects has failed at " + i);
        }
    }
}


function stressTestSort(iterations, size = 1000)
{
    for (let i = 0; i < iterations; ++i)
    {
        testRandomIntSort(size);
        testRandomStringSort(size);
        testRandomObjSort(size);
    }
}

// test arrays with length < 2048 for insertion sort
stressTestSort(200, 512);
// test arrays with length > 2048 for merge sort
stressTestSort(200, 2050);

WScript.Echo("PASS");