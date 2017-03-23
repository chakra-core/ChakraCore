//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function Obj1()
{
    this[4] = 2;
    this.y = 2;
    this.w = 5;
}
Obj1.prototype.x = 3;

function Obj2()
{
    this[5] = 2;
    this.y = 2;
    this.x = 1;
}

Obj2.prototype.x = 3;
Obj2.prototype.z = 4;

function hasOwnUnchanged(obj)
{
    let result = "";
    for(let prop in obj)
    {
        if(obj.hasOwnProperty(prop))
        {
            result += `own: ${prop}, ` ;
        }
        else
        {
            result += `not: ${prop}, ` ;
        }
    }
    return result;
}

print("hasOwnUnchanged:");
print(hasOwnUnchanged(new Obj1()));
print(hasOwnUnchanged(new Obj2()));
print(hasOwnUnchanged(new Obj1()));
print(hasOwnUnchanged(new Obj2()));

function hasOwnDifferentObj(obj1, obj2)
{
    let result = "";
    for(let prop in obj1)
    {
        if(obj2.hasOwnProperty(prop))
        {
            result += `own: ${prop}, ` ;
        }
        else
        {
            result += `not: ${prop}, ` ;
        }
    }
    return result;
}

print("hasOwnDifferentObj:");
print(hasOwnDifferentObj(new Obj2(), new Obj1()));
print(hasOwnDifferentObj(new Obj1(), new Obj2()));
print(hasOwnDifferentObj(new Obj1(), new Obj1()));
print(hasOwnDifferentObj(new Obj2(), new Obj2()));
print(hasOwnDifferentObj(new Obj1(), new Obj2()));
print(hasOwnDifferentObj(new Obj2(), new Obj1()));

function hasOwnModifyLate(obj)
{
    let result = "";
    for(let prop in obj)
    {
        if(obj.hasOwnProperty(prop))
        {
            result += `own: ${prop}, ` ;
        }
        else
        {
            result += `not: ${prop}, ` ;
        }
        obj.x = 1;
    }
    return result;
}

print("hasOwnModifyLate:");
print(hasOwnModifyLate(new Obj1()));
print(hasOwnModifyLate(new Obj2()));
print(hasOwnModifyLate(new Obj1()));
print(hasOwnModifyLate(new Obj2()));

function hasOwnModifyEarly(obj)
{
    let result = "";
    for(let prop in obj)
    {
        obj.x = 1;
        if(obj.hasOwnProperty(prop))
        {
            result += `own: ${prop}, ` ;
        }
        else
        {
            result += `not: ${prop}, ` ;
        }
    }
    return result;
}

print("hasOwnModifyEarly:");
print(hasOwnModifyEarly(new Obj1()));
print(hasOwnModifyEarly(new Obj2()));
print(hasOwnModifyEarly(new Obj1()));
print(hasOwnModifyEarly(new Obj2()));

function hasOwnDelete(obj)
{
    let result = "";
    for(let prop in obj)
    {
        delete obj.x;
        if(obj.hasOwnProperty(prop))
        {
            result += `own: ${prop}, ` ;
        }
        else
        {
            result += `not: ${prop}, ` ;
        }
    }
    return result;
}
print("hasOwnDelete:");
print(hasOwnDelete(new Obj1()));
print(hasOwnDelete(new Obj2()));
print(hasOwnDelete(new Obj1()));
print(hasOwnDelete(new Obj2()));

function hasOwnDeleteLate(obj)
{
    let result = "";
    for(let prop in obj)
    {
        if(obj.hasOwnProperty(prop))
        {
            result += `own: ${prop}, ` ;
        }
        else
        {
            result += `not: ${prop}, ` ;
        }
        delete obj.x;
    }
    return result;
}
print("hasOwnDeleteLate:");
print(hasOwnDeleteLate(new Obj1()));
print(hasOwnDeleteLate(new Obj2()));
print(hasOwnDeleteLate(new Obj1()));
print(hasOwnDeleteLate(new Obj2()));

function hasOwnModifyOther(obj1, obj2)
{
    let result = "";
    for(let prop in obj1)
    {
        if(obj1.hasOwnProperty(prop))
        {
            result += `own: ${prop}, ` ;
        }
        else
        {
            result += `not: ${prop}, ` ;
        }
        obj2.x = "123";
    }
    return result;
}
print("hasOwnModifyOther:");
print(hasOwnModifyOther(new Obj1(), new Obj1()));
print(hasOwnModifyOther(new Obj2(), new Obj2()));
print(hasOwnModifyOther(new Obj1(), new Obj1()));
print(hasOwnModifyOther(new Obj2(), new Obj2()));
print(hasOwnModifyOther(new Obj1(), new Obj2()));
print(hasOwnModifyOther(new Obj2(), new Obj1()));

Obj2.prototype["1"] = {enumerable: true};
function hasOwnNumeric(obj)
{
    let result = "";
    for(let prop in obj)
    {
        if(obj.hasOwnProperty(prop))
        {
            result += `own: ${prop}, ` ;
            obj["1"] = {enumerable: true};
        }
        else
        {
            result += `not: ${prop}, ` ;
        }
    }
    return result;
}
print("hasOwnNumeric:");
print(hasOwnNumeric(new Obj1()));
print(hasOwnNumeric(new Obj2()));
print(hasOwnNumeric(new Obj1()));
print(hasOwnNumeric(new Obj2()));

print("hasOwnUnchangedWithNumeric:");
print(hasOwnUnchanged(new Obj1()));
print(hasOwnUnchanged(new Obj2()));

function hasOwnNumeric2(obj)
{
    let result = "";
    for(let prop in obj)
    {
        if(obj.hasOwnProperty(prop))
        {
            result += `own: ${prop}, ` ;
            delete Obj2.prototype["1"];
        }
        else
        {
            result += `not: ${prop}, ` ;
        }
    }
    return result;
}

print("hasOwnNumeric2:");
print(hasOwnNumeric2(new Obj1()));
Obj2.prototype["1"] = {enumerable: true};
print(hasOwnNumeric2(new Obj2()));
Obj2.prototype["1"] = {enumerable: true};
print(hasOwnNumeric2(new Obj1()));
Obj2.prototype["1"] = {enumerable: true};
print(hasOwnNumeric2(new Obj2()));

function hasOwnNumeric3(obj)
{
    let result = "";
    for(let prop in obj)
    {
        if(obj.hasOwnProperty(prop))
        {
            result += `own: ${prop}, ` ;
            obj[7] = 1;
        }
        else
        {
            result += `not: ${prop}, ` ;
        }
    }
    return result;
}
Obj2.prototype[7] = 2;

print("hasOwnNumeric3:");
print(hasOwnNumeric3(new Obj1()));
print(hasOwnNumeric3(new Obj2()));
print(hasOwnNumeric3(new Obj1()));
print(hasOwnNumeric3(new Obj1()));
print(hasOwnNumeric3(new Obj2()));
print(hasOwnNumeric3(new Obj2()));
