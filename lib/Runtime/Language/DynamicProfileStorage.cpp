//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

#ifdef DYNAMIC_PROFILE_STORAGE

bool DynamicProfileStorage::initialized = false;
bool DynamicProfileStorage::uninitialized = false;
bool DynamicProfileStorage::enabled = false;
bool DynamicProfileStorage::useCacheDir = false;
bool DynamicProfileStorage::collectInfo = false;
HANDLE DynamicProfileStorage::mutex = nullptr;
char16 DynamicProfileStorage::cacheDrive[_MAX_DRIVE];
char16 DynamicProfileStorage::cacheDir[_MAX_DIR];
char16 DynamicProfileStorage::catalogFilename[_MAX_PATH];
CriticalSection DynamicProfileStorage::cs;
DynamicProfileStorage::InfoMap DynamicProfileStorage::infoMap(&NoCheckHeapAllocator::Instance);
DynamicProfileStorage::TimeType DynamicProfileStorage::creationTime = DynamicProfileStorage::TimeType();
int32 DynamicProfileStorage::lastOffset = 0;
DWORD const DynamicProfileStorage::MagicNumber = 20100526;
DWORD const DynamicProfileStorage::FileFormatVersion = 2;
DWORD DynamicProfileStorage::nextFileId = 0;
#if DBG
bool DynamicProfileStorage::locked = false;
#endif

class DynamicProfileStorageReaderWriter
{
public:
    DynamicProfileStorageReaderWriter() : filename(nullptr), file(nullptr) {}
    ~DynamicProfileStorageReaderWriter();
    bool Init(char16 const * filename, char16 const * mode, bool deleteNonClosed, errno_t * err);
    template <typename T>
    bool Read(T * t);
    template <typename T>
    bool ReadArray(T * t, size_t len);

    _Success_(return) bool ReadUtf8String(__deref_out_z char16 ** str, __out DWORD * len);

    template <typename T>
    bool Write(T const& t);
    template <typename T>
    bool WriteArray(T * t, size_t len);

    bool WriteUtf8String(char16 const * str);

    bool Seek(int32 offset);
    bool SeekToEnd();
    int32 Size();
    void Close(bool deleteFile = false);

private:
    char16 const * filename;
    FILE * file;
    bool deleteNonClosed;
};

DynamicProfileStorageReaderWriter::~DynamicProfileStorageReaderWriter()
{
    if (file)
    {
        Close(deleteNonClosed);
    }
}

bool DynamicProfileStorageReaderWriter::Init(char16 const * filename, char16 const * mode, bool deleteNonClosed, errno_t * err = nullptr)
{
    Assert(file == nullptr);
    errno_t e = _wfopen_s(&file, filename, mode);
    if (e != 0)
    {
        if (err)
        {
            *err = e;
        }
        return false;
    }
    this->filename = filename;
    this->deleteNonClosed = deleteNonClosed;
    return true;
}

template <typename T>
bool DynamicProfileStorageReaderWriter::Read(T * t)
{
    return ReadArray(t, 1);
}

template <typename T>
bool DynamicProfileStorageReaderWriter::ReadArray(T * t, size_t len)
{
    Assert(file);
    int32 pos = ftell(file);
    if (fread(t, sizeof(T), len, file) != len)
    {
        Output::Print(_u("ERROR: DynamicProfileStorage: '%s': File corrupted at %d\n"), filename, pos);
        Output::Flush();
        return false;
    }
    return true;
}

_Success_(return) bool DynamicProfileStorageReaderWriter::ReadUtf8String(__deref_out_z char16 ** str, __out DWORD * len)
{
    DWORD urllen;
    if (!Read(&urllen))
    {
        return false;
    }

    utf8char_t* tempBuffer = NoCheckHeapNewArray(utf8char_t, urllen);
    if (tempBuffer == nullptr)
    {
        Output::Print(_u("ERROR: DynamicProfileStorage: Out of memory reading '%s'\n"), filename);
        Output::Flush();
        return false;
    }

    if (!ReadArray(tempBuffer, urllen))
    {
        HeapDeleteArray(urllen, tempBuffer);
        return false;
    }

    charcount_t length = utf8::ByteIndexIntoCharacterIndex(tempBuffer, urllen);
    char16 * name = NoCheckHeapNewArray(char16, length + 1);
    if (name == nullptr)
    {
        Output::Print(_u("ERROR: DynamicProfileStorage: Out of memory reading '%s'\n"), filename);
        Output::Flush();
        HeapDeleteArray(urllen, tempBuffer);
        return false;
    }
    utf8::DecodeUnitsIntoAndNullTerminateNoAdvance(name, tempBuffer, tempBuffer + urllen);
    NoCheckHeapDeleteArray(urllen, tempBuffer);
    *str = name;
    *len = length;
    return true;
}

template <typename T>
bool DynamicProfileStorageReaderWriter::Write(T const& t)
{
    return WriteArray(&t, 1);
}

template <typename T>
bool DynamicProfileStorageReaderWriter::WriteArray(T * t, size_t len)
{
    Assert(file);
    if (fwrite(t, sizeof(T), len, file) != len)
    {
        Output::Print(_u("ERROR: DynamicProfileStorage: Unable to write to file '%s'\n"), filename);
        Output::Flush();
        return false;
    }
    return true;
}

bool DynamicProfileStorageReaderWriter::WriteUtf8String(char16 const * str)
{
    charcount_t len = static_cast<charcount_t>(wcslen(str));
    utf8char_t * tempBuffer = NoCheckHeapNewArray(utf8char_t, len * 3);
    if (tempBuffer == nullptr)
    {
        Output::Print(_u("ERROR: DynamicProfileStorage: Out of memory writing to file '%s'\n"), filename);
        Output::Flush();
        return false;
    }
    DWORD cbNeeded = (DWORD)utf8::EncodeInto(tempBuffer, str, len);
    bool success = Write(cbNeeded) && WriteArray(tempBuffer, cbNeeded);
    NoCheckHeapDeleteArray(len * 3, tempBuffer);
    return success;
}

bool DynamicProfileStorageReaderWriter::Seek(int32 offset)
{
    Assert(file);
    return fseek(file, offset, SEEK_SET) == 0;
}

bool DynamicProfileStorageReaderWriter::SeekToEnd()
{
    Assert(file);
    return fseek(file, 0, SEEK_END) == 0;
}

int32 DynamicProfileStorageReaderWriter::Size()
{
    Assert(file);
    int32 current = ftell(file);
    SeekToEnd();
    int32 end = ftell(file);
    fseek(file, current, SEEK_SET);
    return end;
}

void DynamicProfileStorageReaderWriter::Close(bool deleteFile)
{
    Assert(file);
    fflush(file);
    fclose(file);
    file = nullptr;
    if (deleteFile)
    {
        _wunlink(filename);
    }
    filename = nullptr;
}

void DynamicProfileStorage::StorageInfo::GetFilename(_Out_writes_z_(_MAX_PATH) char16 filename[_MAX_PATH]) const
{
    char16 tempFile[_MAX_PATH];
    wcscpy_s(tempFile, _u("jsdpcache_file"));
    _itow_s(this->fileId, tempFile + _countof(_u("jsdpcache_file")) - 1, _countof(tempFile) - _countof(_u("jsdpcache_file")) + 1, 10);
    _wmakepath_s(filename, _MAX_PATH, cacheDrive, cacheDir, tempFile, _u(".dpd"));
}

char const * DynamicProfileStorage::StorageInfo::ReadRecord() const
{
    char16 cacheFilename[_MAX_PATH];
    this->GetFilename(cacheFilename);
    DynamicProfileStorageReaderWriter reader;
    if (!reader.Init(cacheFilename, _u("rb"), false))
    {
#if DBG_DUMP
        if (DynamicProfileStorage::DoTrace())
        {
            Output::Print(_u("TRACE: DynamicProfileStorage: Unable to open cache dir file '%s'"), cacheFilename);
            Output::Flush();
        }
#endif
        return nullptr;
    }

    int32 size = reader.Size();
    char * record = AllocRecord(size);
    if (record == nullptr)
    {
        Output::Print(_u("ERROR: DynamicProfileStorage: Out of memory reading '%s'"), cacheFilename);
        Output::Flush();
        return nullptr;
    }

    if (!reader.ReadArray(GetRecordBuffer(record), size))
    {
        DeleteRecord(record);
        return nullptr;
    }
    return record;
}

bool DynamicProfileStorage::StorageInfo::WriteRecord(__in_ecount(sizeof(DWORD) + *record)char const * record) const
{
    char16 cacheFilename[_MAX_PATH];
    this->GetFilename(cacheFilename);
    DynamicProfileStorageReaderWriter writer;
    if (!writer.Init(cacheFilename, _u("wcb"), true))
    {
        Output::Print(_u("ERROR: DynamicProfileStorage: Unable open record file '%s'"), cacheFilename);
        Output::Flush();
        return false;
    }
    if (!writer.WriteArray(GetRecordBuffer(record), GetRecordSize(record)))
    {
        return false;
    }
    // Success
    writer.Close();
    return true;
}

#if DBG_DUMP
bool DynamicProfileStorage::DoTrace()
{
    return Js::Configuration::Global.flags.Trace.IsEnabled(Js::DynamicProfileStoragePhase);
}
#endif

char16 const * DynamicProfileStorage::GetMessageType()
{
    if (!DynamicProfileStorage::DoCollectInfo())
    {
        return _u("WARNING");
    }
#if DBG_DUMP
    if (DynamicProfileStorage::DoTrace())
    {
        return _u("TRACE");
    }
#endif
    return nullptr;
}

bool DynamicProfileStorage::Initialize()
{
    AssertMsg(!initialized, "Initialize called multiple times");
    if (initialized)
    {
        return true;
    }

    bool success = true;
    initialized = true;

#ifdef FORCE_DYNAMIC_PROFILE_STORAGE
    enabled = true;
    collectInfo = true;
    if (!SetupCacheDir(nullptr))
    {
        success = false;
    }

#else
    if (Js::Configuration::Global.flags.IsEnabled(Js::DynamicProfileCacheDirFlag))
    {
        enabled = true;
        collectInfo = true;
        if (!SetupCacheDir(Js::Configuration::Global.flags.DynamicProfileCacheDir))
        {
            success = false;
        }
    }
#endif

    // If -DynamicProfileInput is specified, the file specified in -DynamicProfileCache
    // will not be imported and will be overwritten
    if (Js::Configuration::Global.flags.IsEnabled(Js::DynamicProfileInputFlag))
    {
        enabled = true;
        ClearCacheCatalog();

        // -DynamicProfileInput
        //      Without other -DynamicProfile flags - enable in memory profile cache without exporting
        //      With -DynamicProfileCache           - override the dynamic profile cache file
        //      With -DynamicProfileCacheDir        - clear the dynamic profile cache directory

        if (Js::Configuration::Global.flags.DynamicProfileInput != nullptr)
        {
            // Error if we can't in the profile info if we are not using a cache file or directory.
            collectInfo = collectInfo || Js::Configuration::Global.flags.IsEnabled(Js::DynamicProfileCacheFlag);

            // Try to open the DynamicProfileInput.
            // If failure to open, retry at 100 ms intervals until a timeout.

            const uint32 MAX_DELAY = 2000;  // delay at most 2 seconds
            const uint32 DELAY_INTERVAL = 100;
            const uint32 MAX_TRIES = MAX_DELAY / DELAY_INTERVAL;
            bool readSuccessful = false;

            for (uint32 i = 0; i < MAX_TRIES; i++)
            {
                readSuccessful = ImportFile(Js::Configuration::Global.flags.DynamicProfileInput, false);
                if (readSuccessful)
                {
                    break;
                }

                Sleep(DELAY_INTERVAL);
                if (Js::Configuration::Global.flags.Verbose)
                {
                    Output::Print(_u("  Retrying load of dynamic profile from '%s' (attempt %d)...\n"),
                        (char16 const *)Js::Configuration::Global.flags.DynamicProfileInput, i + 1);
                    Output::Flush();
                }
            }

            if (!readSuccessful)
            {
                // If file cannot be read, behave as if DynamicProfileInput == null.
                collectInfo = true;
            }
        }
        else
        {
            // Don't error if we can't find the profile info
            collectInfo = true;
        }
    }
    else if (Js::Configuration::Global.flags.IsEnabled(Js::DynamicProfileCacheFlag))
    {
        enabled = true;
        collectInfo = true;
        if (Js::Configuration::Global.flags.DynamicProfileCache)
        {
            if (!ImportFile(Js::Configuration::Global.flags.DynamicProfileCache, true))
            {
                success = false;
            }
        }
    }

    return success;
}

// We used to have problem with dynamic profile being corrupt and this is to verify it.
// We don't see this any more so we will just disable it to speed up unittest
#if 0
#if DBG && defined(_M_AMD64)
#define DYNAMIC_PROFILE_EXPORT_FILE_CHECK
#endif
#endif

bool DynamicProfileStorage::Uninitialize()
{
    AssertMsg(!uninitialized, "Uninitialize called multiple times");
    if (!initialized || uninitialized)
    {
        return true;
    }
#ifdef DYNAMIC_PROFILE_EXPORT_FILE_CHECK
    bool exportFile = false;
#endif

    uninitialized = true;
    bool success = true;
    if (Js::Configuration::Global.flags.DynamicProfileCache != nullptr)
    {
        Assert(enabled);
        if (!ExportFile(Js::Configuration::Global.flags.DynamicProfileCache))
        {
            success = false;
        }
#ifdef DYNAMIC_PROFILE_EXPORT_FILE_CHECK
        exportFile = true;
#endif
    }

    if (mutex != nullptr)
    {
        CloseHandle(mutex);
    }
#ifdef DYNAMIC_PROFILE_EXPORT_FILE_CHECK
    uint32 oldCount = infoMap.Count();
#endif

    ClearInfoMap(false);
#ifdef DYNAMIC_PROFILE_EXPORT_FILE_CHECK
    if (exportFile)
    {
        HRESULT hr;
        BEGIN_TRANSLATE_EXCEPTION_AND_ERROROBJECT_TO_HRESULT
        {
            if (!ImportFile(Js::Configuration::Global.flags.DynamicProfileCache, false))
            {
                success = false;
            }
            Assert(oldCount == infoMap.Count());
        }
        END_TRANSLATE_EXCEPTION_AND_ERROROBJECT_TO_HRESULT(hr)
        ClearInfoMap(false);
    }
#endif

    return success;
}

void DynamicProfileStorage::ClearInfoMap(bool deleteFileStorage)
{
    uint recordCount = infoMap.Count();
    uint recordFreed = 0;
    for (uint i = 0; recordFreed < recordCount; i++)
    {
        char16 const * name = infoMap.GetKeyAt(i);
        if (name == nullptr)
        {
            continue;
        }
        NoCheckHeapDeleteArray(wcslen(name) + 1, name);

        StorageInfo const& info = infoMap.GetValueAt(i);
        if (info.isFileStorage)
        {
            Assert(useCacheDir);
            if (deleteFileStorage)
            {
                char16 filename[_MAX_PATH];
                info.GetFilename(filename);
                _wunlink(filename);
            }
        }
        else
        {
            DeleteRecord(info.record);
        }

        recordFreed++;
    }
    infoMap.Clear();
}

bool DynamicProfileStorage::ImportFile(__in_z char16 const * filename, bool allowNonExistingFile)
{
    Assert(enabled);
    DynamicProfileStorageReaderWriter reader;
    errno_t e;
    if (!reader.Init(filename, _u("rb"), false, &e))
    {
        if (allowNonExistingFile)
        {
            return true;
        }
        else
        {
            if (Js::Configuration::Global.flags.Verbose)
            {
                Output::Print(_u("ERROR: DynamicProfileStorage: Unable to open file '%s' to import (%d)\n"), filename, e);

                char16 error_string[256];
                _wcserror_s(error_string, e);
                Output::Print(_u("ERROR:   For file '%s': %s (%d)\n"), filename, error_string, e);
                Output::Flush();
            }
            return false;
        }
    }

    DWORD magic;
    DWORD version;
    DWORD recordCount;
    if (!reader.Read(&magic)
        || !reader.Read(&version)
        || !reader.Read(&recordCount))
    {
        return false;
    }

    if (magic != MagicNumber)
    {
        Output::Print(_u("ERROR: DynamicProfileStorage: '%s' is not a dynamic profile data file"), filename);
        Output::Flush();
        return false;
    }
    if (version != FileFormatVersion)
    {
        if (allowNonExistingFile)
        {
            // Treat version mismatch as non-existent file
            return true;
        }
        Output::Print(_u("ERROR: DynamicProfileStorage: '%s' has format version %d; version %d expected"), filename,
            version, FileFormatVersion);
        Output::Flush();
        return false;
    }

    for (uint i = 0; i < recordCount; i++)
    {
        DWORD len;
        char16 * name;
        if (!reader.ReadUtf8String(&name, &len))
        {
            Assert(false);
            return false;
        }

        DWORD recordLen;
        if (!reader.Read(&recordLen))
        {
            Assert(false);
            return false;
        }

        char * record = AllocRecord(recordLen);
        if (record == nullptr)
        {
            Output::Print(_u("ERROR: DynamicProfileStorage: Out of memory importing '%s'\n"), filename);
            Output::Flush();
            NoCheckHeapDeleteArray(len + 1, name);
            return false;
        }

        if (!reader.ReadArray(GetRecordBuffer(record), recordLen))
        {
            NoCheckHeapDeleteArray(len + 1, name);
            DeleteRecord(record);
            Assert(false);
            return false;
        }

        SaveRecord(name, record);

        // Save record will make a copy of the name
        NoCheckHeapDeleteArray(len + 1, name);
    }
#if DBG_DUMP
    if (DynamicProfileStorage::DoTrace())
    {
        Output::Print(_u("TRACE: DynamicProfileStorage: Imported file: '%s'\n"), filename);
        Output::Flush();
    }
#endif
    AssertMsg(recordCount == (uint)infoMap.Count(), "failed to read all the records");
    return true;
}

bool DynamicProfileStorage::ExportFile(__in_z char16 const * filename)
{
    Assert(enabled);

    if (useCacheDir && AcquireLock())
    {
        if (!LoadCacheCatalog()) // refresh the cache catalog
        {
            ReleaseLock();
            Assert(FALSE);
            return false;
        }
    }

    DynamicProfileStorageReaderWriter writer;

    if (!writer.Init(filename, _u("wcb"), true))
    {
        Output::Print(_u("ERROR: DynamicProfileStorage: Unable to open file '%s' to export\n"), filename);
        Output::Flush();
        return false;
    }
    DWORD recordCount = infoMap.Count();
    if (!writer.Write(MagicNumber)
        || !writer.Write(FileFormatVersion)
        || !writer.Write(recordCount))
    {
        Assert(FALSE);
        return false;
    }
    uint recordWritten = 0;
    for (uint i = 0; recordWritten < recordCount; i++)
    {
        char16 const * url = infoMap.GetKeyAt(i);
        if (url == nullptr)
        {
            Assert(false);
            continue;
        }

        StorageInfo const& info = infoMap.GetValueAt(i);

        char const * record;
        if (info.isFileStorage)
        {
            Assert(useCacheDir);
            record = info.ReadRecord();
            if (record == nullptr)
            {
                ReleaseLock();
                Assert(FALSE);
                return false;
            }
        }
        else
        {
            Assert(!useCacheDir);
            Assert(!locked);
            record = info.record;
        }
        DWORD recordSize = GetRecordSize(record);

        bool failed = (!writer.WriteUtf8String(url)
            || !writer.Write(recordSize)
            || !writer.WriteArray(GetRecordBuffer(record), recordSize));

        if (useCacheDir)
        {
            DeleteRecord(record);
        }
        if (failed)
        {
            if (useCacheDir)
            {
                ReleaseLock();
            }
            Assert(FALSE);
            return false;
        }

        recordWritten++;
    }
    writer.Close();
#if DBG_DUMP
    if (DynamicProfileStorage::DoTrace())
    {
        Output::Print(_u("TRACE: DynamicProfileStorage: Exported file: '%s'\n"), filename);
        Output::Flush();
    }
#endif
    return true;
}

void DynamicProfileStorage::DisableCacheDir()
{
    Assert(useCacheDir);
    ClearInfoMap(false);
    useCacheDir = false;
#ifdef FORCE_DYNAMIC_PROFILE_STORAGE
    Js::Throw::FatalInternalError();
#endif
}

bool DynamicProfileStorage::AcquireLock()
{
    Assert(mutex != nullptr);
    Assert(!locked);
    DWORD ret = WaitForSingleObject(mutex, INFINITE);
    if (ret == WAIT_OBJECT_0 || ret == WAIT_ABANDONED)
    {
#if DBG
        locked = true;
#endif
        return true;
    }
    Output::Print(_u("ERROR: DynamicProfileStorage: Unable to acquire mutex %d\n"), ret);
    Output::Flush();
    DisableCacheDir();

    return false;
}

bool DynamicProfileStorage::ReleaseLock()
{
    Assert(locked);
    Assert(mutex != nullptr);
#if DBG
    locked = false;
#endif
    if (ReleaseMutex(mutex))
    {
        return true;
    }
    DisableCacheDir();
    Output::Print(_u("ERROR: DynamicProfileStorage: Unable to release mutex"));
    Output::Flush();
    return false;
}

bool DynamicProfileStorage::SetupCacheDir(__in_z char16 const * dirname)
{
    Assert(enabled);

    mutex = CreateMutex(NULL, FALSE, _u("JSDPCACHE"));
    if (mutex == nullptr)
    {
        Output::Print(_u("ERROR: DynamicProfileStorage: Unable to create mutex"));
        Output::Flush();
        return false;
    }

    useCacheDir = true;
    if (!AcquireLock())
    {
        return false;
    }

    char16 tempPath[_MAX_PATH];
    if (dirname == nullptr)
    {
        uint32 len = GetTempPath(_MAX_PATH, tempPath);
        if (len >= _MAX_PATH || wcscat_s(tempPath, _u("jsdpcache")) != 0)
        {
            DisableCacheDir();
            Output::Print(_u("ERROR: DynamicProfileStorage: Can't setup cache directory: Unable to create directory\n"));
            Output::Flush();
            ReleaseLock();
            return false;
        }

        if (!CreateDirectory(tempPath, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        {
            DisableCacheDir();
            Output::Print(_u("ERROR: DynamicProfileStorage: Can't setup cache directory: Unable to create directory\n"));
            Output::Flush();
            ReleaseLock();
            return false;
        }
        dirname = tempPath;
    }

    char16 cacheFile[_MAX_FNAME];
    char16 cacheExt[_MAX_EXT];
    _wsplitpath_s(dirname, cacheDrive, cacheDir, cacheFile, cacheExt);
    wcscat_s(cacheDir, cacheFile);
    wcscat_s(cacheDir, cacheExt);

    _wmakepath_s(catalogFilename, cacheDrive, cacheDir, _u("jsdpcache_master"), _u(".dpc"));
    bool succeed = LoadCacheCatalog();
    ReleaseLock();

    return succeed;
}

bool DynamicProfileStorage::CreateCacheCatalog()
{
    Assert(enabled);
    Assert(useCacheDir);
    Assert(locked);
    nextFileId = 0;
    creationTime = GetCreationTime();
    DynamicProfileStorageReaderWriter catalogFile;
    if (!catalogFile.Init(catalogFilename, _u("wb"), true)
        || !catalogFile.Write(MagicNumber)
        || !catalogFile.Write(FileFormatVersion)
        || !catalogFile.Write(creationTime)
        || !catalogFile.Write(0)) // count
    {
        DisableCacheDir();
        Output::Print(_u("ERROR: DynamicProfileStorage: Unable to create cache catalog\n"));
        Output::Flush();
        return false;
    }
    lastOffset = catalogFile.Size();
    ClearInfoMap(true);
    catalogFile.Close();

#if DBG_DUMP
    if (DynamicProfileStorage::DoTrace())
    {
        Output::Print(_u("TRACE: DynamicProfileStorage: Cache directory catalog created: '%s'\n"), catalogFilename);
        Output::Flush();
    }
#endif
    return true;
}

bool DynamicProfileStorage::AppendCacheCatalog(__in_z char16 const * url)
{
    Assert(enabled);
    Assert(useCacheDir);
    Assert(locked);
    DWORD magic;
    DWORD version;
    DWORD count;
    DWORD time;
    DynamicProfileStorageReaderWriter catalogFile;
    if (!catalogFile.Init(catalogFilename, _u("rcb+"), false))
    {
        return CreateCacheCatalog();
    }

    if (!catalogFile.Seek(0)
        || !catalogFile.Read(&magic)
        || !catalogFile.Read(&version)
        || !catalogFile.Read(&time)
        || !catalogFile.Read(&count)
        || magic != MagicNumber
        || version < FileFormatVersion)
    {
        catalogFile.Close();
#if DBG_DUMP
        if (DynamicProfileStorage::DoTrace())
        {
            Output::Print(_u("TRACE: DynamicProfileStorage: Overwriting file for cache directory catalog: '%s'\n"), catalogFilename);
            Output::Flush();
        }
#endif
        return CreateCacheCatalog();
    }

    if (version > FileFormatVersion)
    {
        DisableCacheDir();
        Output::Print(_u("ERROR: DynamicProfileStorage: Existing cache catalog has a newer format\n"));
        Output::Flush();
        return false;
    }

    if (time != creationTime || count + 1 != nextFileId)
    {
        // This should not happen, as we are under lock from the LoadCacheCatalog
        DisableCacheDir();
        Output::Print(_u("ERROR: DynamicProfileStorage: Internal error, file modified under lock\n"));
        Output::Flush();
        return false;
    }

    if (!catalogFile.SeekToEnd() ||
        !catalogFile.WriteUtf8String(url) ||
        !catalogFile.Seek(3 * sizeof(DWORD)) ||
        !catalogFile.Write(nextFileId))
    {
#if DBG_DUMP
        if (DynamicProfileStorage::DoTrace())
        {
            Output::Print(_u("TRACE: DynamicProfileStorage: Write failure. Cache directory catalog corrupted: '%s'\n"), catalogFilename);
            Output::Flush();
        }
#endif
        catalogFile.Close();
        return CreateCacheCatalog();
    }

    lastOffset = catalogFile.Size();
    return true;
}

bool DynamicProfileStorage::LoadCacheCatalog()
{
    Assert(enabled);
    Assert(useCacheDir);
    Assert(locked);
    DynamicProfileStorageReaderWriter catalogFile;
    DWORD magic;
    DWORD version;
    DWORD count;
    DWORD time;
    if (!catalogFile.Init(catalogFilename, _u("rb"), false))
    {
        return CreateCacheCatalog();
    }
    if (!catalogFile.Read(&magic)
        || !catalogFile.Read(&version)
        || !catalogFile.Read(&time)
        || !catalogFile.Read(&count)
        || magic != MagicNumber
        || version < FileFormatVersion)
    {
        catalogFile.Close();
#if DBG_DUMP
        if (DynamicProfileStorage::DoTrace())
        {
            Output::Print(_u("TRACE: DynamicProfileStorage: Overwriting file for cache directory catalog: '%s'\n"), catalogFilename);
            Output::Flush();
        }
#endif
        return CreateCacheCatalog();
    }

    if (version > FileFormatVersion)
    {
        DisableCacheDir();
        Output::Print(_u("ERROR: DynamicProfileStorage: Existing cache catalog has a newer format.\n"));
        Output::Flush();
        return false;
    }

    DWORD start = 0;

    Assert(useCacheDir);
    if (time == creationTime)
    {
        // We can reuse existing data
        start = infoMap.Count();
        Assert(count >= start);
        Assert(catalogFile.Size() >= lastOffset);
        if (count == nextFileId)
        {
            Assert(catalogFile.Size() == lastOffset);
            return true;
        }

        if (!catalogFile.Seek(lastOffset))
        {
            catalogFile.Close();
            Output::Print(_u("ERROR: DynamicProfileStorage: Unable to seek to last known offset\n"));
            Output::Flush();
            return CreateCacheCatalog();
        }
    }
    else if (creationTime != 0)
    {
        Output::Print(_u("WARNING: DynamicProfileStorage: Reloading full catalog\n"));
        Output::Flush();
    }

    for (DWORD i = start; i < count; i++)
    {
        DWORD len;
        char16 * url;
        if (!catalogFile.ReadUtf8String(&url, &len))
        {
#if DBG_DUMP
            if (DynamicProfileStorage::DoTrace())
            {
                Output::Print(_u("TRACE: DynamicProfileStorage: Cache dir catalog file corrupted: '%s'\n"), catalogFilename);
                Output::Flush();
            }
#endif
            // REVIEW: the file is corrupted, should we not flush the cache totally?
            catalogFile.Close();
            return CreateCacheCatalog();
        }

        StorageInfo * oldInfo;
        if (infoMap.TryGetReference(url, &oldInfo))
        {
            Assert(oldInfo->isFileStorage);
            oldInfo->fileId = i;
        }
        else
        {
            StorageInfo newInfo;
            newInfo.isFileStorage = true;
            newInfo.fileId = i;
            infoMap.Add(url, newInfo);
        }
    }

#if DBG_DUMP
    if (creationTime == 0 && DynamicProfileStorage::DoTrace())
    {
        Output::Print(_u("TRACE: DynamicProfileStorage: Cache directory catalog loaded: '%s'\n"), catalogFilename);
        Output::Flush();
    }
#endif

    nextFileId = count;
    creationTime = time;
    lastOffset = catalogFile.Size();
    return true;
}

void DynamicProfileStorage::ClearCacheCatalog()
{
    Assert(enabled);
    if (useCacheDir)
    {
        if (!AcquireLock())
        {
            return;
        }
        bool success = CreateCacheCatalog();
        ReleaseLock();
        if (success)
        {
#if DBG_DUMP
            if (DynamicProfileStorage::DoTrace())
            {
                Output::Print(_u("TRACE: DynamicProfileStorage: Cache dir clears\n"));
                Output::Flush();
            }
#endif
            return;
        }
    }
    else
    {
        ClearInfoMap(false);
    }
}

void DynamicProfileStorage::SaveRecord(__in_z char16 const * filename, __in_ecount(sizeof(DWORD) + *record) char const * record)
{
    Assert(enabled);
    AutoCriticalSection autocs(&cs);

    StorageInfo * info;

    if (useCacheDir && AcquireLock())
    {
        if (!LoadCacheCatalog()) // refresh the cache catalog
        {
            ReleaseLock();
        }
    }

    if (infoMap.TryGetReference(filename, &info))
    {
        if (!info->isFileStorage)
        {
            Assert(!useCacheDir);
            if (info->record != nullptr)
            {
                DeleteRecord(info->record);
            }
            info->record = record;
            return;
        }
        Assert(useCacheDir);

        char16 cacheFilename[_MAX_PATH];
        info->GetFilename(cacheFilename);
        DynamicProfileStorageReaderWriter writer;
        if (info->WriteRecord(record))
        {
            // Success
            ReleaseLock();
            return;
        }

        // Fail, try to add it again
        info->fileId = nextFileId++;
        if (info->WriteRecord(record))
        {
            if (AppendCacheCatalog(filename))
            {
                ReleaseLock();
                return;
            }
        }
        else
        {
            DisableCacheDir();
        }

        // Can't add a new file. Disable and use memory mode
        Assert(!useCacheDir);
        ReleaseLock();
    }

    size_t len = wcslen(filename) + 1;
    char16 * newFilename = NoCheckHeapNewArray(char16, len);
    if (newFilename == nullptr)
    {
        // out of memory, don't save anything
        AssertMsg(false, "OOM");
        DeleteRecord(record);
        if (useCacheDir)
        {
            ReleaseLock();
        }
        return;
    }
    wmemcpy_s(newFilename, len, filename, len);

    StorageInfo newInfo;
    if (useCacheDir)
    {
        newInfo.isFileStorage = true;
        newInfo.fileId = nextFileId++;

        if (newInfo.WriteRecord(record))
        {
            infoMap.Add(newFilename, newInfo);
            if (AppendCacheCatalog(newFilename))
            {
                ReleaseLock();
                return;
            }
        }

        // Can't even add a record. Disable and use memory mode
        DisableCacheDir();
        ReleaseLock();
    }

    Assert(!useCacheDir);
    Assert(!locked);

    newInfo.isFileStorage = false;
    newInfo.record = record;
    infoMap.Add(newFilename, newInfo);
}

char * DynamicProfileStorage::AllocRecord(DWORD bufferSize)
{
    Assert(enabled);
    char * buffer = NoCheckHeapNewArray(char, bufferSize + sizeof(DWORD));
    if (buffer != nullptr)
    {
        *(DWORD *)buffer = bufferSize;
    }
    return buffer;
}

DWORD DynamicProfileStorage::GetRecordSize(__in_ecount(sizeof(DWORD) + *buffer) char const * buffer)
{
    Assert(enabled);
    return *(DWORD *)buffer;
}

char const * DynamicProfileStorage::GetRecordBuffer(__in_ecount(sizeof(DWORD) + *buffer) char const * buffer)
{
    Assert(enabled);
    return buffer + sizeof(DWORD);
}

char * DynamicProfileStorage::GetRecordBuffer(__in_ecount(sizeof(DWORD) + *buffer) char * buffer)
{
    Assert(enabled);
    return buffer + sizeof(DWORD);
}

void DynamicProfileStorage::DeleteRecord(__in_ecount(sizeof(DWORD) + *buffer) char const * buffer)
{
    Assert(enabled);
    NoCheckHeapDeleteArray(GetRecordSize(buffer) + sizeof(DWORD), buffer);
}
#endif
