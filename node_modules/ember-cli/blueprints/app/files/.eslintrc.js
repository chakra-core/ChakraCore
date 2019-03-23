module.exports = {
  root: true,
  parserOptions: {
    ecmaVersion: 2017,
    sourceType: 'module'
  },
  plugins: [
    'ember'
  ],
  extends: [
    'eslint:recommended',
    'plugin:ember/recommended'
  ],
  env: {
    browser: true
  },
  rules: {
  },
  overrides: [
    // node files
    {
      files: [
        '.eslintrc.js',
        '.template-lintrc.js',
        'ember-cli-build.js',<% if (blueprint !== 'app') { %>
        'index.js',<% } %>
        'testem.js',
        'blueprints/*/index.js',
        'config/**/*.js'<% if (blueprint === 'app') { %>,
        'lib/*/index.js',
        'server/**/*.js'<% } else { %>,
        'tests/dummy/config/**/*.js'<% } %>
      ],<% if (blueprint !== 'app') { %>
      excludedFiles: [
        'addon/**',
        'addon-test-support/**',
        'app/**',
        'tests/dummy/app/**'
      ],<% } %>
      parserOptions: {
        sourceType: 'script',
        ecmaVersion: 2015
      },
      env: {
        browser: false,
        node: true
      }<% if (blueprint !== 'app') { %>,
      plugins: ['node'],
      rules: Object.assign({}, require('eslint-plugin-node').configs.recommended.rules, {
        // add your custom rules and overrides for node files here
      })<% } %>
    }
  ]
};
