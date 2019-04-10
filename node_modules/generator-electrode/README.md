# generator-electrode

[![NPM version][npm-image]][npm-url] [![Dependency Status][daviddm-image]][daviddm-url] [![devDependency Status][daviddm-dev-image]][daviddm-dev-url] [![npm downloads][npm-downloads-image]][npm-downloads-url]

> Generate Electrode ~~Isomorphic~~ Universal React App with NodeJS backend or a React component with useful clap tasks for development, building and publishing.

## Installation

First, install [Yeoman], [xclap-cli], and generator-electrode using [npm]\(&lt;we assume you have pre-installed [node.js]\(&lt;> 4.2.x required>)>).

```bash
$ npm install -g yo xclap-cli generator-electrode
```

> Note: You may need add `sudo` to the command.

Then generate your new project:

```bash
$ mkdir your-project-name
$ cd your-project-name
$ yo electrode
```

> Note: If the app is not properly generated at the correct destination, make sure you do not have a .yo-rc.json file hidden in your CWD.

[Here](https://github.com/animesh10/tutorials/blob/master/generateApp.gif) is a sample screen capture the app generation process.

## Running the app

Once the application is generated and `npm install` is completed, you are ready to try it out.

```bash
$ npm start
```

Wait for webpack to be ready and navigate to `http://localhost:3000` with your browser.

You can run [clap] to see the list of tasks available.

Some common ones:

-   `clap dev` - start in webpack-dev-server development mode
-   `clap hot` - start in webpack-dev-server hot mode
-   `clap build` - build production `dist` files
-   `clap server-prod` - start server in production mode
-   `clap check` - run unit tests with coverage

## Generating a React Component

  Install the generator if you haven't already:

```bash
npm install -g generator-electrode
```

  Then run the generator:

```bash
yo electrode:component
```

...and follow the prompts.

## Developing Your Component

### Source

Your component source code is in `packages/componentName/src`. You can use JSX and ES6 syntax freely in
your component source; it will be transpiled to `lib` with Babel before being
published to npm so that your users will simply be able to include it.

It's a great idea to add a description, documentation and other information to
your `README.md` file, to help people who are interested in using your
component.

The component project structure uses a [Lerna](https://lernajs.io/) structure, which can help manage multiple repos within your `packages` directory. Your initial project structure will be :

```text
test-component/
├── README.md
├── demo-app
│   ├── LICENSE
│   ├── README.md
│   ├── config
│   │   ├── default.js
│   │   ├── development.json
│   │   ├── production.js
│   │   └── production.json
│   ├── xclap.js
│   ├── package.json
│   ├── src
│   │   ├── client
│   │   │   ├── actions
│   │   │   │   └── index.jsx
│   │   │   ├── app.jsx
│   │   │   ├── components
│   │   │   │   └── home.jsx
│   │   │   ├── images
│   │   │   │   └── electrode.png
│   │   │   ├── reducers
│   │   │   │   └── index.jsx
│   │   │   ├── routes.jsx
│   │   │   └── styles
│   │   │       └── base.css
│   │   └── server
│   │       ├── index.js
│   │       └── views
│   │           └── index-view.jsx
│   └── test
│       ├── client
│       │   └── components
│       │       └── home.spec.jsx
│       └── server
├── lerna.json
├── package.json
└── packages
    └── test-component
        ├── README.md
        ├── xclap.js
        ├── package.json
        ├── src
        │   ├── components
        │   │   └── test-component.jsx
        │   ├── index.js
        │   ├── lang
        │   │   ├── default-messages.js
        │   │   ├── en.json
        │   │   └── tenants
        │   │       └── electrodeio
        │   │           └── default-messages.js
        │   └── styles
        │       └── test-component.css
        └── test
            └── client
                └── components
                    ├── helpers
                    │   └── intl-enzyme-test-helper.js
                    └── test-component.spec.jsx
```

### Adding Components to the Repo

The component structure shown above supports multiple packages under the `packages` folder.
You can add another component to your project by running `yo electrode:component-add` from within the `packages` directory.

    $ cd packages
    $ yo electrode:component-add

And follow the prompts.

This will generate a new package and also update the `demo-app`.
The `demo-app/src/client/components/home.jsx` and `demo-app/package.json` will be overwritten during the update.

[Here](https://github.com/animesh10/tutorials/blob/master/componentGenerator.gif) is a sample screen capture the component generation process.

### Example and Preview

Preview your component by using the `demo-app`. This is an electrode app which uses your newly created component:

```bash
$ cd demo-app
$ clap demo
```

A webserver will be started on [localhost:3000](http://127.0.0.1:3000).
Your new component will be used in `demo-app/src/client/components/home.jsx`

You can use this demo-app to test your component, then publish it to let
potential users try out your component and see what it can do.

## Getting To Know Yeoman

-   Yeoman has a heart of gold.
-   Yeoman is a person with feelings and opinions, but is very easy to work with.
-   Yeoman can be too opinionated at times but is easily convinced not to be.
-   Feel free to [learn more about Yeoman](http://yeoman.io/).

Built with :heart: by [Team Electrode](https://github.com/orgs/electrode-io/people) @WalmartLabs.

## License

Apache-2.0 © WalmartLabs

[npm-image]: https://badge.fury.io/js/generator-electrode.svg

[npm-url]: https://npmjs.org/package/generator-electrode

[daviddm-image]: https://david-dm.org/electrode-io/electrode/status.svg?path=packages/generator-electrode

[daviddm-url]: https://david-dm.org/electrode-io/electrode?path=packages/generator-electrode

[daviddm-dev-image]: https://david-dm.org/electrode-io/electrode/dev-status.svg?path=packages/generator-electrode

[daviddm-dev-url]: https://david-dm.org/electrode-io/electrode?path=packages/generator-electrode?type-dev

[npm-downloads-image]: https://img.shields.io/npm/dm/generator-electrode.svg

[npm-downloads-url]: https://www.npmjs.com/package/generator-electrode

[xclap-cli]: https://www.npmjs.com/package/xclap-cli

[yeoman]: http://yeoman.io

[npm]: https://www.npmjs.com/

[node.js]: https://nodejs.org/
