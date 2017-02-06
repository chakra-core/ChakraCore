;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module 
	(func 
		(export  "func_b32x4_0")  
		(result  i32)  
		(local  b32x4)  
		(set_local 
			0 
			(b32x4.build 
				(i32.const  0)  
				(i32.const  1)  
				(i32.const  -2147483648)  
				(i32.const  2147483647)  
			) 
		) 
		(b32x4.extractLane 
			(get_local  0)  
			(i32.const  0)  
		) 
	) 
	(func 
		(export  "func_b32x4_1")  
		(result  i32)  
		(local  b32x4)  
		(set_local 
			0 
			(b32x4.build 
				(i32.const  0)  
				(i32.const  1)  
				(i32.const  -2147483648)  
				(i32.const  2147483647) 
			) 
		) 
		(b32x4.extractLane 
			(get_local  0)  
			(i32.const  1)  
		) 
	) 
	(func 
		(export  "func_b32x4_2")  
		(result  i32)  
		(local  b32x4)  
		(set_local 
			0 
			(b32x4.build 
				(i32.const  0)  
				(i32.const  1)  
				(i32.const  -2147483648)  
				(i32.const  2147483647)   
			) 
		) 
		(b32x4.extractLane 
			(get_local  0)  
			(i32.const  2)  
		) 
	) 
	(func 
		(export  "func_b32x4_3")  
		(result  i32)  
		(local  b32x4)  
		(set_local 
			0 
			(b32x4.build 
				(i32.const  0)  
				(i32.const  1)  
				(i32.const  -2147483648)  
				(i32.const  2147483647)  
			) 
		) 
		(b32x4.extractLane 
			(get_local  0)  
			(i32.const  3)  
		) 
	) 
) 
