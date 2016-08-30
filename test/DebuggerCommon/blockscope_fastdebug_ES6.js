/*
    The call is to a function is made from different scope
*/

function foo() {
    var marker = 100;
    let foo = 1; /**loc(bp1):
                    stack();
                    setFrame(1);
                    locals()
                  **/
}

function bar(){
    let x = 100;
    {
        let y = 100;
        foo();
        WScript.Echo(y);
        WScript.Echo(x);
    }
    WScript.Echo(x);

}

function Run(){
    bar();
    bar();
    bar(); /**bp:enableBp('bp1')**/
	bar; /**bp:disableBp('bp1')**/
    WScript.Echo('PASSED');
}

WScript.Attach(Run);

