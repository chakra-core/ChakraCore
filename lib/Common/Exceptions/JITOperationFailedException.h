//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js {

    class JITOperationFailedException : public ExceptionBase
    {
    public:
        JITOperationFailedException(DWORD lastError)
            :LastError(lastError)
        {}
        DWORD LastError;
    };

} // namespace Js
