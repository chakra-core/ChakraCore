# How To Contribute

## Installation

* `git clone <repository-url>`
* `cd <%= addonName %>`
* `<% if (yarn) { %>yarn<% } else { %>npm<% } %> install`

## Linting

* `<% if (yarn) { %>yarn lint:hbs<% } else { %>npm run lint:hbs<% } %>`
* `<% if (yarn) { %>yarn lint:js<% } else { %>npm run lint:js<% } %>`
* `<% if (yarn) { %>yarn lint:js --fix<% } else { %>npm run lint:js -- --fix<% } %>`

## Running tests

* `ember test` – Runs the test suite on the current Ember version
* `ember test --server` – Runs the test suite in "watch mode"
* `ember try:each` – Runs the test suite against multiple Ember versions

## Running the dummy application

* `ember serve`
* Visit the dummy application at [http://localhost:4200](http://localhost:4200).

For more information on using ember-cli, visit [https://ember-cli.com/](https://ember-cli.com/).