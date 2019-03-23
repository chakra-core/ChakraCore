'use strict'

const { test } = require('tap')
const rfdc = require('..')
const clone = rfdc()
const cloneProto = rfdc({proto: true})
const cloneCircles = rfdc({circles: true})
const cloneCirclesProto = rfdc({circles: true, proto: true})

types(clone, 'default')
types(cloneProto, 'proto option')
types(cloneCircles, 'circles option')
types(cloneCirclesProto, 'circles and proto option')

test('default – does not copy proto properties', async ({is}) => {
  is(clone(Object.create({a: 1})).a, undefined, 'value not copied')
})
test('proto option – copies enumerable proto properties', async ({is}) => {
  is(cloneProto(Object.create({a: 1})).a, 1, 'value copied')
})
test('circles option - circular object', async ({same, is, isNot}) => {
  const o = {nest: {a: 1, b: 2}}
  o.circular = o
  same(cloneCircles(o), o, 'same values')
  isNot(cloneCircles(o), o, 'different objects')
  isNot(cloneCircles(o).nest, o.nest, 'different nested objects')
  const c = cloneCircles(o)
  is(c.circular, c, 'circular references point to copied parent')
  isNot(c.circular, o, 'circular references do not point to original parent') 
})
test('circles option – deep circular object', async ({same, is, isNot}) => {
  const o = {nest: {a: 1, b: 2}}
  o.nest.circular = o
  same(cloneCircles(o), o, 'same values')
  isNot(cloneCircles(o), o, 'different objects')
  isNot(cloneCircles(o).nest, o.nest, 'different nested objects')
  const c = cloneCircles(o)
  is(c.nest.circular, c, 'circular references point to copied parent')
  isNot(c.nest.circular, o, 'circular references do not point to original parent') 
})
test('circles option alone – does not copy proto properties', async ({is}) => {
  is(cloneCircles(Object.create({a: 1})).a, undefined, 'value not copied')
})
test('circles and proto option – copies enumerable proto properties', async ({is}) => {
  is(cloneCirclesProto(Object.create({a: 1})).a, 1, 'value copied')
})
test('circles and proto option - circular object', async ({same, is, isNot}) => {
  const o = {nest: {a: 1, b: 2}}
  o.circular = o
  same(cloneCirclesProto(o), o, 'same values')
  isNot(cloneCirclesProto(o), o, 'different objects')
  isNot(cloneCirclesProto(o).nest, o.nest, 'different nested objects')
  const c = cloneCirclesProto(o)
  is(c.circular, c, 'circular references point to copied parent')
  isNot(c.circular, o, 'circular references do not point to original parent') 
})
test('circles and proto option – deep circular object', async ({same, is, isNot}) => {
  const o = {nest: {a: 1, b: 2}}
  o.nest.circular = o
  same(cloneCirclesProto(o), o, 'same values')
  isNot(cloneCirclesProto(o), o, 'different objects')
  isNot(cloneCirclesProto(o).nest, o.nest, 'different nested objects')
  const c = cloneCirclesProto(o)
  is(c.nest.circular, c, 'circular references point to copied parent')
  isNot(c.nest.circular, o, 'circular references do not point to original parent') 
})

function types(clone, label) {
  test(label + ' – number', async ({is}) => {
    is(clone(42), 42, 'same value')
  })
  test(label + ' – string', async ({is}) => {
    is(clone('str'), 'str', 'same value')
  })
  test(label + ' – boolean', async ({is}) => {
    is(clone(true), true, 'same value')
  })
  test(label + ' – function', async ({is}) => {
    const fn = () => {}
    is(clone(fn), fn, 'same function')
  })
  test(label + ' – async function', async ({is}) => {
    const fn = async () => {}
    is(clone(fn), fn, 'same function')
  })
  test(label + ' – generator function', async ({is}) => {
    const fn = function * () {}
    is(clone(fn), fn, 'same function')
  })
  test(label + ' – date', async ({is, isNot}) => {
    const date = new Date()
    is(+clone(date), +date, 'same value')
    isNot(clone(date), date, 'different object')
  })
  test(label + ' – null', async ({is}) => {
    is(clone(null), null, 'same value')
  })
  test(label + ' – shallow object', async ({same, isNot}) => {
    const o = {a: 1, b: 2}
    same(clone(o), o, 'same values')
    isNot(clone(o), o, 'different object')
  })
  test(label + ' – shallow array', async ({same, isNot}) => {
    const o = [1, 2]
    same(clone(o), o, 'same values')
    isNot(clone(o), o, 'different arrays')
  })
  test(label + ' – deep object', async ({same, isNot}) => {
    const o = {nest: {a: 1, b: 2}}
    same(clone(o), o, 'same values')
    isNot(clone(o), o, 'different objects')
    isNot(clone(o).nest, o.nest, 'different nested objects')
  })
  test(label + ' – deep array', async ({same, isNot}) => {
    const o = [ {a: 1, b: 2}, [3] ]
    same(clone(o), o, 'same values')
    isNot(clone(o), o, 'different arrays')
    isNot(clone(o)[0], o[0], 'different array elements')
    isNot(clone(o)[1], o[1], 'different array elements')
  })
  test(label + ' – nested number', async ({is}) => {
    is(clone({a: 1}).a, 1, 'same value')
  })
  test(label + ' – nested string', async ({is}) => {
    is(clone({s: 'str'}).s, 'str', 'same value')
  })
  test(label + ' – nested boolean', async ({is}) => {
    is(clone({b: true}).b, true, 'same value')
  })
  test(label + ' – nested function', async ({is}) => {
    const fn = () => {}
    is(clone({fn}).fn, fn, 'same function')
  })
  test(label + ' – nested async function', async ({is}) => {
    const fn = async () => {}
    is(clone({fn}).fn, fn, 'same function')
  })
  test(label + ' – nested generator function', async ({is}) => {
    const fn = function * () {}
    is(clone({fn}).fn, fn, 'same function')
  })
  test(label + ' – nested date', async ({is, isNot}) => {
    const date = new Date()
    is(+clone({d: date}).d, +date, 'same value')
    isNot(clone({d: date}).d, date, 'different object')
  })
  test(label + ' – nested null', async ({is}) => {
    is(clone({n: null}).n, null, 'same value')
  })
  test(label + ' – arguments', async ({isNot, same}) => {
    function fn (...args) {
      same(clone(arguments), args, 'same values')
      isNot(clone(arguments), arguments, 'different object')
    }
    fn(1, 2, 3)
  })
}