//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function getRandomArray(size)
{
    const arr = [];

    for (let i = 0; i < size; ++i)
    {
        arr[i] = Math.random() * 100 | 0;
    }

    return arr;
}

function testRandomSort(size)
{
    const unsorted = getRandomArray(size);

    // Copy the array and sort it
    const sorted = unsorted.slice();
    sorted.sort(function (a, b){ return a - b;});

    // Verify that the array is sorted
    for (let i = 1; i < size; ++i)
    {
        // Sort has not completed correctly
        if (sorted[i] < sorted[i - 1])
        {
            WScript.Echo(`Unsorted: ${unsorted}`);
            WScript.Echo(`Sorted: ${sorted}`);
            throw new Error(`Array is not sorted correctly at index '${i}'`);
        }
    }
}

function stressTestSort(iterations, size = 1000)
{
    for (let i = 0; i < iterations; ++i)
    {
        testRandomSort(size);
    }
}

// Test 1000 random arrays of 1000 elements, print out the failures.
stressTestSort(1000, 1000);

WScript.Echo("PASS");
