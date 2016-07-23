//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class WScriptJsrt
{
public:
    static bool Initialize();

    class CallbackMessage : public MessageBase
    {
        JsValueRef m_function;

        CallbackMessage(CallbackMessage const&);

    public:
        CallbackMessage(unsigned int time, JsValueRef function);
        ~CallbackMessage();

        HRESULT Call(LPCSTR fileName);
        HRESULT CallFunction(LPCSTR fileName);
        template <class Func>
        static CallbackMessage* Create(JsValueRef function, const Func& func, unsigned int time = 0)
        {
            return new CustomMessage<Func, CallbackMessage>(time, function, func);
        }
    };

    class ModuleMessage : public MessageBase
    {
    private:
        JsModuleRecord moduleRecord;
        JsValueRef specifier;

        ModuleMessage(JsModuleRecord module, JsValueRef specifier);

    public:
        ~ModuleMessage();

        virtual HRESULT Call(LPCSTR fileName) override;

        static ModuleMessage* Create(JsModuleRecord module, JsValueRef specifier)
        {
            return new ModuleMessage(module, specifier);
        }

    };

    static void AddMessageQueue(MessageQueue *messageQueue);
    static void PushMessage(MessageBase *message) { messageQueue->InsertSorted(message); }

    static JsErrorCode FetchImportedModule(_In_ JsModuleRecord referencingModule, _In_ JsValueRef specifier, _Outptr_result_maybenull_ JsModuleRecord* dependentModuleRecord);
    static JsErrorCode NotifyModuleReadyCallback(_In_opt_ JsModuleRecord referencingModule, _In_opt_ JsValueRef exceptionVar);

    static LPCWSTR ConvertErrorCodeToMessage(JsErrorCode errorCode)
    {
        switch (errorCode)
        {
        case (JsErrorCode::JsErrorInvalidArgument) :
            return _u("TypeError: InvalidArgument");
        case (JsErrorCode::JsErrorNullArgument) :
            return _u("TypeError: NullArgument");
        case (JsErrorCode::JsErrorArgumentNotObject) :
            return _u("TypeError: ArgumentNotAnObject");
        case (JsErrorCode::JsErrorOutOfMemory) :
            return _u("OutOfMemory");
        case (JsErrorCode::JsErrorScriptException) :
            return _u("ScriptError");
        case (JsErrorCode::JsErrorScriptCompile) :
            return _u("SyntaxError");
        case (JsErrorCode::JsErrorFatal) :
            return _u("FatalError");
        case (JsErrorCode::JsErrorInExceptionState) :
            return _u("ErrorInExceptionState");
        default:
            AssertMsg(false, "Unexpected JsErrorCode");
            return nullptr;
        }
    }

    static bool PrintException(LPCSTR fileName, JsErrorCode jsErrorCode);
    static JsValueRef LoadScript(JsValueRef callee, LPCSTR fileName, LPCSTR fileContent, LPCSTR scriptInjectType, bool isSourceModule);
    static DWORD_PTR GetNextSourceContext();
    static JsValueRef LoadScriptFileHelper(JsValueRef callee, JsValueRef *arguments, unsigned short argumentCount, bool isSourceModule);
    static JsValueRef LoadScriptHelper(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState, bool isSourceModule);
    static bool InstallObjectsOnObject(JsValueRef object, const char* name, JsNativeFunction nativeFunction);
private:
    static bool CreateArgumentsObject(JsValueRef *argsObject);
    static bool CreateNamedFunction(const char*, JsNativeFunction callback, JsValueRef* functionVar);
    static JsValueRef __stdcall EchoCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef __stdcall QuitCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef __stdcall LoadScriptFileCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef __stdcall LoadScriptCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef __stdcall LoadModuleCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef __stdcall SetTimeoutCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef __stdcall ClearTimeoutCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef __stdcall AttachCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef __stdcall DetachCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef __stdcall DumpFunctionPositionCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef __stdcall RequestAsyncBreakCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);

    static JsValueRef __stdcall EmptyCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsErrorCode __stdcall LoadModuleFromString(LPCSTR fileName, LPCSTR fileContent);
    static JsErrorCode __stdcall InitializeModuleInfo(JsValueRef specifier, JsModuleRecord moduleRecord);

    static MessageQueue *messageQueue;
    static DWORD_PTR sourceContext;
    static std::map<std::string, JsModuleRecord> moduleRecordMap;
};
