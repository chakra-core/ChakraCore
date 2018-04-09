//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

#if ENABLE_PROFILE_INFO
#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
#include "activscp_private.h"
#endif
namespace Js
{
    SimpleDataCacheWrapper::SimpleDataCacheWrapper(IActiveScriptDataCache* dataCache) :
        dataCache(dataCache),
        outStream(nullptr),
        bytesWrittenInBlock(0)
    {
#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
        this->dataCache->AddRef();
#endif
    }

    bool SimpleDataCacheWrapper::Close()
    {
        bool success = true;

        if (IsWriteStreamOpen())
        {
#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
            Assert(this->dataCache != nullptr);
            success = this->dataCache->SaveWriteDataStream(this->outStream) == S_OK;
#endif
            this->outStream->Release();
            this->outStream = nullptr;
        }

#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
        if (this->dataCache != nullptr)
        {
            this->dataCache->Release();
            this->dataCache = nullptr;
        }
#endif
        return success;
    }

    bool SimpleDataCacheWrapper::OpenWriteStream()
    {
        HRESULT hr = S_OK;

#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
        Assert(!IsWriteStreamOpen());
        Assert(this->dataCache != nullptr);

        hr = this->dataCache->GetWriteDataStream(&this->outStream);
#endif
        if (FAILED(hr))
        {
            this->outStream = nullptr;
            return false;
        }

        return WriteHeader();
    }

    bool SimpleDataCacheWrapper::WriteHeader()
    {
        DWORD jscriptMajorVersion;
        DWORD jscriptMinorVersion;

        if (FAILED(AutoSystemInfo::GetJscriptFileVersion(&jscriptMajorVersion, &jscriptMinorVersion)))
        {
            return false;
        }

        if (!Write(jscriptMajorVersion))
        {
            return false;
        }

        if (!Write(jscriptMinorVersion))
        {
            return false;
        }

        return true;
    }

    bool SimpleDataCacheWrapper::ReadHeader(_In_ IStream* readStream)
    {
        Assert(readStream != nullptr);

        DWORD jscriptMajorVersion;
        DWORD jscriptMinorVersion;

        if (FAILED(AutoSystemInfo::GetJscriptFileVersion(&jscriptMajorVersion, &jscriptMinorVersion)))
        {
            return false;
        }

        DWORD majorVersion;
        if (!Read(readStream, &majorVersion) || majorVersion != jscriptMajorVersion)
        {
            return false;
        }

        DWORD minorVersion;
        if (!Read(readStream, &minorVersion) || minorVersion != jscriptMinorVersion)
        {
            return false;
        }

        return true;
    }

    bool SimpleDataCacheWrapper::EnsureWriteStream()
    {
        if (IsWriteStreamOpen())
        {
            return true;
        }

        return OpenWriteStream();
    }

    bool SimpleDataCacheWrapper::StartBlock(_In_ BlockType blockType, _In_ ULONG byteCount)
    {
        if (!Write(blockType) || !Write(byteCount))
        {
            return false;
        }

        // Reset the bytes written for the current block
        this->bytesWrittenInBlock = 0;
        return true;
    }

    HRESULT SimpleDataCacheWrapper::GetReadStream(_Out_ IStream** readStream)
    {
        HRESULT hr = S_OK;

#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
        Assert(this->dataCache != nullptr);
        hr = this->dataCache->GetReadDataStream(readStream);
#endif

        if (FAILED(hr))
        {
            return hr;
        }

        return ReadHeader(*readStream) ? S_OK : E_FAIL;
    }

    bool SimpleDataCacheWrapper::HasBlock(_In_ BlockType blockType)
    {
        AutoCOMPtr<IStream> readStream;
        HRESULT hr = this->GetReadStream(&readStream);

        if (FAILED(hr))
        {
            return false;
        }

        return this->SeekReadStreamToBlock(readStream, blockType);
    }

    bool SimpleDataCacheWrapper::SeekReadStreamToBlock(_In_ IStream* readStream, _In_ BlockType blockType, _Out_opt_ ULONG* bytesInBlock)
    {
        Assert(readStream != nullptr);

        BlockType currentBlockType = BlockType_Invalid;
        ULONG byteCount = 0;

        // The header should have already been consumed
        if (!Read(readStream, &currentBlockType))
        {
            return false;
        }
        if (!Read(readStream, &byteCount))
        {
            return false;
        }

        if (currentBlockType == blockType)
        {
            if (bytesInBlock != nullptr)
            {
                *bytesInBlock = byteCount;
            }
            return true;
        }

        // The block we're pointing at is not the requested one, seek forward to the next block
        LARGE_INTEGER dlibMove;
        dlibMove.QuadPart = byteCount;
        if (FAILED(readStream->Seek(dlibMove, STREAM_SEEK_CUR, nullptr)))
        {
            return false;
        }

        return SeekReadStreamToBlock(readStream, blockType);
    }
}
#endif
