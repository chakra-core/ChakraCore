# generator-travis

[![NPM version][npm-image]][npm-url]
[![Build Status][travis-image]][travis-url]
[![Dependency Status][depstat-image]][depstat-url]

> [Yeoman][yo] generator to get and keep `.travis.yml` up-to-date effortlessly.  
> [Works great with other generators too](#composability).

Travis CI uses `.travis.yml` file in the root of repository to learn about project and how developer wants their builds to be executed. Learn how to [get started building NodeJS projects][nodejs-get-started] and how to [customize your builds][travis-customize].

The configuration template includes the following NodeJS versions:

* v11 (From 2018-10-23 until **2019-06-30**)
* [v10][node-10] (until **2021-04-01**)
* [v9][node-9] (Until **2018-06-30**)
* [v8][node-8] (Until **2019-12-31**)
* [v6][node-6] (Until **2019-04-18**)

[yo]: http://yeoman.io/
[nodejs-get-started]: http://docs.travis-ci.com/user/languages/javascript-with-nodejs/
[travis-customize]: http://docs.travis-ci.com/user/customizing-the-build/

## Install

    npm install --global yo generator-travis

## Usage

    yo travis

## NodeJS versions in the config

Every LTS-supported version is included plus current one if its not LTS-supported.
The list of the versions is loaded from <https://nodejs.org/dist/index.json> at
run-time.

* NodeJS v11 will be added on 2018-10-23 and removed on **2019-06-30**.
* NodeJS [v10][node-10] will be removed on **2021-04-01**.
* NodeJS [v9][node-9] will be removed on **2018-06-30**.
* NodeJS [v8][node-8] will be removed on **2019-12-31**.
* NodeJS [v6][node-6] will be removed on **2019-04-18**.

**All other versions, [except for those added through `options.config`](#compose),
are removed from the config.**

[![NodeJS LTS Timeline][node-lts-image]][node-lts-url]

[Read more][node-lts-url] about NodeJS long-term support/LTS.

## Composability

> Composability is a way to combine smaller parts to make one large thing. Sort of [like Voltron®][voltron]  
> — [Yeoman docs](http://yeoman.io/authoring/composability.html)

Just plug in _travis_ into your generator and let it handle your `.travis.yml` for you. Everybody wins.

### Install

    npm install --save generator-travis

#### Compose

```js
this.composeWith('travis', {}, {
  local: require.resolve('generator-travis')
});
```

Add any extra fields you need to `options.config` to extend the default configuration.

```js
this.composeWith('travis', { options: { config: {
  after_script: ['npm run coveralls'],
  node_js: ['v0.12']
}}}, {
  local: require.resolve('generator-travis')
});
```

[voltron]: http://25.media.tumblr.com/tumblr_m1zllfCJV21r8gq9go11_250.gif

## License

MIT © [Vladimir Starkov](https://iamstarkov.com)

[npm-url]: https://npmjs.org/package/generator-travis
[npm-image]: https://img.shields.io/npm/v/generator-travis.svg?style=flat-square

[travis-url]: https://travis-ci.org/iamstarkov/generator-travis
[travis-image]: https://img.shields.io/travis/iamstarkov/generator-travis.svg?style=flat-square

[depstat-url]: https://david-dm.org/iamstarkov/generator-travis
[depstat-image]: https://david-dm.org/iamstarkov/generator-travis.svg?style=flat-square

[node-lts-url]: https://github.com/nodejs/Release
[node-lts-image]: https://raw.githubusercontent.com/nodejs/Release/master/schedule.png

[node-10]: https://nodejs.org/download/release/latest-v10.x/
[node-9]: https://nodejs.org/download/release/latest-v9.x/
[node-8]: https://nodejs.org/download/release/latest-carbon/
[node-6]: https://nodejs.org/download/release/latest-boron/

[travis]: https://travis-ci.org/
