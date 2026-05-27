#pragma once

#include "variable.h"

namespace MemoryUnitManager {

enum eOperation {};

enum eStatus {};

struct Container {
    int empty[144];
    int field_240;
    int field_244;
    char field_248[64];

    //0x007B1160
    Container(const char *a2);

    char *GetGameName()
    {
        return this->field_248;
    }
};

struct Observer {};

struct InsertRemoveObserver {};

extern void RegisterObserver(Observer *a1);

extern void Initialize(uint32_t a1);

extern bool Service();

extern void SetLastError(eStatus a1);

extern eStatus GetLastError();

extern bool StartOperation();

extern void RegisterInsertRemoveObserver(InsertRemoveObserver* a1);


extern eStatus LoadGame(const Container &a1);

extern int LoadGameSync(const Container &a1);

extern void SetCurrentUnit(int unit);


//0x007B1720
extern unsigned int GetGameSaveSize(unsigned int a1);

extern Var<Observer *> mObserver;

extern Var<eStatus> mLastError;

extern Var<InsertRemoveObserver*> mInsertRemoveObserver;

//0x007B1FF0
extern int SaveGame(const Container &a1);

inline Var<eOperation> mCurrentOperation {0x0098481C};

inline Var<Container> mGameSave {0x00984828};

} // namespace MemoryUnitManager
