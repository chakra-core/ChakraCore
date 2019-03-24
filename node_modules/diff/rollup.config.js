import babel from 'rollup-plugin-babel';
import pkg from './package.json';

export default [
  // browser-friendly UMD build
  {
    input: 'src/index.js',
    output: [
      {
        name: 'Diff',
        file: pkg.browser,
        format: 'umd'
      },
      { file: pkg.module, format: 'es' }
    ],
    plugins: [
      babel({
        babelrc: false,
        presets: [['@babel/preset-env', { targets: {ie: '11'}, modules: false }]],
      })
    ]
  }
];
