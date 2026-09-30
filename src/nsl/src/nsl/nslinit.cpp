#include "nslinit.h"

#include "func_wrapper.h"
#include "nslbank.h"
#include "tx_system.h"

#include <variables.h>

int nsdInit()
{
    if constexpr (0)
    {
        DSBUFFERDESC v2 {};
        memset(&v2.dwBufferBytes, 0, 12);
        v2.dwSize = 36;
        v2.dwFlags = 17;
        IDirectSound8_CreateSoundBuffer(g_directSound, &v2, &dword_982570, 0);

        auto & stru_8C0AEC = var<IID>(0x008C0AEC);
        IDirectSoundBuffer_QueryInterface(dword_982570, stru_8C0AEC, &dword_982574);

        (*(void (__stdcall **)(void *, int, int))(*(DWORD *)dword_982574 + 44))(dword_982574, 0x3F800000, 1);
        IDirectSound8_SetSpeakerConfig(g_directSound, 4);
        operator delete((void *)0x480000);

        int v0 = 0;
        CDECL_CALL(0x0079B6B0, v0, 0x480000);
        CDECL_CALL(0x0079C150, 1, 2, 6, 36, 6, 36);
        return 1;
    }
    else
    {
        return CDECL_CALL(0x0079CE20);
    }
}

void nslStart(void *a1)
{
    auto & nsl_work = var<void *>(0x0097DB0C);
    if ( nsl_work == nullptr )
    {
        auto & nsl_workLimit = var<int>(0x0097DB14);
        auto & nsl_workUsed = var<int>(0x0097DB10);

        nsl_workLimit = nsl_workUsed;
        nsl_workUsed = 0;
        nsl_work = a1;
        if ( CDECL_CALL(0x0079A610, nullptr) >= 0 )
        {
            memcpy(nsl_workGuard, "NSL_WORK_GUARD0", 16);
            memcpy(nsl_workGuard1, "NSL_WORK_GUARD1", 16);

            auto & dword_97DAD4 = var<void *>(0x0097DAD4);
            auto & dword_97DB04 = var<void *>(0x0097DB04);

            int & dword_94735C = var<int>(0x0094735C);
            int & dword_947360 = var<int>(0x00947360);

            txSlotPoolInit(&nsl_sourcePool, dword_97DAD4, dword_94735C, 0xCu);
            txSlotPoolInit(&nsl_emitterPool, dword_97DB04, dword_947360, 0xCu);
            float *Master = nsl_GetMaster();
            float *Listener = nsl_GetListener();
            Master[6] = 0.0;
            Master[7] = 0.0;
            Master[2] = 1.0;
            Master[3] = 1.0;
            Listener[2] = 1.0;
            Listener[3] = 1.0;
            Listener[6] = 0.0;
            Listener[7] = 0.0;
            Listener[21] = 0.0;
            Listener[22] = 0.0;
            Listener[23] = 0.0;
            Listener[48] = 0.0;
            Listener[49] = 0.0;
            Listener[50] = 1.0;
            Listener[51] = 0.0;
            Listener[52] = 1.0;
            Listener[53] = 0.0;
            nsdInit();
            nslUpdate();
        }
    }
}

