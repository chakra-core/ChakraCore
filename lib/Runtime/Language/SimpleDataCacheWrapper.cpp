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
        inStream(nullptr),
        bytesWrittenInBlock(0),
        blocksWritten(0)
    {
#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
        this->dataCache->AddRef();
#endif
    }

    HRESULT SimpleDataCacheWrapper::Close()
    {
        HRESULT hr = E_FAIL;

#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
        hr = this->SaveWriteStream();

        if (IsReadStreamOpen())
        {
            this->inStream->Release();
            this->inStream = nullptr;
        }

        if (this->dataCache != nullptr)
        {
            this->dataCache->Release();
            this->dataCache = nullptr;
        }
#endif

        return hr;
    }

    HRESULT SimpleDataCacheWrapper::SaveWriteStream()
    {
        HRESULT hr = E_FAIL;

#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
        if (IsWriteStreamOpen())
        {
#if DBG
            STATSTG statstg = { 0 };
            hr = this->outStream->Stat(&statstg, STATFLAG_DEFAULT);
            if (FAILED(hr))
            {
                OUTPUT_TRACE_DEBUGONLY(Js::DataCachePhase, _u(" Failed to call IStream::Stat on write stream (hr = 0x%08lx)\n"), hr);
            }
#endif
            OUTPUT_TRACE_DEBUGONLY(Js::DataCachePhase, _u(" Attempting to save write stream with %u bytes...\n"), statstg.cbSize.LowPart);

            Assert(this->dataCache != nullptr);
            hr = this->dataCache->SaveWriteDataStream(this->outStream);

            this->outStream->Release();
            this->outStream = nullptr;

            if (FAILED(hr))
            {
                OUTPUT_TRACE_DEBUGONLY(Js::DataCachePhase, _u(" Failed to save write stream (hr = 0x%08lx)\n"), hr);
            }
            else
            {
                OUTPUT_TRACE_DEBUGONLY(Js::DataCachePhase, _u(" Successfully saved write stream\n"), hr);
            }
        }
#endif

        return hr;
    }

    HRESULT SimpleDataCacheWrapper::OpenWriteStream()
    {
#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
        HRESULT hr = E_FAIL;
        Assert(!IsWriteStreamOpen());
        Assert(this->dataCache != nullptr);
        Assert(this->outStream == nullptr);
        hr = this->dataCache->GetWriteDataStream(&this->outStream);

        if (FAILED(hr))
        {
            if (this->outStream != nullptr)
            {
                this->outStream->Release();
                this->outStream = nullptr;
            }
            return hr;
        }
#endif

        return WriteHeader();
    }

    HRESULT SimpleDataCacheWrapper::WriteHeader()
    {
        DWORD jscriptMajorVersion;
        DWORD jscriptMinorVersion;
        HRESULT hr = E_FAIL;

        IFFAILRET(AutoSystemInfo::GetJscriptFileVersion(&jscriptMajorVersion, &jscriptMinorVersion));

        Assert(this->bytesWrittenInBlock == 0);

        IFFAILRET(Write(jscriptMajorVersion));
        IFFAILRET(Write(jscriptMinorVersion));

        Assert(this->bytesWrittenInBlock == sizeof(DWORD) * 2);

        return hr;
    }

    HRESULT SimpleDataCacheWrapper::ReadHeader()
    {
        DWORD jscriptMajorVersion;
        DWORD jscriptMinorVersion;
        HRESULT hr = E_FAIL;

        IFFAILRET(AutoSystemInfo::GetJscriptFileVersion(&jscriptMajorVersion, &jscriptMinorVersion));

        DWORD majorVersion;
        IFFAILRET(Read(&majorVersion));
        if (majorVersion != jscriptMajorVersion)
        {
            return E_FAIL;
        }

        DWORD minorVersion;
        IFFAILRET(Read(&minorVersion));
        if (minorVersion != jscriptMinorVersion)
        {
            return E_FAIL;
        }

        return hr;
    }

    HRESULT SimpleDataCacheWrapper::EnsureWriteStream()
    {
        if (IsWriteStreamOpen())
        {
            return S_OK;
        }

        return OpenWriteStream();
    }

    HRESULT SimpleDataCacheWrapper::EnsureReadStream()
    {
        if (IsReadStreamOpen())
        {
            return S_OK;
        }

        return OpenReadStream();
    }

    HRESULT SimpleDataCacheWrapper::StartBlock(_In_ BlockType blockType, _In_ ULONG byteCount)
    {
        HRESULT hr = E_FAIL;

        if (this->blocksWritten >= MAX_BLOCKS_ALLOWED)
        {
            return hr;
        }

        IFFAILRET(Write(blockType));
        IFFAILRET(Write(byteCount));

        // Reset the bytes written for the current block
        this->bytesWrittenInBlock = 0;
        this->blocksWritten++;
        return hr;
    }

    HRESULT SimpleDataCacheWrapper::OpenReadStream()
    {
#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
        HRESULT hr = E_FAIL;
        Assert(this->dataCache != nullptr);
        Assert(this->inStream == nullptr);
        IFFAILRET(this->dataCache->GetReadDataStream(&this->inStream));
#endif

        return ReadHeader();
    }

    HRESULT SimpleDataCacheWrapper::ResetReadStream()
    {
        HRESULT hr = E_FAIL;

        if (IsReadStreamOpen())
        {
#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
            // Reset the read stream to beginning of the stream - after the header
            LARGE_INTEGER dlibMove;
            dlibMove.QuadPart = sizeof(DWORD) * 2;
            IFFAILRET(this->inStream->Seek(dlibMove, STREAM_SEEK_SET, nullptr));
#endif
        }
        else
        {
            // OpenReadStream opens to the beginning of the stream and consumes the header
            hr = OpenReadStream();
        }

        return hr;
    }

    HRESULT SimpleDataCacheWrapper::SeekReadStreamToBlockHelper(_In_ BlockType blockType, _Out_opt_ ULONG* bytesInBlock)
    {
        HRESULT hr;
        BlockType currentBlockType = BlockType_Invalid;
        ULONG byteCount = 0;

        IFFAILRET(Read(&currentBlockType));
        IFFAILRET(Read(&byteCount));

        if (currentBlockType == blockType)
        {
            if (bytesInBlock != nullptr)
            {
                *bytesInBlock = byteCount;
            }
            return S_OK;
        }
        else if (currentBlockType == BlockType_Invalid || byteCount == 0)
        {
            return E_FAIL;
        }

#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
        // The block we're pointing at is not the requested one, seek forward to the next block
        LARGE_INTEGER dlibMove;
        dlibMove.QuadPart = byteCount;
        IFFAILRET(this->inStream->Seek(dlibMove, STREAM_SEEK_CUR, nullptr));
#endif

        return SeekReadStreamToBlockHelper(blockType, bytesInBlock);
    }

    HRESULT SimpleDataCacheWrapper::SeekReadStreamToBlock(_In_ BlockType blockType, _Out_opt_ ULONG* bytesInBlock)
    {
        HRESULT hr;

        if (bytesInBlock != nullptr)
        {
            *bytesInBlock = 0;
        }

        IFFAILRET(ResetReadStream());

        // Reset above has moved the seek pointer to just after the header, we're at the first block.
        return SeekReadStreamToBlockHelper(blockType, bytesInBlock);
    }
}
#endif
