//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const path = require("path");
const jsBeautify = require("js-beautify");
const fs = require("fs-extra");
const stringArgv = require("string-argv");
const {execFile, spawn} = require("child_process");

const rlRoot = path.join(__dirname, "..");
const baselineDir = path.join(rlRoot, "baselines");

const argv = require("yargs")
  .options({
    bin: {
      string: true,
      alias: "b",
      description: "Path to sexpr-wasm exe",
      demand: true,
    },
    suite: {
      string: true,
      alias: "s",
      description: "Path to the test suite",
      default: path.join(rlRoot, "testsuite"),
      demand: true,
    },
    output: {
      string: true,
      alias: "o",
      description: "Output path of the converted suite",
      default: path.join(rlRoot, "testsuite-bin"),
      demand: true,
    },
    excludes: {
      array: true,
      alias: "e",
      description: "Spec tests to exclude from the conversion (use for known failures)",
      default: [
        "f32",
        "f32_cmp",
        "f64",
        "f64_cmp",
        "float_exprs",
        "float_misc",
        "i32",
        "memory_redundancy",
        "nan-propagation",
        "resizing",
      ]
    },
    rebase: {
      string: true,
      description: "Path to host to run the test create/update the baselines"
    }
  })
  .argv;

// Make sure all arguments are valid
argv.output = path.resolve(argv.output);
fs.statSync(argv.bin).isFile();
fs.statSync(argv.suite).isDirectory();

function changeExtension(filename, from, to) {
  return `${path.basename(filename, from)}${to}`;
}

function convertTest(filename) {
  return new Promise(resolve => {
    execFile(argv.bin, [
      filename,
      "--spec",
      "-o", path.join(argv.output, changeExtension(filename, ".wast", ".json"))
    ], () => {
      // If an error occurs here, handle manually
      // There are official test files that raise errors when converting and it's normal
      resolve();
    });
  });
}

function hostFlags(specFile, {fullpath} = {}) {
  return `-on:wasm -on:wasmlazytrap -args ${
    fullpath ? specFile : path.relative(rlRoot, specFile)
  } -endargs`;
}

function getBaselinePath(specFile) {
  return `${path.relative(rlRoot, path.join(baselineDir, path.basename(specFile, ".json")))}.baseline`;
}

function removePossiblyEmptyFolder(folder) {
  return new Promise((resolve, reject) => {
    fs.remove(folder, err => {
      // ENOENT is the error code if the folder is missing
      if (err && err.code !== "ENOENT") {
        return reject(err);
      }
      resolve();
    });
  });
}

function main() {
  const chakraTestsDestination = path.join(argv.suite, "chakra");
  const chakraTests = require("./generateTests");

  return Promise.all([
    removePossiblyEmptyFolder(argv.output),
    removePossiblyEmptyFolder(chakraTestsDestination),
  ]).then(() => {
    fs.ensureDirSync(chakraTestsDestination);
    return Promise.all(chakraTests.map(test => test.getContent(argv)
      .then(content => new Promise((resolve, reject) => {
        if (!content) {
          return resolve();
        }
        const testPath = path.join(chakraTestsDestination, `${test.name}.wast`);
        fs.writeFile(testPath, `;;AUTO-GENERATED do not modify\n${content}`, err => {
          if (err) {
            return reject(err);
          }
          return resolve();
        });
      }))));
  }).then(() => new Promise((resolve, reject) => {
    fs.ensureDirSync(argv.output);
    const conversions = [];
    fs.walk(argv.suite)
      .on("data", item => {
        if (
          path.extname(item.path) === ".wast" &&
          !argv.excludes.includes(path.basename(item.path, ".wast"))
        ) {
          conversions.push(convertTest(item.path));
        }
        /*
        else if(path.extname(item.path) === ".js") {
          const wastFilePath = path.join(path.dirname(item.path), changeExtension(item.path, ".js", ".wast"));
          conversions.push(
            (new Promise((resolve, reject) => {
              const testDetails = require(item.path);
              fs.writeFile(wastFilePath, `;;AUTO-GENERATED do not modify\n${testDetails}`, err => {
                if (err) {
                  return reject(err);
                }
                resolve();
              });
            }))
            .then(() => convertTest(wastFilePath))
          );
        }*/
      })
      .on("end", () => {
        Promise.all(conversions).then(resolve, reject);
      });
  })).then(() => new Promise((resolve, reject) =>
    fs.readdir(argv.output, (err, files) => {
      if (err) {
        return reject(err);
      }
      resolve(files
        .filter(file => path.extname(file) === ".json")
        .map(file => path.join(argv.output, file))
      );
    })
  )).then(specFiles => {
    const cleanFullPaths = specFiles.map(specFile => new Promise((resolve, reject) => {
      const specDescription = require(specFile);
      for (const module of specDescription.modules) {
        for (const command of module.commands) {
          // Remove hardcoded path here
          command.file = path.basename(command.file);
        }
      }
      fs.writeFile(
        specFile,
        jsBeautify(
          JSON.stringify(specDescription),
          {indent_size: 2, end_with_newline: true}
        ), err => {
          if (err) {
            return reject(err);
          }
          resolve();
        }
      );
    }));
    const rlexe = (
`<?xml version="1.0" encoding="utf-8"?>
<!-- Auto Generated by convert-test-suite -->
<regress-exe>${
  specFiles.map(specFile => `
  <test>
    <default>
      <files>spec.js</files>
      <baseline>${getBaselinePath(specFile)}</baseline>
      <compile-flags>${hostFlags(specFile)}</compile-flags>
    </default>
  </test>
  <test>
    <default>
      <files>spec.js</files>
      <baseline>${getBaselinePath(specFile)}</baseline>
      <compile-flags>${hostFlags(specFile)} -on:wasmdeferred</compile-flags>
    </default>
  </test>`
  ).join("")
}
</regress-exe>
`);
    fs.writeFileSync(path.join(__dirname, "..", "rlexe.xml"), rlexe);
    return Promise.all(cleanFullPaths).then(() => Promise.resolve(specFiles));
  }).then(specFiles => {
    if (!argv.rebase) {
      return;
    }
    fs.removeSync(baselineDir);
    fs.ensureDirSync(baselineDir);
    return Promise.all(specFiles.map(specFile => new Promise((resolve, reject) => {
      const baseline = fs.createWriteStream(getBaselinePath(specFile));
      const args = [path.resolve(rlRoot, "spec.js"), "-nonative"].concat(stringArgv(hostFlags(specFile, {fullpath: true})));
      console.log(argv.rebase, args.join(" "));
      const engine = spawn(
        argv.rebase,
        args,
        {cwd: rlRoot}
      );
      engine.stdout.pipe(baseline);
      engine.stderr.pipe(baseline);
      engine.on("error", reject);
      engine.on("close", resolve);
    })));
  });
}

main().then(() => console.log("done"), err => console.error(err));
