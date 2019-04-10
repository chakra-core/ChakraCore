module.exports = () => ({
  manipulateOptions: (opts, parserOptions) => {
    let decoratorsPluginIndex = parserOptions.plugins.findIndex(
      p =>
        Array.isArray(p) &&
        p[0] === 'decorators' &&
        p[1].decoratorsBeforeExport &&
        p[1].legacy
    );

    parserOptions.plugins.splice(decoratorsPluginIndex, 1);
  },
});
