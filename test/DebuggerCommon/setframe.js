//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function FFF()
{
	var FFF_A = 0;
	var FFF_B = 0;
	var FFF_C = 0;

	GGG();
}

var GGG = (function() {
	var GGG_closure = 0;
	return function GGG()
	{
		var GGG_A = 0;
		var GGG_B = 0;
		var GGG_C = 0;
		var GGG_ARRAY = [1,2,3];
		GGG_closure = 1;
	
		HHH();
	}
})();

function HHH()
{
	var HHH_A = 0;
	var HHH_B = 0;
	var HHH_C = 0;

	HHH_C++;  /**bp:
                     locals(1);

                     setFrame(1);
                     locals(1);

                     setFrame(2);
                     locals(1);

                     setFrame(1);
                     evaluate("GGG_B=2;GGG_closure=3");

                     stack();  // always happens on the top frame

                     setFrame(2);
                     evaluate("FFF_B++");

                     setFrame(1);
                     locals(1);

                     setFrame(2);
                     locals(1);
                  **/
}

FFF();
WScript.Echo("pass");
