
//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_TTD

namespace TTD
{
    //An enumeration of tags for the JsRT action tags (to support dispatch when parsing)
    //IMPORTANT: When adding a new JsRTAction subclass you need to add a corresponding typeid here
    enum class JsRTActionType : int
    {
        Invalid = 0x0,
        VarConvertToObject,
        AllocateObject,
        AllocateArray,
        AllocateArrayBuffer,
        AllocateFunction,
        GetAndClearException,
        GetProperty,
        GetIndex,
        GetOwnPropertyInfo,
        GetOwnPropertiesInfo,
        DefineProperty,
        DeleteProperty,
        SetPrototype,
        SetProperty,
        SetIndex,
        GetTypedArrayInfo,
        ConstructCall,
        CallbackOp,
        CodeParse,
        CallExistingFunctionBegin,
        CallExistingFunctionEnd,
        Limit
    };
    DEFINE_ENUM_FLAG_OPERATORS(JsRTActionType);

    //A class for logging calls into Chakra from JsRT
    class JsRTActionLogEntry : public EventLogEntry
    {
    private:
        //The action tag
        const JsRTActionType m_actionTypeTag;

        //The tag for the script context this action was executed on
        const TTD_LOG_TAG m_ctxTag;

    protected:
        void JsRTBaseEmit(FileWriter* writer) const;
        JsRTActionLogEntry(int64 eTime, TTD_LOG_TAG ctxTag, JsRTActionType actionTypeTag);

    public:

        //During replay get the script context that we want to apply the action to
        Js::ScriptContext* GetScriptContextForAction(ThreadContext* threadContext) const;

        //Get the event as a external call event (and do tag checking for consistency)
        static const JsRTActionLogEntry* As(const EventLogEntry* e);
        static JsRTActionLogEntry* As(EventLogEntry* e);

        //Get the action tag -- try not to use this too much
        JsRTActionType GetActionTypeTag() const;

        //We need to do some special things with root calls -- so check if this is a root call
        bool IsRootCallBegin();
        bool IsRootCallEnd();

        //true if this is exectued in the script context JsRT wrapper
        virtual bool IsExecutedInScriptWrapper() const;

        //Execute the JsRT action and return false if we should yield back to the enclosing scope (top-level or nested JS context)
        virtual void ExecuteAction(ThreadContext* threadContext) const = 0;

        static JsRTActionLogEntry* CompleteParse(bool readSeperator, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime);
    };

    //A class for converting variables
    class JsRTVarConvertToObjectAction : public JsRTActionLogEntry
    {
    private:
        const NSLogValue::ArgRetValue m_var;

    public:
        JsRTVarConvertToObjectAction(int64 eTime, TTD_LOG_TAG ctxTag, const NSLogValue::ArgRetValue& var);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTVarConvertToObjectAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for creating regular and external objects
    class JsRTObjectAllocateAction : public JsRTActionLogEntry
    {
    private:
        const bool m_isRegularObject;

    public:
        JsRTObjectAllocateAction(int64 eTime, TTD_LOG_TAG ctxTag, bool isRegularObject);

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTObjectAllocateAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for creating array objects
    class JsRTArrayAllocateAction : public JsRTActionLogEntry
    {
    private:
        const Js::TypeId m_arrayType;
        const uint32 m_length;

    public:
        JsRTArrayAllocateAction(int64 eTime, TTD_LOG_TAG ctxTag, Js::TypeId arrayType, uint32 length);

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTArrayAllocateAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for creating arraybuffer objects
    class JsRTArrayBufferAllocateAction : public JsRTActionLogEntry
    {
    private:
        const uint32 m_bufferSize;
        const byte* m_bufferData;

    public:
        JsRTArrayBufferAllocateAction(int64 eTime, TTD_LOG_TAG ctxTag, uint32 bufferSize, byte* bufferData);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTArrayBufferAllocateAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for creating function objects
    class JsRTFunctionAllocateAction : public JsRTActionLogEntry
    {
    private:
        const bool m_isNamed;
        const NSLogValue::ArgRetValue m_name;

    public:
        JsRTFunctionAllocateAction(int64 eTime, TTD_LOG_TAG ctxTag, bool isNamed, const NSLogValue::ArgRetValue& name);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTFunctionAllocateAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for getting and clearing the current script exception
    class JsRTGetAndClearExceptionAction : public JsRTActionLogEntry
    {
    public:
        JsRTGetAndClearExceptionAction(int64 eTime, TTD_LOG_TAG ctxTag);

        virtual bool IsExecutedInScriptWrapper() const override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTGetAndClearExceptionAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for getting a property from an object
    class JsRTGetPropertyAction : public JsRTActionLogEntry
    {
    private:
        const Js::PropertyId m_propertyId;
        const NSLogValue::ArgRetValue m_var;

    public:
        JsRTGetPropertyAction(int64 eTime, TTD_LOG_TAG ctxTag, Js::PropertyId pid, const NSLogValue::ArgRetValue& var);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTGetPropertyAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for getting an index from an object
    class JsRTGetIndexAction : public JsRTActionLogEntry
    {
    private:
        const NSLogValue::ArgRetValue m_index;
        const NSLogValue::ArgRetValue m_var;

    public:
        JsRTGetIndexAction(int64 eTime, TTD_LOG_TAG ctxTag, const NSLogValue::ArgRetValue& index, const NSLogValue::ArgRetValue& var);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTGetIndexAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for getting a property descriptor info from an object
    class JsRTGetOwnPropertyInfoAction : public JsRTActionLogEntry
    {
    private:
        const Js::PropertyId m_propertyId;
        const NSLogValue::ArgRetValue m_var;

    public:
        JsRTGetOwnPropertyInfoAction(int64 eTime, TTD_LOG_TAG ctxTag, Js::PropertyId pid, const NSLogValue::ArgRetValue& var);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTGetOwnPropertyInfoAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for getting own property names or symbols from an object
    class JsRTGetOwnPropertiesInfoAction : public JsRTActionLogEntry
    {
    private:
        const bool m_isGetNames;
        const NSLogValue::ArgRetValue m_var;

    public:
        JsRTGetOwnPropertiesInfoAction(int64 eTime, TTD_LOG_TAG ctxTag, bool isGetNames, const NSLogValue::ArgRetValue& var);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTGetOwnPropertiesInfoAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for defining a property descriptor on an object
    class JsRTDefinePropertyAction : public JsRTActionLogEntry
    {
    private:
        const NSLogValue::ArgRetValue m_var;
        const Js::PropertyId m_propertyId;
        const NSLogValue::ArgRetValue m_propertyDescriptor;

    public:
        JsRTDefinePropertyAction(int64 eTime, TTD_LOG_TAG ctxTag, const NSLogValue::ArgRetValue& var, Js::PropertyId pid, const NSLogValue::ArgRetValue& propertyDescriptor);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTDefinePropertyAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for deleting a property on an object
    class JsRTDeletePropertyAction : public JsRTActionLogEntry
    {
    private:
        const NSLogValue::ArgRetValue m_var;
        const Js::PropertyId m_propertyId;
        const bool m_useStrictRules;

    public:
        JsRTDeletePropertyAction(int64 eTime, TTD_LOG_TAG ctxTag, const NSLogValue::ArgRetValue& var, Js::PropertyId pid, bool useStrictRules);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTDeletePropertyAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for setting the prototype on an object
    class JsRTSetPrototypeAction : public JsRTActionLogEntry
    {
    private:
        const NSLogValue::ArgRetValue m_var;
        const NSLogValue::ArgRetValue m_proto;

    public:
        JsRTSetPrototypeAction(int64 eTime, TTD_LOG_TAG ctxTag, const NSLogValue::ArgRetValue& var, const NSLogValue::ArgRetValue& proto);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTSetPrototypeAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for setting a property on an object
    class JsRTSetPropertyAction : public JsRTActionLogEntry
    {
    private:
        const NSLogValue::ArgRetValue m_var;
        Js::PropertyId m_propertyId;
        const NSLogValue::ArgRetValue m_value;
        bool m_useStrictRules;

    public:
        JsRTSetPropertyAction(int64 eTime, TTD_LOG_TAG ctxTag, const NSLogValue::ArgRetValue& var, Js::PropertyId pid, const NSLogValue::ArgRetValue& value, bool useStrictRules);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTSetPropertyAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for setting a indexed property from an object/array
    class JsRTSetIndexAction : public JsRTActionLogEntry
    {
    private:
        const NSLogValue::ArgRetValue m_var;
        const NSLogValue::ArgRetValue m_index;
        const NSLogValue::ArgRetValue m_value;

    public:
        JsRTSetIndexAction(int64 eTime, TTD_LOG_TAG ctxTag, const NSLogValue::ArgRetValue& var, const NSLogValue::ArgRetValue& index, const NSLogValue::ArgRetValue& val);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTSetIndexAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for getting info from a typed array
    class JsRTGetTypedArrayInfoAction : public JsRTActionLogEntry
    {
    private:
        const bool m_returnsArrayBuff;
        const NSLogValue::ArgRetValue m_var;

    public:
        JsRTGetTypedArrayInfoAction(int64 eTime, TTD_LOG_TAG ctxTag, bool returnsArrayBuff, const NSLogValue::ArgRetValue& var);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTGetTypedArrayInfoAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for constructor calls
    class JsRTConstructCallAction : public JsRTActionLogEntry
    {
    private:
        //the function tag for the constructor
        const TTD_LOG_TAG m_functionTagId;

        //the number of arguments and the argument array
        const uint32 m_argCount;
        const NSLogValue::ArgRetValue* m_argArray;

        //A buffer we can use for the actual invocation
        Js::Var* m_execArgs;

    public:
        JsRTConstructCallAction(int64 eTime, TTD_LOG_TAG ctxTag, TTD_LOG_TAG functionTagId, uint32 argCount, NSLogValue::ArgRetValue* argArray, Js::Var* execArgs);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTConstructCallAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for correlating host callback ids that are registed/created/canceled by this call
    class JsRTCallbackAction : public JsRTActionLogEntry
    {
    private:
        //true if this is a cancelation action, false otherwise
        const bool m_isCancel;

        //true if this is a repeating action -- in which case executing it does not remove it from the active list
        const bool m_isRepeating;

        //The id of the current callback (we should be able to recompute this but we store it to simplify later analysis)
        const int64 m_currentCallbackId;

        //the function tag and name for the callback function and the id that we are associating it with
        const TTD_LOG_TAG m_callbackFunctionTag;
        const int64 m_createdCallbackId;

#if ENABLE_TTD_DEBUGGING
        //Info on the time that this registration occours
        mutable int64 m_register_eventTime;
        mutable uint64 m_register_ftime;
        mutable uint64 m_register_ltime;
        mutable uint32 m_register_line;
        mutable uint32 m_register_column;
        mutable uint32 m_register_sourceId;
#endif

    public:
        JsRTCallbackAction(int64 eTime, TTD_LOG_TAG ctxTag, bool isCancel, bool isRepeating, int64 currentCallbackId, TTD_LOG_TAG callbackFunctionTag, int64 createdCallbackId);

        static JsRTCallbackAction* As(JsRTActionLogEntry* action);

        //Get the id that is created/canceled by the host and which is associated with this callback/event
        int64 GetAssociatedHostCallbackId() const;

        //True if this is creating a repeating operation (e.g. it doesn't clear when it executes)
        bool IsRepeatingOp() const;

        //Check if this operation is crating a new callback or canceling an existing one
        bool IsCreateOp() const;
        bool IsCancelOp() const;

        //Get the time info for the debugger to set a breakpoint at this creation (return false if we haven't set the needed times yet)
        bool GetActionTimeInfoForDebugger(int64* rootEventTime, uint64* ftime, uint64* ltime, uint32* line, uint32* column, uint32* sourceId) const;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTCallbackAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for calls to script parse
    class JsRTCodeParseAction : public JsRTActionLogEntry
    {
    private:
        //The actual source code
        const TTString m_sourceCode;

        //If this is from a URI
        const TTString m_sourceUri;
        const DWORD_PTR m_documentID;

        //The flags for loading this script
        const LoadScriptFlag m_loadFlag;

        //The directory to write the source files out to (if needed)
        const TTString m_sourceFile;
        const TTString m_srcDir;

    public:
        JsRTCodeParseAction(int64 eTime, TTD_LOG_TAG ctxTag, const TTString& sourceCode, LoadScriptFlag loadFlag, DWORD_PTR documentId, const TTString& sourceUri, const TTString& srcDir, const TTString& sourceFile);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTCodeParseAction* CompleteParse(ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for calls to that execute existing functions (when we begin the callback)
    class JsRTCallFunctionBeginAction : public JsRTActionLogEntry
    {
    private:
        //the function tag and name for the callback function
        const TTD_LOG_TAG m_functionTagId;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        TTString m_functionName;
#endif

        //The re-entry depth we are at when this happens
        const int32 m_callbackDepth;

        //The id given by the host for this callback (or -1 if this call is not associated with any callback)
        const int64 m_hostCallbackId;

        const double m_beginTime;

        //the number of arguments and the argument array
        const uint32 m_argCount;
        const NSLogValue::ArgRetValue* m_argArray;

        //A buffer we can use for the actual invocation
        Js::Var* m_execArgs;

#if ENABLE_TTD_DEBUGGING
        //ready-to-run snapshot information -- null if not set and if we want to unload it we just throw it away
        mutable SnapShot* m_rtrSnap;
        mutable TTD_LOG_TAG m_rtrRestoreLogTag;
        mutable TTD_IDENTITY_TAG m_rtrRestoreIdentityTag;
#endif

    public:
        JsRTCallFunctionBeginAction(int64 eTime, TTD_LOG_TAG ctxTag, int32 callbackDepth, int64 hostCallbackId, double beginTime, TTD_LOG_TAG functionTagId, uint32 argCount, NSLogValue::ArgRetValue* argArray, Js::Var* execArgs);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        virtual void UnloadSnapshot() const override;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        void SetFunctionName(const TTString& fname);
#endif

        static JsRTCallFunctionBeginAction* As(JsRTActionLogEntry* action);

        //Get/Set/Clear the ready-to-run snapshot information (in debugging mode -- nops for replay mode)
        bool HasReadyToRunSnapshotInfo() const;
        void GetReadyToRunSnapshotInfo(SnapShot** snap, TTD_LOG_TAG* logTag, TTD_IDENTITY_TAG* identityTag) const;
        void SetReadyToRunSnapshotInfo(SnapShot* snap, TTD_LOG_TAG logTag, TTD_IDENTITY_TAG identityTag) const;

        //Get the call depth for this
        int32 GetCallDepth() const;

        //Get the host callback id info for this (toplevel) event
        int64 GetHostCallbackId() const;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTCallFunctionBeginAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for calls to that execute existing functions (when we complete the callback)
    class JsRTCallFunctionEndAction : public JsRTActionLogEntry
    {
    private:
        //The corresponding begin action time
        const int64 m_matchingBeginTime;

        //
        //TODO: later we should record more detail on the script exception for inflation if needed
        //
        const bool m_hasScriptException;
        const bool m_hasTerminiatingException;

        //The re-entry depth we are at when this happens
        const int32 m_callbackDepth;

        const double m_endTime;

#if ENABLE_TTD_DEBUGGING
        //Info on the last executed statement in this call
        mutable bool m_lastExecuted_valid;
        mutable uint64 m_lastExecuted_ftime;
        mutable uint64 m_lastExecuted_ltime;
        mutable uint32 m_lastExecuted_line;
        mutable uint32 m_lastExecuted_column;
        mutable uint32 m_lastExecuted_sourceId;
#endif

    public:
        JsRTCallFunctionEndAction(int64 eTime, TTD_LOG_TAG ctxTag, int64 matchingBeginTime, bool hasScriptException, bool hasTerminatingException, int32 callbackDepth, double endTime);

        static JsRTCallFunctionEndAction* As(JsRTActionLogEntry* action);

        //Set the last executed statement and frame (in debugging mode -- nops for replay mode)
        void SetLastExecutedStatementAndFrameInfo(const SingleCallCounter& lastFrame) const;
        bool GetLastExecutedStatementAndFrameInfoForDebugger(int64* rootEventTime, uint64* ftime, uint64* ltime, uint32* line, uint32* column, uint32* sourceId) const;

        //Get the event time for the matching call begin event
        int64 GetMatchingCallBegin() const;

        //Get the call depth for this
        int32 GetCallDepth() const;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTCallFunctionEndAction* CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };
}

#endif

