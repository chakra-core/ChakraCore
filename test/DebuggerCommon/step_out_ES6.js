/*
    Step out of JITted function - ES6
*/

function foo() {
    let foo = "foo";
    bar();
}

function bar() {
    let z = 1;
    {
        const z = 2; /**loc(bp1):
                        locals(1);
                        resume('step_out')
                        **/
    }
    z++; 
}

function Run(){
    foo();
    foo(); 
    foo(); /**bp:enableBp('bp1')**/
	foo; /**bp:disableBp('bp1')**/
    WScript.Echo('PASSED');
}


WScript.Attach(Run);
WScript.Detach(Run);
WScript.Attach(Run);