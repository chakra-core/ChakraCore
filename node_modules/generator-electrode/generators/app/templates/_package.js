return {
  name: "application-name",
  version: "0.0.1",
  description: "Isomorphic React Application With NodeJS backend",
  homepage: "",
  author: {},
  contributors: [],
  files: ["config", "src", "lib", "dist"],
  main: "lib/server/index.js",
  keywords: [],
  repository: {},
  license: "ISC",
  engines: {
    node: ">= 8",
    npm: ">= 5"
  },
  scripts: {
    build: "clap build",
    "prod-start": "NODE_ENV=production clap -n -x electrode/build prod",
    start: 'if test "$NODE_ENV" = "production"; then npm run prod-start; else clap dev; fi',
    test: "clap check",
    coverage: "clap check",
    prod: "echo 'Starting standalone server in PROD mode'; NODE_ENV=production node ./lib/server/",
    "heroku-postbuild": "clap build", //<% if (isSingleQuote) { %>
    install: "clap ?fix-generator-eslint" //<% } %>
  },
  dependencies: {
    bluebird: "^3.4.6",
    "electrode-archetype-react-app": "^6.0.0",
    "electrode-react-webapp": "^3.2.0",
    //<% if (uiConfigModule) {%>
    "<%= uiConfigModule %>": "<%= uiConfigModuleSemver %>",
    //<% } %>
    "electrode-confippet": "^1.5.0",
    "electrode-redux-router-engine": "^2.1.8", //<% if (isHapi) { %>
    //<% if (isAutoSSR) {%>
    "electrode-auto-ssr": "^1.0.0", //<% } %>
    "electrode-server": "^2.2.0",
    "electrode-static-paths": "^2.0.1",
    inert: "^5.1.2",
    good: "^8.1.1",
    "good-console": "^7.1.0", //<% } else if (isExpress) { %>
    express: "^4.0.0", //<% } else { %>
    koa: "^2.6.2",
    "koa-router": "^7.4.0",
    "koa-send": "^5.0.0",
    "koa-static": "^5.0.0", //<% } if (isPWA) { %>
    "react-notify-toast": "^0.5.0", //<% } %>
    lodash: "^4.17.11",
    "@loadable/component": "^5.7.0",
    "react-router-config": "^1.0.0-beta.4",
    "react-router-dom": "^4.3.1",
    milligram: "^1.3.0",
    //<% if (cookiesModule) {%>
    "<%= cookiesModule %>": "<%= cookiesModuleSemver %>"
    //<% } %>
  },
  devDependencies: {
    "electrode-archetype-react-app-dev": "^6.0.0"
  }, //<% if (isSingleQuote) { %>
  eslintConfig: {
    rules: {
      quotes: [2, "single"]
    }
  } //<% } %>
};
