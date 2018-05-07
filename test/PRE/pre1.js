//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test1(o)
{
    var count = 0;
    for(var i=0;i<10;i++)
    {
        count += o.x.y;
        count += o.x.y;
        count += o.x.w;
        count += o.z;
    }
    return count;
}
var o = {x:{y:2, w:3}, z:1};
print("\ntest1");
test1(o);
test1(o);
test1(o);

function test2(o)
{
    var a=0;var b =0;
    o.x;
    for(var i=0;i<10;i++)
    {
        o.x++
    }
    return b;
}
o.x=1;
print("\ntest2");
test2(o)
test2(o)
test2(o)

var Direction = new Object();
Direction.NONE     = 0;
Direction.FORWARD  = 1;
Direction.BACKWARD = -1;

function inlinee(o)
{
    o.count += Direction.FORWARD;
    return true;
}
function testInlined(o)
{
    
    for(var i = 0; i < 1000; i++)
    {
        if(!inlinee(o))
        {
            o.count = 1;
        }
        inlinee(o);
    }
}

print("\ntestInlined");
var obj = {count:0, dir:1};

obj.count= 1;
o.dir=1;
testInlined(obj);
print(o.count);

obj.count = 1;
obj.dir =2;
testInlined(obj);
print(obj.count);

obj.count = 1;
obj.dir =1;
testInlined(obj);
print(obj.count);


function test3()
{
    function OrderedCollection() {
        this.elms = new Array();
    }

    OrderedCollection.prototype.size = function() {
        return this.elms.length;
    }

    OrderedCollection.prototype.at = function (index) {
        return this.elms[index];
    }

    function Plan() {
        this.v = new OrderedCollection();
    }

    Plan.prototype.add = function(c){
        this.v.elms.push(c);
    }
    Plan.prototype.size = function () {
        return this.v.size();
    }

    Plan.prototype.constraintAt = function (index) {
        return this.v.at(index);
    }

    Plan.prototype.execute = function()
    {
        var count = 0;
        for (var i = 0; i < this.size(); i++) {
            this.constraintAt(i);
            count += Direction.FORWARD;
        }
        return count;
    }

    var plan = new Plan();
    for(var i = 0; i < 10; i++)
    {
        plan.add(1);
    }
    print(plan.execute());
    print(plan.execute());
    print(plan.execute());
}
print("\ntest3");
test3();

