module.exports = {
  root: true,
  parserOptions: {
    ecmaVersion: 6,
  },
  plugins: ['node', 'prettier'],
  extends: ['eslint:recommended', 'plugin:node/recommended'],
  env: {
    node: true,
  },
  rules: {
    strict: 'error',
    'no-var': 'error',
    'no-console': 'off',
    'no-process-exit': 'off',
    'object-shorthand': 'error',
    'prettier/prettier': ['error', require('./prettier.config')],
  },
  overrides: [
    {
      files: ['test/**/*.js'],
      plugins: ['mocha'],
      env: {
        mocha: true,
      },
      rules: {
        'mocha/no-exclusive-tests': 'error',
        'mocha/handle-done-callback': 'error',
      },
    },
  ],
};
