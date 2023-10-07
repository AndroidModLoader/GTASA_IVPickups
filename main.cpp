#include <mod/amlmod.h>
#include <mod/logger.h>
#include <math.h>

#ifdef AML32
    #include "GTASA_STRUCTS.h"
    #define BYVER(__for32, __for64) (__for32)
#else
    #include "GTASA_STRUCTS_210.h"
    #define BYVER(__for32, __for64) (__for64)
#endif
#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))

MYMOD(net.pandagaming.rusjj.4pickups, IVPickups, 1.0, Pandagaming15 & RusJJ)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.1)
END_DEPLIST()

uintptr_t pGTASA;
void* hGTASA;

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//////      Variables
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
uint32_t *m_snTimeInMilliseconds;

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//////      Functions
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
#define RAD(_v) (_v * ( M_PI / 180.0 ) )
bool (*ProcessVerticalLine)(CVector*, float, CColPoint *, CEntity **, bool, bool, bool, bool, bool, bool, CStoredCollPoly *);
inline bool IsMoney(uint16_t mdlidx)
{
    return mdlidx == 1212;
}
inline bool IsSupportedModel(uint16_t mdlidx)
{
    return (mdlidx > 320 && mdlidx < 373) || IsMoney(mdlidx);
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//////      Hooks
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
uintptr_t PickupUpdate_BackTo;
extern "C" void PickupUpdate_Patch(CPickup* self)
{
    CVector worldPos = self->GetPosition();
    CObject* object = self->m_pObject;
    CVector& pos = object->GetPosition();

    CColPoint colPoint;
    CEntity *refEntityPtr; // trash var
    
    if(IsSupportedModel(object->GetModelIndex()) &&
       ProcessVerticalLine(&worldPos, -1000.0, &colPoint, &refEntityPtr, true, false, false, false, false, false, NULL) &&
       fabsf(colPoint.m_vecPoint.z - worldPos.z) < 2.5f)
    {
        object->objectFlags.bIVPickupsAffected = true; // custom flag
        CVector rot = { 0.0f, 0.0f, worldPos.x + worldPos.y };
        self->m_nFlags.bIVPickupsAffected = true; // custom flag
        pos = {worldPos.x, worldPos.y, colPoint.m_vecPoint.z + 0.01f};
        if(!IsMoney(object->GetModelIndex()))
        {
            rot.x = RAD(90.0f);
        }

        //rot = rot * colPoint.m_vecNormal;

        //if(colPoint.m_vecNormal != CVector(0.0f, 0.0f, 1.0f))
        //{
        //    rot.x *= colPoint.m_vecNormal.x;
        //    rot.y *= colPoint.m_vecNormal.y;
        //}

        object->GetMatrix()->SetRotateOnly(rot.x, rot.y, rot.z);
    }
    else
    {
        self->m_nFlags.bIVPickupsAffected = false; // custom flag
        pos = worldPos;
    }
}
__attribute__((optnone)) __attribute__((naked)) void PickupUpdate_Inject(void)
{
    asm volatile(
        "PUSH {R0-R11}\n"
        "LDR R0, [SP, #0x28]\n" // this
        "BL PickupUpdate_Patch\n");
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0-R11}\n"
        "BX R12\n"
    :: "r" (PickupUpdate_BackTo));
}

uintptr_t PickupCEffect_BackTo, PickupMEffect_BackTo, PickupEffect_BackTo;
extern "C" void PickupEffects_Patch(CObject* self)
{
    CMatrix* mat = self->GetMatrix();
    if(!mat) return;

    if(!self->objectFlags.bIVPickupsAffected)
    {
        mat->SetRotateZOnly((float)(*m_snTimeInMilliseconds % 4096) / 650.0f);
    }
}
__attribute__((optnone)) __attribute__((naked)) void PickupCEffect_Inject(void)
{
    asm volatile(
        "PUSH {R0-R11}\n"
        "LDR R0, [SP, #0x10]\n" // this
        "BL PickupEffects_Patch\n");
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0-R11}\n"
        "BX R12\n"
    :: "r" (PickupCEffect_BackTo));
}
__attribute__((optnone)) __attribute__((naked)) void PickupMEffect_Inject(void)
{
    asm volatile(
        "PUSH {R0-R11}\n"
        "LDR R0, [SP, #0x00]\n" // this
        "BL PickupEffects_Patch\n");
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0-R11}\n"
        "BX R12\n"
    :: "r" (PickupMEffect_BackTo));
}
__attribute__((optnone)) __attribute__((naked)) void PickupEffect_Inject(void)
{
    asm volatile(
        "PUSH {R0-R11}\n"
        "LDR R0, [SP, #0x10]\n" // this
        "BL PickupEffects_Patch\n");
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0-R11}\n"
        "BX R12\n"
    :: "r" (PickupEffect_BackTo));
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//////      Main
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
extern "C" void OnAllModsLoaded()
{
    logger->SetTag("IVPickups");

    pGTASA = aml->GetLib("libGTASA.so");
    hGTASA = aml->GetLibHandle("libGTASA.so");

    PickupUpdate_BackTo = pGTASA + 0x31D830 + 0x1;
    aml->Redirect(pGTASA + 0x31D7D8 + 0x1, (uintptr_t)PickupUpdate_Inject);

    PickupCEffect_BackTo = pGTASA + 0x3206CC + 0x1;
    aml->Redirect(pGTASA + 0x3205B4 + 0x1, (uintptr_t)PickupCEffect_Inject);

    PickupMEffect_BackTo = pGTASA + 0x320816 + 0x1;
    aml->Redirect(pGTASA + 0x320700 + 0x1, (uintptr_t)PickupMEffect_Inject);

    PickupEffect_BackTo = pGTASA + 0x3202D4 + 0x1;
    aml->Redirect(pGTASA + 0x3201B0 + 0x1, (uintptr_t)PickupEffect_Inject);

    SET_TO(m_snTimeInMilliseconds, aml->GetSym(hGTASA, "_ZN6CTimer22m_snTimeInMillisecondsE"));
    SET_TO(ProcessVerticalLine, aml->GetSym(hGTASA, "_ZN6CWorld19ProcessVerticalLineERK7CVectorfR9CColPointRP7CEntitybbbbbbP15CStoredCollPoly"));
}