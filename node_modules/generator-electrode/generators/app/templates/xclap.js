/*
 * Tell Electrode app archetype that you want to use ES6 syntax in your server code
 */

process.env.SERVER_ES6 = true;

/*
 * Tell Electrode app archetype that you want to use webpack dev as a middleware
 * This will run webpack dev server as part of your app server.
 */

process.env.WEBPACK_DEV_MIDDLEWARE = true;

/*
 * Tell Electrode app archetype that you want to shorten css names under production env
 */

process.env.ENABLE_SHORTEN_CSS_NAMES = true;

/*
 * Enable webpack's NodeSourcePlugin to simulate NodeJS libs in browser
 *
 * This basically adds a bunch of extra JavaScript to the browser to simulate
 * some NodeJS modules like `process`, `console`, `Buffer`, and `global`.
 *
 * Docs here:
 * https://github.com/webpack/docs/wiki/internal-webpack-plugins#nodenodesourcepluginoptions
 *
 * Note that the extra JavaScript could be substantial and adds more than 100K
 * of minified JS to your browser bundle.
 *
 * But if you see Errors like "Uncaught ReferenceError: global is not defined", then
 * the quick fix is to uncomment the line below.
 */

// process.env.ENABLE_NODESOURCE_PLUGIN = true;

/*
 * Use PhantomJS to run your Karma Unit tests.  Default is "chrome" (Chrome Headless)
 */

// process.env.KARMA_BROWSER = "phantomjs";

/******************************************************************************
 * Begin webpack-dev-server only settings.                                    *
 * These do not apply if WEBPACK_DEV_MIDDLEWARE is enabled                    *
 ******************************************************************************/

/*
 * Turn off using electrode-webpack-reporter to show visual report of your webpack
 * compile results when running in dev mode with `clap dev`
 */

// process.env.HTML_WEBPACK_REPORTER_OFF = true;

/*
 * Use a custom host name instead of localhost, and a diff port instead of 2992
 * for webpack dev server when running in dev mode with `clap dev`
 */

// process.env.WEBPACK_DEV_HOST = "dev.mymachine.net";
// process.env.WEBPACK_DEV_PORT = 8100;

/*
 * Enable HTTPS for webpack dev server when running in dev mode with `clap dev`
 */

// process.env.WEBPACK_DEV_HTTPS = true;

/******************************************************************************
 * End webpack-dev-server only settings.                                      *
 ******************************************************************************/

require("electrode-archetype-react-app")();

//<% if (isSingleQuote) { %>
//
// When single quote option is selected, this will use eslint to fix
// generated files from double to single quote after npm install.
// This is more reliable in case you stop npm install after the generator
// generated the app.
// You may remove this if you want.
//
const xclap = require("xclap");
const Fs = require("fs");
const Path = require("path");
xclap.load("user", {
  "fix-generator-eslint": {
    task: () => {
      const pkg = require("./package.json");
      if (pkg.scripts.install === "clap ?fix-generator-eslint") {
        delete pkg.scripts.install;
        Fs.writeFileSync(Path.resolve("package.json"), JSON.stringify(pkg, null, 2));
        return "~$eslint --fix config src test --ext .js,.jsx";
      }
    }
  }
});
//<% } %>
