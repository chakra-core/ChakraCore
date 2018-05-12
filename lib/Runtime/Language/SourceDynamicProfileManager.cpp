//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

#if ENABLE_PROFILE_INFO
namespace Js
{
    ExecutionFlags
    SourceDynamicProfileManager::IsFunctionExecuted(Js::LocalFunctionId functionId)
    {
        if (cachedStartupFunctions == nullptr || cachedStartupFunctions->Length() <= functionId)
        {
            return ExecutionFlags_HasNoInfo;
        }
        return (ExecutionFlags)cachedStartupFunctions->Test(functionId);
    }

    DynamicProfileInfo *
    SourceDynamicProfileManager::GetDynamicProfileInfo(FunctionBody * functionBody)
    {
        Js::LocalFunctionId functionId = functionBody->GetLocalFunctionId();
        DynamicProfileInfo * dynamicProfileInfo = nullptr;
        if (dynamicProfileInfoMap.Count() > 0 && dynamicProfileInfoMap.TryGetValue(functionId, &dynamicProfileInfo))
        {
            if (dynamicProfileInfo->MatchFunctionBody(functionBody))
            {
                return dynamicProfileInfo;
            }

#if DBG_DUMP
            if (Js::Configuration::Global.flags.Trace.IsEnabled(Js::DynamicProfilePhase))
            {
                Output::Print(_u("TRACE: DynamicProfile: Profile data rejected for function %d in %s\n"),
                    functionId, functionBody->GetSourceContextInfo()->url);
                Output::Flush();
            }
#endif
            // NOTE: We have profile mismatch, we can invalidate all other profile here.
        }
        return nullptr;
    }

    void SourceDynamicProfileManager::UpdateDynamicProfileInfo(LocalFunctionId functionId, DynamicProfileInfo * dynamicProfileInfo)
    {
        Assert(dynamicProfileInfo != nullptr);

        dynamicProfileInfoMap.Item(functionId, dynamicProfileInfo);
    }

    void SourceDynamicProfileManager::RemoveDynamicProfileInfo(LocalFunctionId functionId)
    {
        dynamicProfileInfoMap.Remove(functionId);
#ifdef DYNAMIC_PROFILE_STORAGE
        dynamicProfileInfoMapSaving.Remove(functionId);
#endif
    }

    void SourceDynamicProfileManager::MarkAsExecuted(LocalFunctionId functionId)
    {
        Assert(startupFunctions != nullptr);
        AssertOrFailFast(functionId <= startupFunctions->Length());
        startupFunctions->Set(functionId);
    }

    void SourceDynamicProfileManager::EnsureStartupFunctions(uint numberOfFunctions)
    {
        Assert(numberOfFunctions != 0);
        if(!startupFunctions || numberOfFunctions > startupFunctions->Length())
        {
            BVFixed* oldStartupFunctions = this->startupFunctions;
            startupFunctions = BVFixed::New(numberOfFunctions, this->GetRecycler());
            if(oldStartupFunctions)
            {
                this->startupFunctions->Copy(oldStartupFunctions);
            }
        }
    }

    //
    // Enables re-use of profile managers across script contexts - on every re-use the
    // previous script contexts list of startup functions are transferred over as input to this new script context.
    //
    void SourceDynamicProfileManager::Reuse()
    {
        AssertMsg(dataCacheWrapper == nullptr, "Persisted profiles cannot be re-used");
        cachedStartupFunctions = startupFunctions;
    }

    //
    // Loads the profile from the WININET cache
    //
    bool SourceDynamicProfileManager::LoadFromProfileCache(SimpleDataCacheWrapper* dataCacheWrapper, LPCWSTR url)
    {
#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
        AssertMsg(CONFIG_FLAG(WininetProfileCache), "Profile caching should be enabled for us to get here");
        Assert(dataCacheWrapper);
        AssertMsg(!IsProfileLoadedFromWinInet(), "Duplicate profile cache loading?");

        // Keep a copy of this
        this->dataCacheWrapper = dataCacheWrapper;
        HRESULT hr = dataCacheWrapper->SeekReadStreamToBlock(SimpleDataCacheWrapper::BlockType_ProfileData);
        if(SUCCEEDED(hr))
        {
            uint numberOfFunctions = 0;
            if (FAILED(dataCacheWrapper->Read(&numberOfFunctions)) || numberOfFunctions > MAX_FUNCTION_COUNT)
            {
                return false;
            }
            BVFixed* functions = BVFixed::New(numberOfFunctions, this->recycler);
            if (FAILED(dataCacheWrapper->ReadArray(functions->GetData(), functions->WordCount())))
            {
                return false;
            }
            this->cachedStartupFunctions = functions;
            OUTPUT_TRACE(Js::DynamicProfilePhase, _u("Profile load succeeded. Function count: %d  %s\n"), numberOfFunctions, url);
#if DBG_DUMP
            if(PHASE_TRACE1(Js::DynamicProfilePhase) && Js::Configuration::Global.flags.Verbose)
            {
                OUTPUT_VERBOSE_TRACE(Js::DynamicProfilePhase, _u("Profile loaded:\n"));
                functions->Dump();
            }
#endif
            return true;
        }
        else if (hr == HRESULT_FROM_WIN32(ERROR_WRITE_PROTECT))
        {
            this->isNonCachableScript = true;
            OUTPUT_VERBOSE_TRACE(Js::DynamicProfilePhase, _u("Profile load failed. Non-cacheable resource. %s\n"), url);
        }
        else
        {
            OUTPUT_TRACE(Js::DynamicProfilePhase, _u("Profile load failed. No read stream. %s\n"), url);
        }
#endif
        return false;
    }

    //
    // Saves the profile to the WININET cache and returns the bytes written
    //
    uint SourceDynamicProfileManager::SaveToProfileCacheAndRelease(SourceContextInfo* info)
    {
        uint bytesWritten = 0;
#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
        if(dataCacheWrapper)
        {
            if(ShouldSaveToProfileCache(info))
            {
                OUTPUT_TRACE(Js::DynamicProfilePhase, _u("Saving profile. Number of functions: %d Url: %s...\n"), startupFunctions->Length(), info->url);

                bytesWritten = SaveToProfileCache();

                if (bytesWritten == 0)
                {
                    OUTPUT_TRACE(Js::DynamicProfilePhase, _u("Profile saving FAILED\n"));
                }
                else
                {
                    OUTPUT_TRACE(Js::DynamicProfilePhase, _u("Profile saving succeeded. Bytes written: %d\n"), bytesWritten);
                }
            }

            if (!isNonCachableScript)
            {
                if (FAILED(dataCacheWrapper->SaveWriteStream()))
                {
                    return 0;
                }
            }
            dataCacheWrapper = nullptr;
        }
#endif
        return bytesWritten;
    }

    //
    // Saves the profile to the WININET cache
    //
    uint SourceDynamicProfileManager::SaveToProfileCache()
    {
        AssertMsg(CONFIG_FLAG(WininetProfileCache), "Profile caching should be enabled for us to get here");
        Assert(startupFunctions);

        uint bytesWritten = 0;
#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
        //TODO: Add some diffing logic to not write unless necessary
        ULONG byteCount = startupFunctions->WordCount() * sizeof(BVUnit) + sizeof(BVIndex);
        if (FAILED(dataCacheWrapper->StartBlock(SimpleDataCacheWrapper::BlockType::BlockType_ProfileData, byteCount)))
        {
            return 0;
        }

        if (FAILED(dataCacheWrapper->Write(startupFunctions->Length())))
        {
            return 0;
        }

        if (SUCCEEDED(dataCacheWrapper->WriteArray(startupFunctions->GetData(), startupFunctions->WordCount())))
        {
            bytesWritten = dataCacheWrapper->BytesWrittenInBlock();

#if DBG_DUMP
            if (PHASE_TRACE1(Js::DynamicProfilePhase) && Js::Configuration::Global.flags.Verbose)
            {
                OUTPUT_VERBOSE_TRACE(Js::DynamicProfilePhase, _u("Saved profile:\n"));
                startupFunctions->Dump();
            }
#endif
        }
#endif
        return bytesWritten;
    }

    //
    // Do not save the profile:
    //      - If it is a non-cacheable WININET resource
    //      - If there are no or small number of functions executed
    //      - If there is not substantial difference in number of functions executed.
    //
    bool SourceDynamicProfileManager::ShouldSaveToProfileCache(SourceContextInfo* info) const
    {
        if(isNonCachableScript)
        {
            OUTPUT_VERBOSE_TRACE(Js::DynamicProfilePhase, _u("Skipping save of profile. Non-cacheable resource. %s\n"), info->url);
            return false;
        }

        if (dataCacheWrapper->BlocksWritten() > 0)
        {
            OUTPUT_VERBOSE_TRACE(Js::DynamicProfilePhase, _u("Saving profile. There are other blocks in the cache. %s\n"), info->url);
            return true;
        }

        if(!startupFunctions || startupFunctions->Length() <= DEFAULT_CONFIG_MinProfileCacheSize)
        {
            OUTPUT_VERBOSE_TRACE(Js::DynamicProfilePhase, _u("Skipping save of profile. Small number of functions. %s\n"), info->url);
            return false;
        }

        if(cachedStartupFunctions)
        {
            AssertMsg(cachedStartupFunctions != startupFunctions, "Ensure they are not shallow copies of each other - Reuse() does this for dynamic sources. We should not be invoked for dynamic sources");
            uint numberOfBitsDifferent = cachedStartupFunctions->DiffCount(startupFunctions);
            uint saveThreshold = (cachedStartupFunctions->Length() * DEFAULT_CONFIG_ProfileDifferencePercent) / 100;
            if(numberOfBitsDifferent <= saveThreshold)
            {
                OUTPUT_VERBOSE_TRACE(Js::DynamicProfilePhase, _u("Skipping save of profile. Number of functions different: %d %s\n"), numberOfBitsDifferent, info->url);
                return false;
            }
            else
            {
                OUTPUT_VERBOSE_TRACE(Js::DynamicProfilePhase, _u("Number of functions different: %d \n"), numberOfBitsDifferent);
            }
        }
        return true;
    }

    SourceDynamicProfileManager *
    SourceDynamicProfileManager::LoadFromDynamicProfileStorage(SourceContextInfo* info, ScriptContext* scriptContext, SimpleDataCacheWrapper* dataCacheWrapper)
    {
        SourceDynamicProfileManager* manager = nullptr;
        Recycler* recycler = scriptContext->GetRecycler();

#ifdef DYNAMIC_PROFILE_STORAGE
        if(DynamicProfileStorage::IsEnabled() && info->url != nullptr)
        {
            manager = DynamicProfileStorage::Load(info->url, [recycler](char const * buffer, uint length) -> SourceDynamicProfileManager *
            {
                BufferReader reader(buffer, length);
                return SourceDynamicProfileManager::Deserialize(&reader, recycler);
            });
        }
#endif
        if(manager == nullptr)
        {
            manager = RecyclerNew(recycler, SourceDynamicProfileManager, recycler);
        }
        if(dataCacheWrapper != nullptr)
        {
            bool profileLoaded = manager->LoadFromProfileCache(dataCacheWrapper, info->url);
            if(profileLoaded)
            {
                JS_ETW(EventWriteJSCRIPT_PROFILE_LOAD(info->dwHostSourceContext, scriptContext));
            }
        }
        return manager;
    }

#ifdef DYNAMIC_PROFILE_STORAGE
    void SourceDynamicProfileManager::ClearSavingData()
    {
        dynamicProfileInfoMapSaving.Reset();
    }

    void SourceDynamicProfileManager::AddSavingItem(LocalFunctionId functionId, DynamicProfileInfo *info)
    {
        try
        {
            // our BaseDictionary does not allow nothrow allocator
            AUTO_NESTED_HANDLED_EXCEPTION_TYPE(ExceptionType_OutOfMemory);
            dynamicProfileInfoMapSaving.Item(functionId, info);
        }
        catch (Js::OutOfMemoryException&)
        {
            Output::Print(_u("Hit OOM while saving dynamic profile info\n"));
        }
    }

    void
    SourceDynamicProfileManager::SaveDynamicProfileInfo(LocalFunctionId functionId, DynamicProfileInfo * dynamicProfileInfo)
    {
        Assert(dynamicProfileInfo->GetFunctionBody()->HasExecutionDynamicProfileInfo());
        this->AddSavingItem(functionId, dynamicProfileInfo);
    }

    template <typename T>
    SourceDynamicProfileManager *
    SourceDynamicProfileManager::Deserialize(T * reader, Recycler* recycler)
    {
        uint functionCount;
        if (!reader->Peek(&functionCount))
        {
            Assert(false);
            return nullptr;
        }

        BVFixed * startupFunctions = BVFixed::New(functionCount, recycler);
        if (!reader->ReadArray(((char *)startupFunctions),
            BVFixed::GetAllocSize(functionCount)))
        {
            Assert(false);
            return nullptr;
        }

        uint profileCount;

        if (!reader->Read(&profileCount))
        {
            Assert(false);
            return nullptr;
        }

        ThreadContext* threadContext = ThreadContext::GetContextForCurrentThread();

        SourceDynamicProfileManager * sourceDynamicProfileManager = RecyclerNew(threadContext->GetRecycler(), SourceDynamicProfileManager, recycler);

        sourceDynamicProfileManager->cachedStartupFunctions = startupFunctions;

#if DBG_DUMP
        if(Configuration::Global.flags.Dump.IsEnabled(DynamicProfilePhase))
        {
            Output::Print(_u("Loaded: Startup functions bit vector:"));
            startupFunctions->Dump();
        }
#endif

        for (uint i = 0; i < profileCount; i++)
        {
            Js::LocalFunctionId functionId;
            DynamicProfileInfo * dynamicProfileInfo = DynamicProfileInfo::Deserialize(reader, recycler, &functionId);
            if (dynamicProfileInfo == nullptr || functionId >= functionCount)
            {
                Assert(false);
                return nullptr;
            }
            sourceDynamicProfileManager->dynamicProfileInfoMap.Add(functionId, dynamicProfileInfo);
            sourceDynamicProfileManager->AddSavingItem(functionId, dynamicProfileInfo);
        }
        return sourceDynamicProfileManager;
    }

    template <typename T>
    bool
    SourceDynamicProfileManager::Serialize(T * writer)
    {
        // To simulate behavior of in memory profile cache - let's keep functions marked as executed if they were loaded
        // to be so from the profile - this helps with ensure inlined functions are marked as executed.
        if(!this->startupFunctions)
        {
            this->startupFunctions = const_cast<BVFixed*>(static_cast<const BVFixed*>(this->cachedStartupFunctions));
        }
        else if(cachedStartupFunctions && this->cachedStartupFunctions->Length() == this->startupFunctions->Length())
        {
            this->startupFunctions->Or(cachedStartupFunctions);
        }

        if(this->startupFunctions)
        {
#if DBG_DUMP
             if(Configuration::Global.flags.Dump.IsEnabled(DynamicProfilePhase))
            {
                Output::Print(_u("Saving: Startup functions bit vector:"));
                this->startupFunctions->Dump();
            }
#endif

            size_t bvSize = BVFixed::GetAllocSize(this->startupFunctions->Length()) ;
            if (!writer->WriteArray((char *)static_cast<BVFixed*>(this->startupFunctions), bvSize)
                || !writer->Write(this->dynamicProfileInfoMapSaving.Count()))
            {
                return false;
            }
        }

        for (int i = 0; i < this->dynamicProfileInfoMapSaving.Count(); i++)
        {
            DynamicProfileInfo * dynamicProfileInfo = this->dynamicProfileInfoMapSaving.GetValueAt(i);
            if (dynamicProfileInfo == nullptr || !dynamicProfileInfo->HasFunctionBody())
            {
                continue;
            }

            if (!dynamicProfileInfo->Serialize(writer))
            {
                return false;
            }
        }
        return true;
    }

    void
    SourceDynamicProfileManager::SaveToDynamicProfileStorage(char16 const * url)
    {
        Assert(DynamicProfileStorage::IsEnabled());
        BufferSizeCounter counter;
        if (!this->Serialize(&counter))
        {
            return;
        }

        if (counter.GetByteCount() > UINT_MAX)
        {
            // too big
            return;
        }

        char * record = DynamicProfileStorage::AllocRecord(static_cast<DWORD>(counter.GetByteCount()));
#if DBG_DUMP
        if (PHASE_STATS1(DynamicProfilePhase))
        {
            Output::Print(_u("%-180s : %d bytes\n"), url, counter.GetByteCount());
        }
#endif

        BufferWriter writer(DynamicProfileStorage::GetRecordBuffer(record), counter.GetByteCount());
        if (!this->Serialize(&writer))
        {
            Assert(false);
            DynamicProfileStorage::DeleteRecord(record);
        }

        DynamicProfileStorage::SaveRecord(url, record);
    }

#endif
};
#endif
