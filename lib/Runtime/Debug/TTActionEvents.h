
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
        AllocateNumber,
        VarConvert,
        GetAndClearException,
        GetProperty,
        CallbackOp,
        CodeParse,
        CallExistingFunction,
        Limit
    };
    DEFINE_ENUM_FLAG_OPERATORS(JsRTActionType);

    //A class for logging calls into Chakra from JsRT
    class JsRTActionLogEntry : public EventLogEntry
    {
    private:
        //The action tag
        JsRTActionType m_actionTypeTag;

        //The tag for the script context this action was executed on
        TTD_LOG_TAG m_ctxTag;

    protected:
        void JsRTBaseEmit(FileWriter* writer) const;
        JsRTActionLogEntry(int64 eTime, TTD_LOG_TAG ctxTag, JsRTActionType actionTypeTag);

    public:
        virtual ~JsRTActionLogEntry() override;

        //During replay get the script context that we want to apply the action to
        Js::ScriptContext* GetScriptContextForAction(ThreadContext* threadContext) const;

        //Get the event as a external call event (and do tag checking for consistency)
        static JsRTActionLogEntry* As(EventLogEntry* e);

        //Get the action tag -- try not to use this too much
        JsRTActionType GetActionTypeTag() const;

        //We need to do some special things with root calls -- so check if this is a root call
        bool IsRootCall();

        //Execute the JsRT action and return false if we should yield back to the enclosing scope (top-level or nested JS context)
        virtual void ExecuteAction(ThreadContext* threadContext) const = 0;

        static JsRTActionLogEntry* CompleteParse(bool readSeperator, ThreadContext* threadContext, FileReader* reader, SlabAllocator& alloc, int64 eTime);
    };

    //A class for creating numbers
    class JsRTNumberAllocateAction : public JsRTActionLogEntry
    {
    private:
        bool m_useIntRepresentation;

        union
        {
            int32 u_ival;
            double u_dval;
        };

    public:
        JsRTNumberAllocateAction(int64 eTime, TTD_LOG_TAG ctxTag, bool useIntRepresentation, int32 ival, double dval); 
        virtual ~JsRTNumberAllocateAction() override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTNumberAllocateAction* CompleteParse(FileReader* reader, SlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for converting variables
    class JsRTVarConvertAction : public JsRTActionLogEntry
    {
    private:
        bool m_toBool;
        bool m_toNumber;
        bool m_toString;

        NSLogValue::ArgRetValue* m_var;

    public:
        JsRTVarConvertAction(int64 eTime, TTD_LOG_TAG ctxTag, bool toBool, bool toNumber, bool toString, NSLogValue::ArgRetValue* var);
        virtual ~JsRTVarConvertAction() override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTVarConvertAction* CompleteParse(FileReader* reader, SlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for getting and clearing the current script exception
    class JsRTGetAndClearExceptionAction : public JsRTActionLogEntry
    {
    public:
        JsRTGetAndClearExceptionAction(int64 eTime, TTD_LOG_TAG ctxTag);
        virtual ~JsRTGetAndClearExceptionAction() override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTGetAndClearExceptionAction* CompleteParse(FileReader* reader, SlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for getting a property from an object
    class JsRTGetPropertyAction : public JsRTActionLogEntry
    {
    private:
        Js::PropertyId m_propertyId;
        NSLogValue::ArgRetValue* m_var;

    public:
        JsRTGetPropertyAction(int64 eTime, TTD_LOG_TAG ctxTag, Js::PropertyId pid, NSLogValue::ArgRetValue* var);
        virtual ~JsRTGetPropertyAction() override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTGetPropertyAction* CompleteParse(FileReader* reader, SlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
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
        virtual ~JsRTCallbackAction() override;

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
        static JsRTCallbackAction* CompleteParse(FileReader* reader, SlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for calls to script parse
    class JsRTCodeParseAction : public JsRTActionLogEntry
    {
    private:
        //True if the body has expression semantics
        bool m_isExpression;

        //The actual source code
        LPCWSTR m_sourceCode;

        //If this is from a URI
        DWORD_PTR m_optDocumentID;
        LPCWSTR m_optSourceUri;

        //The directory to write the source files out to (if needed)
        LPCWSTR m_srcDir;

    public:
        JsRTCodeParseAction(int64 eTime, TTD_LOG_TAG ctxTag, bool isExpression, LPCWSTR sourceCode, DWORD_PTR optDocumentId, LPCWSTR optSourceUri, LPCWSTR srcDir);
        virtual ~JsRTCodeParseAction() override;

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTCodeParseAction* CompleteParse(ThreadContext* threadContext, FileReader* reader, SlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };

    //A class for calls to that execute existing functions
    class JsRTCallFunctionAction : public JsRTActionLogEntry
    {
    private:
        //the function tag and name for the callback function
        const TTD_LOG_TAG m_functionTagId;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        LPCWSTR m_functionName;
#endif

        //The re-entry depth we are at when this happens
        const int32 m_callbackDepth;

        //The id given by the host for this callback (or -1 if this call is not associated with any callback)
        const int64 m_hostCallbackId;

        const double m_beginTime;
        double m_elapsedTime;

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

        //Info on the last executed statement in this call
        mutable bool m_lastExecuted_valid;
        mutable uint64 m_lastExecuted_ftime;
        mutable uint64 m_lastExecuted_ltime; 
        mutable uint32 m_lastExecuted_line;
        mutable uint32 m_lastExecuted_column;
        mutable uint32 m_lastExecuted_sourceId;
#endif

    public:
        JsRTCallFunctionAction(int64 eTime, TTD_LOG_TAG ctxTag, int32 callbackDepth, int64 hostCallbackId, double beginTime, TTD_LOG_TAG functionTagId, uint32 argCount, NSLogValue::ArgRetValue* argArray, Js::Var* execArgs);
        virtual ~JsRTCallFunctionAction() override;

        virtual void UnloadSnapshot() const override;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        void SetFunctionName(LPCWSTR fname);
#endif

        static JsRTCallFunctionAction* As(JsRTActionLogEntry* action);

        //Get/Set/Clear the ready-to-run snapshot information (in debugging mode -- nops for replay mode)
        bool HasReadyToRunSnapshotInfo() const;
        void GetReadyToRunSnapshotInfo(SnapShot** snap, TTD_LOG_TAG* logTag, TTD_IDENTITY_TAG* identityTag) const;
        void SetReadyToRunSnapshotInfo(SnapShot* snap, TTD_LOG_TAG logTag, TTD_IDENTITY_TAG identityTag) const;

        //Set the last executed statement and frame (in debugging mode -- nops for replay mode)
        void ClearLastExecutedStatementAndFrameInfo() const;
        void SetLastExecutedStatementAndFrameInfo(const SingleCallCounter& lastFrame) const;
        bool GetLastExecutedStatementAndFrameInfoForDebugger(int64* rootEventTime, uint64* ftime, uint64* ltime, uint32* line, uint32* column, uint32* sourceId) const;

        //Get the call depth for this
        int32 GetCallDepth() const;

        //Get the host callback id info for this (toplevel) event
        int64 GetHostCallbackId() const;

        void SetElapsedTime(double elapsedTime);

        virtual void ExecuteAction(ThreadContext* threadContext) const override;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static JsRTCallFunctionAction* CompleteParse(FileReader* reader, SlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag);
    };
}

#endif

