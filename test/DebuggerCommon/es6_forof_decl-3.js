var x = 1;

for(x of [0,1] /**bp:locals()**/ ){
    const z = 1;
    z;/**bp:locals(1)**/
}
WScript.Echo('PASSED');