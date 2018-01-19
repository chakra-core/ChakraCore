//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//tests import of module that has two different names
import Thing1 from './module_4570_dep2.js';
import Thing2 from '././module_4570_dep2.js';

Thing1();
Thing2();
