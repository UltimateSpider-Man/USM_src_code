#include "nfl_system.h"

#include "common.h"
#include "func_wrapper.h"
#include "log.h"
#include "nfl_driver.h"
#include "trace.h"
#include "tx_system.h"
#include "utility.h"
#include "variable.h"

#include <cassert>
#include <filesystem>
#include <unordered_map>

struct nflFile {
    int m_fileSize;
    int field_4;
    struct {
        struct {
            int *parent;
            int field_4;
            int childCount;
        } subfile;
    } as;

    int field_14;
    int empty[2];
    enum {
        NFS_FILE_TYPE_NATIVE = 0,
        NFS_FILE_TYPE_SUBFILE = 1,
    } fileType;
};

VALIDATE_SIZE(nflFile, 0x24);

VALIDATE_SIZE(nflRequest, 0x44u);
VALIDATE_OFFSET(nflRequest, field_38, 0x38);

VALIDATE_SIZE(nflDriver, 0x48u);

VALIDATE_SIZE(nflCommand, 0x1Cu);

struct Struct_984524
{
    nflRequestState field_0;
    void (*m_callback)(nflRequestState, nflRequestID, resource_pack_streamer *);
    nflRequestID field_8;
    resource_pack_streamer *field_C;
    int field_10;
};

VALIDATE_SIZE(Struct_984524, 0x14);

struct nflStream 
{
    int field_0;
    int field_4;
    int field_8;
    int field_C;
};

VALIDATE_SIZE(nflStream, 0x10);

struct Struct_94983C {
    int field_0;
    nflDriver **field_4;

    Struct_94983C()
    {
        field_0 = 1;

        static nflDriver driver {};
        static auto *v0 = &driver;
        field_4 = &v0;
    }
};

struct Struct_984498
{
    int field_0;
    int used;
    int free;
};


#if !STANDALONE_SYSTEM

static auto & nfl_initParams = var<nflInitParams>(0x00949730);

static auto & stru_94983C = var<const Struct_94983C>(0x0094983C);

static auto & nfl_filePool = var<txSlotPool>(0x009844F4);

static auto & dword_98452C = var<nflFile *>(0x0098452C);

static auto & dword_984528 = var<nflStream *>(0x00984528);

static txSlotPool & nfl_streamPool = var<txSlotPool>(0x009844CC);

static char *& dword_984530 = var<char *>(0x00984530);

static txSlotPool & nfl_requestPool = var<txSlotPool>(0x009844A4);

static Struct_984524 *& dword_984524 = var<Struct_984524 *>(0x00984524);

static nflRequest *& dword_98451C = var<nflRequest *>(0x0098451C);

static auto & hMutex = var<HANDLE>(0x00984490);

static int & dword_984534 = var<int>(0x00984534);

static HANDLE & hHandle = var<HANDLE>(0x00984494);

static HANDLE & dword_98448C = var<HANDLE>(0x0098448C);

static nflStreamID & nfs_defaultStreamID = var<nflStreamID>(0x0094972C);

static int & nfs_callRequestsCount = var<int>(0x00984520);

static Struct_984498 & dword_984498 = var<Struct_984498>(0x00984498);

#else

#define make_var(type, name) \
    static type g_##name {}; \
    type & name {g_##name}


static nflInitParams g_nfl_initParams {0x40, 0x10, 0x100, 3, 0};
static nflInitParams & nfl_initParams {g_nfl_initParams};

make_var(Struct_94983C, stru_94983C);

make_var(txSlotPool, nfl_filePool);

make_var(nflFile *, dword_98452C);

make_var(nflStream *, dword_984528);

make_var(txSlotPool, nfl_streamPool);

make_var(nflRequest *, dword_98451C);

make_var(char *, dword_984530);

make_var(txSlotPool, nfl_requestPool);

make_var(Struct_984524 *, dword_984524);

make_var(HANDLE, hMutex);

make_var(int, dword_984534);

make_var(HANDLE, hHandle);

make_var(HANDLE, dword_98448C);

make_var(nflStreamID, nfs_defaultStreamID);

make_var(int, nfs_callRequestsCount);

make_var(Struct_984498, dword_984498);

#undef make_var

#endif



nflStreamID sub_79F2C0(nflPriority *a1)
{
    nflStreamID v1 = txSlotNew(&nfl_streamPool);
    nflSetStreamPriority(v1, *a1);
    return v1;
}

int nfsGetNativeFileID(nflFileID a1);

nflFile *nflGetFile(nflFileID a2);

nflFile *sub_79DA00(int a1)
{
    auto v1 = nfsGetNativeFileID(a1);
    return nflGetFile(v1);
}

nflMediaID sub_79DA70(nflFileID a1)
{
    nflMediaID result = NFL_MEDIA_ID_INVALID;
    auto *v1 = sub_79DA00(a1.field_0);
    if ( v1 != nullptr )
    {
        result = v1->field_4;
    }

    return result;
}

auto sub_79DB10(nflStreamID a1)
{
    if ( a1 == 0 ) {
        a1 = nfs_defaultStreamID;
    }

    nflStream *result = nullptr;
    auto v1 = txSlotIndex(&nfl_streamPool, a1.field_0);
    if ( v1 != -1 ) {
        result = &dword_984528[v1];
    }

    return result;
}

//0x0079DB40
nflRequest *nflGetRequest(nflRequestID a1)
{
    nflRequest *result = nullptr;
    auto v1 = txSlotIndex(&nfl_requestPool, a1);
    if ( v1 != -1 ) {
        result = &dword_98451C[v1];
    }
    
    return result;
}

int sub_79DB70(nflRequestState a1) {
    int result;

    switch (a1) {
    case 0:
    case 1:
        result = 4;
        break;
    case 2:
        result = 0;
        break;
    case 3:
    case 4:
        result = 1;
        break;
    case 5:
        result = 2;
        break;
    case 6:
    case 7:
        result = 3;
        break;
    default:
        result = -1;
        break;
    }

    return result;
}

void sub_79EC10(nflRequest *a1, int a2)
{
    if ( a2 != 0 || a1->m_callback != nullptr )
    {
        auto *v3 = &dword_984524[nfs_callRequestsCount++];
        v3->field_0 = (nflRequestState) sub_79DB70(a1->m_state);

        auto *v4 = a1;
        v3->field_8 = v4->totalBufferSize;
        v3->m_callback = CAST(v3->m_callback, v4->m_callback);
        v3->field_C = v4->field_14;
        v3->field_10 = a2;
    }
}

void nflUpdate() {
    TRACE("nflUpdate");

    if constexpr (1)
    {
        nfs_callRequestsCount = 0;
        if (nfl_initParams.field_0[4] == 1) {
            WaitForSingleObject(hMutex, 0xFFFFFFFF);
        }

        int v0 = txSlotFirst(&nfl_requestPool);

        auto *v4 = dword_984524;

        while (v0 != -1) {
            auto *request = nflGetRequest(v0);
            assert(request != nullptr);

            v0 = txSlotNext(&nfl_requestPool, v0);
            uint32_t v3 = request->m_state;
            switch (v3) {
            case NFS_REQUEST_STATE_WAITING:
                txTime();
                break;
            case NFS_REQUEST_STATE_WORKDONE:
            case NFS_REQUEST_STATE_CANCELED: {
                sub_79EC10(request, 1);
                break;
            }
            case NFS_REQUREST_STATE_IO_ERROR: {
                request->m_state = NFS_REQUREST_STATE_IO_ERROR_WAITING;
                sub_79EC10(request, 0);

            } break;
            default:
                break;
            }
        }

        if (nfs_callRequestsCount != 0)
        {
            if (nfl_initParams.field_0[4] == 1) {
                ReleaseMutex(hMutex);
                v4 = CAST(v4, dword_984524);
            }

            if (nfs_callRequestsCount > 0)
            {
                for (int i = 0; i < nfs_callRequestsCount; ++i)
                {
                    auto v12 = v4[i].m_callback;
                    if (v12 != nullptr) {
                        //assert(v12 == &resource_pack_streamer::stream_request_callback);

                        (*v12)(v4[i].field_0,
                               v4[i].field_8,
                               v4[i].field_C); // resource_pack_streamer::stream_request_callback

                    }
                }
            }

            if (nfl_initParams.field_0[4] == 1) {
                WaitForSingleObject(hMutex, 0xFFFFFFFF);
            }
        }

        if (nfs_callRequestsCount > 0)
        {
            for (int i = 0; i < nfs_callRequestsCount; ++i) {
                if (v4[i].field_10) {
                    txSlotFree(&nfl_requestPool, v4[i].field_8.field_0);
                }
            }
        }

        nfs_callRequestsCount = 0;
        if (nfl_initParams.field_0[4] == 1) {
            ReleaseMutex(hMutex);
        }

        if (!nfl_initParams.field_0[4]) {
            sub_79EC60();
        }
    }
    else
    {
        CDECL_CALL(0x0079EE00);
    }
}

nflFileID nflOpenFile(nflMediaID a1, const char *a2) {
    TRACE("nflOpenFile");

    auto result = nflSystem::openFile(a1, a2, NFD_FILE_FLAGS_READ, nullptr);
    return result;
}

int nfsGetNativeFileID(nflFileID a1)
{
    TRACE("nfsGetNativeFileID");

    auto *file = nflGetFile(a1);
    if ( file == nullptr )
    {
        return -1;
    }

    if ( file->fileType != 1 )
    {
        return a1.field_0;
    }

    assert(nflGetFile((int) file->as.subfile.parent));

    assert(nflGetFile((int) file->as.subfile.parent)->fileType == nflFile::NFS_FILE_TYPE_NATIVE);

    return (int) file->as.subfile.parent;
}

HANDLE * nflGetNativeFileID(nflFileID a1)
{
    TRACE("nflGetNativeFileID");

    auto v3 = nfsGetNativeFileID(a1);
    int v4 = txSlotIndex(&nfl_filePool, v3);
    if (v4 == -1) {
        return nullptr;
    }

    HANDLE *result = CAST(result, &dword_984530[v4 * dword_984534]);

    return result;
}

void nflSetStreamPriority(nflStreamID a1, nflPriority a2)
{
    TRACE("nflSetStreamPriority");

    if constexpr (1)
    {
        auto *v4 = sub_79DB10(a1);
        if (v4 != nullptr)
        {
            if (nfl_initParams[4] == 1) {
                WaitForSingleObject(hMutex, 0xFFFFFFFF);
            }

            v4->field_C = a2.field_0;

            int v5 = txSlotFirst(&nfl_requestPool);

            while (v5 != -1)
            {
                auto v6 = v5;
                v5 = txSlotNext(&nfl_requestPool, v5);
                auto *request = nflGetRequest(v6);
                assert(request != nullptr);

                if (request->field_8 == a1.field_0) {
                    request->field_20 = (a2.field_0 << 8) | (request->field_20 & 255);
                }
            }

            if (nfl_initParams[4] == 1) {
                ReleaseMutex(hMutex);
            }
        }
    }
    else
    {
        CDECL_CALL(0x0079E060, a1, a2);
    }
}

void __stdcall sub_79ED80([[maybe_unused]] LPVOID lpThreadParameter) {
    while (1)
    {
        WaitForSingleObject(hHandle, 0xFFFFFFFF);
        if (nfl_initParams[4] == 1) {
            WaitForSingleObject(hMutex, 0xFFFFFFFF);
        }

        int v1 = sub_79EC60();

        if (nfl_initParams[4] == 1) {
            ReleaseMutex(hMutex);
        }

        if (v1) {
            Sleep(30u);
            //SwitchToThread();

            if (nfl_initParams[4] == 1) {
                SetEvent(hHandle);
            }
        }
    }
}

//0x0079DBC0
void *nfsPreAllocate(uint32_t size, uint32_t align, Struct_984498 &dword_984498)
{
    TRACE("nfsPreAllocate");

    assert(size >= 1);
    assert(align >= 1);

    uint32_t v14 = size + align - 1;
    if (dword_984498.field_0 != 0) {
        if (v14 > dword_984498.free) {
            sp_log("Out of %d bytes, used %d, free %d, total %d",
                   v14,
                   dword_984498.used,
                   dword_984498.free,
                   dword_984498.free + dword_984498.used);

            return nullptr;
        }

        dword_984498.free -= v14;
    }

    uint32_t v4 = align * ((dword_984498.used + dword_984498.field_0 + align - 1) / align);

    dword_984498.used += v14;
    return (dword_984498.field_0 != 0 ? (void *) v4 : nullptr);
}

size_t nflSystem::init(void *a1)
{
    if constexpr (1)
    {
        uint32_t v3 = 1;
        dword_984498.field_0 = bit_cast<uint32_t>(a1);
        dword_984498.free = ((a1 != nullptr) ? dword_984498.used : 0);
        dword_984498.used = 0;
        int v40 = 0;
        uint32_t v33 = 1;

        void *v42[32];

        for (int i = 0; i < stru_94983C.field_0; ++i) {
            auto *driver = stru_94983C.field_4[i];
            auto *v6 = driver->field_C;
            if (v33 < v6->field_0) {
                v33 = v6->field_0;
            }

            uint32_t v7 = v6->field_4;

            if (v3 < v7) {
                v3 = v7;
            }

            int v8 = driver->init->field_0;
            if (v8 == 2 || v8 == 3 || v8 == 4) {
                auto *v9 = driver->buffer;
                if (v9->field_10 == nullptr) {
                    uint32_t v10 = v9->field_0;
                    if (v10 < (uint32_t) v9->field_4) {
                        v10 = v9->field_4;
                    }

                    v42[i] = nfsPreAllocate(v9->field_C, v10, dword_984498);
                    v40 |= (v42[i] == nullptr) << i;
                }
            }
        }

        uint32_t v34 = v3 * ((v3 + v33 - 1) / v3);

#if 1
        assert(nfl_initParams[1] == 32 && nfl_initParams[2] == 32);
#endif

        nflRequest *v36 =
            static_cast<decltype(v36)>(nfsPreAllocate(sizeof(nflRequest) * nfl_initParams[2], 64, dword_984498));
        Struct_984524 *v38 = static_cast<decltype(v38)>(nfsPreAllocate(sizeof(Struct_984524) * nfl_initParams[2], 4, dword_984498));
        nflStream *v19 = static_cast<decltype(v19)>(nfsPreAllocate(sizeof(nflStream) * nfl_initParams[1], 64, dword_984498));
        nflFile *v22 =
            static_cast<decltype(v22)>(nfsPreAllocate(sizeof(nflFile) * nfl_initParams[0], 64, dword_984498));

        //sp_log("nfsPreAllocate: size = %d * %d, align = %d, %d", v34, nfl_initParams[0], v3, v2);
        auto *v26 = nfsPreAllocate(v34 * nfl_initParams[0], v3, dword_984498);

        if (a1 != nullptr && v36 != nullptr && v19 != nullptr && v22 != nullptr && v26 != nullptr && v40 == 0) {
            dword_984530 = CAST(dword_984530, v26);
            dword_98451C = v36;
            dword_984524 = v38;
            dword_984534 = v34;
            dword_984528 = v19;
            dword_98452C = v22;

            for (int i = 0; i < stru_94983C.field_0; ++i)
            {
                auto *driver = stru_94983C.field_4[i];
                int v30 = driver->init->field_0;
                if (v30 == 2 || v30 == 3 || v30 == 4) {
                    auto *v31 = driver->buffer;
                    if (v31->field_10 == nullptr) {
                        v31->field_10 = static_cast<char *>(v42[i]);
                    }
                }
            }
        }

        return dword_984498.used;
    }
    else {
        __asm("mov esi, %[_a1]\n" ::[_a1] "m"(a1));

        return CDECL_CALL(0x0079DC10);
    }
}

void nflCancelFileRequests(nflFileID a1) {
    TRACE("nflCancelFileRequests");

    int v1 = txSlotFirst(&nfl_requestPool);
    while (v1 != -1)
    {
        int v2 = v1;
        nflRequest *request = nflGetRequest(v1);
        assert(request != nullptr);

        v1 = txSlotNext(&nfl_requestPool, v1);
        if (request->field_C == a1) {
            nflCancelRequest(v2);
        }
    }
}

namespace nflSystem
{
    void cancelRequest(nflRequestID a1)
    {
        if (nfl_initParams[4] == 1) {
            WaitForSingleObject(hMutex, 0xFFFFFFFF);
        }

        int v1 = txSlotIndex(&nfl_requestPool, a1);
        if (v1 != -1)
        {
            nflRequest *request = &(dword_98451C[v1]);
            if (request != nullptr) {
                int v3 = request->m_state;
                if (v3 == NFS_REQUEST_STATE_WORKING) {
                    request->m_state = NFS_REQUEST_STATE_CANCELING;
                } else if (v3 != NFS_REQUEST_STATE_CANCELING) {
                    request->m_state = NFS_REQUEST_STATE_CANCELED;
                }
            }
        }

        if (nfl_initParams[4] == 1) {
            ReleaseMutex(hMutex);
        }
    }

}

void nflCloseFile(nflFileID a1) {
    TRACE("nflCloseFile");

    nflSystem::closeFile(a1);
}

void nflCancelRequest(nflRequestID a1) {
    TRACE("nflCancelRequest");

    nflSystem::cancelRequest(a1);
}

void nflSystem::closeFile(nflFileID a1) {
    auto *file = nflGetFile(a1);
    if (file != nullptr)
    {
        nflCancelFileRequests(a1);

        auto v4 = file->fileType;
        if (v4 != 0)
        {

            if (v4 == nflFile::NFS_FILE_TYPE_SUBFILE)
            {
                auto *parentFile = nflGetFile((int) file->as.subfile.parent);
                assert(parentFile != nullptr && parentFile->as.subfile.childCount >= 0);

                --parentFile->as.subfile.childCount;
                nflCloseFile((int) file->as.subfile.parent);
            }
            else
            {
                sp_log("Invalid filetype");
            }
        }
        else
        {
            assert(file->as.subfile.childCount >= 0);

            if (file->as.subfile.childCount == 0) {
                nflDriver *v6 = CAST(v6, file->as.subfile.parent);
                HANDLE *v7 = nflGetNativeFileID(a1);
                v6->field_C->field_C(v7);
                txSlotFree(&nfl_filePool, a1.field_0);
            }
        }
    }
}

void nflStart(void *work) {
    TRACE("nflStart");

    if constexpr (1)
    {

        if (work != nullptr)
        {
            nflSystem::init(work);

            assert(txSlotPoolInit(&nfl_requestPool, &dword_98451C->field_38, nfl_initParams[2], sizeof(nflRequest)));
            assert(txSlotPoolInit(&nfl_streamPool, dword_984528, nfl_initParams[1], sizeof(nflStream)));
            assert(txSlotPoolInit(&nfl_filePool,
                           &dword_98452C->field_14,
                           nfl_initParams[0],
                           sizeof(nflFile)));

            nflPriority a1 = 2;
            nfs_defaultStreamID = sub_79F2C0(&a1);
            assert(nfs_defaultStreamID != NFL_STREAM_ID_INVALID);

            if (nfl_initParams[4] == 1)
            {
                hMutex = CreateMutexA(nullptr, 0, nullptr);
                hHandle = CreateEventA(nullptr, 0, 0, nullptr);
                dword_98448C = CreateThread(nullptr,
                                              8192u,
                                              bit_cast<LPTHREAD_START_ROUTINE>(&sub_79ED80),
                                              nullptr,
                                              0,
                                              nullptr);
            }
        }
    }
    else
    {
        CDECL_CALL(0x0079F4A0, work);
    }
}

struct nfdFileInfo
{
    int field_0;
    int m_fileSize;
};

//0x0079FFD0
int nfd_win32_FileStatus(HANDLE *a1, nfdFileInfo *a2) {
    a2->m_fileSize = GetFileSize(*a1, nullptr);
    a2->field_0 = 0;
    return 0;
}

//0x0079FF10
int nfd_win32_FileOpen(HANDLE *a1, LPCSTR lpFileName, uint32_t a3, uint32_t liDistanceToMove)
{
    TRACE("nfd_win32_FileOpen");
    //TRACE((std::string {lpFileName} + " " + std::to_string(a3) + " " + std::to_string(liDistanceToMove)).c_str());

    HANDLE v4 = CreateFileA(lpFileName,
                            ((4 * a3) | (a3 & 2)) << 29,
                            a3 & 3,
                            nullptr,
                            ((~(a3 >> 2)) & 1) | 2,
                            ((~(a3 << 28)) & 0x20000000) | 0x40000000,
                            nullptr);

    if (v4 == INVALID_HANDLE_VALUE) {
        auto errorMessageID = GetLastError();
        LPSTR messageBuffer = nullptr;

        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                             NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        std::string message(messageBuffer, size);

        sp_log("Error opening file %s, %s\n", lpFileName, message.c_str());

        return 1;
    }

    if (a3 & 4)
    {
        LARGE_INTEGER dist;
        dist.LowPart = liDistanceToMove;
        dist.HighPart = 0;
        SetFilePointerEx(v4, dist, nullptr, FILE_BEGIN);
        SetEndOfFile(v4);
        SetFilePointer(v4, 0, nullptr, FILE_BEGIN);
    }

    //sp_log("handle = 0x%08X", v4);

    *a1 = v4;
    return 0;
}

nflRequestParams::nflRequestParams() {
    this->fileID = NFL_FILE_ID_INVALID;
    this->streamID = 0;
    this->m_callback = nullptr;
    this->field_C = NFL_REQUEST_TYPE_UNDEFINED;
    this->field_10 = 2;
    this->field_14 = 0u;
    this->field_18 = nullptr;
    this->dataSize = 0;
    this->field_20 = 0;
    this->field_24 = nullptr;
}

int nflAddRequest(const nflRequestParams *a1) {
    TRACE("nflAddRequest");

    if constexpr (1) {
        int result;

        if (nflGetFile(a1->fileID) != nullptr)
        {
            auto *v5 = sub_79DB10(a1->streamID);
            if (v5 == nullptr) {
                sp_log("requestParams->streamID = %p is invalid or dead\n", a1->streamID);
                return -1;
            }

            if (nfl_initParams.field_0[4] == 1) {
                WaitForSingleObject(hMutex, 0xFFFFFFFF);
            }

            auto v6 = txSlotNew(&nfl_requestPool);

            if (v6 == -1)
            {
                if (nfl_initParams.field_0[4] == 1) {
                    ReleaseMutex(hMutex);
                }

                sp_log("out of request slots. Please increase the request numbers in the nflInitParams "
                       "structure");

                result = -1;
            } else {

                nflRequest *request = nflGetRequest(v6);
                assert(request != nullptr);

                request->field_4 = a1->field_C;
                request->field_C = a1->fileID.field_0;
                request->field_8 = a1->streamID;
                request->field_20 = a1->field_10 | (v5->field_C << 8);
                request->m_callback = a1->m_callback;
                request->field_14 = a1->field_24;
                auto v10 = txTime();
                request->field_18 = v10;
                auto v11 = a1->field_20;
                auto v12 = 0x7FFFFFFF;
                if (v11) {
                    v12 = v10 + v11;
                }

                request->field_1C = v12;
                request->field_30 = v10;
                request->m_state = NFS_REQUEST_STATE_WAITING;
                request->field_24 = CAST(request->field_24, a1->field_18);
                request->field_28 = a1->dataSize;
                request->field_0 = (int) a1->field_14;
                request->bytesCompleted = 0;
                if (nfl_initParams.field_0[4] == 1) {
                    ReleaseMutex(hMutex);
                    if (nfl_initParams.field_0[4] == 1) {
                        SetEvent(hHandle);
                    }
                }

                result = v6;
            }
        }

        return result;
    } else {
        auto result = CDECL_CALL(0x0079E6C0, a1);

        return result;
    }
}

size_t nflInit(const nflInitParams *a1) {
    TRACE("nflInit");

    if (a1 != nullptr) {
        nfl_initParams = *a1;
    }

    for (int i = 0; i < stru_94983C.field_0; ++i) {
        auto *driver = stru_94983C.field_4[i];

        assert(driver != nullptr && driver->init != nullptr);

        if (nfl_initParams[3] != -1) {
            driver->init->field_0 = nfl_initParams[3];
        }

        if (driver->init->field_4 != nullptr) {
            driver->init->field_4(driver);
        }
    }

    return nflSystem::init(nullptr);
}

nflFile * nflGetFile(nflFileID a2)
{
    TRACE("nflGetFile");
    //TRACE(std::to_string(a2.field_0).c_str());

    nflFile *file = nullptr;
    auto v3 = txSlotIndex(&nfl_filePool, a2.field_0);
    if ( v3 != -1 ) {
        file = &dword_98452C[v3];
    }

    return file;
}

int nfd_win32_MediaBind(nflMediaID a1, const char *a2, char *a3, uint32_t a4)
{
    TRACE("nfd_win32_MediaBind");

    if ( a1.field_0 != 1 && a1.field_0 != 2 ) {
        return 4;
    }

    a3[0] = '\0';
    strncat(a3, a2, a4);
    return 0;
}

BOOL nfd_win32_FileClose(HANDLE *a1)
{
    return !CloseHandle(*a1);
}

int nfd_win32_Sleep()
{
    SleepEx(0, 1);
    return 0;
}

BOOL nfd_win32_IoCancel(HANDLE *a1)
{
    return CancelIo(*a1) == 0;
}

int nfd_win32_FileHandle(HANDLE *a1, void **a2, int *a3)
{
    *a2 = a1;
    *a3 = 4;
    return 0;
}

nflDriver::nflDriver()
{
    field_0 = "PC(Win32)";

    {
        using init_t = std::remove_pointer_t<decltype(init)>;
        static init_t v0 {2, nullptr};
        init = &v0;
    }

    {
        using type = std::remove_pointer_t<decltype(field_8)>;
        static type v1 {3, nfd_win32_MediaBind};
        field_8 = &v1;
    }

    {
        using type = std::remove_pointer_t<decltype(field_C)>;
        static type v1 {4, 4, nfd_win32_FileOpen, nfd_win32_FileClose, nfd_win32_FileStatus, nfd_win32_FileHandle};
        field_C = &v1;
    }

    {
        using type = std::remove_pointer_t<decltype(field_10)>;
        static type v1 {nfd_win32_IoExecute, nfd_win32_IoCancel, nfd_win32_Sleep};
        field_10 = &v1;
    }

    {
        using type = std::remove_pointer_t<decltype(buffer)>;
        static type v1 {4, 0x1000, 0x800, 0x400000, nullptr};
        buffer = &v1;
    }
    
}

//0x0079D940
int nfsGetMediaIndex(int a1)
{
    for ( auto i = 0; i < stru_94983C.field_0; ++i )
    {
        if ( stru_94983C.field_4[i] != nullptr
            && stru_94983C.field_4[i]->field_8 != nullptr
            && (a1 & stru_94983C.field_4[i]->field_8->field_0) != 0 )
        {
            return i;
        }
    }

    return -1;
}

//0x0079D970
nflDriver *nfsGetMediaDriver(int a1)
{
    TRACE("nfsGetMediaDriver");

    nflDriver *result = nullptr;
    auto v1 = nfsGetMediaIndex(a1);
    if ( v1 != -1 )
    {
        assert(v1 == 0);
        result = stru_94983C.field_4[v1];
    }

    return result;
}

namespace nflSystem {
    std::unordered_map<std::string, std::filesystem::path> ModFileOverrides;
}

nflFileID nflSystem::openFile(nflMediaID a1, const char *a2, nfdFileFlags Flags, uint32_t *p_fileSize)
{
    //sp_log("Open file %s", a2);

    // ---------------------------------------------------------------------
    // Mods: low-level file override
    // ---------------------------------------------------------------------
    // If a file exists in mods/ with the same relative path (case-insensitive),
    // redirect the open request to that file on disk.
    //
    // Examples:
    //   Engine asks:  "data\\sound\\ui\\click.wbk"
    //   Mod provides: "mods\\data\\sound\\ui\\click.wbk"
    //
    // NOTE: This does *not* magically make NSL accept WAV in place of WBK.
    // It simply allows you to override *whatever* file the engine is trying
    // to open (including .WBK wavebanks) and also lets your own code open
    // mod WAV files via nflOpenFile("mods\\...\\file.wav").
{
    auto normalize = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c){ return (char)std::tolower(c); });
        std::replace(s.begin(), s.end(), '/', '\\');
        while (s.rfind(".\\", 0) == 0) s.erase(0, 2);
        return s;
    };

    const std::string req0 = normalize(a2 ? std::string(a2) : std::string());
    std::string req1 = req0;
    if (req1.rfind("data\\", 0) == 0) req1.erase(0, 5);

    auto it = nflSystem::ModFileOverrides.find(req0);
    if (it == nflSystem::ModFileOverrides.end())
        it = nflSystem::ModFileOverrides.find(req1);

    if (it != nflSystem::ModFileOverrides.end()) {
        thread_local std::string s_override;
        s_override = it->second.string();
        a2 = s_override.c_str();
    }
}
   

    if constexpr (1)
    {
        char v22[256] {};

        for (auto i = 0u; i < 32u; ++i)
        {
            uint32_t v7 = a1.field_0 & (1 << i);

            if ((v7 != 0))
            {
                auto func = [](int a1) -> const char * {
                    static char byte_15FE890[64];

                    const char *a2[] = {"DISC:", "HOST:", "LINK:"};

                    byte_15FE890[0] = '\0';
                    for ( auto i = 0u; i < std::size(a2) && a1 != 0; ++i )
                    {
                        if ( (a1 & (1 << i)) != 0 )
                            strncat(byte_15FE890, a2[i], 32);

                        a1 &= ~(1 << i);
                    }

                    if ( a1 != 0 )
                    {
                        strncat(byte_15FE890, "UNKN:", 32);
                    }

                    return byte_15FE890;
                };

                auto *v11 = nfsGetMediaDriver(v7);
                if ( v11 == nullptr )
                {
                    auto *v4 = func(v7);
                    sp_log("%s not registered with any of the file drivers", v4);
                    continue;
                }

                if (
#if 0
                        v11->field_8->fnBind(v7, a2, v22, 256u)
#else
                        nfd_win32_MediaBind(v7, a2, v22, 256u)
#endif
                    != 0 )
                {
                    auto *v5 = func(v7);
                    sp_log("fnBind failed: %s %s\n", v5, a2);
                    continue;
                }

                {
                    if (nfl_initParams.field_0[4] == 1) {
                        WaitForSingleObject(hMutex, 0xffffffff);
                    }

                    nflFileID v5 = txSlotNew(&nfl_filePool);
                    if (v5 == NFL_FILE_ID_INVALID) {
                        if (nfl_initParams.field_0[4] == 1) {
                            ReleaseMutex(hMutex);
                        }

                        sp_log("Out of file handles");
                        
                        return NFL_FILE_ID_INVALID;
                    }

                    auto *file = nflGetFile(v5);
                    assert(file != nullptr);

                    file->fileType = nflFile::NFS_FILE_TYPE_NATIVE;
                    file->as.subfile.childCount = 0;
                    file->field_4 = v7;
                    file->as.subfile.field_4 = Flags;
                    file->as.subfile.parent = bit_cast<int *>(v11);

                    HANDLE *fileHandle = nflGetNativeFileID(v5);
                    assert(fileHandle != nullptr);

                    const uint32_t fileSize =
                            (p_fileSize != nullptr ? (*p_fileSize) : 0u);

                    nfdFileInfo fileInfo;
                    if (
#if 0
                        v11->field_C->field_8(fileHandle, v22, Flags, fileSize)
#else
                        nfd_win32_FileOpen(fileHandle, v22, Flags, fileSize)
#endif
                            != 0 || 
                        
#if 0
                        v11->field_C->field_10(fileHandle, &fileInfo)
#else
                            nfd_win32_FileStatus(fileHandle, &fileInfo)
#endif

                            != 0)
                    {

                        if (nfl_initParams.field_0[4] == 1) {
                            ReleaseMutex(hMutex);
                        }

                        txSlotFree(&nfl_filePool, v5.field_0);
                        auto *flags = [](int a1) -> const char *
                        {
                            static char byte_15FE810[258];

                            strcpy(byte_15FE810, "NFD_FILE_FLAGS");
                            if ( (a1 & 1) != 0 )
                                strcat(byte_15FE810, "_READ");

                            auto a1a = a1 & 0xFFFFFFFE;
                            if ( (a1a & 2) != 0 )
                                strcat(byte_15FE810, "_WRITE");
                        
                            auto a1b = a1a & 0xFFFFFFFD;
                            if ( (a1b & 4) != 0 )
                                 strcat(byte_15FE810, "_CREATE");

                            if ( (a1b & 0xFFFFFFFB) != 0 )
                                strcat(byte_15FE810, "_???");

                            return byte_15FE810;
                        }(Flags);

                        auto *v8 = func(v7);

                        sp_log("nflOpenFile: failed, %s:%s -> %s, flags=%s", v8, a2, v22, flags);
                        continue;
                    }

                    if (p_fileSize != nullptr) {
                        *p_fileSize = fileInfo.m_fileSize;
                    }

                    file->m_fileSize = fileInfo.m_fileSize;

                    if (nfl_initParams.field_0[4] == 1) {
                        ReleaseMutex(hMutex);
                    }

                    return v5;

                }
            }
        }

        return NFL_FILE_ID_INVALID;
    }
    else
    {
        return (nflFileID) CDECL_CALL(0x0079E180, a1, a2, Flags, p_fileSize);
    }
}

const char *enum_to_char(nflIoState a1);

int nflUpdateDriver(nflDriver *driver) {
    VALIDATE_SIZE(driver->field_24, 0x1C);

    if (driver->field_40 == 0) {
        return 1;
    }

    auto *v1 = driver->field_10->field_8;
    if (v1 != nullptr && v1()) {
        return 0;
    }

    auto *request = nflGetRequest(driver->field_24.field_C);
    if (request != nullptr) {
        switch (driver->field_40) {
        case 1: {
            if (request->m_state == 3) {
                auto *v7 = driver->field_10->field_4;
                if (v7 != nullptr && v7(driver->field_24.fileHandle)) {
                    return 0;
                }

                driver->field_40 = NFD_IO_STATE_CANCELING;
            } else if (request->m_state != NFS_REQUEST_STATE_WORKING) {
                assert(0);
            }

            return 0;
        }
        case 2: {
            if (request->m_state == NFS_REQUEST_STATE_CANCELING) {
                request->m_state = NFS_REQUEST_STATE_CANCELED;
            }
            else
            {
                assert(request->m_state == NFS_REQUEST_STATE_WORKING);

                int v5 = driver->field_1C;
                if (v5) {
                    request->bytesCompleted += v5;
                    request->m_state = (request->bytesCompleted < request->field_28)
                        ? NFS_REQUEST_STATE_WAITING
                        : NFS_REQUEST_STATE_WORKDONE;
                } else {
                    request->m_state = NFS_REQUEST_STATE_WORKDONE;
                }
            }

        } break;
        case 4:
            assert(request->m_state == NFS_REQUEST_STATE_CANCELING);
            request->m_state = NFS_REQUEST_STATE_CANCELED;
            break;
        case 5:
            if (request->m_state != NFS_REQUREST_STATE_IO_ERROR_WAITING) {
                request->m_state = NFS_REQUREST_STATE_IO_ERROR;
            }

            return 0;
        default:
            auto *v2 = enum_to_char(driver->field_40);
            sp_log("invalid state %s", v2);
            return 0;
        }
    }

    driver->field_40 = NFD_IO_STATE_IDLE;
    driver->field_24.field_10 = nullptr;
    driver->field_24.field_14 = 0;
    driver->field_24.fileHandle = nullptr;
    driver->field_24.field_4 = 0;
    driver->field_24.field_C = -1;
    driver->field_24.field_18 = NFL_REQUEST_TYPE_UNDEFINED;
    return 1;
}

const char *enum_to_char(nflIoState a1) {
    const char *result;

    switch (a1) {
    case 0:
        result = "NFD_IO_STATE_IDLE";
        break;
    case 1:
        result = "NFD_IO_STATE_WORKING";
        break;
    case 2:
        result = "NFD_IO_STATE_WORKDONE";
        break;
    case 3:
        result = "NFD_IO_STATE_CANCELING";
        break;
    case 4:
        result = "NFD_IO_STATE_CANCELED";
        break;
    case 5:
        result = "NFD_IO_STATE_ERROR";
        break;
    default:
        result = "NFD_IO_STATE_???";
        break;
    }

    return result;
}

nflRequestInfo *nflGetRequestInfo(nflRequestID a1, nflRequestInfo *a2) {
    TRACE("nflGetRequestInfo");

    if constexpr (1) {
        auto *v3 = nflGetRequest(a1);
        if (v3 != nullptr) {
            a2->field_0 = v3->bytesCompleted;
            a2->field_4 = txTime() - v3->field_18;
        }

        return a2;
    } else {
        return (nflRequestInfo *) CDECL_CALL(0x0079E5F0, a1, a2);
    }
}

int nflReadFileAsync(nflFileID a1, uint32_t a2, void *a3, uint32_t a4) {
    TRACE("nflReadFileAsync");

    int result;

    if constexpr (0)
    {
        assert(a1.field_0 != -1);

        nflRequestParams requestParams;

        requestParams.field_C = NFL_REQUEST_TYPE_READ;
        requestParams.streamID = 0;
        requestParams.field_20 = 0;
        requestParams.m_callback = nullptr;
        requestParams.field_24 = nullptr;
        requestParams.fileID = a1;
        requestParams.field_14 = a2;
        requestParams.field_10 = 2;
        requestParams.field_18 = static_cast<uint8_t *>(a3);
        requestParams.dataSize = a4;
        return nflAddRequest(&requestParams);
    }
    else
    {
        result = CDECL_CALL(0x0079D720, a1, a2, a3, a4);
    }

    //sp_log("end: %p %d", a3, result);

    return result;
}

int nflGetRequestState(nflRequestID a1) {
    TRACE("nflGetRequestState"); 

    int result = -1;

    if constexpr (1) {
        auto *v3 = nflGetRequest(a1);
        if (v3 != nullptr) {
            result = sub_79DB70(v3->m_state);
        }

    } else {
        result = CDECL_CALL(0x0079E500, a1);
    }

    return result;
}

int nflExecuteRequest(nflDriver *driver, nflRequestID a2)
{
    TRACE("nflExecuteRequest");
    //TRACE(std::to_string(a2.field_0).c_str());
    if (driver->field_40)
    {
        sp_log("nflExecuteRequest: called a busy state %s", enum_to_char(driver->field_40));
        return 5;
    }

    auto *request = nflGetRequest(a2);
    assert(request != nullptr);

    nflFile *file = nflGetFile(request->field_C);
    assert(file != nullptr);

    int v8 = request->bytesCompleted;

    nflCommand ioCommand;
    ioCommand.field_18 = request->field_4;
    ioCommand.field_10 = &request->field_24[v8];
    ioCommand.field_14 = request->field_28 - v8;
    ioCommand.field_4 = v8 + request->field_0;
    ioCommand.field_C = a2;
    ioCommand.fileHandle = nflGetNativeFileID(request->field_C);
    assert(ioCommand.fileHandle != nullptr);

    ioCommand.field_0 = file->m_fileSize;
    if (file->fileType == nflFile::NFS_FILE_TYPE_SUBFILE)
    {
        txSlotIndex(&nfl_filePool, (int) file->as.subfile.parent);
        int v9 = file->as.subfile.field_4;
        int v10 = file->as.subfile.childCount;
        if (!v9 && !v10) {
            v10 = 0x40000000;
            v9 = 0x40000000;
        }

        int v11 = v8 / v9;
        int v12 = v8 % v9;
        int v13 = v9 - v8 % v9;
        if (ioCommand.field_14 > v13) {
            ioCommand.field_14 = v13;
        }

        ioCommand.field_4 = v12 + file->field_4 + v10 * v11;
        ioCommand.fileHandle = nflGetNativeFileID((int) file->as.subfile.parent);
    }

    request->m_state = NFS_REQUEST_STATE_WORKING;
    int result = nflIoExecute(driver, &ioCommand);
    if (result != 0) {
        request->m_state = NFS_REQUREST_STATE_IO_ERROR;
    }

    return result;
}

HANDLE *nflGetFileHandle(nflFileID a1, int *a2, int *a3, int *p_fileSize, int *a5)
{
    auto *f = nflGetFile(a1);
    if ( f == nullptr )
    {
        return nullptr;
    }

    auto v12 = 0;
    auto v10 = 0;
    auto fileSize = f->m_fileSize;
    if ( f->fileType == 1 )
    {
        v12 = f->field_4;
        v10 = f->as.subfile.childCount;
        a1 = (int) f->as.subfile.parent;
        f = nflGetFile(a1);
        assert(f != nullptr);

        if ( f == nullptr )
        {
            return nullptr;
        }
    }

    HANDLE *v7 = nullptr;
    int v9 = 0;
    auto *v8 = nflGetNativeFileID(a1);
    nflDriver *driver = nullptr;
    if ( v8 == nullptr
    || (driver = CAST(driver, f->as.subfile.parent)) == nullptr
    || driver->field_C == nullptr
    || driver->field_C->field_14 == nullptr
    || driver->field_C->field_14(v8, (void **)&v7, &v9) )
    {
        return nullptr;
    }

    if ( a2 )
        *a2 = v9;

    if ( a3 )
        *a3 = v12;
    
    if ( p_fileSize != nullptr)
        *p_fileSize = fileSize;
    
    if ( a5 )
        *a5 = v10;

    return v7;
}

int nflScheduleRequest(int a1) {
    if constexpr (1)
    {
        int v14 = -1;
        uint32_t v16 = 0;
        int v15 = -1;
        auto v1 = txSlotFirst(&nfl_requestPool);
        while (v1 != -1) {
            auto v2 = v1;
            nflRequest *request = nflGetRequest(v1);
            assert(request != nullptr);

            v1 = txSlotNext(&nfl_requestPool, v1);

            auto func = [](nflFileID id) -> int
            {
                auto *file = [](nflFileID a1) ->nflFile * {
                    nflFileID v1 = nfsGetNativeFileID(a1);
                    return nflGetFile(v1);
                }(id);

                if (file != nullptr)
                {
                    return (int) file->as.subfile.parent;
                }

                return 0;

            };

            if (request->m_state == 0 && (a1 == 0 || a1 == func(request->field_C))) {

                if (request->field_20 <= v16) {
                    if (request->field_20 == v16 && request->field_30 < v14) {
                        v15 = v2;
                        v14 = request->field_30;
                    }

                } else {

                    v15 = v2;
                    v16 = request->field_20;
                    v14 = request->field_30;
                }
            }


        }

        return v15;

    } else {
        return CDECL_CALL(0x0079E9B0, a1);
    }
}

int sub_79EC60()
{
    if constexpr (1)
    {
        const int v0 = stru_94983C.field_0;
        auto **v1 = stru_94983C.field_4;

        int v6[32];
        for (int i = 0; i < v0; ++i) {
            v6[i] = nflUpdateDriver(v1[i]);
        }

        int v9[32];
        for (int i = 0; i < v0; ++i) {
            int v3;
            if (v6[i] != 0) {
                v3 = (v0 == 1) ? nflScheduleRequest(0) : nflScheduleRequest((int) v1[i]);
            } else {
                v3 = -1;
            }

            v9[i] = v3;
        }

        for (int i = 0; i < v0; ++i) {

            if (v9[i] != -1) {
                nflExecuteRequest(v1[i], v9[i]);
            }
        }

        int result = 0;
        for (int i = 0; i < v0; ++i) {
            if (v1[i]->field_40) {
                result = 1;
            }
        }

        return result;
    } else {
        auto result = CDECL_CALL(0x0079EC60);

        return result;
    }
}

void nfl_system_patch()
{
    SET_JUMP(0x0079E5F0, nflGetRequestInfo);

    SET_JUMP(0x0079E500, nflGetRequestState);

    SET_JUMP(0x0079E490, nflOpenFile);

    SET_JUMP(0x0079DA70, sub_79DA70);

    SET_JUMP(0x0079F070, nflGetFileHandle);

    SET_JUMP(0x0079E9B0, nflScheduleRequest);

    SET_JUMP(0x0079EC60, sub_79EC60);

    SET_JUMP(0x0079EE00, nflUpdate);

    SET_JUMP(0x0079F3F0, nflCloseFile);

    SET_JUMP(0x0079E6C0, nflAddRequest);

    SET_JUMP(0x0079DF60, nflInit);

    SET_JUMP(0x0079F4A0, nflStart);

    REDIRECT(0x0079B028, nflReadFileAsync);


    REDIRECT(0x0079EBEB, nflIoExecute);
}
