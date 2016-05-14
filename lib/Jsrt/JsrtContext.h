//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "JsrtRuntime.h"

// This class abstract a pointer value with its last 2 bits set to avoid conservative GC tracking.
template <class T>
class GC_MARKED_OBJECT
{
public:
    operator T*()          const { return GetPointerValue(); }
    bool operator!= (T* p) const { return GetPointerValue() != p; }
    bool operator== (T* p) const { return GetPointerValue() == p; }
    T* operator-> ()       const { return GetPointerValue(); }
    GC_MARKED_OBJECT<T>& operator= (T* inPtr)
    {
        SetPointerValue(inPtr);
        return (*this);
    }
    GC_MARKED_OBJECT(T* inPtr) : ptr(inPtr)
    {
        SetPointerValue(inPtr);
    }

    GC_MARKED_OBJECT() : ptr(NULL) {};
private:
    T * GetPointerValue() const { return reinterpret_cast<T*>(reinterpret_cast<ULONG_PTR>(ptr) & ~3); }
    T * SetPointerValue(T* inPtr)
    {
        AssertMsg((reinterpret_cast<ULONG_PTR>(inPtr) & 3) == 0, "Invalid pointer value, 2 least significant bits must be zero");
        ptr = reinterpret_cast<T*>((reinterpret_cast<ULONG_PTR>(inPtr) | 3));
        return ptr;
    }

    T* ptr;
};

class JsrtContext : public FinalizableObject
{
public:
    static JsrtContext *New(JsrtRuntime * runtime);

    Js::ScriptContext * GetScriptContext() const { return this->javascriptLibrary->scriptContext; }
    Js::JavascriptLibrary* GetJavascriptLibrary() const { return this->javascriptLibrary; }
    JsrtRuntime * GetRuntime() const { return this->runtime; }
    void* GetExternalData() const { return this->externalData; }
    void SetExternalData(void * data) { this->externalData = data; }

    static bool Initialize();
    static void Uninitialize();
    static JsrtContext * GetCurrent();
    static bool TrySetCurrent(JsrtContext * context);
    static bool Is(void * ref);

    virtual void Finalize(bool isShutdown) override sealed;
    virtual void Mark(Recycler * recycler) override sealed;

    void OnScriptLoad(Js::JavascriptFunction * scriptFunction, Js::Utf8SourceInfo* utf8SourceInfo, CompileScriptException* compileException);
protected:
    DEFINE_VTABLE_CTOR_NOBASE(JsrtContext);
    JsrtContext(JsrtRuntime * runtime);
    void Link();
    void Unlink();
    void SetJavascriptLibrary(Js::JavascriptLibrary * library);
    void PinCurrentJsrtContext();
private:
    static DWORD s_tlsSlot;
    Js::JavascriptLibrary * javascriptLibrary;

    JsrtRuntime * runtime;
    void* externalData = nullptr;
    GC_MARKED_OBJECT<JsrtContext> previous;
    GC_MARKED_OBJECT<JsrtContext> next;
};

