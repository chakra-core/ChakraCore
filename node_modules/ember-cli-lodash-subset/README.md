# ember-cli-lodash-subset
[![Build Status](https://travis-ci.org/ember-cli/ember-cli-lodash-subset.svg?branch=master)](https://travis-ci.org/ember-cli/ember-cli-lodash-subset)

Use rollup + lodash-es to create an optimized lodash build, something that improves ember-cli startup time by 20% - 30%

### adding/removing from the subset

1. update https://github.com/ember-cli/ember-cli-lodash-subset/blob/master/lodash.subset.js
2. `npm run build`
3. `node run` and makes sure times haven't gotten out of control
4. commit changes
5. release/publish
