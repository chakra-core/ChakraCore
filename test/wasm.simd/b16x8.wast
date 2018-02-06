;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module 
	(func 
		(export  "func_b16x8_0")  
		(result  i32)  
		(local  b16x8)  
		(set_local 
			0 
			(b16x8.build 
				(i32.const  1)  
				(i32.const  0)  
				(i32.const  2147483647)  
				(i32.const  -2147483648)  
				(i32.const  5)  
				(i32.const  6)  
				(i32.const  7)  
				(i32.const  -10)  
			) 
		) 
		(b16x8.extractLane 
			(get_local  0)  
			(i32.const  0)  
		) 
	) 
	(func 
		(export  "func_b16x8_1")  
		(result  i32)  
		(local  b16x8)  
		(set_local 
			0 
			(b16x8.build 
				(i32.const  1)  
				(i32.const  0)  
				(i32.const  2147483647)  
				(i32.const  -2147483648)  
				(i32.const  5)  
				(i32.const  6)  
				(i32.const  7)  
				(i32.const  -10)  
			) 
		) 
		(b16x8.extractLane 
			(get_local  0)  
			(i32.const  1)  
		) 
	) 
	(func 
		(export  "func_b16x8_2")  
		(result  i32)  
		(local  b16x8)  
		(set_local 
			0 
			(b16x8.build 
				(i32.const  1)  
				(i32.const  0)  
				(i32.const  2147483647)  
				(i32.const  -2147483648)  
				(i32.const  5)  
				(i32.const  6)  
				(i32.const  7)  
				(i32.const  -10)    
			) 
		) 
		(b16x8.extractLane 
			(get_local  0)  
			(i32.const  2)  
		) 
	) 
	(func 
		(export  "func_b16x8_3")  
		(result  i32)  
		(local  b16x8)  
		(set_local 
			0 
			(b16x8.build 
				(i32.const  1)  
				(i32.const  0)  
				(i32.const  2147483647)  
				(i32.const  -2147483648)  
				(i32.const  5)  
				(i32.const  6)  
				(i32.const  7)  
				(i32.const  -10)  
			) 
		) 
		(b16x8.extractLane 
			(get_local  0)  
			(i32.const  3)  
		) 
	) 
	(func 
		(export  "func_b16x8_4")  
		(result  i32)  
		(local  b16x8)  
		(set_local 
			0 
			(b16x8.build 
				(i32.const  1)  
				(i32.const  0)  
				(i32.const  2147483647)  
				(i32.const  -2147483648)  
				(i32.const  5)  
				(i32.const  6)  
				(i32.const  7)  
				(i32.const  -10)   
			) 
		) 
		(b16x8.extractLane 
			(get_local  0)  
			(i32.const  4)  
		) 
	) 
	(func 
		(export  "func_b16x8_5")  
		(result  i32)  
		(local  b16x8)  
		(set_local 
			0 
			(b16x8.build 
				(i32.const  1)  
				(i32.const  0)  
				(i32.const  2147483647)  
				(i32.const  -2147483648)  
				(i32.const  5)  
				(i32.const  6)  
				(i32.const  7)  
				(i32.const  -10)  
			) 
		) 
		(b16x8.extractLane 
			(get_local  0)  
			(i32.const  5)  
		) 
	) 
	(func 
		(export  "func_b16x8_6")  
		(result  i32)  
		(local  b16x8)  
		(set_local 
			0 
			(b16x8.build 
				(i32.const  1)  
				(i32.const  0)  
				(i32.const  2147483647)  
				(i32.const  -2147483648)  
				(i32.const  5)  
				(i32.const  6)  
				(i32.const  7)  
				(i32.const  -10)    
			) 
		) 
		(b16x8.extractLane 
			(get_local  0)  
			(i32.const  6)  
		) 
	) 
	(func 
		(export  "func_b16x8_7")  
		(result  i32)  
		(local  b16x8)  
		(set_local 
			0 
			(b16x8.build 
				(i32.const  1)  
				(i32.const  0)  
				(i32.const  2147483647)  
				(i32.const  -2147483648)  
				(i32.const  5)  
				(i32.const  6)  
				(i32.const  7)  
				(i32.const  -10)   
			) 
		) 
		(b16x8.extractLane 
			(get_local  0)  
			(i32.const  7)  
		) 
	) 
) 
