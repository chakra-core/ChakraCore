# ember-source-channel-url

Retrieve a URL that can be used to reference a tarball representing the latest
`ember-source` build for that channel.

## Usage

### Command Line API

```js
npm install -g ember-source-channel-url
ember-source-channel-url canary
```

Will print out:

```sh
The URL for the latest tarball from ember-source's canary channel is:

        https://s3.amazonaws.com/builds.emberjs.com/canary/shas/<RANDOM SHA HERE>.tgz
```
### Progamatic API
```js
const getURLFor = require('ember-source-channel-url');

getURLFor('canary').then((url) => {
  // use the URL here 
});
```
