/*
	Interaction between let/const, with and eval
*/


function Run(){
    let a = 1;
    const b = 2;
    {
        eval('let a = 2');
        WScript.Echo(a); /**bp:locals(1)**/
    }
    with({b:2}){
        const b = 2;
        {
            let b = 3;
            const a = 4;
			//known issue that locals would print b from with
            WScript.Echo(b); /**bp:locals(1)**/
        }
        WScript.Echo(b); /**bp:locals(1)**/
    }
    WScript.Echo(a); /**bp:locals(1)**/
}


WScript.Attach(Run);