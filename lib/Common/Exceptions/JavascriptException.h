//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js {
    class JavascriptExceptionObject;

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
        Field(JavascriptExceptionObject*)* const addressOfException;

    public:
        // Caller should have stored the JavascriptExceptionObject reference in
        // thread context data. addressOfException should be the thread context data
        // address.
        JavascriptException(Field(JavascriptExceptionObject*)* addressOfException)
            : addressOfException(addressOfException)
        {
            Assert(addressOfException && *addressOfException);
        }

        JavascriptExceptionObject* GetAndClear() const
        {
            Assert(*addressOfException);

            JavascriptExceptionObject* exceptionObject = *addressOfException;
            *addressOfException = nullptr;
            return exceptionObject;
        }
    };

} // namespace Js
