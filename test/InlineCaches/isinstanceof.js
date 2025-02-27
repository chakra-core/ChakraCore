//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// @ts-check

function inherit(child, parent) {
    child.prototype = Object.create(parent.prototype);
    child.prototype.constructor = child;
}

// #region setup "classes"
function ComponentBase() { }
function Component() { }
inherit(Component, ComponentBase);

function Health() { }
inherit(Health, Component);

function ShapeBaseBase() { }
function ShapeBase() { }
inherit(ShapeBase, ShapeBaseBase);
function Shape() { }
inherit(Shape, ShapeBase);

function Box() { }
inherit(Box, Shape);

function Circle() { }
inherit(Circle, Shape);
// #endregion

// #region check functions
function checkA(a, b) {
    return a instanceof b;
}

function checkB(a, b) {
    return a instanceof b;
}

function checkC(a, b) {
    return a instanceof b;
}

function checkD(a, b) {
    return a instanceof b;
}

function checkE(a, b) {
    return a instanceof b;
}
// #endregion

console.log(':Box:Component:', checkA(Box.prototype, Component));

console.log(':Component:ComponentBase:', checkB(Component.prototype, ComponentBase));

console.log(':Health:Component:', checkC(Health.prototype, Component));

console.log(':Shape:ShapeBase:', checkD(Shape.prototype, ShapeBase));

console.log(':ShapeBase:ShapeBaseBase:', checkE(ShapeBase.prototype, ShapeBaseBase));

console.log(" -> Change prototype");
Object.setPrototypeOf(Shape.prototype, Component.prototype);

// Box inherits Shape (changed) -> should invalidate cache (NoCacheHit)
console.log(':Box:Component:', checkA(Box.prototype, Component), Box.prototype instanceof Component);

// New prototype chain remains valid (CacheHit)
console.log(':Component:ComponentBase:', checkB(Component.prototype, ComponentBase), Component.prototype instanceof ComponentBase);

// Seperate prototype chain remains valid (CacheHit)
console.log(':Health:Component:', checkC(Health.prototype, Component), Health.prototype instanceof Component);

// We changed that! (NoCacheHit)
console.log(':Shape:ShapeBase:', checkD(Shape.prototype, ShapeBase), Shape.prototype instanceof ShapeBase);

// Old prototype chain remains valid (CacheHit)
console.log(':ShapeBase:ShapeBaseBase:', checkE(ShapeBase.prototype, ShapeBaseBase), ShapeBase.prototype instanceof ShapeBaseBase);


console.log("====================");


console.log(':Box:Circle', checkA(Box.prototype, Circle));

console.log(" -> Change prototype");
Object.setPrototypeOf(Circle.prototype, Shape.prototype);

// Modifying the function (constructor) does not invalidate the cache
console.log(':Box:Circle', checkA(Box.prototype, Circle), Box.prototype instanceof Circle);

