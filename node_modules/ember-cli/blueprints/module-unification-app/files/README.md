# <%= name %>

This README outlines the details of collaborating on this Ember application.
A short introduction of this app could easily go here.

## Prerequisites

You will need the following things properly installed on your computer.

* [Git](https://git-scm.com/)
* [Node.js](https://nodejs.org/)<% if (yarn) { %>
* [Yarn](https://yarnpkg.com/)<% } else { %> (with npm)<% } %>
* [Ember CLI](https://ember-cli.com/)
* [Google Chrome](https://google.com/chrome/)

## Installation

* `git clone <repository-url>` this repository
* `cd <%= name %>`
* `<% if (yarn) { %>yarn<% } else { %>npm<% } %> install`

## Running / Development

* `ember serve`
* Visit your app at [http://localhost:4200](http://localhost:4200).
* Visit your tests at [http://localhost:4200/tests](http://localhost:4200/tests).

### Code Generators

Make use of the many generators for code, try `ember help generate` for more details

### Running Tests

* `ember test`
* `ember test --server`

### Linting

* `<% if (yarn) { %>yarn lint:hbs<% } else { %>npm run lint:hbs<% } %>`
* `<% if (yarn) { %>yarn lint:js<% } else { %>npm run lint:js<% } %>`
* `<% if (yarn) { %>yarn lint:js --fix<% } else { %>npm run lint:js -- --fix<% } %>`

### Building

* `ember build` (development)
* `ember build --environment production` (production)

### Deploying

Specify what it takes to deploy your app.

## Further Reading / Useful Links

* [ember.js](https://emberjs.com/)
* [ember-cli](https://ember-cli.com/)
* Development Browser Extensions
  * [ember inspector for chrome](https://chrome.google.com/webstore/detail/ember-inspector/bmdblncegkenkacieihfhpjfppoconhi)
  * [ember inspector for firefox](https://addons.mozilla.org/en-US/firefox/addon/ember-inspector/)
