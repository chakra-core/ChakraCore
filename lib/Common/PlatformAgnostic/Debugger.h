//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifndef RUNTIME_PLATFORM_AGNOSTIC_DEBUGGER
#define RUNTIME_PLATFORM_AGNOSTIC_DEBUGGER

#ifndef _WIN32

#define ACTIVPROF_E_PROFILER_PRESENT        0x0200
#define ACTIVPROF_E_PROFILER_ABSENT         0x0201
#define ACTIVPROF_E_UNABLE_TO_APPLY_ACTION  0x0202
#define PROFILER_TOKEN uint

typedef enum {
    PROFILER_SCRIPT_TYPE_USER,
    PROFILER_SCRIPT_TYPE_DYNAMIC,
    PROFILER_SCRIPT_TYPE_NATIVE,
    PROFILER_SCRIPT_TYPE_DOM
} PROFILER_SCRIPT_TYPE;

typedef enum {
    PROFILER_EVENT_MASK_TRACE_SCRIPT_FUNCTION_CALL = 0x00000001,
    PROFILER_EVENT_MASK_TRACE_NATIVE_FUNCTION_CALL = 0x00000002,
    PROFILER_EVENT_MASK_TRACE_DOM_FUNCTION_CALL    = 0x00000004,
    PROFILER_EVENT_MASK_TRACE_ALL =
    PROFILER_EVENT_MASK_TRACE_SCRIPT_FUNCTION_CALL |
    PROFILER_EVENT_MASK_TRACE_NATIVE_FUNCTION_CALL,
    PROFILER_EVENT_MASK_TRACE_ALL_WITH_DOM = PROFILER_EVENT_MASK_TRACE_ALL |
    PROFILER_EVENT_MASK_TRACE_DOM_FUNCTION_CALL
} PROFILER_EVENT_MASK;

interface IEnumDebugCodeContexts : IUnknown
{
    // HRESULT Next( ..

    // HRESULT Skip( ..

    // HRESULT Reset();

    // HRESULT Clone( ..
};

interface IDebugDocumentInfo : IUnknown
{
    HRESULT GetName(char* dn, BSTR *name);

    HRESULT GetDocumentClassId(CLSID *dclsid);
};

interface IDebugDocument : IDebugDocumentInfo
{
};

interface IDebugDocumentContext : IUnknown
{
    HRESULT GetDocument(IDebugDocument **doc);

    HRESULT EnumCodeContexts(IEnumDebugCodeContexts **dctx);
};

class IActiveScriptProfilerCallback
{
public:
  HRESULT Initialize(DWORD ctx)
  {
      return S_OK;
  }

  HRESULT Shutdown(HRESULT _)
  {
      return S_OK;
  }

  HRESULT Release()
  {
      return S_OK;
  }

  HRESULT QueryInterface(IActiveScriptProfilerCallback **_)
  {
      return S_OK;
  }

  HRESULT ScriptCompiled(PROFILER_TOKEN scriptId, PROFILER_SCRIPT_TYPE type, IUnknown *ctx)
  {
      return S_OK;
  }

  HRESULT FunctionCompiled(PROFILER_TOKEN functionId, PROFILER_TOKEN scriptId,
      const WCHAR* pwszFunctionName, const WCHAR* pwszFunctionNameHint, IUnknown *ctx)
  {
      return S_OK;
  }

  HRESULT OnFunctionEnter(PROFILER_TOKEN scriptId, PROFILER_TOKEN functionId)
  {
      return S_OK;
  }

  HRESULT OnFunctionExit(PROFILER_TOKEN scriptId, PROFILER_TOKEN functionId)
  {
      return S_OK;
  }

  // IActiveScriptProfilerCallback2
  HRESULT OnFunctionEnterByName(const WCHAR *functionName, PROFILER_SCRIPT_TYPE _)
  {
      return S_OK;
  }

  HRESULT OnFunctionExitByName(const WCHAR *functionName, PROFILER_SCRIPT_TYPE _)
  {
      return S_OK;
  }

  // IActiveScriptProfilerCallback3
  HRESULT AddRef()
  {
      return S_OK;
  }

  HRESULT SetWebWorkerId(PROFILER_TOKEN _)
  {
      return S_OK;
  }
};

#define IActiveScriptProfilerCallback2 IActiveScriptProfilerCallback
#define IActiveScriptProfilerCallback3 IActiveScriptProfilerCallback

#endif // !_WIN32

#endif // RUNTIME_PLATFORM_AGNOSTIC_DEBUGGER
