//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "JsrtPch.h"
#ifdef ENABLE_SCRIPT_DEBUGGING
#include "JsrtDebugEventObject.h"
#include "RuntimeDebugPch.h"
#include "screrror.h"   // For CompileScriptException

JsrtDebugEventObject::JsrtDebugEventObject(Js::ScriptContext *scriptContext)
{
    Assert(scriptContext != nullptr);
    this->scriptContext = scriptContext;
    this->eventDataObject = scriptContext->GetLibrary()->CreateObject();
    Assert(this->eventDataObject != nullptr);
}

JsrtDebugEventObject::~JsrtDebugEventObject()
{
    this->eventDataObject = nullptr;
    this->scriptContext = nullptr;
}

Js::DynamicObject* JsrtDebugEventObject::GetEventDataObject()
{
    return this->eventDataObject;
}

JsrtDebugDocumentManager::JsrtDebugDocumentManager(JsrtDebugManager* jsrtDebugManager) :
    breakpointDebugDocumentDictionary(nullptr)
{
    Assert(jsrtDebugManager != nullptr);
    this->jsrtDebugManager = jsrtDebugManager;
}

JsrtDebugDocumentManager::~JsrtDebugDocumentManager()
{
    if (this->breakpointDebugDocumentDictionary != nullptr)
    {
        AssertMsg(this->breakpointDebugDocumentDictionary->Count() == 0, "Should have cleared all entries by now?");

        Adelete(this->jsrtDebugManager->GetDebugObjectArena(), this->breakpointDebugDocumentDictionary);

        this->breakpointDebugDocumentDictionary = nullptr;
    }
    this->jsrtDebugManager = nullptr;
}

void JsrtDebugDocumentManager::AddDocument(UINT bpId, Js::DebugDocument * debugDocument)
{
    BreakpointDebugDocumentDictionary* breakpointDebugDocumentDictionary = this->GetBreakpointDictionary();

    Assert(!breakpointDebugDocumentDictionary->ContainsKey(bpId));

    breakpointDebugDocumentDictionary->Add(bpId, debugDocument);
}

void JsrtDebugDocumentManager::ClearDebugDocument(Js::ScriptContext * scriptContext)
{
    if (scriptContext != nullptr)
    {
        scriptContext->MapScript([&](Js::Utf8SourceInfo* sourceInfo)
        {
            if (sourceInfo->HasDebugDocument())
            {
                Js::DebugDocument* debugDocument = sourceInfo->GetDebugDocument();

                // Remove the debugdocument from breakpoint dictionary
                if (this->breakpointDebugDocumentDictionary != nullptr)
                {
                    this->breakpointDebugDocumentDictionary->MapAndRemoveIf([&](JsUtil::SimpleDictionaryEntry<UINT, Js::DebugDocument *> keyValue)
                    {
                        if (keyValue.Value() != nullptr && keyValue.Value() == debugDocument)
                        {
                            return true;
                        }
                        return false;
                    });
                }

                debugDocument->GetUtf8SourceInfo()->ClearDebugDocument();
                HeapDelete(debugDocument);
                debugDocument = nullptr;
            }
        });
    }
}

void JsrtDebugDocumentManager::ClearBreakpointDebugDocumentDictionary()
{
    if (this->breakpointDebugDocumentDictionary != nullptr)
    {
        this->breakpointDebugDocumentDictionary->Clear();
    }
}

bool JsrtDebugDocumentManager::RemoveBreakpoint(UINT breakpointId)
{
    if (this->breakpointDebugDocumentDictionary != nullptr)
    {
        BreakpointDebugDocumentDictionary* breakpointDebugDocumentDictionary = this->GetBreakpointDictionary();
        Js::DebugDocument* debugDocument = nullptr;
        if (breakpointDebugDocumentDictionary->TryGetValue(breakpointId, &debugDocument))
        {
            Js::StatementLocation statement;
            if (debugDocument->FindBPStatementLocation(breakpointId, &statement))
            {
                debugDocument->SetBreakPoint(statement, BREAKPOINT_DELETED);
                return true;
            }
        }
    }

    return false;
}

JsrtDebugDocumentManager::BreakpointDebugDocumentDictionary * JsrtDebugDocumentManager::GetBreakpointDictionary()
{
    if (this->breakpointDebugDocumentDictionary == nullptr)
    {
        this->breakpointDebugDocumentDictionary = Anew(this->jsrtDebugManager->GetDebugObjectArena(), BreakpointDebugDocumentDictionary, this->jsrtDebugManager->GetDebugObjectArena(), 10);
    }
    return breakpointDebugDocumentDictionary;
}
#endif