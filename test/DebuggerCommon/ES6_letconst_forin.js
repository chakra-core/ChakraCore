/*
	Basic for in using let
*/

//Win Blue Bug 184592
var a = {x:2, y:1};


for(let y in a){
        
    y; /**bp:locals(1)**/
}

WScript.Echo('PASSED');