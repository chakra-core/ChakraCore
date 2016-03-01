//---------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

class JsrtDebugEventObject
{
public:
    JsrtDebugEventObject(Js::ScriptContext *scriptContext);
    ~JsrtDebugEventObject();
    Js::DynamicObject* GetEventDataObject();
private:
    Js::DynamicObject* eventDataObject;
    Js::ScriptContext *scriptContext;
};


class DebugDocumentManager
{
public:
    DebugDocumentManager(JsrtDebug* debugObject);
    ~DebugDocumentManager();
    void AddDocument(UINT bpId, Js::DebugDocument* debugDocument);
    void ClearDebugDocument(Js::ScriptContext * scriptContext);
    bool RemoveBreakpoint(UINT breakpointId);
private:
    JsrtDebug* debugObject;
    typedef JsUtil::BaseDictionary<uint, Js::DebugDocument*, ArenaAllocator> BreakpointDebugDocumentDictionary;
    BreakpointDebugDocumentDictionary* breakpointDebugDocumentDictionary;

    BreakpointDebugDocumentDictionary* GetBreakpointDictionary();
};
