//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

objArgs = WScript.Arguments;

function emitToken(token, d, indent) {
    r = "";
    if (d) {
        r += indent + "p += " + d + ";\r\n";
    }
    r += indent + "token = " + token.tk + ";\r\n";
    r += indent + "goto LKeyword;\r\n";

    return r;
}

function noMoreBranches(token) {
    for (var c = token; c.length; c = c[0]) {
        if (c.length > 1) return false;
        if (c.length == 1 && c.tk) return false;
    }
    return true;
}

function emit(token, d, indent) {
    var r = "";
    if (d < 0) throw "d must be gte 0.";

    if (noMoreBranches(token)) {

        var count = d;
        for (var c = token; c.length; c = c[0]) {
            count++;
        }

        var emitIf = token.length;
        if (emitIf) {
            r += indent + "if (";
            for (var c = token; c.length; c = c[0]) {
                r += "p[" + d++ + "] == '" + c[0].char.substring(1) + "'";
                if (c[0].length) {
                    r += " && ";
                }
            }

            r += ")\r\n";
            r += indent + "{\r\n";
        }
        r += emitToken(c, d, indent);
        if (emitIf) {
            r += indent + "}\r\n";
        }
    } else if (token.length >= 1) {

        // Get the count of single-child nodes until the next branch.
        var count = 0;
        for (var c = token; c.length; c = c[0]) {
            if (c.length > 1) break;
            if (c.length == 1 && c.tk) break;
            count++;
        }

        if (token.length && count > 1) {

            r += indent + "if ("

            var i = 0;//, d2 = d;
            for (var c = token; c.length && i < count - 1; c = c[0]) {
                r += "p[" + d++ + "] == '" + c[0].char.substring(1) + "'";
                i++;
                if (c[0].length && i < count - 1) {
                    r += " && ";
                }
            }
            token = c;

            r += ")\r\n";
            r += indent + "{\r\n";

            indent += "    ";
        }

        r += indent + "switch(p[" + d + "])\r\n";
        r += indent + "{\r\n";
        
        for (var i = 0; i < token.length; i++) {

            var tk = token[i];

            r += indent + "case '" + tk.char.substring(1) + "':\r\n";

            r += emit(tk, d + 1, indent + "    ");

            if (tk.tk && tk.length) {
                r += emitToken(tk, d + 1, indent + "    ");
            }

            r += indent + "    break;\r\n";
        }

        r += indent + "}\r\n";
        indent = indent.substring(4);
        
        if (token.length && count > 1) {
            r += indent + "}\r\n";
        }

    }
    else {
        r += indent + "if (p[" + d + "] == '" + token[0].char.substring(1) + "')\r\n";
        r += indent + "{\r\n";
        r += emit(token[0], d + 1, indent + "    ");
        r += indent + "}\r\n";
    }
    return r;
}

if (objArgs.length != 1 && objArgs.length != 2) {
    WScript.Echo("Supply the header file name and optional output file");
}
else {
    var fso = new ActiveXObject("Scripting.FileSystemObject");
    var file = fso.OpenTextFile(objArgs(0), 1);
    var text = file.ReadAll();
    file.Close();

    var reg = /WASM_KEYWORD\(([A-Z0-9\_]+)\s*,\s*([a-z0-9\_]+)\)/g;
    var t = [];
    text.replace(reg, function (a, p1, p2, offset) {
        WScript.Echo("Adding case for: " + p1 + " - " + p2);
        t.push({ tk: "wtk" + p1, word: p2 });
    });

    reg = /WASM_[A-Z]*TYPE\(([A-Z0-9\_]+)\s*,\s*([a-z0-9]+)\)/g;
    text.replace(reg, function (a, p1, p2, offset) {
        WScript.Echo("Adding case for: " + p1 + " - " + p2);
        t.push({ tk: "wtk" + p1, word: p2 });
    });

    reg = /WASM_KEYWORD[\_A-Z]*_F\(([A-Z0-9\_]+)\s*,\s*([a-z0-9]+)[,\sA-z0-9]*\)/g;
    text.replace(reg, function (a, p1, p2, offset) {
        WScript.Echo("Adding F32 case for: " + p1 + " - " + p2);
        t.push({ tk: "wtk" + p1 + "_F32", word: "f32." + p2 });
    });
    reg = /WASM_KEYWORD[\_A-Z]*_D\(([A-Z0-9\_]+)\s*,\s*([a-z0-9]+)[,\sA-z0-9]*\)/g;
    text.replace(reg, function (a, p1, p2, offset) {
        WScript.Echo("Adding F64 case for: " + p1 + " - " + p2);
        t.push({ tk: "wtk" + p1 + "_F64", word: "f64." + p2 });
    });
    reg = /WASM_KEYWORD[\_A-Z]*_I\(([A-Z0-9\_]+)\s*,\s*([a-z0-9]+)[,\sA-z0-9]*\)/g;
    text.replace(reg, function (a, p1, p2, offset) {
        WScript.Echo("Adding I32 case for: " + p1 + " - " + p2);
        t.push({ tk: "wtk" + p1 + "_I32", word: "i32." + p2 });
    });
    reg = /WASM_KEYWORD[\_A-Z]*_FD\(([A-Z0-9\_]+)\s*,\s*([a-z0-9]+)[,\sA-z0-9]*\)/g;
    text.replace(reg, function (a, p1, p2, offset) {
        WScript.Echo("Adding F32/F64 case for: " + p1 + " - " + p2);
        t.push({ tk: "wtk" + p1 + "_F32", word: "f32." + p2 });
        t.push({ tk: "wtk" + p1 + "_F64", word: "f64." + p2 });
    });
    reg = /WASM_KEYWORD[\_A-Z]*_FDI\(([A-Z0-9\_]+)\s*,\s*([a-z0-9]+)[,\sA-z0-9]*\)/g;
    text.replace(reg, function (a, p1, p2, offset) {
        WScript.Echo("Adding F32/F63/I32 case for: " + p1 + " - " + p2);
        t.push({ tk: "wtk" + p1 + "_F32", word: "f32." + p2 });
        t.push({ tk: "wtk" + p1 + "_F64", word: "f64." + p2 });
        t.push({ tk: "wtk" + p1 + "_I32", word: "i32." + p2 });
    });

    var tokens = [];
    var counter = 0;
    WScript.Echo("Keyword count: " + t.length);
    for (var i = 0; i < t.length; i++) {
        var token = t[i];
        var current = tokens;
        WScript.Echo("Token length:" + token.word.length);

        for (var j = 0; j < token.word.length; j++) {
            l = '$' + token.word.substring(j, j + 1);
            WScript.Echo("Token: " + l);
            var n = current[l];
            if (n)
                current = n;
            else {
                var nt = [];
                nt.char = l;
                current[l] = nt;
                current.push(nt);
                current = nt;
            }
            counter++;
        }
        current.tk = token.tk;
    }


    var indent = "    ";
    var r = "//-------------------------------------------------------------------------------------------------------\r\n";
    r += "// Copyright (C) Microsoft. All rights reserved.\r\n";
    r += "// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.\r\n";
    r += "//-------------------------------------------------------------------------------------------------------\r\n\r\n";

    r += "// GENERATED FILE, DO NOT HAND MODIFIY\r\n";
    r += "// Generated with the following command line: cscript SExprScan.js " + objArgs(0) + " " + objArgs(1) + "\r\n";
    r += "// This should be regenerated whenever the keywords change\r\n\r\n";
    // Generate the reserved word recognizer
    for (var i = 0; i < tokens.length; i++) {
        var tk = tokens[i];
        r += indent + "case '" + tk.char.substring(1) + "':\r\n";
        var simple = tk.length == 1 && noMoreBranches(tk);
        r += emit(tk, 0, indent + "    ");
        r += indent + "    goto LError;\r\n";
    }
    r += "\r\n";

    if (objArgs.length == 2) {
        var outfile = fso.CreateTextFile(objArgs(1), true);
        outfile.Write(r);
        outfile.Close();
    }
    else
        WScript.Echo(r);
}
