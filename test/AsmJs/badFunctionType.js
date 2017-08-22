//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
const simpleAsmDef = `
function x(v) {
  v = v | 0;
  return v | 0;
}
return x;`;

const tests = [{
  name: "kFunctionIsAccessor",
  test() { eval(`
    var o = {
      set m(stdlib) {
        "use asm"
        var I32 = stdlib.Int32Array;
        ${simpleAsmDef}
      }
    }
    o.m = 5;
  `);}
}, {
  name: "kFunctionIsClassConstructor & kFunctionIsBaseClassConstructor",
  test() { eval(`
    class BaseClass {}
    class MyClass extends BaseClass {
      f(a,b,c,d,e) {
        print(a);
      }

      constructor() {
        "use asm";
        ${simpleAsmDef}
      }
    }
    var x = new MyClass("df");
    x(3);
  `);
  }
}, {
  name: "kFunctionIsStaticMember",
  test() { eval(`
    class MyClass {
      static asmModuleClassMember() {
        "use asm";
        ${simpleAsmDef}
      }
    }
    MyClass.asmModuleClassMember()(3);
  `);
  }
}, {
  name: "kFunctionIsClassMember",
  test() { eval(`
    class MyClass {
      asmModuleClassMember() {
        "use asm";
        ${simpleAsmDef}
      }
    }
    var v = new MyClass("df")
    v.asmModuleClassMember()(3);
  `)
  }
}, {
  name: "kFunctionIsMethod",
  test() {eval(`
    var obj = {
      asmModuleMethod() {
        "use asm";
        ${simpleAsmDef}
      }
    }
    obj.asmModuleMethod()(3);
  `)
  }
}, {
  name: "kFunctionCallsEval",
  test() { eval(`
    function asmModuleEval() {
      "use asm";
      eval("function bar() {console.log('in bar')}");
      ${simpleAsmDef}
    }
    asmModuleEval()(3);
  `)
  }
}, {
  name: "kFunctionChildCallsEval",
  test() { eval(`
    function asmModuleChildEval() {
      "use asm";
      function x(v) {
        v = v | 0;
        eval("function bar() {console.log('in bar')}");
        return v | 0;
      }
      return x;
    }
    asmModuleChildEval()();
  `)
  }
}, {
  name: "kFunctionUsesArguments",
  test() { eval(`
    function asmModuleArguments() {
      "use asm";
      arguments;
      ${simpleAsmDef}
    }
    asmModuleArguments();
  `)
  }
}, {
  name: "kFunctionHasWithStmt",
  test() { eval(`
    function asmModuleWith() {
      "use asm";
      with(5) {
        ${simpleAsmDef}
      }
    }
    asmModuleWith()(3);
  `)
  }
}, {
  name: "kFunctionIsLambda",
  test() { eval(`
    const asmModuleLambda = () => {
      "use asm";
      ${simpleAsmDef}
    }
    asmModuleLambda()(3);
  `)
  }
}, {
  name: "kFunctionHasNonSimpleParameterList: Module destructuring",
  test() { eval(`
    function asmModuleDestructuring({Math: {sin}}) {
      "use asm";
      function x(v) {
        v = +v;
        return +sin(+v);
      }
      return x;
    }
    asmModuleDestructuring({Math})(3);
  `)
  }
}, {
  name: "kFunctionHasNonSimpleParameterList: function destructuring",
  test() { eval(`
    function asmModuleDestructuringChild() {
      "use asm";
      function x({v}) {
        v = +v;
        return +v;
      }
      return x;
    }
    asmModuleDestructuringChild()({v: 3});
  `)
  }
}, {
  name: "kFunctionHasNonSimpleParameterList: rest",
  test() { eval(`
    function asmModuleRest(...rest) {
      "use asm";
      ${simpleAsmDef}
    }
    asmModuleRest()(3);
  `)
  }
}, {
  name: "kFunctionHasDefaultArguments",
  test() { eval(`
    function asmModuleDefault(stdlib = {Math}) {
      "use asm";
      var sin = stdlib.Math.sin;
      function x(v) {
        v = +v;
        return +(+sin(+v));
      }
      return x;
    }
    asmModuleDefault()(3);
  `)
  }
}, {
  name: "kFunctionIsModule",
  test() { WScript.LoadModule(`
    "use asm"
    export function x(v) {
      v = v | 0;
      return v | 0;
    }`)
  }
}, {
  name: "asm.js function in Module",
  test() {
    WScript.LoadModule(`
    function AsmDefaultExport() {
      "use asm"
      ${simpleAsmDef}
    }`)
  }
}, {
  name: "kFunctionIsDefaultModuleExport",
  test() {
    WScript.LoadModule(`
    export default function AsmDefaultExport() {
      "use asm"
      ${simpleAsmDef}
    }`)
  }
}, {
  name: "kFunctionHasSuperReference",
  test() { eval(`
    var obj = {
      asmModuleSuper() {
        "use asm";
        var a = super.toString;
        ${simpleAsmDef}
      }
    }
    obj.asmModuleSuper()(3);
  `)
  }
}, {
  name: "kFunctionIsGenerator",
  test() { eval(`
    function* asmModuleGenerator() {
      "use asm";
      ${simpleAsmDef}
    }
    asmModuleGenerator().next().value(3);
  `)
  }
}, {
  name: "kFunctionIsAsync",
  test() { eval(`
    async function asmModuleAsync() {
      "use asm";
      ${simpleAsmDef}
    }
    asmModuleAsync().then(f => f(3));
  `)
  }
}];

const start = WScript.Arguments[0] || 0;
for (let i = start; i < tests.length; ++i) {
  const {disabled, name, test} = tests[i];
  console.log(`\nRunning test ${i}: ${name}`);
  if (disabled) {
    console.log("Warning test disabled");
  } else {
    test();
  }
}