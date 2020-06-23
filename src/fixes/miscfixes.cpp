#include "fixes.h"

namespace fixes
{
    class AnimationLoadSignedCrashPatch
    {
    public:
        static void Install()
        {
            // Change "BF" to "B7"
            REL::Offset<std::uintptr_t> target(REL::ID(64198), 0x91);
            SKSE::SafeWrite8(target.address(), 0xB7);
        }
    };

    bool PatchAnimationLoadSignedCrash()
    {
        _VMESSAGE("- animation load signed crash fix -");

        AnimationLoadSignedCrashPatch::Install();

        _VMESSAGE("success");
        return true;
    }

    class ArcheryDownwardAimingPatch
    {
    public:
        static void Install()
        {
            REL::Offset<std::uintptr_t> funcBase = REL::ID(42852);
            auto trampoline = SKSE::GetTrampoline();
            _Move = trampoline->Write5CallEx(funcBase.address() + 0x3E9, Move);
        }

    private:
        static void Move(RE::Projectile* a_this, /*const*/ RE::NiPoint3& a_from, const RE::NiPoint3& a_to)
        {
            auto refShooter = a_this->shooter.get();
            if (refShooter && refShooter->Is(RE::FormType::ActorCharacter))
            {
                auto akShooter = static_cast<RE::Actor*>(refShooter.get());
                [[maybe_unused]] RE::NiPoint3 direction;
                akShooter->GetEyeVector(a_from, direction, true);
            }

            _Move(a_this, a_from, a_to);
        }

        static inline REL::Function<decltype(Move)> _Move;
    };

    bool PatchArcheryDownwardAiming()
    {
        _VMESSAGE("- archery downward aiming fix -");

        ArcheryDownwardAimingPatch::Install();

        _VMESSAGE("- success -");
        return true;
    }

    class BethesdaNetCrashPatch
    {
    public:
        static void Install()
        {
            SKSE::PatchIAT(hk_wcsrtombs_s, "API-MS-WIN-CRT-CONVERT-L1-1-0.dll", "wcsrtombs_s");
        }

    private:
        static errno_t hk_wcsrtombs_s(std::size_t* a_retval, char* a_dst, rsize_t a_dstsz, const wchar_t** a_src, rsize_t a_len, [[maybe_unused]] std::mbstate_t* a_ps)
        {
            int numChars = WideCharToMultiByte(CP_UTF8, 0, *a_src, static_cast<int>(a_len), NULL, 0, NULL, NULL);

            std::string str;
            char* dst = 0;
            rsize_t dstsz = 0;
            if (a_dst)
            {
                dst = a_dst;
                dstsz = a_dstsz;
            }
            else
            {
                str.resize(numChars);
                dst = str.data();
                dstsz = str.max_size();
            }

            bool err;
            if (a_src && numChars != 0 && numChars <= dstsz)
            {
                err = WideCharToMultiByte(CP_UTF8, 0, *a_src, static_cast<int>(a_len), dst, numChars, NULL, NULL) ? false : true;
            }
            else
            {
                err = true;
            }

            if (err)
            {
                if (a_retval)
                {
                    *a_retval = static_cast<std::size_t>(-1);
                }
                if (a_dst && a_dstsz != 0 && a_dstsz <= (std::numeric_limits<rsize_t>::max)())
                {
                    a_dst[0] = '\0';
                }
                return GetLastError();
            }

            if (a_retval)
            {
                *a_retval = static_cast<std::size_t>(numChars);
            }
            return 0;
        }
    };

    bool PatchBethesdaNetCrash()
    {
        _VMESSAGE("- bethesda.net crash fix -");

        BethesdaNetCrashPatch::Install();

        _VMESSAGE("success");
        return true;
    }

    class BSLightingAmbientSpecularPatch
    {
    public:
        static void Install()
        {
            _VMESSAGE("nopping SetupMaterial case");

            constexpr byte nop = 0x90;
            constexpr uint8_t length = 0x20;

            REL::Offset<std::uintptr_t> addAmbientSpecularToSetupGeometry(REL::ID(100565), 0xBAD);
            REL::Offset<std::uintptr_t> ambientSpecularAndFresnel = REL::ID(513256);
            REL::Offset<std::uintptr_t> disableSetupMaterialAmbientSpecular(REL::ID(100563), 0x713);

            for (int i = 0; i < length; ++i)
            {
                SKSE::SafeWrite8(disableSetupMaterialAmbientSpecular.address() + i, nop);
            }

            _VMESSAGE("Adding SetupGeometry case");

            struct Patch : SKSE::CodeGenerator
            {
                Patch(std::uintptr_t a_ambientSpecularAndFresnel, std::uintptr_t a_addAmbientSpecularToSetupGeometry) : SKSE::CodeGenerator()
                {
                    Xbyak::Label jmpOut;
                    // hook: 0x130AB2D (in middle of SetupGeometry, right before if (rawTechnique & RAW_FLAG_SPECULAR), just picked a random place tbh
                    // test
                    test(dword[r13 + 0x94], 0x20000);  // RawTechnique & RAW_FLAG_AMBIENT_SPECULAR
                    jz(jmpOut);
                    // ambient specular
                    push(rax);
                    push(rdx);
                    mov(rax, a_ambientSpecularAndFresnel);  // xmmword_1E3403C
                    movups(xmm0, ptr[rax]);
                    mov(rax, qword[rsp + 0x170 - 0x120 + 0x10]);  // PixelShader
                    movzx(edx, byte[rax + 0x46]);                 // m_ConstantOffsets 0x6 (AmbientSpecularTintAndFresnelPower)
                    mov(rax, ptr[r15 + 8]);                       // m_PerGeometry buffer (copied from SetupGeometry)
                    movups(ptr[rax + rdx * 4], xmm0);             // m_PerGeometry buffer offset 0x6
                    pop(rdx);
                    pop(rax);
                    // original code
                    L(jmpOut);
                    test(dword[r13 + 0x94], 0x200);
                    jmp(ptr[rip]);
                    dq(a_addAmbientSpecularToSetupGeometry + 11);
                }
            };

            Patch patch(ambientSpecularAndFresnel.address(), addAmbientSpecularToSetupGeometry.address());
            patch.ready();

            auto trampoline = SKSE::GetTrampoline();
            trampoline->Write5Branch(addAmbientSpecularToSetupGeometry.address(), reinterpret_cast<std::uintptr_t>(patch.getCode()));
        }
    };

    bool PatchBSLightingAmbientSpecular()
    {
        _VMESSAGE("BSLightingAmbientSpecular fix");

        BSLightingAmbientSpecularPatch::Install();

        _VMESSAGE("success");
        return true;
    }

    bool PatchBSTempEffectNiRTTI()
    {
        _VMESSAGE("- BSTempEffect NiRTTI fix -");

        REL::Offset<RE::NiRTTI*> rttiBSTempEffect(RE::BSTempEffect::Ni_RTTI);
        REL::Offset<RE::NiRTTI*> rttiNiObject(RE::NiObject::Ni_RTTI);
        rttiBSTempEffect->baseRTTI = rttiNiObject.type();

        _VMESSAGE("success");
        return true;
    }

    class CalendarSkippingPatch
    {
    public:
        static void Install()
        {
            constexpr std::size_t CAVE_START = 0x17A;
            constexpr std::size_t CAVE_SIZE = 0x15;

            REL::Offset<std::uintptr_t> funcBase = REL::ID(35402);

            struct Patch : SKSE::CodeGenerator
            {
                Patch(std::uintptr_t a_addr) : SKSE::CodeGenerator(CAVE_SIZE)
                {
                    Xbyak::Label jmpLbl;

                    movaps(xmm0, xmm1);
                    jmp(ptr[rip + jmpLbl]);

                    L(jmpLbl);
                    dq(a_addr);
                }
            };

            Patch patch(unrestricted_cast<std::uintptr_t>(AdvanceTime));
            patch.ready();

            for (std::size_t i = 0; i < patch.getSize(); ++i)
            {
                SKSE::SafeWrite8(funcBase.address() + CAVE_START + i, patch.getCode()[i]);
            }
        }

    private:
        static void AdvanceTime(float a_secondsPassed)
        {
            auto time = RE::Calendar::GetSingleton();
            float hoursPassed = (a_secondsPassed * time->timeScale->value / (60.0F * 60.0F)) + time->gameHour->value - 24.0F;
            if (hoursPassed > 24.0)
            {
                do
                {
                    time->midnightsPassed += 1;
                    time->rawDaysPassed += 1.0F;
                    hoursPassed -= 24.0F;
                } while (hoursPassed > 24.0F);
                time->gameDaysPassed->value = (hoursPassed / 24.0F) + time->rawDaysPassed;
            }
        }
    };

    bool PatchCalendarSkipping()
    {
        _VMESSAGE("- calendar skipping fix -");

        CalendarSkippingPatch::Install();

        _VMESSAGE("success");
        return true;
    }

    class CellInitPatch
    {
    public:
        static void Install()
        {
            auto trampoline = SKSE::GetTrampoline();
            REL::Offset<std::uintptr_t> funcBase = REL::ID(18474);
            _GetLocation = trampoline->Write5CallEx(funcBase.address() + 0x110, GetLocation);
        }

    private:
        static RE::BGSLocation* GetLocation(const RE::ExtraDataList* a_this)
        {
            auto cell = adjust_pointer<RE::TESObjectCELL>(a_this, -0x48);
            auto loc = _GetLocation(a_this);
            if (!cell->IsInitialized())
            {
                auto file = cell->GetFile();
                auto formID = reinterpret_cast<RE::FormID>(loc);
                RE::TESForm::AddCompileIndex(formID, file);
                loc = RE::TESForm::LookupByID<RE::BGSLocation>(formID);
            }
            return loc;
        }

        static inline REL::Function<decltype(GetLocation)> _GetLocation;
    };

    bool PatchCellInit()
    {
        _VMESSAGE("- cell init fix -");

        CellInitPatch::Install();

        _VMESSAGE("success");
        return true;
    }

    // TODO: cleanup
    // Patch for 1.5.97 but should be version-independent
    bool PatchCreateArmorNodeNullptrCrash()
    {
        _VMESSAGE("- CreateArmorNode subfunction crash fix -");

        // sub_1401CAFB0
        const std::uint64_t faddr = REL::ID(15535).address();

        /*
        loc_1401CB520:
          [...]
        .text:00000001401CB538                 mov     esi, ebx
        .text:00000001401CB53A                 test    r12, r12
        .text:00000001401CB53D                 jz      short loc_1401CB5B6 // <-- this jump needs to be patched but the operand is too small to fit
        */
        static const std::uint8_t expected1[] = {
            0x8B, 0xF3,        // mov     esi, ebx
            0x4D, 0x85, 0xE4,  // test    r12, r12
            0x74               // jz short (opcode only); +1 byte displacement
        };

        const std::uint8_t* loc1p = (std::uint8_t*)(std::uintptr_t)(faddr + 0x588);
        if (memcmp(loc1p, expected1, sizeof(expected1)))
        {
            _VMESSAGE("Code at first offset is not as expected, skipping patch");
            return false;
        }

        const std::int8_t disp8 = loc1p[sizeof(expected1)];  // jz short displacement (should be 0x77)
        const std::uint8_t* const loc1end = loc1p + sizeof(expected1) + 1;

        /*
        loc_1401CB5B6:
        .text:00000001401CB5B6                 mov     rdi, [r12+130h] // <-- this is where the crash happens, because r12 == 0
        .text:00000001401CB5BE                 test    rdi, rdi
        .text:00000001401CB5C1                 jz      loc_1401CB751 // <-- This is the location we want to jump to when test r12,r12 fails
        */

        const void* const loc2p = loc1end + disp8;
        const std::ptrdiff_t offs2 = (std::uintptr_t)loc2p - faddr;
        _VMESSAGE("Located second block at offset 0x%x", offs2);

        static const uint8_t expected2[] =  // this should use scanning instead; hardcoding offset 130h is bad
            {
                0x49, 0x8B, 0xBC, 0x24, 0x30, 0x01, 0x00, 0x00,  // mov     rdi, [r12+130h]
                0x48, 0x85, 0xFF,                                // test    rdi, rdi
                0x0F, 0x84                                       // jz (opcode only); +4 bytes displacement
            };

        if (memcmp(loc2p, expected2, sizeof(expected2)))
        {
            _VMESSAGE("Code at second offset is not as expected, skipping patch");
            return false;
        }

        _VMESSAGE("Code confirmed broken; installing fix");

        const std::uint8_t* const loc2end = (const std::uint8_t*)loc2p + sizeof(expected2) + sizeof(std::int32_t);

        std::int32_t disp2 = *unrestricted_cast<std::int32_t*>(((const std::uint8_t*)loc2p) + sizeof(expected2));
        const std::uint8_t* const target = loc2end + disp2;

        struct Code : SKSE::CodeGenerator
        {
            Code(std::uintptr_t contAddr, std::uintptr_t patchedAddr) : SKSE::CodeGenerator()
            {
                Xbyak::Label patchedJmpLbl, contLbl, zeroLbl;

                // original instructions minus jz short
                db(expected1, sizeof(expected1) - 1);

                jz(zeroLbl);              // patched non-short jz
                jmp(ptr[rip + contLbl]);  // continue original code
                L(zeroLbl);               // was zero; skip over code that would crash
                jmp(ptr[rip + patchedJmpLbl]);

                L(contLbl);
                dq(contAddr);
                L(patchedJmpLbl);
                dq(patchedAddr);
            }
        };

        Code code(unrestricted_cast<std::uintptr_t>(loc1end), unrestricted_cast<std::uintptr_t>(target));
        code.ready();

        auto trampoline = SKSE::GetTrampoline();
        if (!trampoline->Write5Branch(unrestricted_cast<std::uintptr_t>(loc1p), code.getCode()))
            return false;

        _VMESSAGE("success");
        return true;
    }

    class ConjurationEnchantAbsorbsPatch
    {
    public:
        static void Install()
        {
            REL::Offset<std::uintptr_t> vtbl = REL::ID(228570);
            _DisallowsAbsorbReflection = vtbl.write_vfunc(0x5E, DisallowsAbsorbReflection);
        }

    private:
        static bool DisallowsAbsorbReflection(RE::EnchantmentItem* a_this)
        {
            using Archetype = RE::EffectArchetypes::ArchetypeID;
            for (auto& effect : a_this->effects)
            {
                if (effect->baseEffect->HasArchetype(Archetype::kSummonCreature))
                {
                    return true;
                }
            }
            return _DisallowsAbsorbReflection(a_this);
        }

        using DisallowsAbsorbReflection_t = decltype(&RE::EnchantmentItem::GetNoAbsorb);  // 5E
        static inline REL::Function<DisallowsAbsorbReflection_t> _DisallowsAbsorbReflection;
    };

    bool PatchConjurationEnchantAbsorbs()
    {
        _VMESSAGE("- enchantment absorption on staff summons fix -");

        ConjurationEnchantAbsorbsPatch::Install();

        _VMESSAGE("success");
        return true;
    }

    class EquipShoutEventSpamPatch
    {
    public:
        static void Install()
        {
            constexpr std::uintptr_t BRANCH_OFF = 0x17A;
            constexpr std::uintptr_t SEND_EVENT_BEGIN = 0x18A;
            constexpr std::uintptr_t SEND_EVENT_END = 0x236;
            constexpr std::size_t EQUIPPED_SHOUT = offsetof(RE::Actor, selectedPower);
            constexpr UInt32 BRANCH_SIZE = 5;
            constexpr UInt32 CODE_CAVE_SIZE = 16;
            constexpr UInt32 DIFF = CODE_CAVE_SIZE - BRANCH_SIZE;
            constexpr UInt8 NOP = 0x90;

            REL::Offset<std::uintptr_t> funcBase = REL::ID(37821);

            struct Patch : SKSE::CodeGenerator
            {
                Patch(std::uintptr_t a_funcBase) : SKSE::CodeGenerator()
                {
                    Xbyak::Label exitLbl;
                    Xbyak::Label exitIP;
                    Xbyak::Label sendEvent;

                    // r14 = Actor*
                    // rdi = TESShout*

                    cmp(ptr[r14 + EQUIPPED_SHOUT], rdi);  // if (actor->equippedShout != shout)
                    je(exitLbl);
                    mov(ptr[r14 + EQUIPPED_SHOUT], rdi);  // actor->equippedShout = shout;
                    test(rdi, rdi);                       // if (shout)
                    jz(exitLbl);
                    jmp(ptr[rip + sendEvent]);

                    L(exitLbl);
                    jmp(ptr[rip + exitIP]);

                    L(exitIP);
                    dq(a_funcBase + SEND_EVENT_END);

                    L(sendEvent);
                    dq(a_funcBase + SEND_EVENT_BEGIN);
                }
            };

            Patch patch(funcBase.address());
            patch.ready();

            auto trampoline = SKSE::GetTrampoline();
            trampoline->Write5Branch(funcBase.address() + BRANCH_OFF, reinterpret_cast<std::uintptr_t>(patch.getCode()));

            for (UInt32 i = 0; i < DIFF; ++i)
            {
                SKSE::SafeWrite8(funcBase.address() + BRANCH_OFF + BRANCH_SIZE + i, NOP);
            }
        }
    };

    bool PatchEquipShoutEventSpam()
    {
        _VMESSAGE("- equip shout event spam fix - ");

        EquipShoutEventSpamPatch::Install();

        _VMESSAGE("success");
        return true;
    }

    class GetKeywordItemCountPatch
    {
    public:
        static void Install()
        {
            auto command = RE::SCRIPT_FUNCTION::LocateScriptCommand(LONG_NAME);
            if (command)
            {
                command->executeFunction = Execute;
                command->conditionFunction = Eval;
            }
        }

    private:
        static bool Execute(const RE::SCRIPT_PARAMETER*, RE::SCRIPT_FUNCTION::ScriptData*, RE::TESObjectREFR* a_thisObj, RE::TESObjectREFR*, RE::Script* a_scriptObj, RE::ScriptLocals*, double& a_result, UInt32&)
        {
            if (!a_scriptObj || a_scriptObj->refObjects.empty())
            {
                return false;
            }

            auto param = a_scriptObj->refObjects.front();
            if (!param->form || param->form->IsNot(RE::FormType::Keyword))
            {
                return false;
            }

            return Eval(a_thisObj, param->form, 0, a_result);
        }

        static bool Eval(RE::TESObjectREFR* a_thisObj, void* a_param1, [[maybe_unused]] void* a_param2, double& a_result)
        {
            a_result = 0.0;
            if (!a_thisObj || !a_param1)
            {
                return true;
            }

            auto log = RE::ConsoleLog::GetSingleton();
            if (!a_thisObj->GetContainer())
            {
                if (log->IsConsoleMode())
                {
                    log->Print("Calling Reference is not a Container Object");
                }
                return true;
            }

            auto keyword = static_cast<RE::BGSKeyword*>(a_param1);
            auto inv = a_thisObj->GetInventoryCounts([&](RE::TESBoundObject* a_object) -> bool {
                auto keywordForm = a_object->As<RE::BGSKeywordForm>();
                return keywordForm && keywordForm->HasKeyword(keyword);
            });

            for (auto& elem : inv)
            {
                a_result += elem.second;
            }

            if (log->IsConsoleMode())
            {
                log->Print("GetKeywordItemCount >> %0.2f", a_result);
            }

            return true;
        }

        static constexpr char LONG_NAME[] = "GetKeywordItemCount";
    };

    bool PatchGetKeywordItemCount()
    {
        _VMESSAGE("- broken GetKeywordItemCount condition function fix -");

        GetKeywordItemCountPatch::Install();

        _VMESSAGE("success");
        return true;
    }

    class GHeapLeakDetectionCrashPatch
    {
    public:
        static void Install()
        {
            constexpr std::uintptr_t START = 0x4B;
            constexpr std::uintptr_t END = 0x5C;
            constexpr UInt8 NOP = 0x90;
            REL::Offset<std::uintptr_t> funcBase = REL::ID(85757);

            for (std::uintptr_t i = START; i < END; ++i)
            {
                SKSE::SafeWrite8(funcBase.address() + i, NOP);
            }
        }
    };

    bool PatchGHeapLeakDetectionCrash()
    {
        _VMESSAGE("- GHeap leak detection crash fix -");

        GHeapLeakDetectionCrashPatch::Install();

        _VMESSAGE("success");
        return true;
    }

    class LipSyncPatch
    {
    public:
        static void Install()
        {
            constexpr std::array<UInt8, 4> OFFSETS = {
                0x1E,
                0x3A,
                0x9A,
                0xD8
            };

            REL::Offset<std::uintptr_t> funcBase = REL::ID(16023);
            for (auto& offset : OFFSETS)
            {
                SKSE::SafeWrite8(funcBase.address() + offset, 0xEB);  // jns -> jmp
            }
        }
    };

    bool PatchLipSync()
    {
        _VMESSAGE("- lip sync bug fix -");

        LipSyncPatch::Install();

        _VMESSAGE("- success -");
        return true;
    }

    class MemoryAccessErrorsPatch
    {
    public:
        static void Install()
        {
            PatchSnowMaterialCase();
            PatchShaderParticleGeometryDataLimit();
            PatchUseAfterFree();
        }

    private:
        static void PatchSnowMaterialCase()
        {
            _VMESSAGE("patching BSLightingShader::SetupMaterial snow material case");

            struct Patch : SKSE::CodeGenerator
            {
                Patch(std::uintptr_t a_vtbl, std::uintptr_t a_hook, std::uintptr_t a_exit) : SKSE::CodeGenerator()
                {
                    Xbyak::Label vtblAddr;
                    Xbyak::Label snowRetnLabel;
                    Xbyak::Label exitRetnLabel;

                    mov(rax, ptr[rip + vtblAddr]);
                    cmp(rax, qword[rbx]);
                    je("IS_SNOW");

                    // not snow, fill with 0 to disable effect
                    mov(eax, 0x00000000);
                    mov(dword[rcx + rdx * 4 + 0xC], eax);
                    mov(dword[rcx + rdx * 4 + 0x8], eax);
                    mov(dword[rcx + rdx * 4 + 0x4], eax);
                    mov(dword[rcx + rdx * 4], eax);
                    jmp(ptr[rip + exitRetnLabel]);

                    // is snow, jump out to original except our overwritten instruction
                    L("IS_SNOW");
                    movss(xmm2, dword[rbx + 0xAC]);
                    jmp(ptr[rip + snowRetnLabel]);

                    L(vtblAddr);
                    dq(a_vtbl);

                    L(snowRetnLabel);
                    dq(a_hook + 0x8);

                    L(exitRetnLabel);
                    dq(a_exit);
                }
            };

            REL::Offset<std::uintptr_t> vtbl = REL::ID(304565);
            REL::Offset<std::uintptr_t> funcBase = REL::ID(100563);
            std::uintptr_t hook = funcBase.address() + 0x4E0;
            std::uintptr_t exit = funcBase.address() + 0x5B6;
            Patch patch(vtbl.address(), hook, exit);
            patch.ready();

            auto trampoline = SKSE::GetTrampoline();
            trampoline->Write6Branch(hook, patch.getCode<std::uintptr_t>());
        }

        static void PatchShaderParticleGeometryDataLimit()
        {
            _VMESSAGE("patching BGSShaderParticleGeometryData limit");

            REL::Offset<std::uintptr_t> vtbl = REL::ID(234671);
            _Load = vtbl.write_vfunc(0x6, Load);
        }

        static void PatchUseAfterFree()
        {
            _VMESSAGE("patching BSShadowDirectionalLight use after free");

            // Xbyak is used here to generate the ASM to use instead of just doing it by hand
            struct Patch : SKSE::CodeGenerator
            {
                Patch() : SKSE::CodeGenerator(100)
                {
                    mov(r9, r15);
                    nop();
                    nop();
                    nop();
                    nop();
                }
            };

            Patch patch;
            patch.ready();

            REL::Offset<std::uintptr_t> target = REL::ID(101499);
            for (std::size_t i = 0; i < patch.getSize(); ++i)
            {
                SKSE::SafeWrite8(target.address() + 0x1AFD + i, patch.getCode()[i]);
            }
        }

        // BGSShaderParticleGeometryData::Load
        static bool Load(RE::BGSShaderParticleGeometryData* a_this, RE::TESFile* a_file)
        {
            const bool retVal = _Load(a_this, a_file);

            // the game doesn't allow more than 10 here
            if (a_this->data.size() >= 12)
            {
                const auto particleDensity = a_this->data[11];
                if (particleDensity.f > 10.0)
                    a_this->data[11].f = 10.0f;
            }

            return retVal;
        }

        inline static REL::Function<decltype(&RE::BGSShaderParticleGeometryData::Load)> _Load;
    };

    bool PatchMemoryAccessErrors()
    {
        _VMESSAGE("- memory access errors fix -");

        MemoryAccessErrorsPatch::Install();

        _VMESSAGE("success");
        return true;
    }

    class MO5STypoPatch
    {
    public:
        static void Install()
        {
            // Change "D" to "5"
            REL::Offset<std::uintptr_t> typo = REL::ID(14653);
            SKSE::SafeWrite8(typo.address() + 0x83, 0x35);
        }
    };

    bool PatchMO5STypo()
    {
        _VMESSAGE("- MO5S Typo fix -");

        MO5STypoPatch::Install();

        _VMESSAGE("success");
        return true;
    }

    class NullProcessCrashPatch
    {
    public:
        static void Install()
        {
            auto trampoline = SKSE::GetTrampoline();

            {
                REL::Offset<std::uintptr_t> funcBase = REL::ID(37943);
                trampoline->Write5Call(funcBase.address() + 0x6C, GetEquippedLeftHand);
                trampoline->Write5Call(funcBase.address() + 0x9C, GetEquippedRightHand);
            }

            {
                REL::Offset<std::uintptr_t> funcBase = REL::ID(46074);
                trampoline->Write5Call(funcBase.address() + 0x47, GetEquippedLeftHand);
                trampoline->Write5Call(funcBase.address() + 0x56, GetEquippedRightHand);
            }
        }

    private:
        static RE::TESForm* GetEquippedLeftHand(RE::AIProcess* a_process)
        {
            return a_process ? a_process->GetEquippedLeftHand() : nullptr;
        }

        static RE::TESForm* GetEquippedRightHand(RE::AIProcess* a_process)
        {
            return a_process ? a_process->GetEquippedRightHand() : nullptr;
        }
    };

    bool PatchNullProcessCrash()
    {
        _VMESSAGE("- null process crash fix -");

        NullProcessCrashPatch::Install();

        _VMESSAGE("success");
        return true;
    }

    class PerkFragmentIsRunningPatch
    {
    public:
        static void Install()
        {
            REL::Offset<std::uintptr_t> funcBase = REL::ID(21119);
            auto trampoline = SKSE::GetTrampoline();
            trampoline->Write5Call(funcBase.address() + 0x22, IsRunning);
        }

    private:
        static bool IsRunning(RE::Actor* a_this)
        {
            return a_this ? a_this->IsRunning() : false;
        }
    };

    bool PatchPerkFragmentIsRunning()
    {
        _VMESSAGE("- perk fragment IsRunning fix -");

        PerkFragmentIsRunningPatch::Install();

        _VMESSAGE("success");
        return true;
    }

    class RemovedSpellBookPatch
    {
    public:
        static void Install()
        {
            REL::Offset<std::uintptr_t> vtbl = REL::ID(234122);
            _LoadGame = vtbl.write_vfunc(0xF, LoadGame);
        }

    private:
        static void LoadGame(RE::TESObjectBOOK* a_this, RE::BGSLoadFormBuffer* a_buf)
        {
            using Flag = RE::OBJ_BOOK::Flag;

            _LoadGame(a_this, a_buf);

            if (a_this->data.teaches.actorValueToAdvance == RE::ActorValue::kNone)
            {
                if (a_this->TeachesSkill())
                {
                    a_this->data.flags &= ~Flag::kAdvancesActorValue;
                }

                if (a_this->TeachesSpell())
                {
                    a_this->data.flags &= ~Flag::kTeachesSpell;
                }
            }
        }

        static inline REL::Function<decltype(&RE::TESObjectBOOK::LoadGame)> _LoadGame;  // 0xF
    };

    bool PatchRemovedSpellBook()
    {
        _VMESSAGE("- removed spell book fix -");

        RemovedSpellBookPatch::Install();

        _VMESSAGE("success");
        return true;
    }

    class SlowTimeCameraMovementPatch
    {
    public:
        static void Install()
        {
            _VMESSAGE("patching camera movement to use frame timer that ignores slow time");

            constexpr std::array<std::pair<std::uint64_t, std::size_t>, 5> TARGETS = {
                std::make_pair(49977, 0x2F),
                std::make_pair(49977, 0x96),
                std::make_pair(49977, 0x1FD),
                std::make_pair(49980, 0xBA),
                std::make_pair(49981, 0x17)
            };

            for (auto& target : TARGETS)
            {
                REL::Offset<std::int16_t*> offset(REL::ID(target.first), target.second);
                SKSE::SafeWrite16(offset.address(), *offset + 0x4);
            }
        }
    };

    bool PatchSlowTimeCameraMovement()
    {
        _VMESSAGE("- slow time camera movement fix -");

        SlowTimeCameraMovementPatch::Install();

        _VMESSAGE("success");
        return true;
    }

    class TorchLandscapePatch
    {
    public:
        static void Install()
        {
            REL::Offset<std::uintptr_t> target{ REL::ID(17208), 0x52D };

            struct Patch :
                SKSE::CodeGenerator
            {
                Patch(std::uintptr_t a_func) : SKSE::CodeGenerator()
                {
                    Xbyak::Label f;

                    mov(r9, rdi);
                    jmp(ptr[rip + f]);

                    L(f);
                    dq(a_func);
                }
            };

            Patch patch(unrestricted_cast<std::uintptr_t>(AddLight));
            patch.ready();

            auto trampoline = SKSE::GetTrampoline();
            _AddLight = trampoline->Write5CallEx(target.address(), patch.getCode());
        }

    private:
        static RE::BSLight* AddLight(
            RE::ShadowSceneNode* a_this,
            RE::NiLight* a_list,
            RE::ShadowSceneNode::LIGHT_CREATE_PARAMS& a_params,
            RE::not_null<RE::TESObjectREFR*> a_requester)
        {
            if (a_requester->Is(RE::FormType::ActorCharacter))
            {
                a_params.affectLand = true;
            }

            return _AddLight(a_this, a_list, a_params);
        }

        static inline REL::Offset<RE::BSLight*(RE::ShadowSceneNode*, RE::NiLight*, const RE::ShadowSceneNode::LIGHT_CREATE_PARAMS&)> _AddLight;
    };

    bool PatchTorchLandscape()
    {
        _VMESSAGE("- torch landscape fix -");

        TorchLandscapePatch::Install();

        _VMESSAGE("success");
        return true;
    }

    class TreeReflectionsPatch
    {
    public:
        static bool Install()
        {
            const auto handle = GetModuleHandleA("d3dcompiler_46e.dll");

            if (handle)
            {
                _VMESSAGE("enb detected - disabling fix, please use ENB's tree reflection fix instead");
                return true;
            }

            _VMESSAGE("patching BSDistantTreeShader vfunc 3");
            struct Patch : SKSE::CodeGenerator
            {
                Patch(std::uintptr_t a_target) : SKSE::CodeGenerator()
                {
                    Xbyak::Label retnLabel;

                    // current: if(bUseEarlyZ) v3 |= 0x10000u;
                    // goal: if(bUseEarlyZ || v3 == 0) v3 |= 0x10000u;
                    // if (bUseEarlyZ)
                    // .text:0000000141318C50                 cmp     cs:bUseEarlyZ, r13b
                    // need 6 bytes to branch jmp so enter here
                    // enter 1318C57
                    // .text:0000000141318C57                 jz      short loc_141318C5D
                    jnz("CONDITION_MET");
                    // edi = v3
                    // if (v3 == 0)
                    test(edi, edi);
                    jnz("JMP_OUT");
                    // .text:0000000141318C59                 bts     edi, 10h
                    L("CONDITION_MET");
                    bts(edi, 0x10);
                    L("JMP_OUT");
                    // exit 1318C5D
                    jmp(ptr[rip + retnLabel]);

                    L(retnLabel);
                    dq(a_target + 0x6);
                }
            };

            REL::Offset<std::uintptr_t> target(REL::ID(100771), 0x37);
            Patch patch(target.address());
            patch.ready();

            auto trampoline = SKSE::GetTrampoline();
            trampoline->Write6Branch(target.address(), patch.getCode<std::uintptr_t>());

            _VMESSAGE("success");
            return true;
        }
    };

    bool PatchTreeReflections()
    {
        _VMESSAGE("- blocky tree reflections fix -");
        return TreeReflectionsPatch::Install();
    }

    class VerticalLookSensitivityPatch
    {
    public:
        static void Install()
        {
            PatchThirdPersonStateHook();
            PatchDragonCameraStateHook();
            PatchHorseCameraStateHook();
        }

    private:
        static void PatchThirdPersonStateHook()
        {
            _VMESSAGE("patching third person state...");

            struct Patch : SKSE::CodeGenerator
            {
                Patch(std::uintptr_t a_hookTarget, std::uintptr_t a_frameTimer) : SKSE::CodeGenerator()
                {
                    Xbyak::Label retnLabel;
                    Xbyak::Label magicLabel;
                    Xbyak::Label timerLabel;

                    // enter 850D81
                    // r8 is unused
                    //.text:0000000140850D81                 movss   xmm4, cs:frame_timer_without_slow
                    // use magic instead
                    mov(r8, ptr[rip + magicLabel]);
                    movss(xmm4, dword[r8]);
                    //.text:0000000140850D89                 movaps  xmm3, xmm4
                    // use timer
                    mov(r8, ptr[rip + timerLabel]);
                    movss(xmm3, dword[r8]);

                    // exit 850D8C
                    jmp(ptr[rip + retnLabel]);

                    L(retnLabel);
                    dq(a_hookTarget + 0xB);

                    L(magicLabel);
                    dq(uintptr_t(&MAGIC));

                    L(timerLabel);
                    dq(a_frameTimer);
                }
            };

            REL::Offset<std::uintptr_t> hookTarget(REL::ID(49978), 0x71);
            REL::Offset<float*> noSlowFrameTimer = REL::ID(523661);
            Patch patch(hookTarget.address(), noSlowFrameTimer.address());
            patch.ready();

            auto trampoline = SKSE::GetTrampoline();
            trampoline->Write6Branch(hookTarget.address(), patch.getCode<std::uintptr_t>());

            _VMESSAGE("success");
        }

        static void PatchDragonCameraStateHook()
        {
            _VMESSAGE("patching dragon camera state...");

            struct Patch : SKSE::CodeGenerator
            {
                Patch(std::uintptr_t a_hookTarget, std::uintptr_t a_frameTimer) : SKSE::CodeGenerator()
                {
                    Xbyak::Label retnLabel;
                    Xbyak::Label magicLabel;
                    Xbyak::Label timerLabel;

                    // enter 850D81
                    // r8 is unused
                    //.text:0000000140850D81                 movss   xmm4, cs:frame_timer_without_slow
                    // use magic instead
                    mov(r8, ptr[rip + magicLabel]);
                    movss(xmm4, dword[r8]);
                    //.text:0000000140850D89                 movaps  xmm3, xmm4
                    // use timer
                    mov(r8, ptr[rip + timerLabel]);
                    movss(xmm3, dword[r8]);

                    // exit 850D8C
                    jmp(ptr[rip + retnLabel]);

                    L(retnLabel);
                    dq(a_hookTarget + 0xB);

                    L(magicLabel);
                    dq(uintptr_t(&MAGIC));

                    L(timerLabel);
                    dq(a_frameTimer);
                }
            };

            REL::Offset<std::uintptr_t> hookTarget(REL::ID(32370), 0x5F);
            REL::Offset<float*> noSlowFrameTimer = REL::ID(523661);
            Patch patch(hookTarget.address(), noSlowFrameTimer.address());
            patch.ready();

            auto trampoline = SKSE::GetTrampoline();
            trampoline->Write6Branch(hookTarget.address(), patch.getCode<std::uintptr_t>());

            _VMESSAGE("success");
        }

        static void PatchHorseCameraStateHook()
        {
            _VMESSAGE("patching horse camera state...");

            struct Patch : SKSE::CodeGenerator
            {
                Patch(std::uintptr_t a_hookTarget, std::uintptr_t a_frameTimer) : SKSE::CodeGenerator()
                {
                    Xbyak::Label retnLabel;
                    Xbyak::Label magicLabel;
                    Xbyak::Label timerLabel;

                    // enter 850D81
                    // r8 is unused
                    //.text:0000000140850D81                 movss   xmm4, cs:frame_timer_without_slow
                    // use magic instead
                    mov(r8, ptr[rip + magicLabel]);
                    movss(xmm4, dword[r8]);
                    //.text:0000000140850D89                 movaps  xmm3, xmm4
                    // use timer
                    mov(r8, ptr[rip + timerLabel]);
                    movss(xmm3, dword[r8]);

                    // exit 850D8C
                    jmp(ptr[rip + retnLabel]);

                    L(retnLabel);
                    dq(a_hookTarget + 0xB);

                    L(magicLabel);
                    dq(uintptr_t(&MAGIC));

                    L(timerLabel);
                    dq(a_frameTimer);
                }
            };

            REL::Offset<std::uintptr_t> hookTarget(REL::ID(49839), 0x5F);
            REL::Offset<float*> noSlowFrameTimer = REL::ID(523661);
            Patch patch(hookTarget.address(), noSlowFrameTimer.address());
            patch.ready();

            auto trampoline = SKSE::GetTrampoline();
            trampoline->Write6Branch(hookTarget.address(), patch.getCode<std::uintptr_t>());

            _VMESSAGE("success");
        }

        static inline int MAGIC = 0x3CC0C0C0;  // 1 / 42.5
    };

    bool PatchVerticalLookSensitivity()
    {
        _VMESSAGE("- vertical look sensitivity fix -");

        VerticalLookSensitivityPatch::Install();

        _VMESSAGE("success");
        return true;
    }

    class WeaponBlockScalingPatch
    {
    public:
        static void Install()
        {
            constexpr UInt8 NOP = 0x90;
            constexpr std::size_t START = 0x3B8;
            constexpr std::size_t END = 0x3D1;
            constexpr std::size_t CAVE_SIZE = END - START;

            REL::Offset<std::uintptr_t> target(REL::ID(42842), START);

            struct Patch : SKSE::CodeGenerator
            {
                Patch(std::uintptr_t a_func) : SKSE::CodeGenerator(CAVE_SIZE)
                {
                    // rbx = Actor*

                    mov(rcx, rbx);
                    mov(rdx, a_func);
                    call(rdx);
                    movaps(xmm8, xmm0);
                }
            };

            Patch patch(unrestricted_cast<std::uintptr_t>(CalcWeaponDamage));
            patch.ready();

            for (std::size_t i = 0; i < patch.getSize(); ++i)
            {
                SKSE::SafeWrite8(target.address() + i, patch.getCode()[i]);
            }

            for (std::size_t i = patch.getSize(); i < CAVE_SIZE; ++i)
            {
                SKSE::SafeWrite8(target.address() + i, NOP);
            }
        }

    private:
        static float CalcWeaponDamage(RE::Actor* a_target)
        {
            auto weap = GetWeaponData(a_target);
            if (weap)
            {
                return static_cast<float>(weap->GetAttackDamage());
            }
            else
            {
                return 0.0;
            }
        }

        static RE::TESObjectWEAP* GetWeaponData(RE::Actor* a_actor)
        {
            auto proc = a_actor->currentProcess;
            if (!proc || !proc->middleHigh)
            {
                return nullptr;
            }

            auto middleProc = proc->middleHigh;
            std::array<RE::InventoryEntryData*, 3> entries = {
                middleProc->bothHands,
                middleProc->rightHand,
                middleProc->leftHand
            };

            for (auto& entry : entries)
            {
                if (entry)
                {
                    auto obj = entry->GetObject();
                    if (obj && obj->Is(RE::FormType::Weapon))
                    {
                        return static_cast<RE::TESObjectWEAP*>(obj);
                    }
                }
            }

            return nullptr;
        }
    };

    bool PatchWeaponBlockScaling()
    {
        _VMESSAGE("- weapon block scaling fix -");

        WeaponBlockScalingPatch::Install();

        _VMESSAGE("success");
        return true;
    }

    // Patch for 1.5.97 but should be version-independent
    bool PatchCreateArmorNodeNullptrCrash()
    {
        _VMESSAGE("-fix for crash in CreateArmorNode subfunction-");

        // sub_1401CAFB0
        const uint64_t faddr = offset_CreateArmorNode_subfunc.GetAddress();

        /*
        loc_1401CB520:
          [...]
        .text:00000001401CB538                 mov     esi, ebx
        .text:00000001401CB53A                 test    r12, r12
        .text:00000001401CB53D                 jz      short loc_1401CB5B6 // <-- this jump needs to be patched but the operand is too small to fit
        */
        static const uint8_t expected1[] =
        {
            0x8B, 0xF3,       // mov     esi, ebx
            0x4D, 0x85, 0xE4, // test    r12, r12
            0x74              // jz short (opcode only); +1 byte displacement
        };

        const uint8_t *loc1p = (uint8_t*)(uintptr_t)(faddr + 0x588);
        if(memcmp(loc1p, expected1, sizeof(expected1)))
        {
            _VMESSAGE("Code at first offset is not as expected, skipping patch");
            return false;
        }

        const int8_t disp8 = loc1p[sizeof(expected1)]; // jz short displacement (should be 0x77)
        const uint8_t * const loc1end = loc1p + sizeof(expected1) + 1;

        /*
        loc_1401CB5B6:
        .text:00000001401CB5B6                 mov     rdi, [r12+130h] // <-- this is where the crash happens, because r12 == 0
        .text:00000001401CB5BE                 test    rdi, rdi
        .text:00000001401CB5C1                 jz      loc_1401CB751 // <-- This is the location we want to jump to when test r12,r12 fails
        */

        const void * const loc2p = loc1end + disp8;
        const ptrdiff_t offs2 = (uintptr_t)loc2p - faddr;
        _VMESSAGE("Located second block at offset 0x%x", offs2);

        static const uint8_t expected2[] = // this should use scanning instead; hardcoding offset 130h is bad
        {
            0x49, 0x8B, 0xBC, 0x24, 0x30, 0x01, 0x00, 0x00, // mov     rdi, [r12+130h]
            0x48, 0x85, 0xFF,                               // test    rdi, rdi
            0x0F, 0x84                                      // jz (opcode only); +4 bytes displacement
        };

        if(memcmp(loc2p, expected2, sizeof(expected2)))
        {
            _VMESSAGE("Code at second offset is not as expected, skipping patch");
            return false;
        }

        _VMESSAGE("Code confirmed broken; installing fix");

        const uint8_t * const loc2end = (const uint8_t*)loc2p + sizeof(expected2) + sizeof(int32_t);

        int32_t disp2 = *unrestricted_cast<int32_t*>(((const uint8_t*)loc2p) + sizeof(expected2));
        const uint8_t * const target = loc2end + disp2;

        struct Code : SKSE::CodeGenerator
        {
            Code(std::uintptr_t contAddr, std::uintptr_t patchedAddr) : SKSE::CodeGenerator()
            {
                Xbyak::Label patchedJmpLbl, contLbl, zeroLbl;

                // original instructions minus jz short
                db(expected1, sizeof(expected1) - 1);

                jz(zeroLbl); // patched non-short jz
                jmp(ptr[rip + contLbl]); // continue original code
                L(zeroLbl); // was zero; skip over code that would crash
                jmp(ptr[rip + patchedJmpLbl]);

                L(contLbl);
                dq(contAddr);
                L(patchedJmpLbl);
                dq(patchedAddr);
            }
        };

        Code code(unrestricted_cast<std::uintptr_t>(loc1end), unrestricted_cast<std::uintptr_t>(target));
        code.finalize();

        auto trampoline = SKSE::GetTrampoline();
        if(!trampoline->Write5Branch(unrestricted_cast<std::uintptr_t>(loc1p), uintptr_t(code.getCode())))
            return false;

        _VMESSAGE("success");
        return true;
    }
}
