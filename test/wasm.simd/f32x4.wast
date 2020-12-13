;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module 
	(func 
		(export  "func_f32x4_0")  
		(result f32)  
		(f32x4.extract_lane 0
			(v128.const i32x4 0x3f800000 0x7f7fffff 0x4f000000 0xcf000000))
	) 
	(func 
		(export  "func_f32x4_1")  
		(result f32)  
		(f32x4.extract_lane 1
			(v128.const i32x4 0x3f800000 0x7f7fffff 0x4f000000 0xcf000000))
	) 
	(func 
		(export  "func_f32x4_2")  
		(result f32)  
		(f32x4.extract_lane 2
			(v128.const i32x4 0x3f800000 0x7f7fffff 0x4f000000 0xcf000000))
	) 
	(func 
		(export  "func_f32x4_3")  
		(result f32)  
		(f32x4.extract_lane 3
			(v128.const i32x4 0x3f800000 0x7f7fffff 0x4f000000 0xcf000000))
	) 
) 
