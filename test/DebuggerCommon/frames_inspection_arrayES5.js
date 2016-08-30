/*
    Inspecting frames - Array ES5
*/


try {
    e++;
} catch (e) {
    with ({ o: 2 }) {
        var arr = [];
        arr.push(1);

        arr.forEach(function (key, val, map) {
            key;/**bp:stack();locals()**/
        });
    }
}
WScript.Echo('PASSED');
