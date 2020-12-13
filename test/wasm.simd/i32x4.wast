;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module 
	(func 
		(export  "func_i32x4_0")  
		(result  i32)  
		(local  v128)  
		(set_local 
			0 
			(v128.const i32x4 2147483647 0 0x80000000 0xFFFFFF9C) 
		) 
		(i32x4.extract_lane 0 (get_local  0)) 
	) 
	(func 
		(export  "func_i32x4_1")  
		(result  i32)  
		(local  v128)  
		(set_local 
			0 
			(v128.const i32x4 2147483647 0 0x80000000 0xFFFFFF9C) 
		) 
		(i32x4.extract_lane 1 (get_local  0))
	) 
	(func 
		(export  "func_i32x4_2")  
		(result  i32)  
		(local  v128)  
		(set_local 
			0 
			(v128.const i32x4 2147483647 0 0x80000000 0xFFFFFF9C) 
		) 
		(i32x4.extract_lane 2 (get_local  0)) 
	) 
	(func 
		(export  "func_i32x4_3")  
		(result  i32)  
		(local  v128)  
		(set_local 
			0 
			(v128.const i32x4 2147483647 0 0x80000000 0xFFFFFF9C) 
		) 
		(i32x4.extract_lane 3 (get_local  0)) 
	) 
) 
