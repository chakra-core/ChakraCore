# {%= name %} {%= badge("fury") %} {%= badge("travis") %}

> {%= description %}

## Install
{%= include("install-npm", {save: true}) %}

## Usage

```js
var ascending = require('{%= name %}');
['d', 'c', 'b', 'a'].sort(ascending);
//=> ['a', 'b', 'c', 'd']
```

## Related projects
{%= related(verb.related.list, {remove: name}) %}

## Running tests
{%= include("tests") %}

## Contributing
{%= include("contributing") %}

## Author
{%= include("author") %}

## License
{%= copyright() %}
{%= license() %}

***

{%= include("footer") %}
