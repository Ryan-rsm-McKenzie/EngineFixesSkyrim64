#include "patches.h"

#include "offsets.h"

namespace fixes
{
    struct BSRenderPass
    {
        const static std::int32_t MaxLightInArrayC = 16;

        void* m_Shader;
        void* m_Property;
        void* m_Geometry;
        std::uint32_t m_TechniqueID;
        std::uint8_t Byte1C;
        std::uint8_t Byte1D;  // Instance index (offset) in an instance group?
        struct                // LOD information
        {
            std::uint8_t Index : 7;  // Also referred to as "texture degrade level"
            bool SingleLevel : 1;
        } m_Lod;
        std::uint8_t m_LightCount;
        uint16_t Word20;
        BSRenderPass* m_Previous;  // Previous sub-pass
        BSRenderPass* m_Next;      // Next sub-pass
        void** m_SceneLights;      // Pointer to an array of 16 lights (MaxLightInArrayC, restricted to 3?)
        std::uint32_t UnkDword40;  // Set from TLS variable. Pool index in BSRenderPassCache? Almost always zero.
    };

    typedef void (*_BSBatchRenderer_SetupAndDrawPass)(BSRenderPass* pass, std::uint32_t technique, bool alphaTest, std::uint32_t renderFlags);
    REL::Relocation<_BSBatchRenderer_SetupAndDrawPass*> BSBatchRenderer_SetupAndDrawPass_origLoc{ BSBatchRenderer_SetupAndDrawPass_offset };
    _BSBatchRenderer_SetupAndDrawPass BSBatchRenderer_SetupAndDrawPass_Orig;

    REL::Relocation<std::uintptr_t> BSLightingShader_vtbl{ BSLightingShader_vtbl_offset };

    std::uint32_t RAW_FLAG_RIM_LIGHTING = 1 << 11;
    std::uint32_t RAW_FLAG_DO_ALPHA_TEST = 1 << 20;
    std::uint32_t RAW_TECHNIQUE_EYE = 16;
    std::uint32_t RAW_TECHNIQUE_MULTILAYERPARALLAX = 11;
    std::uint32_t RAW_TECHNIQUE_ENVMAP = 1;
    std::uint32_t RAW_TECHNIQUE_PARALLAX = 3;

    void hk_BSBatchRenderer_SetupAndDrawPass(BSRenderPass* pass, std::uint32_t technique, bool alphaTest, std::uint32_t renderFlags)
    {
        if (*(std::uintptr_t*)pass->m_Shader == BSLightingShader_vtbl.address() && alphaTest)
        {
            auto rawTechnique = technique - 0x4800002D;
            auto subIndex = (rawTechnique >> 24) & 0x3F;
            if (subIndex != RAW_TECHNIQUE_EYE && subIndex != RAW_TECHNIQUE_ENVMAP && subIndex != RAW_TECHNIQUE_MULTILAYERPARALLAX && subIndex != RAW_TECHNIQUE_PARALLAX)
            {
                technique = technique | RAW_FLAG_DO_ALPHA_TEST;
                pass->m_TechniqueID = technique;
            }
        }

        BSBatchRenderer_SetupAndDrawPass_Orig(pass, technique, alphaTest, renderFlags);
    }

    bool PatchBSLightingShaderForceAlphaTest()
    {
        logger::trace("- BSLightingShader Force Alpha Testing -"sv);
        {
            struct BSBatchRenderer_SetupAndDrawPass_Code : Xbyak::CodeGenerator
            {
                BSBatchRenderer_SetupAndDrawPass_Code()
                {
                    // 131F810
                    mov(ptr[rsp + 0x10], rbx);
                    mov(ptr[rsp + 0x18], rbp);
                    // 131F81A

                    // exit
                    jmp(ptr[rip]);
                    dq(BSBatchRenderer_SetupAndDrawPass_origLoc.address() + 0xA);
                }
            };

            BSBatchRenderer_SetupAndDrawPass_Code code;
            code.ready();

            auto& trampoline = SKSE::GetTrampoline();
            BSBatchRenderer_SetupAndDrawPass_Orig = _BSBatchRenderer_SetupAndDrawPass(trampoline.allocate(code));
            trampoline.write_branch<6>(
                BSBatchRenderer_SetupAndDrawPass_origLoc.address(),
                reinterpret_cast<std::uintptr_t>(hk_BSBatchRenderer_SetupAndDrawPass));
        }
        logger::trace("patched"sv);
        return true;
    }

    REL::Relocation<std::uintptr_t> BSLightingShader_SetupGeometry_ParallaxFixHookLoc{ offset_BSLightingShader_SetupGeometry_ParallaxTechniqueFix, 0x577 };

    bool PatchBSLightingShaderSetupGeometryParallax()
    {
        logger::trace("- BSLightingShader SetupGeometry - Parallax Bug -"sv);
        {
            struct BSLightingShader_SetupGeometry_Parallax_Code : Xbyak::CodeGenerator
            {
                BSLightingShader_SetupGeometry_Parallax_Code()
                {
                    // orig code
                    and_(eax, 0x21C00);
                    cmovnz(edx, r8d);

                    // new code
                    cmp(ebx, 0x3);    // technique ID = PARALLAX
                    cmovz(edx, r8d);  // set eye update true

                    // jmp out
                    jmp(ptr[rip]);
                    dq(BSLightingShader_SetupGeometry_ParallaxFixHookLoc.address() + 0x9);
                };
            };

            BSLightingShader_SetupGeometry_Parallax_Code code;
            code.ready();

            auto& trampoline = SKSE::GetTrampoline();
            trampoline.write_branch<6>(
                BSLightingShader_SetupGeometry_ParallaxFixHookLoc.address(),
                trampoline.allocate(code));
        }
        logger::trace("patched"sv);
        return true;
    }
}
