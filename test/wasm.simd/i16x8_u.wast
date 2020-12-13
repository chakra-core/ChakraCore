;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module 
	(func 
		(export  "func_i16x8_u_0")  
		(result  i32)  
		(i16x8.extract_lane_u 0
			(v128.const i16x8 1 2 32768 0 32767 6 65535 65526))
	) 
	(func 
		(export  "func_i16x8_u_1")  
		(result  i32)  
		(i16x8.extract_lane_u 1
			(v128.const i16x8 1 2 32768 0 32767 6 65535 65526))
	) 
	(func 
		(export  "func_i16x8_u_2")  
		(result  i32)  
		(i16x8.extract_lane_u 2
			(v128.const i16x8 1 2 32768 0 32767 6 65535 65526))
	) 
	(func 
		(export  "func_i16x8_u_3")  
		(result  i32)  
		(i16x8.extract_lane_u 3
			(v128.const i16x8 1 2 32768 0 32767 6 65535 65526))
	) 
	(func 
		(export  "func_i16x8_u_4")  
		(result  i32)  
		(i16x8.extract_lane_u 4
			(v128.const i16x8 1 2 32768 0 32767 6 65535 65526))
	) 
	(func 
		(export  "func_i16x8_u_5")  
		(result  i32)  
		(i16x8.extract_lane_u 5
			(v128.const i16x8 1 2 32768 0 32767 6 65535 65526))
	) 
	(func 
		(export  "func_i16x8_u_6")  
		(result  i32)  
		(i16x8.extract_lane_u 6
			(v128.const i16x8 1 2 32768 0 32767 6 65535 65526))
	) 
	(func 
		(export  "func_i16x8_u_7")  
		(result  i32)  
		(i16x8.extract_lane_u 7
			(v128.const i16x8 1 2 32768 0 32767 6 65535 65526))
	) 
) 
