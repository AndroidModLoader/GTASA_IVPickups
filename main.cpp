#include <mod/amlmod.h>
#include <mod/logger.h>
#include <math.h>
#include <time.h>

#ifdef AML32
    #include "GTASA_STRUCTS.h"
    #define BYVER(__for32, __for64) (__for32)
#else
    #include "GTASA_STRUCTS_210.h"
    #define BYVER(__for32, __for64) (__for64)
#endif
#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))

MYMOD(net.pandagaming.rusjj.4pickups, IVPickups, 1.0.4, PandaGaming15 & RusJJ)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.1)
END_DEPLIST()

uintptr_t pGTASA;
void* hGTASA;

#define LIGHT_SHADOW_RADIUS (0.55f)
#define RAD(_v) (float)( (_v) * ( M_PI / 180.0 ) )

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//////      Variables
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
uint32_t *m_snTimeInMilliseconds;
int timeStart;
CRGBA Money(155, 200, 25);
CRGBA Other(255, 155, 0);

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//////      Functions
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
bool (*ProcessVerticalLine)(CVector*, float, CColPoint *, CEntity **, bool, bool, bool, bool, bool, bool, CStoredCollPoly *);
void (*StoreShadowToBeRendered)(uint8_t shadowTextureType, CVector const *posn, float frontX, float frontY, float sideX, float sideY, short intensity, uint8_t red, uint8_t green, uint8_t blue);
inline bool IsMoney(uint16_t mdlidx)
{
    return mdlidx == 1212;
}
inline bool IsJetpack(uint16_t mdlidx)
{
    return mdlidx == 370;
}
inline bool IsShovel(uint16_t mdlidx)
{
    return mdlidx == 337;
}
inline bool IsSupportedModel(uint16_t mdlidx)
{
    return (mdlidx > 320 && mdlidx < 373) || IsMoney(mdlidx);
}
inline float CalculateIntensity(uintptr_t s = 0)
{
    int seed = s + *m_snTimeInMilliseconds;
    float v = sinf(((seed % 4096 - 2048) * M_PI) / 4096.0f);
    return v > 0 ? v : 0;
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
    uint16_t model = object->GetModelIndex();

    CColPoint colPoint;
    CEntity *refEntityPtr; // trash var
    
    if(IsSupportedModel(model) &&
       ProcessVerticalLine(&worldPos, -1000.0, &colPoint, &refEntityPtr, true, false, false, false, false, false, NULL) &&
       fabsf(colPoint.m_vecPoint.z - worldPos.z) < 2.5f
    )
    {
        //CVector rot =
        //{
        //    colPoint.m_vecNormal.x * RAD(90.0f),
        //    colPoint.m_vecNormal.y * RAD(90.0f),
        //    worldPos.x + worldPos.y
        //};
        CVector rot =
        {
            0.0f,
            0.0f,
            timeStart + worldPos.x + worldPos.y
        };
        if(!IsMoney(model))
        {
            if(IsJetpack(model))
            {
                rot.x = 0.0f;
                colPoint.m_vecPoint.z += 0.32f;
            }
            else
            {
                rot.x += RAD(90.0f);
            }
        }

        object->objectFlags.bIVPickupsAffected = true; // custom flag
        self->m_nFlags.bIVPickupsAffected = true; // custom flag
        pos = { worldPos.x, worldPos.y, colPoint.m_vecPoint.z + 0.01f };
        
        object->GetMatrix()->SetRotateOnly(rot.x, rot.y, rot.z);
    }
    else
    {
        object->objectFlags.bIVPickupsAffected = false; // custom flag
        self->m_nFlags.bIVPickupsAffected = false; // custom flag
        pos = worldPos;
    }
}
#ifdef AML32
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
#else
__attribute__((optnone)) __attribute__((naked)) void PickupUpdate_Inject(void)
{
    asm volatile(
        "MOV X0, X19\n" // this
        "BL PickupUpdate_Patch\n");
    asm volatile(
        "MOV X16, %0\n"
        "BR X16\n"
    :: "r" (PickupUpdate_BackTo));
}
#endif

uintptr_t PickupCEffect_BackTo, PickupMEffect_BackTo, PickupEffect_BackTo;
extern "C" void PickupEffects_Patch(CObject* self)
{
    CMatrix* mat = self->GetMatrix();
    if(!mat) return;

    if(self->objectFlags.bIVPickupsAffected)
    {
        float intens = 0.22f * CalculateIntensity((uintptr_t)self);
        #define INTENS(_v) (uint8_t)(_v * intens)

        if(IsMoney(self->GetModelIndex()))
        {
            StoreShadowToBeRendered(3, &self->GetPosition(), LIGHT_SHADOW_RADIUS, 0, 0, -LIGHT_SHADOW_RADIUS, 255, INTENS(Money.r), INTENS(Money.g), INTENS(Money.b));
        }
        else
        {
            StoreShadowToBeRendered(3, &self->GetPosition(), LIGHT_SHADOW_RADIUS, 0, 0, -LIGHT_SHADOW_RADIUS, 255, INTENS(Other.r), INTENS(Other.g), INTENS(Other.b));
        }

        #undef INTENS
    }
    else
    {
        mat->SetRotateZOnly((float)(*m_snTimeInMilliseconds % 4096) / 650.0f);
    }
}
#ifdef AML32
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
#else
__attribute__((optnone)) __attribute__((naked)) void PickupCEffect_Inject(void)
{
    asm volatile(
        "MOV X0, X19\n" // this
        "BL PickupEffects_Patch\n");
    asm volatile(
        "MOV X16, %0\n"
        "BR X16\n"
    :: "r" (PickupCEffect_BackTo));
}
__attribute__((optnone)) __attribute__((naked)) void PickupMEffect_Inject(void)
{
    asm volatile(
        "BL PickupEffects_Patch\n");
    asm volatile(
        "MOV X16, %0\n"
        "BR X16\n"
    :: "r" (PickupMEffect_BackTo));
}
__attribute__((optnone)) __attribute__((naked)) void PickupEffect_Inject(void)
{
    asm volatile(
        "MOV X0, X19\n" // this
        "BL PickupEffects_Patch\n");
    asm volatile(
        "MOV X16, %0\n"
        "BR X16\n"
    :: "r" (PickupEffect_BackTo));
}
#endif

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//////      Main
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
extern "C" void OnAllModsLoaded()
{
    logger->SetTag("IVPickups");
    timeStart = time(NULL);

    pGTASA = aml->GetLib("libGTASA.so");
    hGTASA = aml->GetLibHandle("libGTASA.so");

  #ifdef AML32
    PickupUpdate_BackTo = pGTASA + 0x31D830 + 0x1;
    aml->Redirect(pGTASA + 0x31D7D8 + 0x1, (uintptr_t)PickupUpdate_Inject);

    PickupCEffect_BackTo = pGTASA + 0x3206CC + 0x1;
    aml->Redirect(pGTASA + 0x3205B4 + 0x1, (uintptr_t)PickupCEffect_Inject);

    PickupMEffect_BackTo = pGTASA + 0x320816 + 0x1;
    aml->Redirect(pGTASA + 0x320700 + 0x1, (uintptr_t)PickupMEffect_Inject);

    PickupEffect_BackTo = pGTASA + 0x3202D4 + 0x1;
    aml->Redirect(pGTASA + 0x3201B0 + 0x1, (uintptr_t)PickupEffect_Inject);
  #else
    PickupUpdate_BackTo = pGTASA + 0x3E4B60;
    aml->Redirect(pGTASA + 0x3E4B04, (uintptr_t)PickupUpdate_Inject);

    PickupCEffect_BackTo = pGTASA + 0x3E827C;
    aml->Redirect(pGTASA + 0x3E80FC, (uintptr_t)PickupCEffect_Inject);

    PickupMEffect_BackTo = pGTASA + 0x3E842C;
    aml->Redirect(pGTASA + 0x3E82A4, (uintptr_t)PickupMEffect_Inject);

    PickupEffect_BackTo = pGTASA + 0x3E7E68;
    aml->Redirect(pGTASA + 0x3E7CDC, (uintptr_t)PickupEffect_Inject);
  #endif

    SET_TO(m_snTimeInMilliseconds,  aml->GetSym(hGTASA, "_ZN6CTimer22m_snTimeInMillisecondsE"));
    SET_TO(ProcessVerticalLine,     aml->GetSym(hGTASA, "_ZN6CWorld19ProcessVerticalLineERK7CVectorfR9CColPointRP7CEntitybbbbbbP15CStoredCollPoly"));
    SET_TO(StoreShadowToBeRendered, aml->GetSym(hGTASA, "_ZN8CShadows23StoreShadowToBeRenderedEhP7CVectorffffshhh"));
}