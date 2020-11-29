;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module 
	(func 
		(export  "func_i16x8_s_0")  
		(result  i32)  
		(i16x8.extract_lane_s 0
			(v128.const i16x8 1 2 -32768 0 32767 6 7 -10))
	) 
	(func 
		(export  "func_i16x8_s_1")  
		(result  i32)  
		(i16x8.extract_lane_s 1
			(v128.const i16x8 1 2 -32768 0 32767 6 7 -10))
	) 
	(func 
		(export  "func_i16x8_s_2")  
		(result  i32)  
		(i16x8.extract_lane_s 2
			(v128.const i16x8 1 2 -32768 0 32767 6 7 -10))
	) 
	(func 
		(export  "func_i16x8_s_3")  
		(result  i32)  
		(i16x8.extract_lane_s 3
			(v128.const i16x8 1 2 -32768 0 32767 6 7 -10))
	) 
	(func 
		(export  "func_i16x8_s_4")  
		(result  i32)  
		(i16x8.extract_lane_s 4
			(v128.const i16x8 1 2 -32768 0 32767 6 7 -10))
	) 
	(func 
		(export  "func_i16x8_s_5")  
		(result  i32)  
		(i16x8.extract_lane_s 5
			(v128.const i16x8 1 2 -32768 0 32767 6 7 -10))
	) 
	(func 
		(export  "func_i16x8_s_6")  
		(result  i32)  
		(i16x8.extract_lane_s 6
			(v128.const i16x8 1 2 -32768 0 32767 6 7 -10))
	) 
	(func 
		(export  "func_i16x8_s_7")  
		(result  i32)  
		(i16x8.extract_lane_s 7
			(v128.const i16x8 1 2 -32768 0 32767 6 7 -10))
	) 
) 
