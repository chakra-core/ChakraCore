//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let xxx = new Uint32Array(0x10000);

xxx.slice = Array.prototype.slice;

function jit(arr, index){
	let ut = arr.slice(0,0);   //become definite Uint32Array but |arr| is a VirtualUint32Array
	for(let i = 0; i < 0x30; i++){
		arr[i] = 0;   //will be crash at |Op_memset|
	}
}

for(let i = 0;i < 0x10000; i++){
	jit(xxx, 2);
}

if (xxx[0] === 0)
{
    WScript.Echo('pass');
}

