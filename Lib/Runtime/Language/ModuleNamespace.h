//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    /// Module namespace object
    // This object, while the object is special that the properties are indirect bindings referencing variable
    // definitions from different module, we can add the properties to the object and [[get]] them accordingly.
    class ModuleNamespace : public DynamicObject
    {
    public:
        ModuleNamespace();
    private:

    };
}
