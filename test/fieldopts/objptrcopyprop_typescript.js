function foo(obj, obj2)
{
    if (obj.x == 10)
    {
	obj = obj2;

	if (obj.x == 20)
	{
	    return;
	}
    }

    return obj.x;
}


function test()
{
    var o1 = new Object();
    var o2 = new Object();

    o1.x = 10;
    o2.x = 30;

    for (var i = 0; i < 1000; i++)
    {
        var result = foo(o1,o2);
        if (result != 30)
        {
            WScript.Echo("FAILED\n");
            return;
        }
    }

    WScript.Echo("Passed");
}

test();