var t = new jake.TestTask('sourcemap-validator', function () {
  this.testFiles.include('tests/*');
  this.testFiles.exclude('tests/fixtures');
});

var d = new jake.NpmPublishTask('sourcemap-validator', function () {
  this.packageFiles.include([
    'index.js'
  , 'package.json'
  , 'Jakefile'
    ]);
  this.packageFiles.exclude([
  ]);
});
