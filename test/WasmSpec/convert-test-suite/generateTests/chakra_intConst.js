//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const {parse} = require("sexpr-plus");
const fs = require("fs-extra");
const path = require("path");

function getNodeType(node) {
  if (
    node.type === "list" &&
    node.content.length > 0 &&
    node.content[0].type === "atom"
  ) {
    return node.content[0].content;
  }
}

function nodeToString(node) {
  if (Array.isArray(node)) {
    return node.map(nodeToString)
      .filter(s => s.length > 0)
      .join(" ");
  }

  switch(node.type) {
    case "atom": return node.content;
    case "string": return `"${node.content}"`;
    case "list": {
      const listString = node.content.map(nodeToString).filter(s => s.length > 0).join(" ");
      return `(${listString})`;
    }
  }
  return "";
}

function getAssertInfo(node) {
  const fnName = node.content[1].content[1].content;
  const args = node.content[1].content.slice(2);
  const returnInfo = node.content[2];
  return {
    fnName,
    args,
    returnInfo,
  };
}

function extractModule(moduleNode) {
  const module = {exports: {}};
  let iFunc = 0;
  for(const node of moduleNode.content.slice(1)) {
    const nodeType = getNodeType(node);
    switch(nodeType) {
      case "func":
        const nItems = node.content.length;
        const func = {
          params: [],
          return: undefined,
          export: "" + iFunc,
          body: node.content[nItems - 1].content,
        };
        for(const subnode of node.content.slice(1, nItems - 1)) {
          switch(getNodeType(subnode)) {
            case "export": func.export = subnode.content[1].content; break;
            case "param": func.params.push(subnode); break;
            case "result": func.return = subnode; break;
          }
        }
        module.exports[func.export] = func;
        iFunc++;
        break;
    }
  }
  return module;
}

module.exports = function(src, suite) {
  const originalI64Wast = path.resolve(suite, "core", `${src}.wast`);
  const originalContent = fs.readFileSync(originalI64Wast).toString();
  return new Promise((resolve, reject) => {
    let id = 0;
    const newModuleContent = [];
    const asserts = [];
    const sexpr = parse(originalContent);
    let wasmModule;
    for(const root of sexpr) {
      const rootType = getNodeType(root);
      switch(rootType) {
        case "module":
          wasmModule = extractModule(root);
          break;
        case "assert_trap":
        case "assert_return": {
          const assertInfo = getAssertInfo(root);
          const func = wasmModule.exports[assertInfo.fnName];
          let genInfos;
          if (assertInfo.args.length === 1) {
            genInfos = [{
              call: {
                type: "list",
                content: [func.body[0], assertInfo.args[0]]
              },
              params: [{
                type: "atom",
                content: ""
              }],
              invoke: [{
                type: "atom",
                content: ""
              }]
            }];
          } else {
            // 2 arguments
            genInfos = [{
              call: {
                type: "list",
                content: [func.body[0], assertInfo.args[0], assertInfo.args[1]]
              },
              params: [{
                type: "atom",
                content: ""
              }],
              invoke: [{
                type: "atom",
                content: ""
              }]
            }, {
              call: {
                type: "list",
                content: [func.body[0], func.body[1], assertInfo.args[1]]
              },
              params: [func.params[0]],
              invoke: [assertInfo.args[0]]
            }, {
              call: {
                type: "list",
                content: [func.body[0], assertInfo.args[0], func.body[2]]
              },
              params: [func.params[1]],
              invoke: [assertInfo.args[1]]
            }];
          }
          for(const info of genInfos) {
            const fnName = `${assertInfo.fnName}${id++}`;

            const newFunc = nodeToString({
              type: "list",
              content: [
                {type: "atom", content: "func"},
                {type: "list", content: [
                  {type: "atom", content: "export"},
                  {type: "string", content: fnName},
                ]},
                ...info.params,
                func.return,
                info.call
              ]
            });
            const newAssert = nodeToString({
              type: "list",
              content: [
                {type: "atom", content: rootType},
                {type: "list", content: [
                  {type: "atom", content: "invoke"},
                  {type: "string", content: fnName},
                  info.invoke,
                ]},
                assertInfo.returnInfo
              ]
            });
            //`(${rootType} (invoke "${fnName}" ${nodeToString(info.invoke)}) ${nodeToString(assertInfo.returnInfo)})`;

            newModuleContent.push(newFunc);
            asserts.push(newAssert);
          }
          break;
        }
      }
    }

    resolve(`
(module
  ${newModuleContent.join("\n  ")}
)

${asserts.join("\n")}`);
  });
};

