//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
this.WScript.LoadScriptFile("..\\UnitTestFramework\\SimdJsHelpers.js");
function asmModule(stdlib, imports, buffer) {
    "use asm";
    
    var ui4 = stdlib.SIMD.Uint32x4;
    var ui4check = ui4.check;
    var ui4add = ui4.add;
    var ui4load  = ui4.load;  
    var ui4store  = ui4.store
    
    var ui4load1 = ui4.load1;
    var ui4load2 = ui4.load2;
    var ui4load3 = ui4.load3;

    var ui4store1 = ui4.store1;
    var ui4store2 = ui4.store2;
    var ui4store3 = ui4.store3;

    var globImportui4 = ui4check(imports.g1); 
    var g2 = ui4(1065353216, 1073741824, 1077936128, 1082130432);
    var gval = 1234;
    var gval2 = 1234.0;

    var loopCOUNT = 5;

    var Int8Heap = new stdlib.Int8Array (buffer);    
    var Uint8Heap = new stdlib.Uint8Array (buffer);    

    var Int16Heap = new stdlib.Int16Array(buffer);
    var Uint16Heap = new stdlib.Uint16Array(buffer);
    var Int32Heap = new stdlib.Int32Array(buffer);
    var Uint32Heap = new stdlib.Uint32Array(buffer);
    var Float32Heap = new stdlib.Float32Array(buffer);

    function func1()
    {
        var x = ui4(1, 2, 3, 4);
        var t = ui4(0, 0, 0, 0);
        var y = ui4(0, 0, 0, 0);
        var index = 100;
        var size = 10;
        var loopIndex = 0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            ui4store(Int8Heap, index >> 0, x);
            index = (index + 16 ) | 0;
        }
        
        index = 100;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = ui4load(Uint8Heap, index >> 0);
            y = ui4add(y, t);
            index = (index + 16 ) | 0;
        }
        return ui4check(y);
    }

    function func1OOB_1()
    {
        var x = ui4(1, 2, 3, 4);
        var t = ui4(0, 0, 0, 0);
        var y = ui4(0, 0, 0, 0);
        var index = 0;
        var size = 10;
        var loopIndex = 0;

        index = (0x10000-160)|0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            ui4store(Int8Heap, index >> 0, x);
            index = (index + 16 ) | 0;
        }

        // No OOB
        index = (0x10000-160)|0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = ui4load(Uint8Heap, index >> 0);
            y = ui4add(y, t);
            index = (index + 16 ) | 0;
        }
        return ui4check(y);
    }
    
    function func1OOB_2()
    {
        var x = ui4(1, 2, 3, 4);
        var t = ui4(0, 0, 0, 0);
        var y = ui4(0, 0, 0, 0);
        var index = 0;
        var size = 10;
        var loopIndex = 0;
        
        index = (0x10000-160)|0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            ui4store(Int8Heap, index >> 0, x);
            index = (index + 16 ) | 0;
        }
        
        // OOB
        index = ((0x10000-160) + 1)|0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = ui4load(Uint8Heap, index >> 0);
            y = ui4add(y, t);
            index = (index + 16 ) | 0;
        }
        return ui4check(y);
    }

    function func2()
    {
        var x = ui4(1, 2, 3, 4);
        var t = ui4(0, 0, 0, 0);
        var y = ui4(0, 0, 0, 0);
        var index = 100;
        var size = 10;
        var loopIndex = 0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            ui4store3(Uint16Heap, index >> 1, x);
            index = (index + 16 ) | 0;
        }

        index = 100;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = ui4load3(Int16Heap, index >> 1);
            y = ui4add(y, t);
            index = (index + 16 ) | 0;
        }
        return ui4check(y);
    }

    function func2OOB_1()
    {
        var x = ui4(1, 2, 3, 4);
        var t = ui4(0, 0, 0, 0);
        var y = ui4(0, 0, 0, 0);
        var index = 0;
        var size = 10;
        var loopIndex = 0;
        
        index = (0x10000-160)|0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            ui4store3(Uint16Heap, index >> 1, x);
            index = (index + 16 ) | 0;
        }

        // No OOB here
        index = ((0x10000 - 160) + 4)|0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = ui4load3(Int16Heap, index >> 1);
            y = ui4add(y, t);
            index = (index + 16 ) | 0;
        }
        return ui4check(y);
    }

    function func2OOB_2()
    {
        var x = ui4(1, 2, 3, 4);
        var t = ui4(0, 0, 0, 0);
        var y = ui4(0, 0, 0, 0);
        var index = 0;
        var size = 10;
        var loopIndex = 0;
        
        index = (0x10000-160)|0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            ui4store3(Uint16Heap, index >> 1, x);
            index = (index + 16 ) | 0;
        }

        index = ((0x10000 - 160) + 6)|0;
        // OOB here
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = ui4load3(Int16Heap, index >> 1);
            y = ui4add(y, t);
            index = (index + 16 ) | 0;
        }
    }

    function func3()
    {
        var x = ui4(1, 2, 3, 4);
        var t = ui4(0, 0, 0, 0);
        var y = ui4(0, 0, 0, 0);
        var index = 100;
        var size = 10;
        var loopIndex = 0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            ui4store2(Uint16Heap, index >> 1, x);
            index = (index + 16 ) | 0;
        }
        
        index = 100;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = ui4load2(Int32Heap, index >> 2);
            y = ui4add(y, t);
            index = (index + 16 ) | 0;
        }
        return ui4check(y);
    }

    function func3OOB_1()
    {
        var x = ui4(1, 2, 3, 4);
        var t = ui4(0, 0, 0, 0);
        var y = ui4(0, 0, 0, 0);
        var index = 0;
        var size = 10;
        var loopIndex = 0;
        
        index = (0x10000-160)|0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            ui4store2(Uint16Heap, index >> 1, x);
            index = (index + 16 ) | 0;
        }
        
        index = ((0x10000 - 160) + 8)|0;
        // No OOB
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = ui4load2(Int32Heap, index >> 2);
            y = ui4add(y, t);
            index = (index + 16 ) | 0;
        }
        return ui4check(y);
    }

    function func3OOB_2()
    {
        var x = ui4(1, 2, 3, 4);
        var t = ui4(0, 0, 0, 0);
        var y = ui4(0, 0, 0, 0);
        var index = 0;
        var size = 10;
        var loopIndex = 0;
        
        index = (0x10000-160)|0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            ui4store2(Uint16Heap, index >> 1, x);
            index = (index + 16 ) | 0;
        }

        index = ((0x10000 - 160) + 32)|0;
        // OOB
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = ui4load2(Int32Heap, index >> 2);
            y = ui4add(y, t);
            index = (index + 16 ) | 0;
        }
        return ui4check(y);
    }

    function func4()
    {
        var x = ui4(1, 2, 3, 4);
        var t = ui4(0, 0, 0, 0);
        var y = ui4(0, 0, 0, 0);
        var index = 100;
        var size = 10;
        var loopIndex = 0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            ui4store1(Uint32Heap, index >> 2, x);
            index = (index + 16 ) | 0;
        }
        
        index = 100;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = ui4load1(Float32Heap, index >> 2);
            y = ui4add(y, t);
            index = (index + 16 ) | 0;
        }
        return ui4check(y);
    }

    function func4OOB_1()
    {
        var x = ui4(1, 2, 3, 4);
        var t = ui4(0, 0, 0, 0);
        var y = ui4(0, 0, 0, 0);
        var index = 0;
        var size = 10;
        var loopIndex = 0;
        
        index = (0x10000-160)|0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            ui4store1(Uint32Heap, index >> 2, x);
            index = (index + 16 ) | 0;
        }
        
        index = ((0x10000 - 160) + 12)|0;
        // No OOB
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = ui4load1(Float32Heap, index >> 2);
            y = ui4add(y, t);
            index = (index + 16 ) | 0;
        }
        return ui4check(y);
    }

    function func4OOB_2()
    {
        var x = ui4(1, 2, 3, 4);
        var t = ui4(0, 0, 0, 0);
        var y = ui4(0, 0, 0, 0);
        var index = 0;
        var size = 10;
        var loopIndex = 0;
        
        index = (0x10000-160)|0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            ui4store1(Uint32Heap, index >> 2, x);
            index = (index + 16 ) | 0;
        }
        
        index = ((0x10000 - 160) + 16)|0;
        // OOB
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = ui4load1(Float32Heap, index >> 2);
            y = ui4add(y, t);
            index = (index + 16 ) | 0;
        }
        return ui4check(y);
    }

    function func5()
    {
        var x = ui4(1, 2, 3, 4);
        var t = ui4(0, 0, 0, 0);
        var y = ui4(0, 0, 0, 0);
        var index = 100;
        var size = 10;
        var loopIndex = 0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            ui4store(Uint32Heap, index >> 2, x);
            index = (index + 16 ) | 0;
        }

        index = 100;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = ui4load(Float32Heap, index >> 2);
            y = ui4add(y, t);
            index = (index + 16 ) | 0;
        }
        return ui4check(y);
    }

    function func5OOB_1()
    {
        var x = ui4(1, 2, 3, 4);
        var t = ui4(0, 0, 0, 0);
        var y = ui4(0, 0, 0, 0);
        var index = 0;
        var size = 10;
        var loopIndex = 0;
        index = (0x10000-160)|0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            ui4store(Uint32Heap, index >> 2, x);
            index = (index + 16 ) | 0;
        }
        
        index = ((0x10000 - 160) + 8)|0;
        // No OOB
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = ui4load2(Float32Heap, index >> 2);
            y = ui4add(y, t);
            index = (index + 16 ) | 0;
        }
        return ui4check(y);
    }

    function func5OOB_2()
    {
        var x = ui4(1, 2, 3, 4);
        var t = ui4(0, 0, 0, 0);
        var y = ui4(0, 0, 0, 0);
        var index = 0;
        var size = 10;
        var loopIndex = 0;
        
        index = (0x10000-160)|0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            ui4store(Uint32Heap, index >> 2, x);
            index = (index + 16 ) | 0;
        }
        
        index = ((0x10000 - 160) + 12)|0;
        // OOB
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = ui4load(Float32Heap, index >> 2);
            y = ui4add(y, t);
            index = (index + 16 ) | 0;
        }
        return ui4check(y);
    }

    function func6()
    {
        var x = ui4(1, 2, 3, 4);
        var t = ui4(0, 0, 0, 0);
        var y = ui4(0, 0, 0, 0);
        var index = 100;
        var size = 10;
        var loopIndex = 0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            ui4store(Uint32Heap, index >> 2, x);
            index = (index + 16 ) | 0;
        }
        
        index = 100;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = ui4load(Float32Heap, index >> 2);
            y = ui4add(y, t);
            index = (index + 16 ) | 0;
        }
        return ui4check(y);
    }

    function func6OOB_1()
    {
        var x = ui4(1, 2, 3, 4);
        var t = ui4(0, 0, 0, 0);
        var y = ui4(0, 0, 0, 0);
        var index = 0;
        var size = 10;
        var loopIndex = 0;

        index = (0x10000-160)|0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            ui4store(Uint32Heap, index >> 2, x);
            index = (index + 16 ) | 0;
        }

        index = ((0x10000 - 160) + 12)|0;
        // No OOB
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = ui4load1(Float32Heap, index >> 2);
            y = ui4add(y, t);
            index = (index + 16 ) | 0;
        }
        return ui4check(y);
    }

    function func6OOB_2()
    {
        var x = ui4(1, 2, 3, 4);
        var t = ui4(0, 0, 0, 0);
        var y = ui4(0, 0, 0, 0);
        var index = 0;
        var size = 10;
        var loopIndex = 0;
        
        index = (0x10000-160)|0;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            ui4store(Uint32Heap, index >> 2, x);
            index = (index + 16 ) | 0;
        }
        
        index = ((0x10000 - 160) + 16)|0;
        // OOB
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = ui4load(Float32Heap, index >> 2);
            y = ui4add(y, t);
            index = (index + 16 ) | 0;
        }
        return ui4check(y);
    }

    // TODO: Test conversion of returned value
    function value()
    {
        var ret = 1.0;
        var i = 1.0;
        var loopIndex = 0;
        
        while ( (loopIndex|0) < (loopCOUNT|0)) {

            ret = ret + i;

            loopIndex = (loopIndex + 1) | 0;
        }

        return +ret;
    }

    return {
        func1:func1, 
        func1OOB_1:func1OOB_1, 
        func1OOB_2:func1OOB_2, 
        
        func2:func2, 
        func2OOB_1:func2OOB_1, 
        func2OOB_2:func2OOB_2, 
        
        func3:func3, 
        func3OOB_1:func3OOB_1, 
        func3OOB_2:func3OOB_2, 
        
        func4:func4, 
        func4OOB_1:func4OOB_1, 
        func4OOB_2:func4OOB_2, 
 
        func5:func5, 
        func5OOB_1:func5OOB_1, 
        func5OOB_2:func5OOB_2, 
        
        func6:func6,
        func6OOB_1:func6OOB_1,
        func6OOB_2:func6OOB_2
        };
}
var buffer = new ArrayBuffer(0x10000);
var m = asmModule(this, {g1: SIMD.Uint32x4(1065353216, 1073741824, 1077936128, 1082130432)}, buffer);

var ret;

ret = m.func1();
equalSimd([10, 20, 30, 40], ret, SIMD.Uint32x4, "Test Load Store1");


ret = m.func2();
equalSimd([10, 20, 30, 0], ret, SIMD.Uint32x4, "Test Load Store2");


ret = m.func3();
equalSimd([10, 20, 0, 0], ret, SIMD.Uint32x4, "Test Load Store3");


ret = m.func4();
equalSimd([10, 0, 0, 0], ret, SIMD.Uint32x4, "Test Load Store4");


ret = m.func5();
equalSimd([10, 20, 30, 40], ret, SIMD.Uint32x4, "Test Load Store5");


ret = m.func6();
equalSimd([10, 20, 30, 40], ret, SIMD.Uint32x4, "Test Load Store6");


//

var funcOOB1 = [m.func1OOB_1, m.func2OOB_1 ,m.func3OOB_1, m.func4OOB_1, m.func5OOB_1, m.func6OOB_1];
var RESULTS = [SIMD.Uint32x4(10, 20, 30, 40),
SIMD.Uint32x4(20, 30, 40, 0),
SIMD.Uint32x4(30, 40, 0, 0),
SIMD.Uint32x4(40, 0, 0, 0),
SIMD.Uint32x4(30, 40, 0, 0),
SIMD.Uint32x4(40, 0, 0, 0)
];

for (var i = 0; i < funcOOB1.length; i ++)
{
    try
    {
        ret = funcOOB1[i]();
        //print("func" + (i+1) + "OOB_1");
        equalSimd(RESULTS[i], ret, SIMD.Uint32x4, "Test Load Store");

    } catch(e)
    {
        print("Wrong");
    }
}

//
var funcOOB2 = [m.func1OOB_2, m.func2OOB_2 ,m.func3OOB_2, m.func4OOB_2, m.func5OOB_2, m.func6OOB_2];

for (var i = 0; i < funcOOB2.length; i ++)
{
    //print("func" + (i+1) + "OOB_2");
    try
    {
        ret = funcOOB2[i]();
        print("Wrong");
        
    } catch(e)
    {
        if (e instanceof RangeError) {
            //print("Correct");
        }
        else
            print("Wrong");
        
    }
}
print("PASS");