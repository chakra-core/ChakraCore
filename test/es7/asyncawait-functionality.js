//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// ES6 Async Await tests -- verifies functionality of async/await

function echo(str) {
    WScript.Echo(str);
}

var tests = [
    {
        name: "Async keyword with a lambda expressions",
        body: function (index) {
            var x = 12;
            var y = 14;
            var lambdaParenNoArg = async() => x < y;
            var lambdaArgs = async(a, b, c) => a + b + c;

            lambdaParenNoArg().then(result => {
                echo(`Test #${index} - Success lambda expression with no argument called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error lambda expression with no argument called with err = ${err}`);
            });

            lambdaArgs(10, 20, 30).then(result => {
                echo(`Test #${index} - Success lambda expression with several arguments called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error lambda expression with several arguments called with err = ${err}`);
            });
        }
    },
    {
        name: "Async keyword with a lambda expressions and local variable captured and shadowed",
        body: function (index) {
            var x = 12;
            var lambdaSingleArgNoParen = async x => x;
            var lambdaSingleArg = async(x) => x;

            lambdaSingleArgNoParen(x).then(result => {
                echo(`Test #${index} - Success lambda expression with single argument and no paren called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error lambda expression with single argument and no paren called with err = ${err}`);
            });

            lambdaSingleArg(x).then(result => {
                echo(`Test #${index} - Success lambda expression with a single argument a called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error lambda expression with a single argument called with err = ${err}`);
            });
        }
    },
    {
        name: "Async function in a statement",
        body: function (index) {
            {
                var namedNonAsyncMethod = function async(x, y) { return x + y; }
                var unnamedAsyncMethod = async function (x, y) { return x + y; }
                async function async(x, y) { return x - y; }

                var result = namedNonAsyncMethod(10, 20);
                echo(`Test #${index} - Success function #1 called with result = '${result}'`);

                unnamedAsyncMethod(10, 20).then(result => {
                    echo(`Test #${index} - Success function #2 called with result = '${result}'`);
                }, err => {
                    echo(`Test #${index} - Error function #2 called with err = ${err}`);
                });

                async(10, 20).then(result => {
                    echo(`Test #${index} - Success function #3 called with result = '${result}'`);
                }, err => {
                    echo(`Test #${index} - Error function #3 called with err = ${err}`);
                });
            }
            {
                async function async() { return 12; }

                async().then(result => {
                   echo(`Test #${index} - Success function #4 called with result = '${result}'`);
                }, err => {
                    echo(`Test #${index} - Error function #4 called with err = ${err}`);
                });
            }
            {
                function async() { return 12; }

                var result = namedNonAsyncMethod(10, 20);
                echo(`Test #${index} - Success function #5 called with result = '${result}'`);
            }
        }
    },
    {
        name: "Async function in an object",
        body: function (index) {
            var object = {
                async async() { return 12; }
            };

            var object2 = {
                async() { return 12; }
            };

            var object3 = {
                async "a"() { return 12; },
                async 0() { return 12; },
                async 3.14() { return 12; },
                async else() { return 12; },
            };

            object.async().then(result => {
                echo(`Test #${index} - Success function in a object #1 called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error function in a object #1 called with err = ${err}`);
            });

            var result = object2.async();
            echo(`Test #${index} - Success function in a object #2 called with result = '${result}'`);

            object3.a().then(result => {
                echo(`Test #${index} - Success function in a object #3 called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error function in a object #3 called with err = ${err}`);
            });

            object3['0']().then(result => {
                echo(`Test #${index} - Success function in a object #4 called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error function in a object #4 called with err = ${err}`);
            });

            object3['3.14']().then(result => {
                echo(`Test #${index} - Success function in a object #5 called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error function in a object #5 called with err = ${err}`);
            });

            object3.else().then(result => {
                echo(`Test #${index} - Success function in a object #6 called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error function in a object #6 called with err = ${err}`);
            });
        }
    },
    {
        name: "Async classes",
        body: function (index) {
            class MyClass {
                async asyncMethod(a) { return a; }
                async async(a) { return a; }
                async "a"() { return 12; }
                async 0() { return 12; }
                async 3.14() { return 12; }
                async else() { return 12; }
                static async staticAsyncMethod(a) { return a; }
            }

            class MySecondClass {
                async(a) { return a; }
            }

            class MyThirdClass {
                static async(a) { return a; }
            }

            var x = "foo";
            class MyFourthClass {
                async [x](a) { return a; }
            }

            var myClassInstance = new MyClass();
            var mySecondClassInstance = new MySecondClass();
            var myFourthClassInstance = new MyFourthClass();

            myClassInstance.asyncMethod(10).then(result => {
                echo(`Test #${index} - Success async in a class #1 called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error async in a class #1 called with err = ${err}`);
            });

            myClassInstance.async(10).then(result => {
                echo(`Test #${index} - Success async in a class #2 called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error async in a class #2 called with err = ${err}`);
            });

            myClassInstance.a().then(result => {
                echo(`Test #${index} - Success async in a class #3 called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error async in a class #3 called with err = ${err}`);
            });

            myClassInstance['0']().then(result => {
                echo(`Test #${index} - Success async in a class #4 called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error async in a class #4 called with err = ${err}`);
            });

            myClassInstance['3.14']().then(result => {
                echo(`Test #${index} - Success async in a class #5 called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error async in a class #5 called with err = ${err}`);
            });

            myClassInstance.else().then(result => {
                echo(`Test #${index} - Success async in a class #6 called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error async in a class #6 called with err = ${err}`);
            });

            MyClass.staticAsyncMethod(10).then(result => {
                echo(`Test #${index} - Success async in a class #7 called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error async in a class #7 called with err = ${err}`);
            });

            var result = mySecondClassInstance.async(10);
            echo(`Test #${index} - Success async in a class #8 called with result = '${result}'`);

            var result = MyThirdClass.async(10);
            echo(`Test #${index} - Success async in a class #9 called with result = '${result}'`);

            myFourthClassInstance.foo(10).then(result => {
                echo(`Test #${index} - Success async in a class #10 called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error async in a class #10 called with err = ${err}`);
            });
        }
    },
    {
        name: "Await in an async function",
        body: function (index) {
            async function asyncMethod(val, factor) {
                val = val * factor;
                if (val > 0)
                     val = await asyncMethod(val, -1);
                return val;
            }

            function await(x) {
                return x;
            }

            async function secondAsyncMethod(x) {
                return await(x);
            }

            function rejectedPromiseMethod() {
                return new Promise(function (resolve, reject) {
                    reject(Error('My Error'));
                });
            }

            async function rejectAwaitMethod() {
                return await rejectedPromiseMethod();
            }

            async function asyncThrowingMethod() {
                throw 32;
            }

            async function throwAwaitMethod() {
                return await asyncThrowingMethod();
            }

            asyncMethod(2, 2).then(result => {
                echo(`Test #${index} - Success await in an async function #1 called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error await in an async function #1 called with err = ${err}`);
            });

            secondAsyncMethod(2).then(result => {
                echo(`Test #${index} - Success await in an async function #2 called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error await in an async function #2 called with err = ${err}`);
            });

            rejectAwaitMethod(2).then(result => {
                echo(`Test #${index} - Failed await in an async function doesn't catch a rejected Promise. Result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Success await in an async function catch a rejected Promise in 'err'. Error = '${err}'`);
            });

            throwAwaitMethod(2).then(result => {
                echo(`Test #${index} - Failed await in an async function doesn't catch an error. Result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Success await in an async function catch an error in 'err'. Error = '${err}'`);
            });
        }
    },
    {
        name: "Await keyword with a lambda expressions",
        body: function (index) {
            {
                async function asyncMethod(x, y, z) {
                    var lambdaExp = async(a, b, c) => a * b * c; 
                    var lambdaResult = await lambdaExp(x, y, z);
                    return lambdaResult;
                }

                asyncMethod(5, 5, 5).then(result => {
                    echo(`Test #${index} - Success await keyword with a lambda expressions #1 called with result = '${result}'`);
                }, err => {
                    echo(`Test #${index} - Error await keyword with a lambda expressions #1 called with err = ${err}`);
                });
            };
            {
                async function await(lambda, x, y, z) {
                    return await lambda(x, y, z);
                }

                await(async(a, b, c) => a + b + c, 10, 20, 30).then(result => {
                    echo(`Test #${index} - Success await keyword with a lambda expressions #1 called with result = '${result}'`);
                }, err => {
                    echo(`Test #${index} - Error await keyword with a lambda expressions #1 called with err = ${err}`);
                });
            };
        }
    },
    {
        name: "Async function with default arguments's value",
        body: function (index) {
            {
                function thrower() {
                    throw "expected error"
                }

                async function asyncMethod(argument = thrower()) {
                    return true;
                }

                async function secondAsyncMethod(argument = false) {
                    return true;
                }

                asyncMethod(true).then(result => {
                    echo(`Test #${index} - Success async function with default arguments's value overwritten #1 called with result = '${result}'`);
                }, err => {
                    echo(`Test #${index} - Error async function with default arguments's value overwritten #1 called with err = ${err}`);
                });

                // TODO:[aneeshd] Need to fix the default parameter evaluation order for both async functions and generators
                asyncMethod().then(result => {
                    echo(`Test #${index} - Failed async function with default arguments's value has not been rejected as expected #2 called with result = '${result}'`);
                }, err => {
                    echo(`Test #${index} - Success async function with default arguments's value has been rejected as expected by 'err' #2 called with err = '${err}'`);
                });

                secondAsyncMethod().then(result => {
                    echo(`Test #${index} - Success async function with default arguments's value #3 called with result = '${result}'`);
                }, err => {
                    echo(`Test #${index} - Error async function with default arguments's value #3 called with err = ${err}`);
                });
            };
        }
    },
    {
        name: "Promise in an Async function",
        body: function (index) {
            {
                async function asyncMethodResolved() {
                    let p = new Promise(function (resolve, reject) {
                        resolve("resolved");
                    });

                    return p.then(function (result) {
                        return result;
                    });
                }

                async function asyncMethodResolvedWithAwait() {
                    let p = new Promise(function (resolve, reject) {
                        resolve("resolved");
                    });

                    return await p;
                }

                async function asyncMethodRejected() {
                    let p = new Promise(function (resolve, reject) {
                        reject("rejected");
                    });

                    return p.then(function (result) {
                        return result;
                    });
                }

                asyncMethodResolved().then(result => {
                    echo(`Test #${index} - Success resolved promise in an async function #1 called with result = '${result}'`);
                }, err => {
                    echo(`Test #${index} - Error resolved promise in an async function #1 called with err = ${err}`);
                });

                asyncMethodResolvedWithAwait().then(result => {
                    echo(`Test #${index} - Success resolved promise in an async function #2 called with result = '${result}'`);
                }, err => {
                    echo(`Test #${index} - Error resolved promise in an async function #2 called with err = ${err}`);
                });

                asyncMethodRejected().then(result => {
                    echo(`Test #${index} - Failed promise in an async function has not been rejected as expected #3 called with result = '${result}'`);
                }, err => {
                    echo(`Test #${index} - Success promise in an async function has been rejected as expected by 'err' #3 called with err = '${err}'`);
                });
            };
        }
    },
    {
        name: "%AsyncFunction% constructor creates async functions analogous to Function constructor",
        body: function (index) {
            var AsyncFunction = Object.getPrototypeOf(async function () { }).constructor;

            var af = new AsyncFunction('return await Promise.resolve(0);');

            af().then(result => {
                echo(`Test #${index} - Success %AsyncFunction% created async function #1 called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error %AsyncFunction% created async function #1 called with err = ${err}`);
            });

            af = new AsyncFunction('a', 'b', 'c', 'a = await a; b = await b; c = await c; return a + b + c;');

            af(Promise.resolve(1), Promise.resolve(2), Promise.resolve(3)).then(result => {
                echo(`Test #${index} - Success %AsyncFunction% created async function #2 called with result = '${result}'`);
            }, err => {
                echo(`Test #${index} - Error %AsyncFunction% created async function #2 called with err = ${err}`);
            });
        }
    },
    {
        name: "local variables with same names as formal parameters have proper redeclaration semantics (non-error cases, var and function)",
        body: function (index) {
            async function af1(x) { var y = x; var x = 'b'; return y + x; }

            af1('a').then(result => {
                if (result === 'ab') {
                    echo(`Test #${index} - Success inner var x overwrote formal parameter x only after the declaration statement`);
                } else {
                    echo(`Test #${index} - Failure x appears to have an unexpected value x = '${result}'`);
                }
            }, err => {
                echo(`Test #${index} - Error var redeclaration with err = ${err}`);
            });

            async function af2(x) { var xx = x(); function x() { return 'afx'; } return xx; }

            af2(function () { return ''; }).then(result => {
                if (result === 'afx') {
                    echo(`Test #${index} - Success inner function x() overwrote formal parameter x`);
                } else {
                    echo(`Test #${index} - Failure x appears not assigned with inner function x(), x = '${result}'`);
                }
            }, err => {
                echo(`Test #${index} - Error err = ${err}`);
            });
        }
    },
    {
        name: "this value in async functions behaves like it does in normal functions",
        body: function (index) {
            async function af() {
                return this;
            }

            af.call(5).then(result => {
                if (result == 5) {
                    echo(`Test #${index} - Success this value set to 5`);
                } else {
                    echo(`Test #${index} - Failure this value is not 5, this = '${result}'`);
                }
            }, err => {
                echo(`Test #${index} - Error err = ${err}`);
            });

            var o = {
                af: af,
                b: "abc"
            };

            o.af().then(result => {
                if (result.af === af && result.b === "abc") {
                    echo(`Test #${index} - Success this value set to { af: af, b: "abc" }`);
                } else {
                    echo(`Test #${index} - Failure this value set to something else, this = '${result}'`);
                }
            }, err => {
                echo(`Test #${index} - Error err = ${err}`);
            });
        }
    },
    {
        name: "arguments value in async functions behaves like it does in normal functions",
        body: function (index) {
            async function af() {
                return arguments[0] + arguments[1];
            }

            af('a', 'b').then(result => {
                if (result === 'ab') {
                    echo(`Test #${index} - Success result is 'ab' from arguments 'a' + 'b'`);
                } else {
                    echo(`Test #${index} - Failure result is not 'ab', result = '${result}'`);
                }
            }, err => {
                echo(`Test #${index} - Error err = ${err}`);
            });
        }
    },
    {
        name: "super value in async methods behaves like it does in normal methods",
        body: function (index) {
            class B {
                af() {
                    return "base";
                }
            }

            class C extends B {
                async af() {
                    return super.af() + " derived";
                }
            }

            var c = new C();

            c.af().then(result => {
                if (result === 'base derived') {
                    echo(`Test #${index} - Success result is 'base derived' from derived method call`);
                } else {
                    echo(`Test #${index} - Failure result is not 'base derived', result = '${result}'`);
                }
            }, err => {
                echo(`Test #${index} - Error err = ${err}`);
            });
        }
    },
    {
        name:"Async function with formal captured in a lambda",
        body: function (index) {
            async function af(d = 1) {
                return () => d;
            }

            af().then(result => {
                if (result() === 1) {
                    print(`Test #${index} - Success lambda returns 1 when no arguments passed`);
                } else {
                    print(`Test #${index} - Failure result is not 1, result = '${result()}'`);
                }
            }, err => {
                print(`Test #${index} - Error err = ${err}`);
            });  
        }
    },
    {
        name:"Async function with formal captured in a nested function",
        body: function (index) {
            async function af(d = 1) {
                return function () { return d; };
            }

            af().then(result => {
                if (result() === 1) {
                    print(`Test #${index} - Success nested function returns 1 when no arguments passed`);
                } else {
                    print(`Test #${index} - Failure result is not 1, result = '${result()}'`);
                }
            }, err => {
                print(`Test #${index} - Error err = ${err}`);
            });  
        }
    },
    {
        name:"Async function with formal captured in eval",
        body: function (index) {
            async function af(d = 1) {
                return eval("d");
            }

            af().then(result => {
                if (result === 1) {
                    print(`Test #${index} - Success eval returns 1 when no arguments passed`);
                } else {
                    print(`Test #${index} - Failure result is not 1, result = '${result}'`);
                }
            }, err => {
                print(`Test #${index} - Error err = ${err}`);
            });  
        }
    },
    {
        name: "Async function with formal capturing in param scope",
        body: function (index) {
            async function af1(a, b = () => a, c = b) {
                function b() {
                    return a;
                }
                var a = 2;
                return [b, c];
            }

            af1(1).then(result => {
                if (result[0]() === 2) {
                    echo(`Test #${index} - Success inner function declaration captures the body variable`);
                } else {
                    echo(`Test #${index} - Failure a appears to have an unexpected value a = '${result}'`);
                }
                if (result[1]() === 1) {
                    echo(`Test #${index} - Success function defined in the param scope captures the param scope variable`);
                } else {
                    echo(`Test #${index} - Failure a appears to have an unexpected value in the param scope function a = '${result}'`);
                }
            }, err => {
                echo(`Test #${index} - Error in split scope with err = ${err}`);
            });
        }
    },
    {
        name: "Async function with formal capturing in param scope with eval in the body",
        body: function (index) {
            async function af1(a, b = () => a, c = b) {
                function b() {
                    return a;
                }
                var a = 2;
                return eval("[b, c]");
            }

            af1(1).then(result => {
                if (result[0]() === 2) {
                    echo(`Test #${index} - Success inner function declaration captures the body variable with eval in the body`);
                } else {
                    echo(`Test #${index} - Failure a appears to have an unexpected value a = '${result}'`);
                }
                if (result[1]() === 1) {
                    echo(`Test #${index} - Success function defined in the param scope captures the param scope variable with eval in the body`);
                } else {
                    echo(`Test #${index} - Failure a appears to have an unexpected value in the param scope function a = '${result}'`);
                }
            }, err => {
                echo(`Test #${index} - Error in split scope with eval in the body with err = ${err}`);
            });
        }
    },
    {
        name: "Async function with duplicate variable declaration in the body with eval",
        body: function (index) {
            async function af1(a, b) {
                var a = 10;
                return eval("a + b");
            }

            af1(1, 2).then(result => {
                if (result === 12) {
                    echo(`Test #${index} - Success inner variable declaration shadows the formal`);
                } else {
                    echo(`Test #${index} - Failure sum appears to have an unexpected value sum = '${result}'`);
                }
            }, err => {
                echo(`Test #${index} - Error in variable redeclaration with eval with err = ${err}`);
            });
        }
    },
    {
        name: "Async function with duplicate variable declaration in the body with child having eval",
        body: function (index) {
            async function af1(a, b) {
                var a = 10;
                return function () { return eval("a + b"); };
            }

            af1(1, 2).then(result => {
                if (result() === 12) {
                    echo(`Test #${index} - Success inner variable declaration shadows the formal with eval in child function`);
                } else {
                    echo(`Test #${index} - Failure sum appears to have an unexpected value sum = '${result}'`);
                }
            }, err => {
                echo(`Test #${index} - Error in variable redeclaration with eval with err = ${err}`);
            });
        }
    },
    {
        name: "Async function with more than one await",
        body: function (index) {
            async function af1() {
                return 1;
            }

            async function af2() {
                return 2;
            }

            async function af3() {
                return await af1() + await af2();
            }
            af3().then(result => {
                if (result === 3) {
                    echo(`Test #${index} - Success functions completes both await calls`);
                } else {
                    echo(`Test #${index} - Failed function failed to complete both await calls and returned ${result}`);
                }
            }, err => {
                echo(`Test #${index} - Error in multiple awaits in a function err = ${err}`);
            });
        }
    },
    {
        name: "Async function with more than one await with branching",
        body: function (index) {
            async function af1() {
                return 1;
            }

            async function af2() {
                return 2;
            }

            async function af3(a) {
                return a ? await af1() : await af2();
            }

            af3(1).then(result => {
                if (result === 1) {
                    echo(`Test #${index} - Success functions completes the first await call`);
                } else {
                    echo(`Test #${index} - Failed function failed to complete the first await call and returned ${result}`);
                }
            }, err => {
                echo(`Test #${index} - Error in multiple awaits with branching in a function err = ${err}`);
            });
            
            af3().then(result => {
                if (result === 2) {
                    echo(`Test #${index} - Success functions completes the second await call`);
                } else {
                    echo(`Test #${index} - Failed function failed to complete the second await call and returned ${result}`);
                }
            }, err => {
                echo(`Test #${index} - Error in multiple awaits with branching in a function err = ${err}`);
            });
        }
    },
    {
        name: "Async function with an exception in an await expression",
        body: function (index) {
            var obj =  { x : 1 };
            async function af1() {
                throw obj;
            }

            async function af2() {
                echo(`Failed : This function was not expected to be executed`);
            }

            async function af3() {
                return await af1() + await af2();
            }
            af3().then(result => {
                echo(`Test #${index} - Error an expected exception does not seem to be thrown`);
            }, err => {
                if (err === obj) {
                    echo(`Test #${index} - Success caught the expected exception`);
                } else {
                    echo(`Test #${index} - Error an unexpected exception was thrown = ${err}`);
                }
            });
        }
    },
    {
        name: "Async functions throws on an await",
        body: function (index) {
            var obj =  { x : 1 };
            async function af1() {
                throw obj;
            }

            async function af2() {
                echo(`Test #${index} Failed This function was not expected to be executed`);
            }

            async function af3() {
                return await af1() + await af2();
            }

            af3().then(result => {
                print(`Test #${index} Failed an expected exception does not seem to be thrown`);
            }, err => {
                if (err === obj) {
                    print(`Test #${index} - Success caught the expected exception`);
                } else {
                    print(`Test #${index} - Failed an unexpected exception was thrown = ${err}`);
                }
            });
        }  
    },
    {
        name: "Awaiting a function with multiple awaits",
        body: function (index) {
            async function af1(a, b) {
                return await af2();
                
                async function af2() {
                    a = await a * a;
                    b = await b * b;

                    return a + b;
                }
            }

            af1(1, 2).then(result => {
                if (result === 5) {
                    echo(`Test #${index} - Success Multiple awaits in the inner function completed`);
                } else {
                    echo(`Test #${index} - Failed function failed to complete the multiple awaits in the inner function ${result}`);
                }
            }, err => {
                echo(`Test #${index} - Error in multiple awaits in an inner function err = ${err}`);
            });
        }
    },
    {
        name: "Async function with nested try-catch in the body",
        body: function (index) {
            async function af1() {
                throw 42;
            }

            async function af2() {
                try {
                    try {
                        await af1();
                    } catch (e) {
                        echo(`Test #${index} - Success Caught the expected exception inside the inner catch in async body`);
                        throw e;
                    }
                } catch (e) {
                    echo(`Test #${index} - Success Caught the expected exception inside catch in async body`);
                    throw e;
                }
                echo(`Test #${index} - Failed Didn't throw an expected exception`);
            }

            af2().then(result => {
                echo(`Test #${index} - Failed an the expected was not thrown`);
            }, err => {
                if (err.x === obj.x) {
                    echo(`Test #${index} - Success Caught the expected exception in the promise`);
                } else {
                    echo(`Test #${index} - Failed Caught an unexpected exception in the promise ${error}`);
                }
            });
        }
    },
    {
        name: "Async function with try-catch and try-finally in the body",
        body: function (index) {
            async function af1() {
                throw 42;
            }

            async function af2() {
                try {
                    try {
                        await af1();
                    } catch (e) {
                        echo(`Test #${index} - Success Caught the expected exception inside the inner catch in async body`);
                        throw e;
                    }
                } finally {
                    echo(`Test #${index} - Success finally block is executed in async body`);
                }
                echo(`Test #${index} - Failed Didn't throw an expected exception`);
            }

            af2().then(result => {
                echo(`Test #${index} - Failed an the expected was not thrown`);
            }, err => {
                if (err.x === obj.x) {
                    echo(`Test #${index} - Success Caught the expected exception in the promise`);
                } else {
                    echo(`Test #${index} - Failed Caught an unexpected exception in the promise ${error}`);
                }
            });
        }
    },
    {
        name: "Async function and with",
        body: function (index) {
            var obj = {
                async af() {
                    this.b = await this.a + 10;
                    return this; 
                },
                a : 1,
                b : 0
            };

            async function af(x) {
                var x = 0;
                with (obj) {
                    x = await af();
                }

                return x;
            }

            af().then(result => {
                if (result.a === 1 && result.b === 11) {
                    echo(`Test #${index} - Success functions call inside with returns the right this object`);
                } else {
                    echo(`Test #${index} - Failed function failed to execute with inside an async function got ${result}`);
                }
            }, err => {
                echo(`Test #${index} - Error in with construct inside an async method err = ${err}`);
            });
        }
    },
    {
        name: "Async and arguments.callee",
        body: function (index) {
            async function asyncMethod() {
                return arguments.callee;
            }
            asyncMethod().then(result => {
                if (result === asyncMethod) {
                    echo(`Test #${index} - Success async function and arguments.callee`);
                } else {
                    echo(`Test #${index} - Failed async function and arguments.callee called with result = '${result}'`);
                }
            }, err => {
                echo(`Test #${index} - Error async function and arguments.callee called with err = ${err}`);
            });
        }
    },
    {
        name: "Async and arguments.caller",
        body: function (index) {
            var func = function () {
                return func.caller;
            }
            async function asyncMethod(flag, value) {
                if (!flag) {
                    return await func();
                }
                return value * value;
            }

            asyncMethod().then(
                result => {
                    if (result === asyncMethod) {
                        echo(`Test #${index} - Success async function returned through caller property is the same as the original async function`);
                    } else {
                        echo(`Test #${index} - Failed async function returned through the caller property is not the same as the original async function = ${result}`);
                    }
                    result(true, 10).then(
                        r => {
                            if (r === 100) {
                                echo(`Test #${index} - Success async function returned through caller property behaves the same way as the original async function`);
                            } else {
                                echo(`Test #${index} - Failed async function returned through caller property behaves different from the original async function with value = ${r}`);
                            }
                        },
                        e => {
                            echo(`Test #${index} - Failed while trying to execute the async function returned through caller property with err = ${e}`);
                        }
                    );
                },
                error => {
                    echo(`Test #${index} - Failed while trying to retrieve the async function through caller property with err = ${error}`);
                }
            )
        }
    },
    {
        name: "Async and split scope",
        body: function () {
            async function asyncMethod1(b) {
                return b() + 100;
            }
            async function asynMethod2(a = 10, b = () => a) {
                if (a === 10) {
                    echo(`Test #${index} - Success initial value of the formal is the same as the default param value`);
                } else {
                    echo(`Test #${index} - Failed initial value of the formal is not the same as the default param value, expected 10, result = ${a}`);
                }
                a = await asyncMethod1(b);
                if (a === 110) {
                    echo(`Test #${index} - Success updated value of the formal is the same as the value returned from the second async function`);
                } else {
                    echo(`Test #${index} - Failed updated value of the formal is not the same as the value returned from the second async function, expected 110, result = ${a}`);
                }
                return b;
            }
            asynMethod2().then(
                result => {
                    if (result() === 110) {
                        echo(`Test #${index} - Success value returned through await is assigned to the formal`);
                    } else {
                        echo(`Test #${index} - Failed value returned through the await is different from the expected 110, result = ${result()}`);
                    }
                },
                error => {
                    echo(`Test #${index} - Failed error while trying to return through the await in a split scope function, expected 100, error = ${error}`);
                }
            );

            async function asyncMethod3(b) {
                return b() + 100;
            }
            async function asynMethod4(a = 10, b = () => a) {
                if (a === 10) {
                    echo(`Test #${index} - Success initial value of the body symbol is the same as the default param value`);
                } else {
                    echo(`Test #${index} - Failed initial value of the body symbol is not the same as the default param value, expected 10, result = ${a}`);
                }
                var a = await asyncMethod3(b);
                if (a === 110) {
                    echo(`Test #${index} - Success updated value of the body symbol is the same as the value returned from the second async function`);
                } else {
                    echo(`Test #${index} - Failed updated value of the body symbol is not the same as the value returned from the second async function, expected 110, result = ${a}`);
                }
                return b;
            }
            asynMethod4().then(
                result => {
                    if (result() === 10) {
                        echo(`Test #${index} - Success value returned through await is not assigned to the formal`);
                    } else {
                        echo(`Test #${index} - Failed value of the formal is different from the expected 10, result = ${result()}`);
                    }
                },
                error => {
                    echo(`Test #${index} - Failed error while trying to return through the await in a split scope function with duplicate symbol in the body, expected 100, error = ${error}`);
                }
            );
        }
    }
];

var index = 1;

function runTest(test) {
    echo('Executing test #' + index + ' - ' + test.name);

    try {
        test.body(index);
    } catch(e) {
        echo('Caught exception: ' + e);
    }

    index++;
}

tests.forEach(runTest);

echo('\nCompletion Results:');
