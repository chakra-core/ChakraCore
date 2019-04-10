module.exports = {
  extends: ['eslint:recommended', 'plugin:prettier/recommended'],
  parser: 'typescript-eslint-parser',
  parserOptions: {
    sourceType: 'module'
  },
  rules: {
    // Handled by TS
    'no-undef': 'off',
    'no-unused-vars': 'off'
  }
};
