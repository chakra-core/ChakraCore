//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

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


class JsrtDebugDocumentManager
{
public:
    JsrtDebugDocumentManager(JsrtDebugManager* jsrtDebugManager);
    ~JsrtDebugDocumentManager();
    void AddDocument(UINT bpId, Js::DebugDocument* debugDocument);
    void ClearDebugDocument(Js::ScriptContext * scriptContext);
    void ClearBreakpointDebugDocumentDictionary();
    bool RemoveBreakpoint(UINT breakpointId);
private:
    JsrtDebugManager* jsrtDebugManager;

    typedef JsUtil::BaseDictionary<uint, Js::DebugDocument*, ArenaAllocator> BreakpointDebugDocumentDictionary;
    BreakpointDebugDocumentDictionary* breakpointDebugDocumentDictionary;

    BreakpointDebugDocumentDictionary* GetBreakpointDictionary();
};
