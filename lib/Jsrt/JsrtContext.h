//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "JsrtRuntime.h"

class JsrtContext : public FinalizableObject
{
public:
    static JsrtContext *New(JsrtRuntime * runtime);

    Js::ScriptContext * GetScriptContext() const { return this->javascriptLibrary->scriptContext; }
    Js::JavascriptLibrary* GetJavascriptLibrary() const { return this->javascriptLibrary; }
    JsrtRuntime * GetRuntime() const { return this->runtime; }
    void* GetExternalData() const { return this->externalData; }
    void SetExternalData(void * data) { this->externalData = data; }

    static JsrtContext * GetCurrent();
    static bool TrySetCurrent(JsrtContext * context);
    static bool Is(void * ref);

    virtual void Mark(Recycler * recycler) override sealed;

#if ENABLE_TTD
    void OnScriptLoad_TTDCallback(Js::FunctionBody* body, Js::Utf8SourceInfo* utf8SourceInfo, CompileScriptException* compileException, bool notify);
    static void OnReplayDisposeContext_TTDCallback(FinalizableObject* jsrtCtx);
#endif
    void OnScriptLoad(Js::JavascriptFunction * scriptFunction, Js::Utf8SourceInfo* utf8SourceInfo, CompileScriptException* compileException);
protected:
    DEFINE_VTABLE_CTOR_NOBASE(JsrtContext);
    JsrtContext(JsrtRuntime * runtime);
    void Link();
    void Unlink();
    void SetJavascriptLibrary(Js::JavascriptLibrary * library);
private:
    Field(Js::JavascriptLibrary *) javascriptLibrary;

    Field(JsrtRuntime *) runtime;
    Field(void*) externalData = nullptr;
    Field(TaggedPointer<JsrtContext>) previous;
    Field(TaggedPointer<JsrtContext>) next;
};
