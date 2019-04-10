module.exports = {
  root: true,
  extends: ['eslint:recommended', 'plugin:node/recommended', 'prettier'],
  plugins: ['prettier', 'node'],
  parserOptions: {
    ecmaVersion: 2017,
  },
  env: {
    node: true,
  },
  rules: {
    'prettier/prettier': ['error', { singleQuote: true, trailingComma: 'es5' }],
  },
  overrides: [
    {
      files: ['scripts/*.js'],
      rules: {
        'no-console': 'off',
      }
    },
    {
      files: ['tests/*.test.js'],
      plugins: ['jest'],

      // can't use `extends` in nested config :sob:
      rules: require('eslint-plugin-jest').configs.recommended.rules,

      env: {
        'jest': true
      }
    },
    {
      files: ['scripts/**/*.js', 'tests/**/*.js'],
      rules: {
        // prevent fallback to node 4 only support, this package only distributes *.json
        // node version isn't super important...
        'node/no-unsupported-features': 'off',
      }
    }
  ]
};
