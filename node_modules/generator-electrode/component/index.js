"use strict";

var _ = require("lodash");
var chalk = require("chalk");
var Generator = require("yeoman-generator");
var path = require("path");
var extend = _.merge;
var parseAuthor = require("parse-author");
var optionOrPrompt = require("yeoman-option-or-prompt");
var nodeFS = require("fs");

const pkg = require("../package.json");

module.exports = class extends Generator {
  constructor(args, options) {
    super(args, options);

    this.isAddon = this.options.isAddon || false;
    if (!this.isAddon) {
      this.log(chalk.green("Yeoman Electrode Component generator version"), pkg.version);
      this.log("Loaded from", chalk.magenta(path.dirname(require.resolve("../package.json"))));
    }

    this.quotes = this.options.quotes;
    this.githubUrl = this.options.githubUrl || "https://github.com";
    this.optionOrPrompt = optionOrPrompt;
  }

  initializing() {
    //check if the command is being run from within an existing app
    if (this.fs.exists(this.destinationPath("package.json"))) {
      var appPkg = this.fs.readJSON(this.destinationPath("package.json"));
      if (appPkg.dependencies && appPkg.dependencies["electrode-archetype-react-app"]) {
        this.env.error(
          "Please do not run this command from within an application." +
            "\nComponent structure should be generated in its own folder."
        );
      }
    }

    this.demoAppName = this.options.demoAppName;
    this.pkg = this.fs.readJSON(this.destinationPath("package.json"), {});

    // Pre set the default props from the information we have at this point
    this.props = {
      packageName: this.pkg.name || this.options.name,
      projectName: this.options.name
    };

    if (_.isObject(this.pkg.author)) {
      this.props.developerName = this.pkg.author.name;
      this.props.createDirectory = true;
    } else if (_.isString(this.pkg.author)) {
      var info = parseAuthor(this.pkg.author);
      this.props.developerName = info.name;
    }
    this.props.quoteType = this.quotes;
  }

  _askFor() {
    if (this.pkg.name || this.options.name) {
      this.props.name = this.pkg.name || _.kebabCase(this.options.name);
    }

    if (this.options.componentName) {
      this.props.componentName = this.options.componentName;
    }

    var prompts = [
      {
        type: "input",
        name: "projectName",
        message: "What is your Package/GitHub project name? (e.g., 'wysiwyg-component')",
        default: "wysiwyg-component",
        when: !this.props.name
      },
      {
        type: "input",
        name: "packageName",
        message: "What is the ClassName for your component?",
        default: this.props.componentName || this.props.projectName,
        when: !this.props.componentName
      },
      {
        type: "input",
        name: "packageName",
        message: "What will be the npm package name?",
        default: this.props.packageName
      },
      {
        type: "input",
        name: "packageGitHubOrg",
        message: "What will be the GitHub organization username (e.g., 'walmartlabs')?",
        default: this.props.packageGitHubOrg
      },
      {
        type: "input",
        name: "developerName",
        message: "What is your name? (for copyright notice, etc.)",
        default: this.props.developerName
      },
      {
        type: "input",
        name: "ghUser",
        message: "What is your GitHub Username?",
        default: this.props.developerName
      },
      {
        type: "list",
        name: "quoteType",
        message: "Use double quotes or single quotes?",
        choices: ['"', "'"],
        default: '"',
        when: !this.props.quoteType
      },
      {
        type: "input",
        name: "ghRepo",
        message: "What is the name of the GitHub repo where this will be published?",
        default: this.packageName
      },
      {
        type: "confirm",
        name: "createDirectory",
        message: "Would you like to create a new directory for your project?",
        default: true
      },
      {
        type: "confirm",
        name: "flow",
        message: "Would you like to generate .flowconfig for flow usage?",
        when: this.props.flow === undefined,
        default: false
      }
    ];

    return this.optionOrPrompt(prompts).then(props => {
      this.props = extend(this.props, props);
      this.projectName = this.props.projectName
        .split(" ")
        .map(_.toLower)
        .join("");
      this.packageName = this.props.projectName
        .split(" ")
        .map(_.toLower)
        .join("");
      this.developerName = this.props.developerName;
      this.quoteType = this.props.quoteType;
      this.ghUser = this.props.ghUser;
      this.ghRepo = this.props.ghRepo;
      this.packageGitHubOrg = this.props.packageGitHubOrg;
      this.createDirectory = this.props.createDirectory;
      this.flow = this.props.flow;
      this.componentName = _
        .kebabCase(_.deburr(this.props.projectName))
        .replace(/^\s+|\s+$/g, "")
        .replace(/(^|[-_ ])+(.)/g, function(match, first, second) {
          return second.toUpperCase();
        });
      this.currentYear = new Date().getFullYear();
      if (this.props.createDirectory) {
        var newRoot = this.destinationPath() + "/" + this.packageName;
        this.destinationRoot(newRoot);
      }
      this.rootPath = this.isAddon ? "" : "packages/" + this.projectName + "/";
    });
  }

  prompting() {
    if (!this.isAddon) {
      this.log(
        "\n" +
          chalk.bold.underline("Welcome to the Electrode Component Generator") +
          "\n" +
          "\nWe're going to set up a new " +
          chalk.bold("Electrode") +
          " component, ready for development with" +
          "\n" +
          chalk.bold("react, webpack, demo, electrode component archetype, and live-reload")
      );
    }

    return this._askFor();
  }

  writing() {
    lernaStructure: {
      // copy lerna and top level templates
      if (!this.isAddon) {
        this.fs.copyTpl(this.templatePath("gitignore"), this.destinationPath(".gitignore"));

        this.fs.copyTpl(this.templatePath("_package.json"), this.destinationPath("package.json"), {
          packageName: this.packageName,
          projectName: this.projectName,
          developerName: this.developerName,
          githubUrl: this.githubUrl,
          ghUser: this.ghUser,
          packageGitHubOrg: this.packageGitHubOrg,
          ghRepo: this.ghRepo
        });

        this.fs.copyTpl(this.templatePath("_readme.md"), this.destinationPath("README.md"), {
          projectName: this.projectName
        });

        this.fs.copyTpl(this.templatePath("lerna.json"), this.destinationPath("lerna.json"));
      }
    }

    component: {
      this.fs.copy(
        this.templatePath("packages/component/babelrc"),
        this.destinationPath(this.rootPath + ".babelrc")
      );
      this.fs.copy(
        this.templatePath("packages/component/gitignore"),
        this.destinationPath(this.rootPath + ".gitignore")
      );
      this.fs.copy(
        this.templatePath("packages/component/npmignore"),
        this.destinationPath(this.rootPath + ".npmignore")
      );
      this.fs.copy(
        this.templatePath("packages/component/editorconfig"),
        this.destinationPath(this.rootPath + ".editorconfig")
      );
      this.fs.copyTpl(
        this.templatePath("packages/component/_xclap.js"),
        this.destinationPath(this.rootPath + "xclap.js"),
        { quoteType: this.quoteType }
      );
      this.fs.copyTpl(
        this.templatePath("packages/component/_package.json"),
        this.destinationPath(this.rootPath + "package.json"),
        {
          projectName: this.projectName,
          packageName: this.projectName,
          componentName: this.componentName,
          developerName: this.developerName,
          githubUrl: this.githubUrl,
          ghUser: this.ghUser,
          packageGitHubOrg: this.packageGitHubOrg,
          ghRepo: this.ghRepo,
          currentYear: this.currentYear
        }
      );
      this.fs.copyTpl(
        this.templatePath("packages/component/_readme.md"),
        this.destinationPath(this.rootPath + "README.md"),
        {
          projectName: this.projectName,
          packageName: this.projectName,
          componentName: this.componentName,
          developerName: this.developerName,
          githubUrl: this.githubUrl,
          ghUser: this.ghUser,
          packageGitHubOrg: this.packageGitHubOrg,
          ghRepo: this.ghRepo,
          currentYear: this.currentYear
        }
      );
      if (this.quoteType === "'") {
        this.fs.copy(
          this.templatePath("packages/component/eslintrc"),
          this.destinationPath(".eslintrc")
        );
      }
      if (this.flow) {
        this.fs.copy(
          this.templatePath("packages/component/flowconfig"),
          this.destinationPath(this.rootPath + ".flowconfig")
        );
      }
      this.fs.copyTpl(
        this.templatePath("packages/component/src/components/_component.jsx"),
        this.destinationPath(this.rootPath + "src/components/" + this.projectName + ".jsx"),
        {
          componentName: this.componentName,
          projectName: this.projectName
        }
      );
      this.fs.copy(
        this.templatePath("packages/component/src/components/_accordion.jsx"),
        this.destinationPath(this.rootPath + "src/components/accordion.jsx")
      );
      this.fs.copy(
        this.templatePath("packages/component/src/styles/_component.css"),
        this.destinationPath(this.rootPath + "src/styles/" + this.projectName + ".css")
      );
      this.fs.copy(
        this.templatePath("packages/component/src/styles/_accordion.css"),
        this.destinationPath(this.rootPath + "src/styles/accordion.css")
      );

      // demo folder files
      this.fs.copyTpl(
        this.templatePath("packages/component/demo-examples/_component.example"),
        this.destinationPath(this.rootPath + "demo/examples/" + this.projectName + ".example"),
        {
          componentName: this.componentName
        }
      );

      this.fs.copyTpl(
        this.templatePath("packages/component/demo/"),
        this.destinationPath((this.isAddon ? "../" : "packages/") + this.projectName + "/demo"),
        { packageName: this.projectName }
      );

      this.fs.copyTpl(
        this.templatePath("packages/component/_components.md"),
        this.destinationPath(
          (this.isAddon ? "../" : "packages/") + this.projectName + "/components.md"
        ),
        { packageName: this.projectName }
      );

      // l10n language templates
      this.fs.copyTpl(
        this.templatePath("packages/component/src/lang/_DefaultMessages.js"),
        this.destinationPath(this.rootPath + "src/lang/default-messages.js"),
        {
          componentName: this.componentName
        }
      );

      this.fs.copyTpl(
        this.templatePath("packages/component/src/lang/_en.json"),
        this.destinationPath(this.rootPath + "src/lang/en.json"),
        {
          componentName: this.componentName
        }
      );

      this.fs.copyTpl(
        this.templatePath("packages/component/src/lang/tenants/electrodeio/_defaultMessages.js"),
        this.destinationPath(this.rootPath + "src/lang/tenants/electrodeio/default-messages.js"),
        {
          componentName: this.componentName
        }
      );

      this.fs.copyTpl(
        this.templatePath("packages/component/src/_Component.js"),
        this.destinationPath(this.rootPath + "src/index.js"),
        {
          componentName: this.componentName,
          projectName: this.projectName
        }
      );
    }

    test: {
      this.fs.copyTpl(
        this.templatePath("packages/component/test/client/eslintrc"),
        this.destinationPath(this.rootPath + "test/client/.eslintrc"),
        {
          quoteType: this.quoteType
        }
      );

      this.fs.copyTpl(
        this.templatePath("packages/component/test/client/components/_component.spec.jsx"),
        this.destinationPath(
          this.rootPath + "test/client/components/" + this.projectName + ".spec.jsx"
        ),
        {
          componentName: this.componentName,
          projectName: this.projectName
        }
      );

      this.fs.copy(
        this.templatePath(
          "packages/component/test/client/components/helpers/_intlEnzymeTestHelper.js"
        ),
        this.destinationPath(
          this.rootPath + "test/client/components/helpers/intl-enzyme-test-helper.js"
        )
      );
    }

    demoApp: {
      //Do not generate the demo app if called from the add on generator
      this.originalDemoAppName = "demo-app";
      if (!this.isAddon) {
        //custom props to pass to the App Generator
        this.props.description = this.description || "The demo App";
        this.props.createDirectory = false;
        this.props.packageName = this.packageName;
        this.props.className = this.componentName;
        this.props.name = this.originalDemoAppName;
        this.props.homepage = this.githubUrl + "/" + this.packageGitHubOrg + "/" + this.ghRepo;
        this.props.serverType = "HapiJS";
        this.props.githubUrl = this.githubUrl;
        this.props.authorName = this.developerName || this.user.git.name();
        this.props.authorEmail = this.ghUser + "@" + this.packageGitHubOrg + ".com";
        this.props.authorUrl = this.githubUrl + "/" + this.ghUser;
        this.props.pwa = false;
        this.props.autoSsr = false;
        this.props.license = "nolicense";
        this.props.githubAccount = this.ghUser || this.user.git.name();
        this.props.keywords = ["electrode"];

        let options = {
          isDemoApp: true,
          props: this.props
        };

        //change the destinationRoot for generating the demo app
        this.oldRoot = this.destinationRoot();
        var newRoot = this.destinationPath() + "/" + this.originalDemoAppName;
        this.destinationRoot(newRoot);
        this.composeWith(require.resolve("../generators/app"), options);

        const packageName = this.packageName;
        const className = this.props.className;

        this.fs.copyTpl(
          this.templatePath("demo-app/archetype"),
          this.destinationPath("archetype"),
          { components: [packageName] }
        );

        //copy home file
        this.fs.copyTpl(
          this.templatePath("demo-app/src/client/components/home.jsx"),
          this.destinationPath("src/client/components/home.jsx"),
          { className, packageName, pwa: false }
        );

        //copy reducer file
        this.fs.copyTpl(
          this.templatePath("demo-app/src/client/reducers/index.js"),
          this.destinationPath("src/client/reducers/index.jsx")
        );

        //copy actions file
        this.fs.copyTpl(
          this.templatePath("demo-app/src/client/actions/index.js"),
          this.destinationPath("src/client/actions/index.jsx")
        );
      }
    }
  }

  install() {
    let locations = [
      this.destinationPath(),
      path.join(this.destinationPath(), "..", "..", this.originalDemoAppName)
    ];

    if (!this.isAddon) {
      //git init and npmi for lerna lernaStructure
      this.spawnCommandSync("git", ["init"], {
        cwd: this.destinationPath(this.oldRoot)
      });
      locations = [
        this.destinationPath(this.oldRoot),
        path.join(this.oldRoot, "/", this.rootPath),
        this.destinationPath(path.join(this.oldRoot, "/", this.originalDemoAppName))
      ];
    }

    locations.reduce(
      (previousValue, currentValue) => {
        if (!previousValue.signal && previousValue.status === 0) {
          return this.spawnCommandSync("npm", ["install"], {
            cwd: currentValue
          });
        } else {
          return previousValue;
        }
      },
      { status: 0 }
    );
  }

  end() {
    if (this.quoteType === "'") {
      this.spawnCommandSync("node_modules/.bin/eslint", [
        "--fix",
        "src",
        "demo",
        "example",
        "test",
        "--ext",
        ".js,.jsx"
      ]);
    }

    if (!this.isAddon) {
      this.appName = _.isEmpty(this.demoAppName) ? this.originalDemoAppName : this.demoAppName;
      this.log(
        "\n" +
          chalk.green.underline("Your new Electrode component is ready!") +
          "\n" +
          "Your component is in " +
          this.packageName +
          "/packages" +
          " and your demo app is " +
          this.packageName +
          "/" +
          this.appName +
          "\n" +
          "\nType 'cd " +
          this.packageName +
          "/" +
          this.appName +
          "' then 'clap dev' to run the development build for the demo app." +
          "\n"
      );
    }
  }
};
