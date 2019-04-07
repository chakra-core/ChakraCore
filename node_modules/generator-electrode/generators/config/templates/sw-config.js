module.exports = {
  cache: {
    cacheId: "<%= projectName %>",
    runtimeCaching: [
      {
        handler: "fastest",
        urlPattern: "/$"
      }
    ],
    staticFileGlobs: ["dist/**/*"]
  },
  manifest: {
    background: "#FFFFFF",
    title: "<%= projectName %>",
    short_name: "PWA",
    theme_color: "#FFFFFF"
  }
};
