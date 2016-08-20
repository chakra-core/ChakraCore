const genTests = [
  "chakra_i64"
];

module.exports = genTests.map(f => ({name: f, getContent: require(`./${f}`)}));
