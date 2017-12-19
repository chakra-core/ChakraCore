;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module 
	(func 
		(export  "func_f32x4_0")  
		(result  f32)  
		(local  f32x4)  
		(set_local 
			0 
			(f32x4.build 
				(f32.const  1.0)  
				(f32.const  2147483647)  
				(f32.const  2147483646.9)  
				(f32.const  -2147483647.5)  
			) 
		) 
		(f32x4.extractLane
			(get_local  0)  
			(i32.const  0)  
		) 
	) 
	(func 
		(export  "func_f32x4_1")  
		(result  f32)  
		(local  f32x4)  
		(set_local 
			0 
			(f32x4.build 
				(f32.const  1.0)  
				(f32.const  2147483647)  
				(f32.const  2147483646.9)  
				(f32.const  -2147483647.5)   
			) 
		) 
		(f32x4.extractLane
			(get_local  0)  
			(i32.const  1)  
		) 
	) 
	(func 
		(export  "func_f32x4_2")  
		(result  f32)  
		(local  f32x4)  
		(set_local 
			0 
			(f32x4.build 
				(f32.const  1.0)  
				(f32.const  2147483647)  
				(f32.const  2147483646.9)  
				(f32.const  -2147483647.5)   
			) 
		) 
		(f32x4.extractLane 
			(get_local  0)  
			(i32.const  2)  
		) 
	) 
	(func 
		(export  "func_f32x4_3")  
		(result  f32)  
		(local  f32x4)  
		(set_local 
			0 
			(f32x4.build 
				(f32.const  1.0)  
				(f32.const  2147483647)  
				(f32.const  2147483646.9)  
				(f32.const  -2147483647.5)   
			) 
		) 
		(f32x4.extractLane 
			(get_local  0)  
			(i32.const  3)  
		) 
	) 
) 
