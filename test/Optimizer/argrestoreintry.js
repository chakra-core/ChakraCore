function bar() {
    throw new Error();
}
function foo() {
    try {
        x = arguments;
        bar();
    } catch (e) {
        return x.length;
    } 
    var x = {
        j: 1,
        k: 2.2
    };
}
foo();
foo();
let pass = foo() === 0;

print(pass ? "Pass" : "Fail")