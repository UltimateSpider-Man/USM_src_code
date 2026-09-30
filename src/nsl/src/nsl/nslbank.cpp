#include "nslbank.h"

#include "func_wrapper.h"
#include "log.h"
#include "common.h"
#include "nslbankload.h"
#include "tl_system.h"
#include "trace.h"

VALIDATE_SIZE(nslBank, 0x34);

Var<nslBank[1]> nsl_banks {0x00946FD0};

nslBank * nslGetBank(nslBankID a1);

void nslAramFree(unsigned int a1)
{
    CDECL_CALL(0x0079B710, a1);
}

const char *to_string(nslBankID a1)
{
    auto *v3 = nslGetBank(a1);
    const char *v2 = nullptr;
    if ( v3 != nullptr && v3->field_18 != nullptr )
    {
        v2 = v3->field_18->field_20;
    }
    
    return v2;
}

void nslFreeBank(nslBankID a1)
{
    TRACE("nslFreeBank");
    if constexpr (0)
    {
        auto *nsl_bank = nslGetBank(a1);
        if (nsl_bank == nullptr)
        {
            sp_log("Freeing of invalid bank %p has been ignored.\n", a1);
            return;
        }

        switch ( nsl_bank->field_4 )
        {
            case 1:
            {
                auto *v1 = to_string(a1);
                sp_log("Freeing pending bank %p (%s)\n", a1, v1); 
                break;
            }
            case 2:
            {
                auto *v2 = to_string(a1);
                sp_log("Begin canceling bank loading %p (%s)\n", a1, v2);
                if ( nsl_bankLoad().field_0 != -1 && nsl_bankLoad().field_0 != 256 )
                {
                    if ( nsl_bankLoad().rid != -1 )
                    {
                        nflCancelRequest(nsl_bankLoad().rid);
                    }

                    nsl_bankLoad().field_0 = 256;
                }

                break;
            }
            case 3:
            {
                auto *v3 = to_string(a1);
                sp_log("Freeing loaded bank %p (%s)\n", a1, v3);
                auto v4 = nsl_bank->field_18;
                if ( v4 )
                {
                    nslAramFree(v4->field_88);
                    v4->field_88 = 0;

                    tlMemFree(nsl_bank->field_18);
                    nsl_bank->field_18 = nullptr;
                }
                
                break;
            }
            default:
                break;
        }

        nsl_bank->field_0 = (nslBankID) (nsl_bank->field_0 + 0x200000);
        nsl_bank->field_4 = 0;
    }
    else
    {
        CDECL_CALL(0x00798550, a1);
    }
}

nslBank *nslGetBank(nslBankID a1)
{
    [[maybe_unused]] auto HIWORD = [](int a2) {
        return (a2 >> 16);    
    };

    const auto idx = HIWORD(a1) % 32u;
    //sp_log("idx = %d", idx);

    return (a1 == nsl_banks()[idx].field_0 ? &nsl_banks()[idx] : nullptr);
}

int nslGetBankState(nslBankID a2)
{
    TRACE("nslGetBankState");
    // The native PC bank table uses a 0x1C-byte stride (32 entries), while
    // the reconstructed nslBank declaration below the source API is 0x34.
    // Indexing that declaration reads a different bank's state for every
    // slot after zero, making readiness/failure decisions incorrect.
    // Keep the verified native accessor until the full ABI is reconstructed.
    return CDECL_CALL(0x007984D0, a2);
}

int nslBank_WaveCompare(const void *a1, const void *a2)
{
    return CDECL_CALL(0x00798120, a1, a2);
}

void nslUpdateBanks()
{
    TRACE("nslUpdateBanks");

    if constexpr (0)
    {
        auto find_idx = -1;
        uint32_t v1 = -1;

        for (auto i = 0; i < 32; ++i)
        {
            auto *nsl_bank = &nsl_banks()[i];
            auto v4 = nsl_bank->field_4;
            if ( v4 != 0 && v4 != 3 )
            {
                if ( v4 == 1 )
                {
                    if ( v1 > nsl_bank->field_14 )
                    {
                        v1 = nsl_bank->field_14;
                        find_idx = i;
                    }
                }
                else if (nsl_bank->field_4 == 2)
                {
                    auto v5 = nslBankLoad_Update(&nsl_bankLoad());
                    if ( v5 == 1 )
                    {
                        auto *v6 = nsl_bankLoad().field_18;
                        nsl_bank->field_4 = 3;
                        qsort(v6->field_44, v6->field_40, 0x28u, nslBank_WaveCompare);
                    }
                    else if ( v5 == -1 )
                    {
                        sp_log("Canceled loading of bank %p\n", nsl_bank->field_0);
                        nsl_bank->field_4 = 3;
                        nslFreeBank(nsl_bank->field_0);
                    }
                    else
                    {
                        if ( v5 >= 0 )
                        {
                            continue;
                        }

                        sp_log("Failure %d loading bank %p\n", v5, nsl_bank->field_0);

                        nsl_bank->field_4 = 3;
                        nslFreeBank(nsl_bank->field_0);
                    }

                    nsl_bankLoad().field_0 = 0;
                }
            }
        }

        if ( find_idx != -1 && nsl_bankLoad().field_0 == 0 )
        {
            auto *v7 = &nsl_banks()[find_idx];
            auto v11 = v7->field_10;
            auto v9 = v7->field_C;
            auto v8 = v7->field_8;
            v7->field_4 = 2;
            nslBankLoad_Init(&nsl_bankLoad(), v8, v9, v11);
            nslBankLoad_Update(&nsl_bankLoad());
        }
    }
    else
    {
        CDECL_CALL(0x00798900);
    }
}

void nslUpdate()
{
    TRACE("nslUpdate");

    CDECL_CALL(0x0079A770);
}

void nsl_patch()
{
    {
        REDIRECT(0x0054D4F2, nslUpdate);
        REDIRECT(0x0055234C, nslUpdate);
    }

    REDIRECT(0x0079A7EC, nslUpdateBanks);

    {
        REDIRECT(0x0079893C, nslBankLoad_Update);
        REDIRECT(0x007989E7, nslBankLoad_Update);
    }

    return;

    REDIRECT(0x0079898A, nslFreeBank);

}
