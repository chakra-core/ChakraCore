//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const path = require("path");
const jsBeautify = require("js-beautify");
const fs = require("fs-extra");
const stringArgv = require("string-argv");
const {execFile, spawn} = require("child_process");
const which = require("which");
const slash = require("slash");

const rlRoot = path.join(__dirname, "..");
const baselineDir = path.join(rlRoot, "baselines");

const argv = require("yargs")
  .help()
  .alias("help", "h")
  .options({
    bin: {
      string: true,
      alias: "b",
      description: "Path to wast2wasm exe",
      default: (() => {
        try {
          return which.sync("wast2wasm");
        } catch (e) {
          return undefined;
        }
      })(),
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
      default: []
    },
    "xplat-excludes": {
      array: true,
      description: "Spec tests to exclude when running on xplat (use for known failures)",
      default: [
        "address",
        "binary",
        "call",
        "call_indirect",
        "chakra_i64",
        "conversions",
        "fac",
        "func_ptrs",
        "i32",
        "i64",
        "imports",
        "int_exprs",
        "linking",
        "memory_trap",
        "page",
        "resizing",
        "select",
        "skip-stack-guard-page",
        "start",
        "traps",
        "unreachable",
        "unwind"
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
if(typeof argv.bin == 'undefined') {
  throw new Error("Unable to automatically find wast2wasm; please specify it with --bin");
}
fs.statSync(argv.bin).isFile();
fs.statSync(argv.suite).isDirectory();

function changeExtension(filename, from, to) {
  return `${path.basename(filename, from)}${to}`;
}

function convertTest(filename) {
  const testDir = path.dirname(filename);
  const outputPath = path.join(argv.output, changeExtension(filename, ".wast", ".json"));
  const args = [
    path.basename(filename),
    "--spec",
    "--no-check",
    "-o", outputPath
  ];
  console.log(`${testDir}: ${argv.bin} ${args.join(" ")}`);
  return new Promise((resolve, reject) => {
    execFile(argv.bin, args, {
      cwd: testDir
    }, err => err ? reject(err) : resolve());
  });
}

function hostFlags(specFile, {useFullpath} = {}) {
  return `-wasm -args ${
    useFullpath ? specFile : slash(path.relative(rlRoot, specFile))
  } -endargs`;
}

function getBaselinePath(specFile) {
  return `${slash(path.relative(rlRoot, path.join(baselineDir, path.basename(specFile, ".json"))))}.baseline`;
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
    return Promise.all(chakraTests.map(test => test.getContent(argv.suite)
      .then(content => new Promise((resolve, reject) => {
        if (!content) {
          return resolve();
        }
        const testPath = path.join(chakraTestsDestination, `${test.name}.wast`);
        const copyrightHeader =
`;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------
`;
        fs.writeFile(testPath, `${copyrightHeader};;AUTO-GENERATED do not modify\n${content}`, err => {
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
          item.path.indexOf(".fail") === -1 &&
          !argv.excludes.includes(path.basename(item.path, ".wast"))
        ) {
          conversions.push(convertTest(item.path));
        }
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
  ))/*.then(specFiles => {
    const cleanFullPaths = specFiles.map(specFile => new Promise((resolve, reject) => {
      const specDescription = require(specFile);
      specDescription.source_filename = slash(path.basename(specDescription.source_filename));
      fs.writeFile(
        specFile,
        jsBeautify(
          JSON.stringify(specDescription),
          {indent_size: 2, end_with_newline: true, wrap_line_length: 200}
        ), err => {
          if (err) {
            return reject(err);
          }
          resolve();
        }
      );
    }));
    return Promise.all(cleanFullPaths).then(() => Promise.resolve(specFiles));
  })*/.then(specFiles => {
    const runs = specFiles.map(specFile => {
      const isXplatExcluded = argv.xplatExcludes.indexOf(path.basename(specFile, ".json")) !== -1;
      const baseline = getBaselinePath(specFile);
      const flags = hostFlags(specFile);
      const tests = [{
        tags: [],
        baseline: baseline,
        flags: [flags]
      }, {
        tags: ["exclude_dynapogo"],
        baseline: baseline,
        flags: [flags, "-nonative"]
      }]
      if (isXplatExcluded) {
        for (const test of tests) test.tags.push("exclude_xplat");
      }
      return tests;
    });
    const rlexe = (
`<?xml version="1.0" encoding="utf-8"?>
<!-- Auto Generated by convert-test-suite -->
<regress-exe>${
  runs.map(run => run.map(test => `
  <test>
    <default>
      <files>spec.js</files>
      <baseline>${test.baseline}</baseline>
      <compile-flags>${test.flags.join(" ")}</compile-flags>${test.tags.length > 0 ? `
      <tags>${test.tags.join(",")}</tags>` : ""}
    </default>
  </test>`).join("")).join("")
}
</regress-exe>
`);
    return new Promise((resolve, reject) => {
      fs.writeFile(path.join(__dirname, "..", "rlexe.xml"), rlexe, err => err ? reject(err) : resolve(specFiles));
    });
  }).then(specFiles => {
    if (!argv.rebase) {
      return;
    }
    fs.removeSync(baselineDir);
    fs.ensureDirSync(baselineDir);
    return Promise.all(specFiles.map(specFile => new Promise((resolve, reject) => {
      const baseline = fs.createWriteStream(getBaselinePath(specFile));
      const args = [path.resolve(rlRoot, "spec.js"), "-nonative"].concat(stringArgv(hostFlags(specFile, {useFullpath: true})));
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
