//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Test HostPromiseRejectionTracker - see ecma262 section 25.4.1.9

let tests = [
    {
        name: "Reject promise with no reactions.",
        body: function(index)
        {
            let controller;
            let promise = new Promise((resolve, reject)=>{
                controller = {resolve, reject};
            });
            controller.reject("Rejection from test " +  index);//Should notify rejected
        }
    },
    {
        name: "Reject promise with a catch reaction only.",
        body: function(index)
        {
            let controller;
            let promise = new Promise((resolve, reject)=>{
                controller = {resolve, reject};
            }).catch(()=>{});
            controller.reject("Rejection from test " +  index);//Should NOT notify
        }
    },
    {
        name: "Reject promise with catch and then reactions.",
        body: function(index)
        {
            let controller;
            let promise = new Promise((resolve, reject)=>{
                controller = {resolve, reject};
            }).then(()=>{}).catch(()=>{});
            controller.reject("Rejection from test " +  index);//Should NOT notify
        }
    },
    {
        name: "Reject promise then add  a catch afterwards.",
        body: function(index)
        {
            let controller;
            let promise = new Promise((resolve, reject)=>{
                controller = {resolve, reject};
            });
            controller.reject("Rejection from test " +  index);//Should notify rejected
            promise.catch(()=>{});//Should notify handled
        }
    },
    {
        name: "Reject promise then add two catches afterwards.",
        body: function(index)
        {
            let controller;
            let promise = new Promise((resolve, reject)=>{
                controller = {resolve, reject};
            });
            controller.reject("Rejection from test " +  index);//Should notify rejected
            promise.catch(()=>{});//Should notify handled
            promise.catch(()=>{});//Should NOT notify
        }
    },
    {
        name: "Async function that throws.",
        body: function(index)
        {
            async function aFunction()
            {
                throw ("Rejection from test " +  index);
            }
            aFunction();//Should notify rejected
        }
    },
    {
        name: "Async function that throws but is caught.",
        body: function(index)
        {
            async function aFunction()
            {
                throw ("Rejection from test " +  index);
            }
            aFunction().catch(()=>{});//Should notify rejected AND then handled
        }
    },
    {
        name: "Async function that awaits a function that throws.",
        body: function(index)
        {
            async function aFunction()
            {
                throw ("Rejection from test " +  index);//Should notify rejected
            }
            async function bFunction()
            {
                await aFunction();//Should notify handled
            }
            bFunction();//Should notify rejected in the async section
        },
    },
    {
        name: "Reject a handled promise then handle one of the handles but not the other.",
        body: function(index)
        {
            let controller;
            let promise = new Promise((resolve, reject) => {  controller = {resolve, reject};});
            let a = promise.then(() => {});//a is not handled
            let b = promise.then(() => {});//b is not handled
            controller.reject("Rejection from test " +  index);//no notification as handled

            let c = a.then(() => {}); //handle a

            c.catch(() => {b.then(()=>{})}); // handle c
            //b is still not handled -> notify once in async section
            //b has an async handler -> will notify handled in async section
            //the .then() on b is not handled so will notify in async section
        },
    },
    {
        name: "Reject a handled promise and don't handle either path.",
        body: function(index)
        {
            let controller;
            let promise = new Promise((resolve, reject) => {  controller = {resolve, reject};});
            let a = promise.then(() => {});//a is not handled
            let b = promise.then(() => {});//b is not handled
            controller.reject("Rejection from test " +  index);//no notification as handled

            let c = a.then(() => {}); //handle a

            //b is not handled -> will notify in async section
            //c is not handled -> will notify in async section
        }
    }
];

for(let i = 0; i < tests.length; ++i)
{
    WScript.Echo('Executing test #' + (i + 1) + ' - ' + tests[i].name);
    tests[i].body(i+1);
}
WScript.Echo("Begin async results:");
