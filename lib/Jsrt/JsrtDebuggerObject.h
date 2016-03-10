//---------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

class JsrtDebug;
class DebuggerObjectsManager;

// Type of objects we give to debugger
typedef enum DebuggerObjectType
{
    DebuggerObjectType_Function,
    DebuggerObjectType_Globals,
    DebuggerObjectType_Property,
    DebuggerObjectType_Scope,
    DebuggerObjectType_Script,
    DebuggerObjectType_StackFrame
} DebuggerObjectType;

// Base class representing a debugger object
class DebuggerObjectBase {
public:
    DebuggerObjectBase(DebuggerObjectType type, DebuggerObjectsManager* debuggerObjectsManager);
    virtual ~DebuggerObjectBase();

    DebuggerObjectType GetType() { return type; }
    uint GetHandle() const { return handle; }
    DebuggerObjectsManager* GetDebuggerObjectsManager();
    virtual Js::DynamicObject* GetJSONObject(Js::ScriptContext* scriptContext) = 0;
    virtual Js::DynamicObject* GetChildrens(Js::ScriptContext* scriptContext, uint fromCount, uint totalCount);

    template<class DebuggerObjectType, class PostFunction>
    static void CreateDebuggerObject(DebuggerObjectsManager* debuggerObjectsManager, Js::ResolvedObject resolvedObject, Js::ScriptContext* scriptContext, PostFunction postFunction)
    {
        AutoPtr<WeakArenaReference<Js::IDiagObjectModelDisplay>> objectDisplayWeakRef = resolvedObject.GetObjectDisplay();
        Js::IDiagObjectModelDisplay* objectDisplay = objectDisplayWeakRef->GetStrongReference();
        if (objectDisplay != nullptr)
        {
            DebuggerObjectBase* debuggerObject = DebuggerObjectType::Make(debuggerObjectsManager, objectDisplayWeakRef);
            Js::DynamicObject* object = debuggerObject->GetJSONObject(resolvedObject.scriptContext);
            Assert(object != nullptr);
            Js::Var marshaledObj = Js::CrossSite::MarshalVar(scriptContext, object);
            postFunction(marshaledObj);
            objectDisplayWeakRef.Detach();
        }
    }

protected:
    Js::DynamicObject* GetChildrens(WeakArenaReference<Js::IDiagObjectModelWalkerBase>* walkerRef, Js::ScriptContext * scriptContext, uint fromCount, uint totalCount);
private:
    DebuggerObjectType type;
    uint handle;
    DebuggerObjectsManager* debuggerObjectsManager;
};

// Class managing objects we give to debugger, it maintains various mappings
class DebuggerObjectsManager
{
public:
    DebuggerObjectsManager(JsrtDebug* debugObject);
    ~DebuggerObjectsManager();

    void ClearAll();
    JsrtDebug* GetDebugObject() { return this->debugObject; };
    ArenaAllocator* GetDebugObjectArena();
    uint GetNextHandle() { return ++handleId; }

    bool TryGetDebuggerObjectFromHandle(uint handle, DebuggerObjectBase** debuggerObject);
    bool TryGetFrameObjectFromFrameIndex(uint frameIndex, DebuggerObjectBase** debuggerObject);

    void AddToDataToDebuggerObjectsDictionary(void* data, DebuggerObjectBase* debuggerObject);
    bool TryGetDataFromDataToDebuggerObjectsDictionary(void* data, DebuggerObjectBase** debuggerObject);

private:
    uint handleId;
    JsrtDebug* debugObject;

    typedef JsUtil::BaseDictionary<uint, DebuggerObjectBase*, ArenaAllocator> DebuggerObjectsDictionary;
    DebuggerObjectsDictionary* handleToDebuggerObjectsDictionary;

    typedef JsUtil::BaseDictionary<void*, DebuggerObjectBase*, ArenaAllocator> DataToDebuggerObjectsDictionary;
    DataToDebuggerObjectsDictionary* dataToDebuggerObjectsDictionary;

    void AddToDebuggerObjectsDictionary(DebuggerObjectBase* debuggerObject);
};

class DebuggerObjectFunction : public DebuggerObjectBase
{
public:
    static DebuggerObjectBase* Make(DebuggerObjectsManager* debuggerObjectsManager, Js::FunctionBody* functionBody);
    Js::DynamicObject* GetJSONObject(Js::ScriptContext* scriptContext);

private:
    DebuggerObjectFunction(DebuggerObjectsManager* debuggerObjectsManager, Js::FunctionBody* functionBody);
    ~DebuggerObjectFunction();
    Js::FunctionBody* functionBody;
    Js::DynamicObject* functionObject;
};

class DebuggerObjectProperty : public DebuggerObjectBase
{
public:
    static DebuggerObjectBase* Make(DebuggerObjectsManager* debuggerObjectsManager, WeakArenaReference<Js::IDiagObjectModelDisplay>* objectDisplay);

    Js::DynamicObject* GetJSONObject(Js::ScriptContext* scriptContext);
    Js::DynamicObject* GetChildrens(Js::ScriptContext* scriptContext, uint fromCount, uint totalCount);

private:
    DebuggerObjectProperty(DebuggerObjectsManager* debuggerObjectsManager, WeakArenaReference<Js::IDiagObjectModelDisplay>* objectDisplay);
    ~DebuggerObjectProperty();
    WeakArenaReference<Js::IDiagObjectModelDisplay>* objectDisplay;
    WeakArenaReference<Js::IDiagObjectModelWalkerBase>* walkerRef;
    Js::DynamicObject* propertyObject;
};

class DebuggerObjectGlobalsNode : public DebuggerObjectBase
{
public:
    static DebuggerObjectBase* Make(DebuggerObjectsManager* debuggerObjectsManager, WeakArenaReference<Js::IDiagObjectModelDisplay>* objectDisplay);

    Js::DynamicObject* GetJSONObject(Js::ScriptContext* scriptContext);
    Js::DynamicObject* GetChildrens(Js::ScriptContext* scriptContext, uint fromCount, uint totalCount);

private:
    DebuggerObjectGlobalsNode(DebuggerObjectsManager* debuggerObjectsManager, WeakArenaReference<Js::IDiagObjectModelDisplay>* objectDisplay);
    ~DebuggerObjectGlobalsNode();
    WeakArenaReference<Js::IDiagObjectModelDisplay>* objectDisplay;
    WeakArenaReference<Js::IDiagObjectModelWalkerBase>* walkerRef;
    Js::DynamicObject* propertyObject;
};

class DebuggerObjectScope : public DebuggerObjectBase
{
public:
    static DebuggerObjectBase* Make(DebuggerObjectsManager* debuggerObjectsManager, WeakArenaReference<Js::IDiagObjectModelDisplay>* objectDisplay, uint index);

    Js::DynamicObject* GetJSONObject(Js::ScriptContext* scriptContext);
    Js::DynamicObject* GetChildrens(Js::ScriptContext* scriptContext, uint fromCount, uint totalCount);

private:
    DebuggerObjectScope(DebuggerObjectsManager* debuggerObjectsManager, WeakArenaReference<Js::IDiagObjectModelDisplay>* objectDisplay, uint index);
    ~DebuggerObjectScope();
    WeakArenaReference<Js::IDiagObjectModelDisplay>* objectDisplay;
    WeakArenaReference<Js::IDiagObjectModelWalkerBase>* walkerRef;
    Js::DynamicObject* scopeObject;
    uint index;
};

class DebuggerObjectScript : public DebuggerObjectBase
{
public:
    static DebuggerObjectBase* Make(DebuggerObjectsManager* debuggerObjectsManager, Js::Utf8SourceInfo* utf8SourceInfo);

    Js::DynamicObject* GetJSONObject(Js::ScriptContext* scriptContext);

private:
    DebuggerObjectScript(DebuggerObjectsManager* debuggerObjectsManager, Js::Utf8SourceInfo* utf8SourceInfo);
    ~DebuggerObjectScript();

    Js::Utf8SourceInfo* utf8SourceInfo;
    Js::DynamicObject* sourceObject;
};

class DebuggerObjectStackFrame : public DebuggerObjectBase
{
public:
    static DebuggerObjectBase* Make(DebuggerObjectsManager* debuggerObjectsManager, Js::DiagStackFrame* stackFrame, uint frameIndex);

    Js::DynamicObject* GetJSONObject(Js::ScriptContext* scriptContext);
    Js::DynamicObject* GetLocalsObject();
    Js::DynamicObject* Evaluate(const char16* pszSrc, bool isLibraryCode);
    uint GetIndex() const { return this->frameIndex; }

private:
    DebuggerObjectStackFrame(DebuggerObjectsManager* debuggerObjectsManager, Js::DiagStackFrame* stackFrame, uint frameIndex);
    ~DebuggerObjectStackFrame();

    uint frameIndex;
    Js::DiagStackFrame* stackFrame;
    WeakArenaReference<Js::IDiagObjectModelWalkerBase>* pObjectModelWalker;
    Js::DynamicObject* stackTraceObject;
    Js::DynamicObject* propertiesObject;
};

class JsrtDebugStackFrames
{
public:
    JsrtDebugStackFrames(JsrtDebug* debugObject);
    ~JsrtDebugStackFrames();
    Js::JavascriptArray* StackFrames(Js::ScriptContext* scriptContext);
    bool TryGetFrameObjectFromFrameIndex(uint frameIndex, DebuggerObjectBase ** debuggerObject);
private:
    Js::DynamicObject* GetStackFrame(Js::DiagStackFrame * stackFrame, uint frameIndex);
    Js::JavascriptArray* stackTraceArray;
    JsrtDebug* debugObject;

    typedef JsUtil::BaseDictionary<uint, DebuggerObjectBase*, ArenaAllocator> FramesDictionary;
    FramesDictionary* framesDictionary;

};
