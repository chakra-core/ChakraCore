"use strict";

WScript.LoadScriptFile("ast.js");
WScript.LoadScriptFile("basic.js");
WScript.LoadScriptFile("caseless_map.js");
WScript.LoadScriptFile("lexer.js");
WScript.LoadScriptFile("number.js");
WScript.LoadScriptFile("parser.js");
WScript.LoadScriptFile("random.js");
WScript.LoadScriptFile("state.js");
WScript.LoadScriptFile("util.js");

WScript.LoadScriptFile("benchmark.js");

let result = runBenchmark();
print("That took " + result + " ms.");
