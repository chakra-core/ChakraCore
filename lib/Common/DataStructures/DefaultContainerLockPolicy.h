//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once


namespace Js
{
    class DefaultContainerLockPolicy
    {
    public:
        class NoLock
        {
        public:
            template <class SyncObject>
            NoLock(const SyncObject&)
            {
                // No lock, do nothing.
            }
        };

        typedef AutoCriticalSection DefaultLock;

        typedef NoLock      ReadLock;       // To read/map items. Default to no lock.
        typedef NoLock      WriteLock;      // To write an item. Default to no lock.
        typedef DefaultLock AddRemoveLock;  // To add/remove item
    };
}
