module.exports = {
  root: true,
  parserOptions: {
    ecmaVersion: 6,
  },
  extends: 'eslint:recommended',
  env: {
    browser: false,
    node: true,
    es6: true,
  },
  overrides: [
   {
     files: ['test/**/*.js'],
     env: {
       mocha: true,
     }
   }
  ]
};
