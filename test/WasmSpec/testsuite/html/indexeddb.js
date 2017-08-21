/*
 * Copyright 2017 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

(function testIndexedDB() {
    const IDB_NAME = "spec-test";
    const OBJECT_STORE_NAME = "wasm";

    let db = null;

    function openDB() {
        console.log('Opening db...');
        return new Promise((resolve, reject) => {
            request = indexedDB.open(IDB_NAME, 1);
            request.onerror = reject;
            request.onsuccess = () => {
                db = request.result;
                console.log('Retrieved db:', db);
                resolve();
            };
            request.onupgradeneeded = () => {
                console.log('Creating object store...');
                request.result.createObjectStore(OBJECT_STORE_NAME);
                request.onerror = reject;
                request.onupgradeneeded = reject;
                request.onsuccess = () => {
                    db = request.result;
                    console.log('Created db:', db);
                    resolve();
                };
            };
        });
    }

    function getObjectStore() {
        return db.transaction([OBJECT_STORE_NAME], "readwrite").objectStore(OBJECT_STORE_NAME);
    }

    function clearStore() {
        console.log('Clearing store...');
        return new Promise((resolve, reject) => {
            var request = getObjectStore().clear();
            request.onerror = reject;
            request.onupgradeneeded = reject;
            request.onsuccess = resolve;
        });
    }

    function makeModule() {
        return new Promise(resolve => {
            let builder = new WasmModuleBuilder();
            builder.addFunction('run', kSig_i_v)
                .addBody([
                    kExprI32Const,
                    42,
                    kExprReturn,
                    kExprEnd
                ])
                .exportFunc();
            let source = builder.toBuffer();

            let module = new WebAssembly.Module(source);
            let i = new WebAssembly.Instance(module);
            assert_equals(i.exports.run(), 42);

            resolve(module);
        });
    }

    function storeWasm(module) {
        console.log('Storing wasm object...', module);
        return new Promise((resolve, reject) => {
            request = getObjectStore().add(module, 1);
            request.onsuccess = resolve;
            request.onerror = reject;
            request.onupgradeneeded = reject;
        });
    }

    function loadWasm() {
        console.log('Loading wasm object...');
        return new Promise((resolve, reject) => {
            var request = getObjectStore().get(1);
            request.onsuccess = () => {
                let i = new WebAssembly.Instance(request.result);
                assert_equals(i.exports.run(), 42);
                resolve();
            }
            request.onerror = reject;
            request.onupgradeneeded = reject;
        });
    }

    function run() {
        return openDB()
        .then(() => clearStore())
        .then(() => makeModule())
        .then(wasm => storeWasm(wasm))
        .then(() => loadWasm());
    }

    promise_test(run, "store and load from indexeddb");
})();
