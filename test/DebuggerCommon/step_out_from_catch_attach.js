/*
    Step out from try catch
*/


function foo() {
    let foo = "foo";
    bar();
}

function bar() {
    let z = 1;
    try{
        throw new Error();
    }catch(e){   
        let x = 1; 
        x; /**loc(bp1):
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