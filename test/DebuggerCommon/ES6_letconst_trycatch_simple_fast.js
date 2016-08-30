//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
    Try catch - fast debug
*/

var callcount = 0;

function Call() {
    let foo = "foo";
    bar();   
}

function bar() {
    callcount++;
    let z = 1;
    try{
        {    
            const z = 2; 
            z; /**loc(bp1):locals(1)**/
        }
        if(callcount == 3) throw new Error();
    }catch(e){
        z++; 
        z;/**loc(bp3):locals(1)**/
    }
}

function Run(){
    Call();
    Call();
    Call(); /**bp:enableBp('bp3');enableBp('bp1')**/    
}

Run();
WScript.Echo('PASSED');