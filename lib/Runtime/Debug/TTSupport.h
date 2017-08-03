//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

//This file contains definitions for general-ish purpose structures/algorithms that we use in the TTD system
//We may want to replace them with other versions (e.g. that are already in the codebase) at some later time

#if ENABLE_TTD

class HostScriptContextCallbackFunctor;
namespace TTD
{
    class ThreadContextTTD;
    class ScriptContextTTD;
    class RuntimeContextInfo;
    class ExecutionInfoManager;

    //We typedef Js::Var into a TTD version that has the same bit layout but we want to avoid confusion
    //if this bit layout is for the "live" state or potentially only for the snapshot state or the representations change later
    typedef Js::Var TTDVar;

    namespace NSSnapType
    {
        struct SnapPropertyRecord;
        struct SnapHandlerPropertyEntry;
        struct SnapHandler;
        struct SnapType;
    }

    namespace NSSnapValues
    {
        struct SnapPrimitiveValue;
        struct SlotArrayInfo;
        struct ScriptFunctionScopeInfo;
        struct SnapFunctionBodyScopeChain;
        struct TopLevelScriptLoadFunctionBodyResolveInfo;
        struct TopLevelNewFunctionBodyResolveInfo;
        struct TopLevelEvalFunctionBodyResolveInfo;
        struct FunctionBodyResolveInfo;
        struct SnapContext;
    }

    namespace NSSnapObjects
    {
        struct SnapObject;
    }

    class SnapShot;
    class SnapshotExtractor;
    class TTDExceptionFramePopper;
    struct SingleCallCounter;

    namespace NSLogEvents
    {
        struct EventLogEntry;
    }

    class EventLog;
    class TTDebuggerAbortException;
    class TTDebuggerSourceLocation;
}

void _NOINLINE __declspec(noreturn) TTDAbort_fatal_error(const char* msg);

////////
//Memory allocators used by the TT code
template <typename T>
T* TTD_MEM_ALLOC_CHECK(T* alloc)
{
    if(alloc == nullptr)
    {
        TTDAssert(false, "OOM in TTD");
    }

    return alloc;
}

#define TT_HEAP_NEW(T, ...) TTD_MEM_ALLOC_CHECK(HeapNewNoThrow(T, __VA_ARGS__))
#define TT_HEAP_ALLOC_ARRAY(T, SIZE_IN_ELEMENTS) TTD_MEM_ALLOC_CHECK(HeapNewNoThrowArray(T, SIZE_IN_ELEMENTS))
#define TT_HEAP_ALLOC_ARRAY_ZERO(T, SIZE_IN_ELEMENTS) TTD_MEM_ALLOC_CHECK(HeapNewNoThrowArrayZ(T, SIZE_IN_ELEMENTS))

#define TT_HEAP_DELETE(T, ELEM) HeapDelete(ELEM)
#define TT_HEAP_FREE_ARRAY(T, ELEM, SIZE_IN_ELEMENTS) HeapDeleteArray(SIZE_IN_ELEMENTS, ELEM)

////////
//The default number of elements per arrayblock, the size of "small" arrays and the block size we use when extracting
#define TTD_ARRAY_BLOCK_SIZE 0x200
#define TTD_ARRAY_SMALL_ARRAY 0x100

//Convert from Js::Var to TTDVar
#define TTD_CONVERT_JSVAR_TO_TTDVAR(X) ((TTD::TTDVar)(X))
#define TTD_CONVERT_TTDVAR_TO_JSVAR(X) ((Js::Var)(X))

//The ids for objects in a snapshot
typedef uint64 TTD_PTR_ID;
#define TTD_INVALID_PTR_ID 0ul

#define TTD_CONVERT_VAR_TO_PTR_ID(X) reinterpret_cast<TTD_PTR_ID>(PointerValue(X))
#define TTD_CONVERT_TYPEINFO_TO_PTR_ID(X) reinterpret_cast<TTD_PTR_ID>(X)
#define TTD_CONVERT_FUNCTIONBODY_TO_PTR_ID(X) reinterpret_cast<TTD_PTR_ID>(X)
#define TTD_CONVERT_ENV_TO_PTR_ID(X) reinterpret_cast<TTD_PTR_ID>(X)
#define TTD_CONVERT_SLOTARRAY_TO_PTR_ID(X) reinterpret_cast<TTD_PTR_ID>(X)
#define TTD_CONVERT_SCOPE_TO_PTR_ID(X) reinterpret_cast<TTD_PTR_ID>(X)
#define TTD_CONVERT_DEBUGSCOPE_TO_PTR_ID(X) reinterpret_cast<TTD_PTR_ID>(X)

//Promises have a wide range of heap allocated bits -- we define He-Man casts for all of them -- ugly but so is having a bunch of specific functions
#define TTD_CONVERT_PROMISE_INFO_TO_PTR_ID(X) reinterpret_cast<TTD_PTR_ID>(PointerValue(X))
#define TTD_CONVERT_PROMISE_INFO_TO_SPECIFIC_TYPE(T, X) static_cast<T*>(X)

#define TTD_COERCE_PTR_ID_TO_VAR(X) (reinterpret_cast<Js::Var>(X))
#define TTD_COERCE_PTR_ID_TO_FUNCTIONBODY(X) (reinterpret_cast<Js::FunctionBody*>(X))

//The representation of LOG ids (based on object pointers)
typedef uint64 TTD_LOG_PTR_ID;
#define TTD_INVALID_LOG_PTR_ID 0ul

#define TTD_CONVERT_OBJ_TO_LOG_PTR_ID(X) reinterpret_cast<TTD_LOG_PTR_ID>(X)

//The representation of an identifier (currently access path) for a well known object/primitive/function body/etc. in the JS engine or HOST
typedef const char16* TTD_WELLKNOWN_TOKEN;
#define TTD_INVALID_WELLKNOWN_TOKEN nullptr
#define TTD_DIAGNOSTIC_COMPARE_WELLKNOWN_TOKENS(T1, T2) ((T1 == T2) || ((T1 != TTD_INVALID_WELLKNOWN_TOKEN) && (T2 != TTD_INVALID_WELLKNOWN_TOKEN) && wcscmp(T1, T2) == 0))

////////

//2MB slabs in allocator with a threshold of 4Kb to allocate a single block in our large space
#define TTD_SLAB_BLOCK_ALLOCATION_SIZE_LARGE 2097152
#define TTD_SLAB_BLOCK_ALLOCATION_SIZE_MID 262144
#define TTD_SLAB_BLOCK_ALLOCATION_SIZE_SMALL 32768

//A threshold of 4Kb to allocate a single block in our large space
#define TTD_SLAB_LARGE_BLOCK_SIZE 4096

#define TTD_WORD_ALIGN_ALLOC_SIZE(ASIZE) (((ASIZE) + 0x3) & 0xFFFFFFFC)

#define TTD_SLAB_BLOCK_SIZE TTD_WORD_ALIGN_ALLOC_SIZE(sizeof(SlabBlock))
#define TTD_SLAB_BLOCK_USABLE_SIZE(SLABSIZE) (SLABSIZE - TTD_SLAB_BLOCK_SIZE)

#define TTD_LARGE_SLAB_BLOCK_SIZE TTD_WORD_ALIGN_ALLOC_SIZE(sizeof(LargeSlabBlock))

//allocation increments for the array list -- we pick sizes that are just larger than the number generated by a "simple" programs
#define TTD_ARRAY_LIST_SIZE_LARGE 4096
#define TTD_ARRAY_LIST_SIZE_DEFAULT 2048
#define TTD_ARRAY_LIST_SIZE_MID 512
#define TTD_ARRAY_LIST_SIZE_SMALL 128
#define TTD_ARRAY_LIST_SIZE_XSMALL 32

//A basic universal hash function for our dictionary
#define TTD_DICTIONARY_LOAD_FACTOR 2
#define TTD_DICTIONARY_HASH(X, P) ((uint32)((X) % P))
#define TTD_DICTIONARY_INDEX(X, M) ((uint32)((X) & (M - 1)))

//Support for our mark table
#define TTD_MARK_TABLE_INIT_SIZE 65536
#define TTD_MARK_TABLE_INIT_H2PRIME 32749
#define TTD_MARK_TABLE_HASH1(X, S) ((uint32)((X) & (S - 1)))
#define TTD_MARK_TABLE_HASH2(X, P) ((uint32)((X) % P))
#define TTD_MARK_TABLE_INDEX(X, M) ((uint32)((X) & (M - 1)))

#define TTD_TABLE_FACTORLOAD_BASE(C, P1, P2) if(targetSize < C) { *powerOf2 = C; *closePrime = P1; *midPrime = P2; }
#define TTD_TABLE_FACTORLOAD(C, P1, P2) else TTD_TABLE_FACTORLOAD_BASE(C, P1, P2)
#define TTD_TABLE_FACTORLOAD_FINAL(C, P1, P2) else { *powerOf2 = C; *closePrime = P1; *midPrime = P2; }

namespace TTD
{
    //Mode of the time-travel debugger
    enum class TTDMode
    {
        Invalid = 0x0,
        CurrentlyEnabled = 0x1,  //The TTD system is enabled and actively performing record/replay/debug
        RecordMode = 0x2,     //The system is being run in Record mode
        ReplayMode = 0x4,  //The system is being run in Replay mode
        DebuggerMode = (ReplayMode | 0x8),  //The system is being run in with Debugger actions enabled
        AnyMode = (RecordMode | ReplayMode | DebuggerMode),

        ExcludedExecutionTTAction = 0x20,  //Set when the system is executing code on behalf of the TTD system (so we don't want to record/replay things for it)
        ExcludedExecutionDebuggerAction = 0x40,  //Set when the system is executing code on behalf of the Debugger system (so we don't want to record/replay things for it)
        AnyExcludedMode = (ExcludedExecutionTTAction | ExcludedExecutionDebuggerAction),

        DebuggerSuppressGetter = 0x200, //Set when the system is doing a property access for the debugger (so we don't want to accidentally trigger a getter execution)
        DebuggerSuppressBreakpoints = 0x400, //Set to prevent breakpoints (or break on exception) when moving in TT mode
        DebuggerLogBreakpoints = 0x800 //Set to indicate we want to log breakpoints encountered when executing (but not actually halt)
    };
    DEFINE_ENUM_FLAG_OPERATORS(TTDMode)

    class TTModeStack
    {
    private:
        TTDMode* m_stackEntries;

        uint32 m_stackTop;
        uint32 m_stackMax;

    public:
        TTModeStack();
        ~TTModeStack();

        uint32 Count() const;
        TTDMode GetAt(uint32 index) const;
        void SetAt(uint32 index, TTDMode m);

        void Push(TTDMode m);
        TTDMode Peek() const;
        void Pop();
    };

    //Typedef for list of contexts that we are recording/replaying on -- used in the EventLog
    typedef JsUtil::List<Js::ScriptContext*, HeapAllocator> TTDContextList;

    namespace NSSnapObjects
    {
        //An enumeration of tags for the SnapObjects (to support dispatch when parsing)
        //IMPORTANT: When adding a new SnapObject subclass you need to add a corresponding typeid here
        enum class SnapObjectType : int
        {
            Invalid = 0x0,

            SnapUnhandledObject,
            SnapDynamicObject,
            SnapExternalObject,
            SnapScriptFunctionObject,
            SnapRuntimeFunctionObject,
            SnapExternalFunctionObject,
            SnapRuntimeRevokerFunctionObject,
            SnapBoundFunctionObject,
            SnapActivationObject,
            SnapBlockActivationObject,
            SnapPseudoActivationObject,
            SnapConsoleScopeActivationObject,
            SnapHeapArgumentsObject,
            SnapES5HeapArgumentsObject,
            SnapBoxedValueObject,
            SnapDateObject,
            SnapRegexObject,
            SnapErrorObject,
            SnapArrayObject,
            SnapNativeIntArrayObject,
            SnapNativeFloatArrayObject,
            SnapES5ArrayObject,
            SnapArrayBufferObject,
            SnapTypedArrayObject,
            SnapSetObject,
            SnapMapObject,
            SnapProxyObject,
            SnapPromiseObject,
            SnapPromiseResolveOrRejectFunctionObject,
            SnapPromiseReactionTaskFunctionObject,
            SnapPromiseAllResolveElementFunctionObject,

            //objects that should always be well known but which may have other info we want to restore
            SnapWellKnownObject,

            Limit
        };
        DEFINE_ENUM_FLAG_OPERATORS(SnapObjectType);
    }

    //A struct that maintains the relation between a globally stable top-level body counter and the PTR id it has in this particular script context
    struct TopLevelFunctionInContextRelation
    {
        //The globally unique body counter id from the log
        uint32 TopLevelBodyCtr;

        //The PTR_ID that is used to refer to this top-level body within the given script context
        TTD_PTR_ID ContextSpecificBodyPtrId;
    };

    //Function pointer definitions and a struct for writing data out of memory (presumably to stable storage)
    typedef void* JsTTDStreamHandle;

    typedef JsTTDStreamHandle(CALLBACK *TTDOpenResourceStreamCallback)(size_t uriLength, const char* uri, size_t filenameLength, const char* filename, bool read, bool write);
    typedef bool(CALLBACK *TTDReadBytesFromStreamCallback)(JsTTDStreamHandle handle, byte* buff, size_t size, size_t* readCount);
    typedef bool(CALLBACK *TTDWriteBytesToStreamCallback)(JsTTDStreamHandle handle, const byte* buff, size_t size, size_t* writtenCount);
    typedef void(CALLBACK *TTDFlushAndCloseStreamCallback)(JsTTDStreamHandle handle, bool read, bool write);

    struct TTDataIOInfo
    {
        TTDOpenResourceStreamCallback pfOpenResourceStream;

        TTDReadBytesFromStreamCallback pfReadBytesFromStream;
        TTDWriteBytesToStreamCallback pfWriteBytesToStream;
        TTDFlushAndCloseStreamCallback pfFlushAndCloseStream;

        //Current location that we are writing TT data into as a utf8 encoded uri (we may have several sub paths from the root for writing different parts of the log)
        size_t ActiveTTUriLength;
        const char* ActiveTTUri;
    };

    //Function pointer definitions for creating/interacting with external objects
    typedef void(CALLBACK *TTDCreateExternalObjectCallback)(Js::ScriptContext* ctx, Js::Var* object);

    typedef void(CALLBACK *TTDCreateJsRTContextCallback)(void* runtimeHandle, Js::ScriptContext** ctx); //Create and pin the context so it does not get GC'd
    typedef void(CALLBACK *TTDReleaseJsRTContextCallback)(FinalizableObject* jsrtCtx); //Release an un-needed context during replay
    typedef void(CALLBACK *TTDSetActiveJsRTContext)(void* runtimeHandle, Js::ScriptContext* ctx); //Set active jsrtcontext

    struct ExternalObjectFunctions
    {
        TTDCreateExternalObjectCallback pfCreateExternalObject;

        TTDCreateJsRTContextCallback pfCreateJsRTContextCallback;
        TTDReleaseJsRTContextCallback pfReleaseJsRTContextCallback;
        TTDSetActiveJsRTContext pfSetActiveJsRTContext;
    };

    namespace UtilSupport
    {
        //A basic auto-managed null terminated string with value semantics
        class TTAutoString
        {
        private:
            int64 m_allocSize;
            char16* m_contents;
            char16* m_optFormatBuff;

        public:
            TTAutoString();
            TTAutoString(const char16* str);
            TTAutoString(const TTAutoString& str);

            TTAutoString& operator=(const TTAutoString& str);

            ~TTAutoString();

            void Clear();

            TTAutoString(TTAutoString&& str) = delete;
            TTAutoString& operator=(TTAutoString&& str) = delete;

            bool IsNullString() const;

            void Append(const char16* str, size_t start = 0, size_t end = SIZE_T_MAX);
            void Append(const TTAutoString& str, size_t start = 0, size_t end = SIZE_T_MAX);

            void Append(uint64 val);

            void Append(LPCUTF8 strBegin, LPCUTF8 strEnd);

            int32 GetLength() const;
            char16 GetCharAt(int32 pos) const;
            const char16* GetStrValue() const;
        };
    }

    //Given a target capacity return (1) the nearest upper power of 2, (2) a good close prime, and (3) a good mid prime
    void LoadValuesForHashTables(uint32 targetSize, uint32* powerOf2, uint32* closePrime, uint32* midPrime);

    //////////////////

    //String representation to use when copying things into/between slab allocators
    struct TTString
    {
        //Length of the string in terms of char16s -- excluding any '\0' termination
        uint32 Length;

        //The char contents of the string (null terminated -- may be null if empty)
        char16* Contents;
    };

    //initialize and return true if the given string should map to a nullptr char* representaiton
    void InitializeAsNullPtrTTString(TTString& str);
    bool IsNullPtrTTString(const TTString& str);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
    //This is for diagnostic purposes only
    bool TTStringEQForDiagnostics(const TTString& str1, const TTString& str2);
#endif

    //A class that implements a simple slab memory allocator
    template <int32 canUnlink>
    class SlabAllocatorBase
    {
    private:
        //A block that the slab allocator uses
        struct SlabBlock
        {
            //The actual block for the data -- should be laid out immediately after this
            byte* BlockData;

            //The previous/next block in the list
            SlabBlock* Previous;
            SlabBlock* Next;

            //The reference counter for this -- invalid if canUnlink == 0
            int32 RefCounter;
        };

        //A "large" block allocation list
        struct LargeSlabBlock
        {
            //The actual block for the data -- should be laid out immediately after this
            byte* BlockData;

            //The size of the block in bytes (includes both LargeSlabBlock and data memory)
            uint32 TotalBlockSize;

            //The previous/next block in the list
            LargeSlabBlock* Previous;
            LargeSlabBlock* Next;

            //Place well known meta-data ptr distance here (0) so we know it is a large block (not a regular slab alloc).
            ptrdiff_t MetaDataSential;
        };

        //We inline the fields for the current/end of the block that we currently are allocating from a
        byte* m_currPos;
        byte* m_endPos;

        //The size we want to allocate for each SlabBlock -- set in constructor and unchanged after
        const uint32 m_slabBlockSize;

        //The list of slab blocks used in the allocator (the current block to allocate from is always at the head)
        SlabBlock* m_headBlock;

        //The large allocation list
        LargeSlabBlock* m_largeBlockList;

        //Allow us to speculatively reserve blocks for data and make sure we don't double allocate anything
        uint32 m_reserveActiveBytes;
        LargeSlabBlock* m_reserveActiveLargeBlock;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        //The amount of allocated memory with useful data
        uint64 m_totalAllocatedSize;
#endif

        //Get a new block in the slab
        void AddNewBlock()
        {
            byte* allocBlock = TT_HEAP_ALLOC_ARRAY(byte, this->m_slabBlockSize);
            TTDAssert((reinterpret_cast<uint64>(allocBlock) & 0x3) == 0, "We have non-word aligned allocations so all our later work is not so useful");

            SlabBlock* newBlock = (SlabBlock*)allocBlock;
            byte* dataArray = (allocBlock + TTD_SLAB_BLOCK_SIZE);

            this->m_currPos = dataArray;
            this->m_endPos = dataArray + TTD_SLAB_BLOCK_USABLE_SIZE(this->m_slabBlockSize);

            newBlock->BlockData = dataArray;
            newBlock->Next = nullptr;
            newBlock->Previous = this->m_headBlock;
            newBlock->RefCounter = 0;

            this->m_headBlock->Next = newBlock;
            this->m_headBlock = newBlock;
        }

        //Allocate a byte* of the the template size (word aligned)
        template <size_t n>
        byte* SlabAllocateTypeRawSize()
        {
            TTDAssert(this->m_reserveActiveBytes == 0, "Don't double allocate memory.");
            TTDAssert(n <= TTD_SLAB_LARGE_BLOCK_SIZE, "Don't allocate large requests in the bump pool.");

            uint32 desiredsize = TTD_WORD_ALIGN_ALLOC_SIZE(n + canUnlink); //make alloc size word aligned
            TTDAssert((desiredsize % 4 == 0) & (desiredsize >= (n + canUnlink)) & (desiredsize < TTD_SLAB_BLOCK_USABLE_SIZE(this->m_slabBlockSize)), "We can never allocate a block this big with the slab allocator!!");

            if(this->m_currPos + desiredsize > this->m_endPos)
            {
                this->AddNewBlock();
            }

            byte* res = this->m_currPos;
            this->m_currPos += desiredsize;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            this->m_totalAllocatedSize += TTD_WORD_ALIGN_ALLOC_SIZE(n);
#endif

            if(canUnlink)
            {
                TTDAssert(canUnlink == sizeof(ptrdiff_t), "We need enough space for a ptr to the meta-data.");

                //record the block associated with this allocation
                *((ptrdiff_t*)res) = res - ((byte*)this->m_headBlock);

                res += canUnlink;

                //update the ctr for this block
                this->m_headBlock->RefCounter++;
            }

            return res;
        }

        //Allocate a byte* of the the given size (word aligned)
        template <bool reserve, bool commit>
        byte* SlabAllocateRawSize(size_t requestedBytes)
        {
            TTDAssert(requestedBytes != 0, "Don't allocate empty arrays.");
            TTDAssert(requestedBytes <= TTD_SLAB_LARGE_BLOCK_SIZE, "Don't allocate large requests in the bump pool.");

            byte* res = nullptr;
            uint32 desiredsize = TTD_WORD_ALIGN_ALLOC_SIZE(requestedBytes + canUnlink); //make alloc size word aligned
            TTDAssert((desiredsize % 4 == 0) & (desiredsize >= (requestedBytes + canUnlink)) & (desiredsize < TTD_SLAB_BLOCK_USABLE_SIZE(this->m_slabBlockSize)), "We can never allocate a block this big with the slab allocator!!");

            if(reserve)
            {
                TTDAssert(this->m_reserveActiveBytes == 0, "Don't double allocate memory.");

                if(this->m_currPos + desiredsize > this->m_endPos)
                {
                    this->AddNewBlock();
                }

                res = this->m_currPos;

                if(canUnlink)
                {
                    TTDAssert(canUnlink == sizeof(ptrdiff_t), "We need enough space for a ptr to the meta-data.");

                    //record the block associated with this allocation
                    *((ptrdiff_t*)res) = res - ((byte*)this->m_headBlock);

                    res += canUnlink;
                }
            }

            if(commit)
            {
                this->m_currPos += desiredsize;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
                this->m_totalAllocatedSize += desiredsize;
#endif

                if(canUnlink)
                {
                    this->m_headBlock->RefCounter++;
                }
            }

            if(reserve && !commit)
            {
                this->m_reserveActiveBytes = desiredsize;
            }

            if(!reserve && commit)
            {
                TTDAssert(desiredsize <= this->m_reserveActiveBytes, "We are commiting more that we reserved.");

                this->m_reserveActiveBytes = 0;
            }

            return res;
        }

        //Commit the allocation of a large block
        void CommitLargeBlockAllocation(LargeSlabBlock* newBlock, size_t blockSize)
        {
#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            this->m_totalAllocatedSize += blockSize;
#endif

            if(this->m_largeBlockList != nullptr)
            {
                this->m_largeBlockList->Next = newBlock;
            }

            this->m_largeBlockList = newBlock;
        }

        //Allocate a byte* of the the given size (word aligned) in the large block pool
        template<bool commit>
        byte* SlabAllocateLargeBlockSize(size_t requestedBytes)
        {
            TTDAssert(requestedBytes > TTD_SLAB_LARGE_BLOCK_SIZE, "Don't allocate small requests in the large pool.");

            uint32 desiredsize = TTD_WORD_ALIGN_ALLOC_SIZE(requestedBytes + TTD_LARGE_SLAB_BLOCK_SIZE); //make alloc size word aligned
            TTDAssert((desiredsize % 4 == 0) & (desiredsize >= (requestedBytes + TTD_LARGE_SLAB_BLOCK_SIZE)), "We can never allocate a block this big with the slab allocator!!");

            TTDAssert(this->m_reserveActiveBytes == 0, "Don't double allocate memory.");

            byte* tmp = TT_HEAP_ALLOC_ARRAY(byte, desiredsize);

            LargeSlabBlock* newBlock = (LargeSlabBlock*)tmp;
            newBlock->BlockData = (tmp + TTD_LARGE_SLAB_BLOCK_SIZE);

            newBlock->TotalBlockSize = desiredsize;
            newBlock->Previous = this->m_largeBlockList;
            newBlock->Next = nullptr;

            newBlock->MetaDataSential = 0;

            if(commit)
            {
                this->CommitLargeBlockAllocation(newBlock, desiredsize);
            }
            else
            {
                this->m_reserveActiveBytes = desiredsize;
                this->m_reserveActiveLargeBlock = newBlock;
            }

            return newBlock->BlockData;
        }

    public:
        SlabAllocatorBase(uint32 slabBlockSize)
            : m_largeBlockList(nullptr), m_slabBlockSize(slabBlockSize)
        {
            byte* allocBlock = TT_HEAP_ALLOC_ARRAY(byte, this->m_slabBlockSize);
            TTDAssert((reinterpret_cast<uint64>(allocBlock) & 0x3) == 0, "We have non-word aligned allocations so all our later work is not so useful");

            this->m_headBlock = (SlabBlock*)allocBlock;
            byte* dataArray = (allocBlock + TTD_SLAB_BLOCK_SIZE);

            this->m_currPos = dataArray;
            this->m_endPos = dataArray + TTD_SLAB_BLOCK_USABLE_SIZE(this->m_slabBlockSize);

            this->m_headBlock->BlockData = dataArray;

            this->m_headBlock->Next = nullptr;
            this->m_headBlock->Previous = nullptr;

            this->m_headBlock->RefCounter = 0;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            this->m_totalAllocatedSize = 0;
#endif

            this->m_reserveActiveBytes = 0;
            this->m_reserveActiveLargeBlock = nullptr;
        }

        ~SlabAllocatorBase()
        {
            SlabBlock* currBlock = this->m_headBlock;
            while(currBlock != nullptr)
            {
                SlabBlock* tmp = currBlock;
                currBlock = currBlock->Previous;

                TT_HEAP_FREE_ARRAY(byte, (byte*)tmp, this->m_slabBlockSize);
            }

            LargeSlabBlock* currLargeBlock = this->m_largeBlockList;
            while(currLargeBlock != nullptr)
            {
                LargeSlabBlock* tmp = currLargeBlock;
                currLargeBlock = currLargeBlock->Previous;

                TT_HEAP_FREE_ARRAY(byte, (byte*)tmp, tmp->TotalBlockSize);
            }

            if(this->m_reserveActiveLargeBlock != nullptr)
            {
                TT_HEAP_FREE_ARRAY(byte, (byte*)this->m_reserveActiveLargeBlock, this->m_reserveActiveBytes);
                this->m_reserveActiveLargeBlock = nullptr;
            }
        }

        //eliminate some potentially dangerous operators
        SlabAllocatorBase(const SlabAllocatorBase&) = delete;
        SlabAllocatorBase& operator=(SlabAllocatorBase const&) = delete;

        //clone a null terminated char16* string (or nullptr) into the allocator -- currently only used for wellknown tokens
        const char16* CopyRawNullTerminatedStringInto(const char16* str)
        {
            if(str == nullptr)
            {
                return nullptr;
            }
            else
            {
                size_t length = wcslen(str) + 1;
                size_t byteLength = length * sizeof(char16);

                char16* res = this->SlabAllocateArray<char16>(length);
                js_memcpy_s(res, byteLength, str, byteLength);

                return res;
            }
        }

        //clone a string into the allocator of a known length
        void CopyStringIntoWLength(const char16* str, uint32 length, TTString& into)
        {
            TTDAssert(str != nullptr, "Not allowed for string + length");

            into.Length = length;
            into.Contents = this->SlabAllocateArray<char16>(into.Length + 1);

            //don't js_memcpy if the contents length is 0
            js_memcpy_s(into.Contents, into.Length * sizeof(char16), str, length * sizeof(char16));
            into.Contents[into.Length] = '\0';
        }

        //initialize and allocate the memory for a string of a given length (but don't set contents yet)
        void InitializeAndAllocateWLength(uint32 length, TTString& into)
        {
            into.Length = length;
            into.Contents = this->SlabAllocateArray<char16>(into.Length + 1);
            into.Contents[0] = '\0';
        }

        //clone a string into the allocator
        void CopyNullTermStringInto(const char16* str, TTString& into)
        {
            if(str == nullptr)
            {
                into.Length = 0;
                into.Contents = nullptr;
            }
            else
            {
                this->CopyStringIntoWLength(str, (uint32)wcslen(str), into);
            }
        }

        //Return the memory that contains useful data in this slab & the same as the reserved space
        void ComputeMemoryUsed(uint64* usedSpace, uint64* reservedSpace) const
        {
            uint64 memreserved = 0;

            for(SlabBlock* currBlock = this->m_headBlock; currBlock != nullptr; currBlock = currBlock->Previous)
            {
                memreserved += (uint64)this->m_slabBlockSize;
            }

            for(LargeSlabBlock* currLargeBlock = this->m_largeBlockList; currLargeBlock != nullptr; currLargeBlock = currLargeBlock->Previous)
            {
                memreserved += (uint64)(currLargeBlock->TotalBlockSize);
            }

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            *usedSpace = this->m_totalAllocatedSize;
#else
            *usedSpace = 0;
#endif

            *reservedSpace = memreserved;
        }

        //Allocate with "new" from the slab allocator
        template<typename T, typename... Args>
        T* SlabNew(Args... args)
        {
            return new (this->SlabAllocateTypeRawSize<sizeof(T)>()) T(args...);
        }

        //Allocate a T* of the size needed for the template type
        template <typename T>
        T* SlabAllocateStruct()
        {
            return (T*)this->SlabAllocateTypeRawSize<sizeof(T)>();
        }

        //Allocate T* of the size needed to hold count elements of the given type
        template <typename T>
        T* SlabAllocateArray(size_t count)
        {
            size_t size = count * sizeof(T);
            if(size <= TTD_SLAB_LARGE_BLOCK_SIZE)
            {
                return (T*)this->SlabAllocateRawSize<true, true>(size);
            }
            else
            {
                return (T*)this->SlabAllocateLargeBlockSize<true>(size);
            }
        }

        //Allocate T* of the size needed to hold count elements of the given type
        template <typename T>
        T* SlabAllocateArrayZ(size_t count)
        {
            T* res = nullptr;

            size_t size = count * sizeof(T);
            if(size <= TTD_SLAB_LARGE_BLOCK_SIZE)
            {
                res = (T*)this->SlabAllocateRawSize<true, true>(size);
            }
            else
            {
                res = (T*)this->SlabAllocateLargeBlockSize<true>(size);
            }

            memset(res, 0, size);
            return res;
        }

        //Allocate T* of the size needed to hold count (a compile time constant) number of elements of the given type
        template <typename T, size_t count>
        T* SlabAllocateFixedSizeArray()
        {
            if(count * sizeof(T) <= TTD_SLAB_LARGE_BLOCK_SIZE)
            {
                return (T*)this->SlabAllocateRawSize<true, true>(count * sizeof(T));
            }
            else
            {
                return (T*)this->SlabAllocateLargeBlockSize<true>(count * sizeof(T));
            }
        }

        //Reserve BUT DO NOT COMMIT and allocation of size needed to hold count elements of the given type
        template <typename T>
        T* SlabReserveArraySpace(size_t count)
        {
            size_t size = count * sizeof(T);
            if(size <= TTD_SLAB_LARGE_BLOCK_SIZE)
            {
                return (T*)this->SlabAllocateRawSize<true, false>(size);
            }
            else
            {
                return (T*)this->SlabAllocateLargeBlockSize<false>(size);
            }
        }

        //Commit the allocation of count (or fewer) elements of the given type that were reserved previously
        template <typename T>
        void SlabCommitArraySpace(size_t actualCount, size_t reservedCount)
        {
            TTDAssert(this->m_reserveActiveBytes != 0, "We don't have anything reserved.");

            size_t reservedSize = reservedCount * sizeof(T);
            if(reservedSize <= TTD_SLAB_LARGE_BLOCK_SIZE)
            {
                TTDAssert(this->m_reserveActiveLargeBlock == nullptr, "We should not have a large block active!!!");

                size_t actualSize = actualCount * sizeof(T);
                this->SlabAllocateRawSize<false, true>(actualSize);
            }
            else
            {
                TTDAssert(this->m_reserveActiveLargeBlock != nullptr, "We should have a large block active!!!");

                this->CommitLargeBlockAllocation(this->m_reserveActiveLargeBlock, this->m_reserveActiveBytes);

                this->m_reserveActiveLargeBlock = nullptr;
                this->m_reserveActiveBytes = 0;
            }
        }

        //Abort the allocation of the elements of the given type that were reserved previously
        template <typename T>
        void SlabAbortArraySpace(size_t reservedCount)
        {
            TTDAssert(this->m_reserveActiveBytes != 0, "We don't have anything reserved.");

            size_t reservedSize = reservedCount * sizeof(T);
            if(reservedSize <= TTD_SLAB_LARGE_BLOCK_SIZE)
            {
                TTDAssert(this->m_reserveActiveLargeBlock == nullptr, "We should not have a large block active!!!");

                this->m_reserveActiveBytes = 0;
            }
            else
            {
                TTDAssert(this->m_reserveActiveLargeBlock != nullptr, "We should have a large block active!!!");

                TT_HEAP_FREE_ARRAY(byte, (byte*)this->m_reserveActiveLargeBlock, this->m_reserveActiveBytes);

                this->m_reserveActiveLargeBlock = nullptr;
                this->m_reserveActiveBytes = 0;
            }
        }

        //If allowed unlink the memory allocation specified and free the block if it is no longer used by anyone
        void UnlinkAllocation(const void* allocation)
        {
            TTDAssert(canUnlink != 0, "Unlink not allowed with this slab allocator.");
            TTDAssert(this->m_reserveActiveBytes == 0, "We don't have anything reserved.");

            //get the meta-data for this allocation and see if it is a
            byte* realBase = ((byte*)allocation) - canUnlink;
            ptrdiff_t offset = *((ptrdiff_t*)realBase);

            if(offset == 0)
            {
                //it is a large allocation just free it
                LargeSlabBlock* largeBlock = (LargeSlabBlock*)(((byte*)allocation) - TTD_LARGE_SLAB_BLOCK_SIZE);

                if(largeBlock == this->m_largeBlockList)
                {
                    TTDAssert(largeBlock->Next == nullptr, "Should always have a null next at head");

                    this->m_largeBlockList = this->m_largeBlockList->Previous;
                    if(this->m_largeBlockList != nullptr)
                    {
                        this->m_largeBlockList->Next = nullptr;
                    }
                }
                else
                {
                    if(largeBlock->Next != nullptr)
                    {
                        largeBlock->Next->Previous = largeBlock->Previous;
                    }

                    if(largeBlock->Previous != nullptr)
                    {
                        largeBlock->Previous->Next = largeBlock->Next;
                    }
                }

                TT_HEAP_FREE_ARRAY(byte, (byte*)largeBlock, largeBlock->TotalBlockSize);
            }
            else
            {
                //lookup the slab block and do ref counting
                SlabBlock* block = (SlabBlock*)(realBase - offset);

                block->RefCounter--;
                if(block->RefCounter == 0)
                {
                    if(block == this->m_headBlock)
                    {
                        //we always need a head block to allocate out of -- so instead of deleting just reset it
                        this->m_currPos = this->m_headBlock->BlockData;
                        this->m_endPos = this->m_headBlock->BlockData + TTD_SLAB_BLOCK_USABLE_SIZE(this->m_slabBlockSize);

                        memset(this->m_currPos, 0, TTD_SLAB_BLOCK_USABLE_SIZE(this->m_slabBlockSize));

                        this->m_headBlock->RefCounter = 0;
                    }
                    else
                    {
                        if(block->Next != nullptr)
                        {
                            block->Next->Previous = block->Previous;
                        }

                        if(block->Previous != nullptr)
                        {
                            block->Previous->Next = block->Next;
                        }

                        TT_HEAP_FREE_ARRAY(byte, (byte*)block, this->m_slabBlockSize);
                    }
                }
            }
        }

        //Unlink the memory used by a string
        void UnlinkString(const TTString& str)
        {
            if(str.Contents != nullptr)
            {
                this->UnlinkAllocation(str.Contents);
            }
        }
    };

    //The standard slab allocator implemention does not allow for unlinking
    typedef SlabAllocatorBase<0> SlabAllocator;

    //The unlinkable slab allocator implemention (as expected) allows for unlinking
    typedef SlabAllocatorBase<sizeof(ptrdiff_t)> UnlinkableSlabAllocator;

    //////////////////
    //An (unordered) array list class that has fast insertion/write-into and does not move contents

    template<typename T, size_t allocSize>
    class UnorderedArrayList
    {
    private:
        struct UnorderedArrayListLink
        {
            //The current end of the allocated data in the block
            T* CurrPos;

            //The last valid position in the block -- a little unusual but then the "alloc part does not depend on the outcome of the if condition"
            T* EndPos;

            //The actual block for the data
            T* BlockData;

            //The next block in the list
            UnorderedArrayListLink* Next;
        };

        //The the data in this
        UnorderedArrayListLink m_inlineHeadBlock;

        //the allocators we use for this work
        SlabAllocator* m_alloc;

        void AddArrayLink()
        {
            UnorderedArrayListLink* copiedOldBlock = this->m_alloc->template SlabAllocateStruct<UnorderedArrayListLink>();
            *copiedOldBlock = this->m_inlineHeadBlock;

            T* newBlockData = this->m_alloc->template SlabAllocateFixedSizeArray<T, allocSize>();

            this->m_inlineHeadBlock.BlockData = newBlockData;
            this->m_inlineHeadBlock.CurrPos = newBlockData;
            this->m_inlineHeadBlock.EndPos = newBlockData + allocSize;

            this->m_inlineHeadBlock.Next = copiedOldBlock;
        }

    public:
        UnorderedArrayList(SlabAllocator* alloc)
            : m_alloc(alloc)
        {
            T* newBlockData = this->m_alloc->template SlabAllocateFixedSizeArray<T, allocSize>();

            this->m_inlineHeadBlock.BlockData = newBlockData;
            this->m_inlineHeadBlock.CurrPos = newBlockData;
            this->m_inlineHeadBlock.EndPos = newBlockData + allocSize;

            this->m_inlineHeadBlock.Next = nullptr;
        }

        ~UnorderedArrayList()
        {
            //everything is slab allocated so disapears with that
        }

        //Add the entry to the unordered list
        void AddEntry(T data)
        {
            TTDAssert(this->m_inlineHeadBlock.CurrPos <= this->m_inlineHeadBlock.EndPos, "We are off the end of the array");
            TTDAssert((((byte*)this->m_inlineHeadBlock.CurrPos) - ((byte*)this->m_inlineHeadBlock.BlockData)) / sizeof(T) <= allocSize, "We are off the end of the array");

            if(this->m_inlineHeadBlock.CurrPos == this->m_inlineHeadBlock.EndPos)
            {
                this->AddArrayLink();
            }

            *(this->m_inlineHeadBlock.CurrPos) = data;
            this->m_inlineHeadBlock.CurrPos++;
        }

        //Get the next uninitialized entry (at the front of the sequence)
        //We expect the caller to initialize this memory appropriately
        T* NextOpenEntry()
        {
            TTDAssert(this->m_inlineHeadBlock.CurrPos <= this->m_inlineHeadBlock.EndPos, "We are off the end of the array");
            TTDAssert((((byte*)this->m_inlineHeadBlock.CurrPos) - ((byte*)this->m_inlineHeadBlock.BlockData)) / sizeof(T) <= allocSize, "We are off the end of the array");

            if(this->m_inlineHeadBlock.CurrPos == this->m_inlineHeadBlock.EndPos)
            {
                this->AddArrayLink();
            }

            T* entry = this->m_inlineHeadBlock.CurrPos;
            this->m_inlineHeadBlock.CurrPos++;

            return entry;
        }

        //NOT constant time
        uint32 Count() const
        {
            size_t count = (((byte*)this->m_inlineHeadBlock.CurrPos) - ((byte*)this->m_inlineHeadBlock.BlockData)) / sizeof(T);
            TTDAssert(count <= allocSize, "We somehow wrote in too much data.");

            for(UnorderedArrayListLink* curr = this->m_inlineHeadBlock.Next; curr != nullptr; curr = curr->Next)
            {
                size_t ncount = (((byte*)curr->CurrPos) - ((byte*)curr->BlockData)) / sizeof(T);
                TTDAssert(ncount <= allocSize, "We somehow wrote in too much data.");

                count += ncount;
            }

            return (uint32)count;
        }

        class Iterator
        {
        private:
            UnorderedArrayListLink m_currLink;
            T* m_currEntry;

        public:
            Iterator(const UnorderedArrayListLink& head)
                : m_currLink(head), m_currEntry(head.BlockData)
            {
                //check for empty list and invalidate the iter if it is
                if(this->m_currEntry == this->m_currLink.CurrPos)
                {
                    this->m_currEntry = nullptr;
                }
            }

            const T* Current() const
            {
                return this->m_currEntry;
            }

            T* Current()
            {
                return this->m_currEntry;
            }

            bool IsValid() const
            {
                return this->m_currEntry != nullptr;
            }

            void MoveNext()
            {
                this->m_currEntry++;

                if(this->m_currEntry == this->m_currLink.CurrPos)
                {
                    if(this->m_currLink.Next == nullptr)
                    {
                        this->m_currEntry = nullptr;
                    }
                    else
                    {
                        this->m_currLink = *(this->m_currLink.Next);
                        this->m_currEntry = this->m_currLink.BlockData;
                    }
                }
            }
        };

        Iterator GetIterator() const
        {
            return Iterator(this->m_inlineHeadBlock);
        }
    };

    //////////////////
    //A specialized map for going from TTD_PTR_ID values to some information associated with them

    template <typename Tag, typename T>
    class TTDIdentifierDictionary
    {
    private:
        //An entry struct for the dictionary
        struct Entry
        {
            Tag Key;
            T Data;
        };

        //The 2 prime numbers we use for our hasing function
        uint32 m_h1Prime;
        uint32 m_h2Prime;

        //The hash max capcity and data array
        uint32 m_capacity;
        Entry* m_hashArray;

        //Count of elements in the dictionary
        uint32 m_count;

        template <bool findEmpty>
        Entry* FindSlotForId(Tag id) const
        {
            TTDAssert(this->m_h1Prime != 0 && this->m_h2Prime != 0, "Not valid!!");
            TTDAssert(this->m_hashArray != nullptr, "Not valid!!");

            Tag searchKey = findEmpty ? 0 : id;

            //h1Prime is less than table size by construction so we dont need to re-index
            uint32 primaryIndex = TTD_DICTIONARY_HASH(id, this->m_h1Prime);
            if(this->m_hashArray[primaryIndex].Key == searchKey)
            {
                return (this->m_hashArray + primaryIndex);
            }

            //do a hash for the second offset to avoid clustering and then do linear probing
            uint32 offset = TTD_DICTIONARY_HASH(id, this->m_h2Prime);
            uint32 probeIndex = TTD_DICTIONARY_INDEX(primaryIndex + offset, this->m_capacity);
            while(true)
            {
                Entry* curr = (this->m_hashArray + probeIndex);
                if(curr->Key == searchKey)
                {
                    return curr;
                }
                probeIndex = TTD_DICTIONARY_INDEX(probeIndex + 1, this->m_capacity);

                TTDAssert(probeIndex != TTD_DICTIONARY_INDEX(primaryIndex + offset, this->m_capacity), "The key is not here (or we messed up).");
            }
        }

        void InitializeEntry(Entry* entry, Tag id, const T& item)
        {
            entry->Key = id;
            entry->Data = item;
        }

    public:
        void Unload()
        {
            if(this->m_hashArray != nullptr)
            {
                TT_HEAP_FREE_ARRAY(Entry, this->m_hashArray, this->m_capacity);
                this->m_hashArray = nullptr;
                this->m_capacity = 0;

                this->m_count = 0;

                this->m_h1Prime = 0;
                this->m_h2Prime = 0;
            }
        }

        void Initialize(uint32 capacity)
        {
            TTDAssert(this->m_hashArray == nullptr, "Should not already be initialized.");

            uint32 desiredSize = capacity * TTD_DICTIONARY_LOAD_FACTOR;

            LoadValuesForHashTables(desiredSize, &(this->m_capacity), &(this->m_h1Prime), &(this->m_h2Prime));

            this->m_hashArray = TT_HEAP_ALLOC_ARRAY_ZERO(Entry, this->m_capacity);
        }

        TTDIdentifierDictionary()
            : m_h1Prime(0), m_h2Prime(0), m_capacity(0), m_hashArray(nullptr), m_count(0)
        {
        }

        ~TTDIdentifierDictionary()
        {
            this->Unload();
        }

        void MoveDataInto(TTDIdentifierDictionary& src)
        {
            this->m_h1Prime = src.m_h1Prime;
            this->m_h2Prime = src.m_h2Prime;
            this->m_capacity = src.m_capacity;
            this->m_hashArray = src.m_hashArray;
            this->m_count = src.m_count;

            src.m_hashArray = TT_HEAP_ALLOC_ARRAY_ZERO(Entry, src.m_capacity);
            src.m_count = 0;
        }

        bool IsValid() const
        {
            return this->m_hashArray != nullptr;
        }

        uint32 Count() const
        {
            return this->m_count;
        }

        bool Contains(Tag id) const
        {
            TTDAssert(this->m_h1Prime != 0 && this->m_h2Prime != 0, "Not valid!!");
            TTDAssert(this->m_hashArray != nullptr, "Not valid!!");

            //h1Prime is less than table size by construction so we dont need to re-index
            uint32 primaryIndex = TTD_DICTIONARY_HASH(id, this->m_h1Prime);
            if(this->m_hashArray[primaryIndex].Key == id)
            {
                return true;
            }

            if(this->m_hashArray[primaryIndex].Key == 0)
            {
                return false;
            }

            //do a hash for the second offset to avoid clustering and then do linear probing
            uint32 offset = TTD_DICTIONARY_HASH(id, this->m_h2Prime);
            uint32 probeIndex = TTD_DICTIONARY_INDEX(primaryIndex + offset, this->m_capacity);
            while(true)
            {
                Entry* curr = (this->m_hashArray + probeIndex);
                if(curr->Key == id)
                {
                    return true;
                }

                if(curr->Key == 0)
                {
                    return false;
                }

                probeIndex = TTD_DICTIONARY_INDEX(probeIndex + 1, this->m_capacity);

                TTDAssert(probeIndex != TTD_DICTIONARY_INDEX(primaryIndex + offset, this->m_capacity), "The key is not here (or we messed up).");
            }
        }

        void AddItem(Tag id, const T& item)
        {
            TTDAssert(this->m_count * TTD_DICTIONARY_LOAD_FACTOR < this->m_capacity, "The dictionary is being sized incorrectly and will likely have poor performance");
            Entry* entry = this->FindSlotForId<true>(id);

            InitializeEntry(entry, id, item);
            this->m_count++;
        }

        //Lookup an item which is known to exist in the dictionary (and errors if it is not present)
        const T& LookupKnownItem(Tag id) const
        {
            Entry* entry = this->FindSlotForId<false>(id);

            return entry->Data;
        }
    };

    //////////////////
    //A simple table to do object marking

    //
    //TODO: This table isn't great performance wise (around 2/3 of the time for a snapshot is spent in the mark-phase)
    //      Right now perf. isn't a limiting factor but we probably want to improve on this.
    //

    //Mark the table and what kind of value is at the address
    enum class MarkTableTag : byte
    {
        Clear = 0x0,
        //
        TypeHandlerTag = 0x1,
        TypeTag = 0x2,
        PrimitiveObjectTag = 0x3,
        CompoundObjectTag = 0x4,
        FunctionBodyTag = 0x5,
        EnvironmentTag = 0x6,
        SlotArrayTag = 0x7,
        KindTagCount = 0x8,
        AllKindMask = 0xF,
        //
        JsWellKnownObj = 0x10, //mark for objects that the JS runtime creates and are well known (Array, Undefined, ...)
        SpecialTagMask = 0xF0
    };
    DEFINE_ENUM_FLAG_OPERATORS(MarkTableTag);

    class MarkTable
    {
    private:
        //The addresses and their marks
        uint64* m_addrArray;
        MarkTableTag* m_markArray;

        //Capcity and count of the table (we use capcity for fast & hashing instead of %);
        uint32 m_capcity;
        uint32 m_count;
        uint32 m_h2Prime;

        //iterator position
        uint32 m_iterPos;

        //Counts of how many handlers/types/... we have marked
        uint32 m_handlerCounts[(uint32)MarkTableTag::KindTagCount];

        int32 FindIndexForKey(uint64 addr) const
        {
            TTDAssert(this->m_addrArray != nullptr, "Not valid!!");

            uint32 primaryMask = this->m_capcity - 1;

            uint32 primaryIndex = TTD_MARK_TABLE_HASH1(addr, this->m_capcity);
            uint64 primaryAddr = this->m_addrArray[primaryIndex];
            if((primaryAddr == addr) | (primaryAddr == 0))
            {
                return (int32)primaryIndex;
            }

            //do a hash for the second offset to avoid clustering and then do linear probing
            uint32 offset = TTD_MARK_TABLE_HASH2(addr, this->m_h2Prime);
            uint32 probeIndex = TTD_MARK_TABLE_INDEX(primaryIndex + offset, this->m_capcity);
            while(true)
            {
                uint64 currAddr = this->m_addrArray[probeIndex];
                if((currAddr == addr) | (currAddr == 0))
                {
                    return (int32)probeIndex;
                }
                probeIndex = TTD_MARK_TABLE_INDEX(probeIndex + 1, this->m_capcity);

                TTDAssert(probeIndex != ((primaryIndex + offset) & primaryMask), "We messed up.");
            }
        }

        void Grow()
        {
            uint32 oldCapacity = this->m_capcity;
            uint64* oldAddrArray = this->m_addrArray;
            MarkTableTag* oldMarkArray = this->m_markArray;

            this->m_capcity = this->m_capcity << 1; //double capacity

            uint32 dummyPowerOf2 = 0;
            uint32 dummyNearPrime = 0;
            LoadValuesForHashTables(this->m_capcity, &dummyPowerOf2, &dummyNearPrime, &(this->m_h2Prime));

            this->m_addrArray = TT_HEAP_ALLOC_ARRAY_ZERO(uint64, this->m_capcity);
            this->m_markArray = TT_HEAP_ALLOC_ARRAY_ZERO(MarkTableTag, this->m_capcity);

            for(uint32 i = 0; i < oldCapacity; ++i)
            {
                int32 idx = this->FindIndexForKey(oldAddrArray[i]);
                this->m_addrArray[idx] = oldAddrArray[i];
                this->m_markArray[idx] = oldMarkArray[i];
            }

            TT_HEAP_FREE_ARRAY(uint64, oldAddrArray, oldCapacity);
            TT_HEAP_FREE_ARRAY(MarkTableTag, oldMarkArray, oldCapacity);
        }

        int32 FindIndexForKeyWGrow(const void* addr)
        {
            //keep the load factor < 25%
            if((this->m_capcity >> 2) < this->m_count)
            {
                this->Grow();
            }

            return this->FindIndexForKey(reinterpret_cast<uint64>(addr));
        }

    public:
        MarkTable();
        ~MarkTable();

        void Clear();

        //Check if the address has the given tag (return true if it *DOES NOT*) and put the mark tag value in the location
        template <MarkTableTag kindtag>
        bool MarkAndTestAddr(const void* vaddr)
        {
            int32 idx = this->FindIndexForKeyWGrow(vaddr);

            //we really want to do the check on m_markArray but since we know nothing has been cleared we can check the addrArray for better cache behavior
            bool notMarked = this->m_addrArray[idx] == 0;

            if(notMarked)
            {
                this->m_addrArray[idx] = reinterpret_cast<uint64>(vaddr);
                this->m_markArray[idx] = kindtag;

                this->m_count++;
                (this->m_handlerCounts[(uint32)kindtag])++;
            }
            TTDAssert(this->m_markArray[idx] == kindtag, "We had some sort of collision.");

            return notMarked;
        }

        //Mark the address as containing a well known object of the given kind
        template <MarkTableTag specialtag>
        void MarkAddrWithSpecialInfo(const void* vaddr)
        {
            int32 idx = this->FindIndexForKey(reinterpret_cast<uint64>(vaddr));

            if(this->m_markArray[idx] != MarkTableTag::Clear)
            {
                this->m_markArray[idx] |= specialtag;
            }
        }

        //Return true if the location is marked
        bool IsMarked(const void* vaddr) const
        {
            int32 idx = this->FindIndexForKey(reinterpret_cast<uint64>(vaddr));

            return this->m_markArray[idx] != MarkTableTag::Clear;
        }

        //Return true if the location is tagged as well-known
        bool IsTaggedAsWellKnown(const void* vaddr) const
        {
            int32 idx = this->FindIndexForKey(reinterpret_cast<uint64>(vaddr));

            return (this->m_markArray[idx] & MarkTableTag::JsWellKnownObj) != MarkTableTag::Clear;
        }

        //return clear if no more addresses
        MarkTableTag GetTagValue() const
        {
            if(this->m_iterPos >= this->m_capcity)
            {
                return MarkTableTag::Clear;
            }
            else
            {
                return this->m_markArray[this->m_iterPos];
            }
        }

        bool GetTagValueIsWellKnown() const
        {
            return (this->m_markArray[this->m_iterPos] & MarkTableTag::JsWellKnownObj) != MarkTableTag::Clear;
        }

        template<typename T>
        T GetPtrValue() const
        {
            return reinterpret_cast<T>(this->m_addrArray[this->m_iterPos]);
        }

        //Clear the mark at the given address
        void ClearMark(const void* vaddr)
        {
            int32 idx = this->FindIndexForKey(reinterpret_cast<uint64>(vaddr));

            //DON'T CLEAR THE ADDR JUST CLEAR THE MARK
            this->m_markArray[idx] = MarkTableTag::Clear;
        }

        template <MarkTableTag kindtag>
        uint32 GetCountForTag()
        {
            return this->m_handlerCounts[(uint32)kindtag];
        }

        void MoveToNextAddress()
        {
            this->m_iterPos++;

            while(this->m_iterPos < this->m_capcity)
            {
                MarkTableTag tag = this->m_markArray[this->m_iterPos];

                if((tag & MarkTableTag::AllKindMask) != MarkTableTag::Clear)
                {
                    return;
                }

                this->m_iterPos++;
            }
        }

        void InitializeIter()
        {
            this->m_iterPos = 0;

            if(this->m_markArray[0] == MarkTableTag::Clear)
            {
                this->MoveToNextAddress();
            }
        }
    };
}

#endif
