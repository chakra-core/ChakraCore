//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
/* eslint-env node */
const path = require("path");
const fs = require("fs-extra");
const {spawn} = require("child_process");
const slash = require("slash");

const config = require("./config.json");
const rlRoot = path.join(__dirname, "..");
const baselineDir = path.join(rlRoot, "baselines");

const argv = require("yargs")
  .help()
  .alias("help", "h")
  .options({
    suite: {
      string: true,
      alias: "s",
      description: "Path to the test suite",
      default: path.join(rlRoot, "testsuite"),
      demand: true,
    },
    rebase: {
      string: true,
      description: "Path to host to run the test create/update the baselines"
    }
  })
  .argv;

// Make sure all arguments are valid
fs.statSync(argv.suite).isDirectory();

function hostFlags(specFile) {
  return `-wasm -args ${slash(path.relative(rlRoot, specFile))} -endargs`;
}

function getBaselinePath(specFile) {
  return `${slash(path.relative(rlRoot, path.join(baselineDir, path.basename(specFile, path.extname(specFile)))))}.baseline`;
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
          console.log(`Generated ${testPath}`);
          return resolve(testPath);
        });
      }))));
  }).then(chakraTests => new Promise((resolve) => {
    const specFiles = [...chakraTests];
    fs.walk(path.join(argv.suite, "core"))
      .on("data", item => {
        if (
          path.extname(item.path) === ".wast" &&
          item.path.indexOf(".fail") === -1 &&
          !config.excludes.includes(path.basename(item.path, ".wast"))
        ) {
          specFiles.push(item.path);
        }
      })
      .on("end", () => {
        resolve(specFiles);
      });
  })).then(specFiles => new Promise((resolve) => {
    fs.walk(path.join(argv.suite, "js-api"))
      .on("data", item => {
        if (path.extname(item.path) === ".js") {
          specFiles.push(item.path);
        }
      })
      .on("end", () => {
        resolve(specFiles);
      });
  })).then(specFiles => {
    const runners = {
      ".wast": "spec.js",
      ".js": "jsapi.js",
    };
    const runs = specFiles.map(specFile => {
      const ext = path.extname(specFile);
      const basename = path.basename(specFile, ext);
      const isXplatExcluded = config["xplat-excludes"].indexOf(path.basename(specFile, ext)) !== -1;
      const baseline = getBaselinePath(specFile);
      const flags = hostFlags(specFile);
      const runner = runners[ext];
      const tests = [{
        runner,
        tags: [],
        baseline,
        flags: [flags]
      }, {
        runner,
        tags: ["exclude_dynapogo"],
        baseline,
        flags: [flags, "-nonative"]
      }].concat(config.features
        .filter(feature => feature.files.includes(basename))
        .map(feature => ({
          runner,
          baseline,
          tags: [].concat(feature.rltags || []),
          flags: [flags].concat(feature.flags || [])
        }))
      );
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
      <files>${test.runner}</files>
      <baseline>${test.baseline}</baseline>
      <compile-flags>${test.flags.join(" ")}</compile-flags>${test.tags.length > 0 ? `
      <tags>${test.tags.join(",")}</tags>` : ""}
    </default>
  </test>`).join("")).join("")
}
</regress-exe>
`);
    return new Promise((resolve, reject) => {
      fs.writeFile(path.join(__dirname, "..", "rlexe.xml"), rlexe, err => err ? reject(err) : resolve(runs));
    });
  }).then(runs => {
    if (!argv.rebase) {
      return;
    }
    fs.removeSync(baselineDir);
    fs.ensureDirSync(baselineDir);
    return Promise.all(runs.map(run => new Promise((resolve, reject) => {
      const test = run[0];
      const baseline = fs.createWriteStream(test.baseline);
      baseline.on("open", () => {
        const args = [path.resolve(rlRoot, test.runner)].concat(test.flags);
        console.log(argv.rebase, args.join(" "));
        const engine = spawn(
          argv.rebase,
          args,
          {
            cwd: rlRoot,
            stdio: [baseline, baseline, baseline],
            shell: true
          }
        );
        engine.on("error", reject);
        engine.on("close", resolve);
      });
      baseline.on("error", reject);
    })));
  });
}

main().then(() => console.log("done"), err => console.error(err));
