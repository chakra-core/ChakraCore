var x = 1;

for(let y of [0,1,2] /**bp:locals()**/ ){
    const z = 1;
    z;/**bp:locals(1)**/
}
WScript.Echo('PASSED');