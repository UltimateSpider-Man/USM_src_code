#pragma once

#include <config.h>

#include <windows.h>

#include "win32/nfl_win32_driver.h"

#define PC_VERSION 1 

#pragma once
#include <filesystem>
#include <unordered_map>
#include <string>

struct resource_pack_header;
struct resource_pack_streamer;

struct nflRequestInfo {
    int field_0;
    int field_4;
};


struct nflFileID {
    int field_0;

    nflFileID() = default;

    constexpr nflFileID(int a1) : field_0(a1) {}

    bool operator==(const nflFileID &id) const
    {
        return field_0 == id.field_0;
    }

    bool operator!=(const nflFileID &id) const
    {
        return !(field_0 == id.field_0);
    }
};

enum nfdFileFlags
{
    NFD_FILE_FLAGS_READ = 0x1,
    NFD_FILE_FLAGS_WRITE = 0x2,
    NFD_FILE_FLAGS_CREATE = 0x4,
    NFD_FILE_FLAGS_UNDEFINED
};

inline constexpr nflFileID NFL_FILE_ID_INVALID {-1};

struct nflRequestID {
    int field_0;

    nflRequestID() = default;

    nflRequestID(int a1) : field_0(a1) {}

    operator int() const {
        return field_0;
    }
};

static inline constexpr auto NFL_REQUEST_ID_INVALID = -1;

enum nflRequestState
{
    NFS_REQUEST_STATE_WAITING = 0,
    NFS_REQUEST_STATE_WORKING = 1,
    NFS_REQUEST_STATE_WORKDONE = 2,
    NFS_REQUEST_STATE_CANCELING = 3,
    NFS_REQUEST_STATE_CANCELED = 4,
    NFS_REQUREST_STATE_TIMEOUT = 5,
    NFS_REQUREST_STATE_IO_ERROR = 6,
    NFS_REQUREST_STATE_IO_ERROR_WAITING = 7,
};

struct nflRequest {
    int field_0;

    nflRequestType field_4;
    int field_8;
    nflFileID field_C;
    void (*m_callback)(nflRequestState, nflRequestID, resource_pack_streamer *);
    resource_pack_streamer *field_14;
    int field_18;
    int field_1C;
    uint32_t field_20;
    char *field_24;
    int field_28;
    nflRequestState m_state;
    int field_30;
    int bytesCompleted;
    int field_38;
    int field_3C;
    int totalBufferSize;
};

struct nflRequestParams
{
    nflFileID fileID;
    int streamID;
    void (*m_callback)(nflRequestState, nflRequestID, resource_pack_streamer *);
    nflRequestType field_C;
    int field_10;
    uint32_t field_14;
    uint8_t *field_18;
    int dataSize;
    int field_20;
    resource_pack_streamer *field_24;

    nflRequestParams();
};

//0x0079EE00
void nflUpdate();

struct nflMediaID {
    int field_0;

    nflMediaID() {
        field_0 = 0;
    }

    nflMediaID(int a1) {
        field_0 = a1;
    }
};

extern inline const nflMediaID NFL_MEDIA_ID_INVALID {-1};

struct nflCommand {
    int field_0;
    int field_4;
    HANDLE *fileHandle;
    int field_C;
    char *field_10;
    int field_14;
    nflRequestType field_18;
};

enum nflIoState {
    NFD_IO_STATE_IDLE = 0,
    NFD_IO_STATE_WORKING = 1,
    NFD_IO_STATE_WORKDONE = 2,
    NFD_IO_STATE_CANCELING = 3,
    NFD_IO_STATE_CANCELED = 4,
    NFD_IO_STATE_ERROR = 5
};

struct nfdFileInfo;

struct nflDriver {
    const char *field_0;
    struct {
        int field_0;
        void (*field_4)(nflDriver *);
    } *init;

    struct {
        int field_0;
        int (*fnBind)(nflMediaID, const char *, char *, uint32_t);

    } *field_8;

    struct {
        unsigned int field_0;
        int field_4;
        int (*field_8)(HANDLE *a1, LPCSTR lpFileName, uint32_t a3, uint32_t liDistanceToMove);
        BOOL (*field_C)(HANDLE *);
        int (*field_10)(HANDLE *, nfdFileInfo *);
        int (*field_14)(HANDLE *, void **, int *);
    } *field_C;

    struct {
        BOOL(*field_0)(nflDriver *a1,
         HANDLE *a2,
         nflRequestType a3,
         uint32_t a4,
         LPVOID lpBuffer,
         uint32_t nNumberOfBytesToWrite);
        BOOL (*field_4)(HANDLE *a1);

        int (*field_8)();
    } *field_10;

    struct {
        int field_0;
        int field_4;
        int field_8;
        int field_C;
        char *field_10;
    } *buffer;

    int field_18;
    int field_1C;
    int field_20;
    nflCommand field_24;
    nflIoState field_40;
    int field_44;

    nflDriver();
};

struct nflInitParams {
    int field_0[5];

    nflInitParams() = default;
    nflInitParams(const nflInitParams &) = default;

    int &operator[](int idx) {
        return field_0[idx];
    }

    const int &operator[](int idx) const {
        return field_0[idx];
    }
};

namespace nflSystem {
//0x0079E180
nflFileID openFile(nflMediaID a1, const char *a2, nfdFileFlags a3, uint32_t *a4);
    extern std::unordered_map<std::string, std::filesystem::path> ModFileOverrides;

void closeFile(nflFileID a1);

void cancelRequest(nflRequestID a1);

size_t init(void *);

} // namespace nflSystem

//0x0079F380
void nflCancelFileRequests(int a1);

//0x0079F3F0
void nflCloseFile(nflFileID a1);

//0x0079E490
nflFileID nflOpenFile(nflMediaID a1, const char *a2);

int sub_79FFD0(HANDLE *a1, uint32_t *a2);

signed int sub_79FF10(HANDLE *a1, LPCSTR lpFileName, uint32_t a3, uint32_t liDistanceToMove);

//0x0079F4A0
extern void nflStart(void *a1);

struct nflStreamID
{
    int field_0;

    nflStreamID() = default;

    constexpr nflStreamID(int p) : field_0(p) {}

    bool operator==(const nflStreamID &id) const
    {
        return field_0 == id.field_0;
    } 

    bool operator!=(const nflStreamID &id) const
    {
        return field_0 != id.field_0;
    } 
};


static constexpr nflStreamID NFL_STREAM_ID_INVALID {-1};

struct nflPriority
{
    int field_0;

    nflPriority() = default;

    nflPriority(int p) : field_0(p) {}
};

//0x0079E060
void nflSetStreamPriority(nflStreamID a1, nflPriority a2);

//0x0079DF60
extern size_t nflInit(const nflInitParams *a1);

//0x0079E6C0
extern int nflAddRequest(const nflRequestParams *a1);

//0x0079DAB0
extern HANDLE *nflGetNativeFileID(nflFileID a1);

//0x0079E8B0
extern int nflUpdateDriver(nflDriver *driver);

//0x0079D720
extern int nflReadFileAsync(nflFileID a1, uint32_t a2, void *a3, uint32_t a4);

//0x0079E500
extern int nflGetRequestState(nflRequestID a1);

//0x0079E5F0
extern nflRequestInfo *nflGetRequestInfo(nflRequestID a1, nflRequestInfo *a2);

//0x0079EAE0
extern int nflExecuteRequest(nflDriver *driver, nflRequestID a2);

extern int sub_79EC60();

//0x0079E9B0
extern int nflScheduleRequest(int a1);

extern void nfl_system_patch();

//0x0079E830
extern void nflCancelRequest(nflRequestID a1);


