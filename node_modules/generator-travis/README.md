# generator-travis

[![NPM version][npm-image]][npm-url]
[![Build Status][travis-image]][travis-url]
[![Dependency Status][depstat-image]][depstat-url]

> [Yeoman][yo] generator to get and keep `.travis.yml` up-to-date effortlessly.  
> [Works great with other generators too](#composability).

Travis CI uses `.travis.yml` file in the root of repository to learn about project and how developer wants their builds to be executed. Learn how to [get started building NodeJS projects][nodejs-get-started] and how to [customize your builds][travis-customize].

The configuration template includes the following NodeJS versions:

* v7
* v6
* v4

[yo]: http://yeoman.io/
[nodejs-get-started]: http://docs.travis-ci.com/user/languages/javascript-with-nodejs/
[travis-customize]: http://docs.travis-ci.com/user/customizing-the-build/

## Install

    npm install --global yo generator-travis

## Usage

    yo travis

## NodeJS versions in the config

Every LTS-supported version is included plus current one if its not LTS-supported. Once NodeJS versions list is changed this package will get a minor version update.

* NodeJS `v6.x` will be removed **April 18, 2019**.
* NodeJS `v4.x` will be removed **April 1, 2018**.

![NodeJS LTS Timeline](https://raw.githubusercontent.com/nodejs/LTS/master/schedule.png)

[Read more][NodeJS/LTS] about NodeJS long-term support/LTS.

[NodeJS/LTS]: https://github.com/nodejs/LTS/

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
  after_script: ['npm run coveralls']
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


[travis]: https://travis-ci.org/
