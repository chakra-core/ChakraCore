# username.sync

[![Build Status](https://travis-ci.org/stefanpenner/username-sync.svg?branch=master)](https://travis-ci.org/stefanpenner/username-sync)
[![Build status](https://ci.appveyor.com/api/projects/status/89vktdhm1vsh2yno/branch/master?svg=true)](https://ci.appveyor.com/project/embercli/username-sync/branch/master)


```sh
yarn add username-sync
npm install username-sync --save
```

```js
var username = require('username-sync');

username() // => "best-guess-system-username"
```

If you are using new versions of node => 4x, please use
[username](https://github.com/sindresorhus/username) module. This project
exists to help users make the transition.

