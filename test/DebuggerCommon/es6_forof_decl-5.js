var q = 1
for(let x of [0,1,2] /**bp:locals()**/ ){
    for (let y of [0,1,2] /**bp:locals(1)**/) {
        const z = 1;
        z;/**bp:locals(1)**/
    }
}
WScript.Echo('PASSED');