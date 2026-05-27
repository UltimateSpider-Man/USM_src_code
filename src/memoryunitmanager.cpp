#include "memoryunitmanager.h"

#include "common.h"
#include "func_wrapper.h"
#include "log.h"
#include "variables.h"
#include <direct.h>
#include <string.h>

namespace MemoryUnitManager {

VALIDATE_SIZE(Container, 0x288);

Var<Observer *> mObserver{0x00984818};

Var<eStatus> mLastError{0x009847CC};

Var<InsertRemoveObserver*> mInsertRemoveObserver{0x00984820};

bool get_path(const char *a1, const char *a2, char *out, unsigned int str_len)
{
    return (bool) CDECL_CALL(0x0081BE00, a1, a2, out, str_len);
}

void create_directory(const char *Source)
{
    char Dest[260] {};
    strncpy(Dest, Source, 260u);
    if ( Dest[0] )
    {
        auto *v1 = Dest;
        do
        {
            if ( *v1 == '\\' )
            {
                *v1 = 0;
                _mkdir(Dest);
                *v1 = '\\';
            }
        }
        while ( *++v1 );
    }

    _mkdir(Dest);
}

void sub_7B11C0(char *a1)
{
    static char byte_984BC8[260] {};
    static Var<bool> byte_984CCC {0x00984CCC};
    if ( !byte_984CCC() )
    {
        get_path("Activision\\Ultimate Spider-Man\\", "Save\\", byte_984BC8, 260u);
        create_directory(byte_984BC8);
        byte_984CCC() = true;
    }

    std::strcpy(a1, byte_984BC8);
}

Container::Container(const char *a2) {
    strcpy(this->field_248, a2);

    this->field_240 = 0;
    this->field_244 = 0;
}

void RegisterObserver(Observer *a1)
{
    mObserver() = a1;
}

void Initialize(uint32_t a1)
{
    CDECL_CALL(0x007B16D0, a1);
}

bool Service()
{
    return (bool) CDECL_CALL(0x007B1EE0);
}

void SetLastError(eStatus a1)
{
    mLastError() = a1;
}

void RegisterInsertRemoveObserver(InsertRemoveObserver* a1)
{
  MemoryUnitManager::InsertRemoveObserver() = *a1;
}

bool StartOperation()
{
    return (bool) CDECL_CALL(0x007B1E00);
}

eStatus GetLastError()
{
    return mLastError();
}

void SetCurrentUnit(int unit)
{
    dword_151691C() = unit;
}

eStatus LoadGame(const Container &a1)
{
    if ( mCurrentOperation() != 0 )
    {
        SetLastError((eStatus)-17);
    }
    else
    {
        mGameSave() = a1;
        mCurrentOperation() = static_cast<eOperation>(1);
        auto *GameName = mGameSave().GetGameName();

        static Var<char[260]> PathName {0x00984AB8};
        sub_7B11C0(PathName());
        strcat(PathName(), GameName);
        StartOperation();
    }

    return GetLastError();
}

int LoadGameSync(const Container &a1)
{
    if (MemoryUnitManager::SaveGame(a1) == 0) {
        while (Service()) {
            ;
        }
    }

    return mLastError();
}

int SaveGame(const Container &a1) {
    return CDECL_CALL(0x007B1FF0, &a1);
}

unsigned int GetGameSaveSize(unsigned int a1) {
    return a1;
}

} // namespace MemoryUnitManager
