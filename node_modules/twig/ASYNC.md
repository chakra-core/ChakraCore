# Twig Asynchronous Rendering

## Synchronous promises

The asynchronous behaviour of Twig.js relies on promises, in order to support both the synchronous and asynchronous behaviour there is an internal promise implementation that runs fully synchronous.

The internal implementation of promises does not use `setTimeout` to run through the promise chain, but instead synchronously runs through the promise chain.

The different promise implementations can be mixed, synchronous behaviour however is no longer guaranteed as soon as the regular promise implementation is run.

### Examples

**Internal (synchronous) implementation**

[Internal implementation](https://github.com/JorgenEvens/twig.js/tree/master/src/twig.async.js#L40)

```javascript
console.log('start');
Twig.Promise.resolve('1')
.then(function(v) {
    console.log(v);
    return '2';
})
.then(function(v) {
    console.log(v);
});
console.log('stop');

/**
 * Prints to the console:
 * start
 * 1
 * 2
 * stop
 */
```

**Regular / native promises**

Implementations such as the native promises or [bluebird](http://bluebirdjs.com/docs/getting-started.html) promises.

```javascript
console.log('start');
Promise.resolve('1')
.then(function(v) {
    console.log(v);
    return '2';
})
.then(function(v) {
    console.log(v);
});
console.log('stop');

/**
 * Prints to the console:
 * start
 * stop
 * 1
 * 2
 */
```

**Mixing promises**

```javascript
console.log('start');
Twig.Promise.resolve('1')
.then(function(v) {
    console.log(v);
    return Promise.resolve('2');
})
.then(function(v) {
    console.log(v);
});
console.log('stop');

/**
 * Prints to the console:
 * start
 * 1
 * stop
 * 2
 */
```


## Async helpers

To preserve the correct order of execution there is an implemenation of `Twig.forEach()` that waits any promises returned from the callback before executing the next iteration of the loop. If no promise is returned the next iteration is invoked immediately.

```javascript
var arr = new Array(5);

Twig.async.forEach(arr, function(value, index) {
    console.log(index);

    if (index % 2 == 0)
        return index;

    return Promise.resolve(index);
})
.then(function() {
    console.log('finished');
});

/**
 * Prints to the console:
 * 0
 * 1
 * 2
 * 3
 * 4
 * finished
 */
```

## Switching render mode

The rendering mode of Twig.js internally is determined by the `allow_async` argument that can be passed into `Twig.expression.parse`, `Twig.logic.parse`, `Twig.parse` and `Twig.Template.render`. Detecting if at any point code runs asynchronously is explained in [detecting asynchronous behaviour](#detecting-asynchronous-behaviour).

For the end user switching between synchronous and asynchronous is as simple as using a different method on the template instance.

**Render template synchronously**

```javascript
var output = twig({
    data: 'a {{value}}'
}).render({
    value: 'test'
});

/**
 * Prints to the console:
 * a test
 */
```

**Render template asynchronously**

```javascript
var template = twig({
    data: 'a {{value}}'
}).renderAsync({
    value: 'test'
})
.then(function(output) {
    console.log(output);
});

/**
 * Prints to the console:
 * a test
 */
```

## Detecting asynchronous behaviour

The pattern used to detect asynchronous behaviour is the same everywhere it is used and follows a simple pattern.

1. Set a variable `is_async = true`
2. Run the promise chain that might contain some asynchronous behaviour.
3. As the last method in the promise chain set `is_async = false`
4. Underneath the promise chain test whether `is_async` is `true`

This pattern works because the last method in the chain will be executed in the next run of the eventloop (`setTimeout`/`setImmediate`).

### Examples

**Synchronous promises only**

```javascript
var is_async = true;

Twig.Promise.resolve()
.then(function() {
    // We run our work in here such to allow for asynchronous work
    // This example is fully synchronous
    return 'hello world';
})
.then(function() {
    is_async = false;
});

if (is_async)
    console.log('method ran asynchronous');

console.log('method ran synchronous');

/**
 * Prints to the console:
 * method ran synchronous
 */
```

**Mixed promises**

```javascript
var is_async = true;

Twig.Promise.resolve()
.then(function() {
    // We run our work in here such to allow for asynchronous work
    return Promise.resolve('hello world');
})
.then(function() {
    is_async = false;
});

if (is_async)
    console.log('method ran asynchronous');

console.log('method ran synchronous');

/**
 * Prints to the console:
 * method ran asynchronous
 */
```
