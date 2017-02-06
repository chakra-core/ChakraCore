;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module 
	(func 
		(export  "func_i16x8_s_0")  
		(result  i32)  
		(local  i16x8)  
		(set_local 
			0 
			(i16x8.build 
				(i32.const  1)  
				(i32.const  2)  
				(i32.const  -32768)  
				(i32.const  0)  
				(i32.const  32767)  
				(i32.const  6)  
				(i32.const  7)  
				(i32.const  -10)  
			) 
		) 
		(i16x8.extractLane_s 
			(get_local  0)  
			(i32.const  0)  
		) 
	) 
	(func 
		(export  "func_i16x8_s_1")  
		(result  i32)  
		(local  i16x8)  
		(set_local 
			0 
			(i16x8.build 
				(i32.const  1)  
				(i32.const  2)  
				(i32.const  -32768)  
				(i32.const  0)  
				(i32.const  32767)  
				(i32.const  6)  
				(i32.const  7)  
				(i32.const  -10)  
			) 
		) 
		(i16x8.extractLane_s 
			(get_local  0)  
			(i32.const  1)  
		) 
	) 
	(func 
		(export  "func_i16x8_s_2")  
		(result  i32)  
		(local  i16x8)  
		(set_local 
			0 
			(i16x8.build 
				(i32.const  1)  
				(i32.const  2)  
				(i32.const  -32768)  
				(i32.const  0)  
				(i32.const  32767)  
				(i32.const  6)  
				(i32.const  7)  
				(i32.const  -10)   
			) 
		) 
		(i16x8.extractLane_s 
			(get_local  0)  
			(i32.const  2)  
		) 
	) 
	(func 
		(export  "func_i16x8_s_3")  
		(result  i32)  
		(local  i16x8)  
		(set_local 
			0 
			(i16x8.build 
				(i32.const  1)  
				(i32.const  2)  
				(i32.const  -32768)  
				(i32.const  0)  
				(i32.const  32767)  
				(i32.const  6)  
				(i32.const  7)  
				(i32.const  -10)   
			) 
		) 
		(i16x8.extractLane_s 
			(get_local  0)  
			(i32.const  3)  
		) 
	) 
	(func 
		(export  "func_i16x8_s_4")  
		(result  i32)  
		(local  i16x8)  
		(set_local 
			0 
			(i16x8.build 
				(i32.const  1)  
				(i32.const  2)  
				(i32.const  -32768)  
				(i32.const  0)  
				(i32.const  32767)  
				(i32.const  6)  
				(i32.const  7)  
				(i32.const  -10)   
			) 
		) 
		(i16x8.extractLane_s 
			(get_local  0)  
			(i32.const  4)  
		) 
	) 
	(func 
		(export  "func_i16x8_s_5")  
		(result  i32)  
		(local  i16x8)  
		(set_local 
			0 
			(i16x8.build 
				(i32.const  1)  
				(i32.const  2)  
				(i32.const  -32768)  
				(i32.const  0)  
				(i32.const  32767)  
				(i32.const  6)  
				(i32.const  7)  
				(i32.const  -10)   
			) 
		) 
		(i16x8.extractLane_s 
			(get_local  0)  
			(i32.const  5)  
		) 
	) 
	(func 
		(export  "func_i16x8_s_6")  
		(result  i32)  
		(local  i16x8)  
		(set_local 
			0 
			(i16x8.build 
				(i32.const  1)  
				(i32.const  2)  
				(i32.const  -32768)  
				(i32.const  0)  
				(i32.const  32767)  
				(i32.const  6)  
				(i32.const  7)  
				(i32.const  -10)    
			) 
		) 
		(i16x8.extractLane_s 
			(get_local  0)  
			(i32.const  6)  
		) 
	) 
	(func 
		(export  "func_i16x8_s_7")  
		(result  i32)  
		(local  i16x8)  
		(set_local 
			0 
			(i16x8.build 
				(i32.const  1)  
				(i32.const  2)  
				(i32.const  -32768)  
				(i32.const  0)  
				(i32.const  32767)  
				(i32.const  6)  
				(i32.const  7)  
				(i32.const  -10)  
			) 
		) 
		(i16x8.extractLane_s 
			(get_local  0)  
			(i32.const  7)  
		) 
	) 
) 
