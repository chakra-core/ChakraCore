//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class ThreadContext;

namespace Js
{
    class JavascriptExceptionObject;

    // Implemented by runtime to temporarily save (keep alive during throw) / clear
    // an exception object.
    void SaveTempUncaughtException(
        ThreadContext* threadContext, Js::JavascriptExceptionObject* exception);
    void ClearTempUncaughtException(
        ThreadContext* threadContext, Js::JavascriptExceptionObject* exception);

    //
    // JavascriptException wraps a runtime JavascriptExceptionObject. To ensure
    // a thrown JavascriptExceptionObject is kept alive before being caught by
    // a handler, we store the JavascriptExceptionObject reference in thread context
    // data (which GC scans). After the exception is caught, call GetAndClear()
    // to retrieve the wrapped JavascriptExceptionObject and clear it from thread
    // context data.
    //
    class JavascriptException : public ExceptionBase
    {
    private:
        ThreadContext* const threadContext;
        mutable Js::JavascriptExceptionObject* exception;

    public:
        JavascriptException(ThreadContext* threadContext,
                            Js::JavascriptExceptionObject* exception = nullptr)
            : threadContext(threadContext), exception(exception)
        {
            SaveTempUncaughtException(threadContext, exception);
        }

        JavascriptException(const JavascriptException& other)
            : threadContext(other.threadContext), exception(other.exception)
        {
            other.exception = nullptr;  // transfer
        }

        ~JavascriptException()
        {
            if (exception)
            {
                GetAndClear();
            }
        }

        JavascriptExceptionObject* GetAndClear() const
        {
            JavascriptExceptionObject* tmp = exception;
            if (exception)
            {
                ClearTempUncaughtException(threadContext, exception);
                exception = nullptr;
            }
            return tmp;
        }

        PREVENT_ASSIGN(JavascriptException);
    };

} // namespace Js
