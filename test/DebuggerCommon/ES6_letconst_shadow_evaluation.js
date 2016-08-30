/*
   Shadoned Evaluation
*/

function Run(){
    var x = 1;
    {
        let x = 2; 
        x; /**bp:locals();evaluate('x')**/
    }
    x; /**bp:locals()**/
	WScript.Echo('PASSED');
}

var foo = Run;

WScript.Attach(foo);
WScript.Detach(foo);
WScript.Attach(foo);

