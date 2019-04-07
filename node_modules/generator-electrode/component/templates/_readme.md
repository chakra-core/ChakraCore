# <%= projectName %>

Welcome to the top level of the repo for React component <%= projectName %>.

This is designed to be a lerna repo with the following two directories:

-   `packages`
-   `demo-app`

`packages` contains the actual component modules.

`demo-app` contains an Electrode App that allows you to test and develop the components.

To start developing, first install the lerna dependencies at the top level:

```bash
npm install
```

And then bootstrap the modules under `packages`:

```bash
npm run bootstrap
```

To start playing with the included demo components:

```bash
cd demo-app
npm install
clap dev
```

Then open your browser to `http://localhost:3000`

# publishing

To publish your components, make sure you are at the top level directory and use `learn publish`.

You have to run it from `node_modules` directly, or you can use the `npm` `publish` script provided.

For example:

```bash
node_modules/.bin/lerna publish
```

OR:

```bash
npm run publish
```

The component package versions are all locked as one.  The version is stored inside the file `lerna.json`.

Initial verion is started at `0.0.1` for you.  You can instruct lerna to publish major or minor to start at `1.0.0` or `0.1.0`.
