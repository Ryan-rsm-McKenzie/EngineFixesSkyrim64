#pragma once

class config
{
private:
    class ISetting
    {
    public:
        ISetting() = delete;
        ISetting(const ISetting&) = delete;
        ISetting(ISetting&&) = delete;

        inline ISetting(std::string a_group, std::string a_key)
            : _group(std::move(a_group)),
              _key(std::move(a_key))
        {
            auto& settings = get_settings();
            settings.push_back(this);
        }

        inline virtual ~ISetting() = 0 {}

        ISetting& operator=(const ISetting&) = delete;
        ISetting& operator=(ISetting&&) = delete;

        static inline void load_all(const INIReader& a_ini)
        {
            auto& settings = get_settings();
            for (auto& setting : settings)
            {
                setting->load(a_ini);
            }
        }

        inline void load(const INIReader& a_ini)
        {
            do_load(a_ini);
        }

    protected:
        [[nodiscard]] constexpr const std::string& group() const noexcept { return _group; }
        [[nodiscard]] constexpr const std::string& key() const noexcept { return _key; }

        virtual void do_load(const INIReader& a_ini) = 0;

    private:
        static std::vector<ISetting*>& get_settings()
        {
            static std::vector<ISetting*> settings;
            return settings;
        }

        std::string _group;
        std::string _key;
    };

    class bSetting : ISetting
    {
    private:
        using super = ISetting;

    public:
        using value_type = bool;

        bSetting() = delete;
        bSetting(const bSetting&) = delete;
        bSetting(bSetting&&) = delete;

        inline bSetting(std::string a_group, std::string a_key, value_type a_value)
            : super(std::move(a_group), std::move(a_key)),
              _value(a_value)
        {}

        ~bSetting() = default;

        bSetting& operator=(const bSetting&) = delete;
        bSetting& operator=(bSetting&&) = delete;

        [[nodiscard]] constexpr value_type& operator*() noexcept { return _value; }
        [[nodiscard]] constexpr const value_type& operator*() const noexcept { return _value; }

    protected:
        void do_load(const INIReader& a_ini) override
        {
            _value = a_ini.GetBoolean(group(), key(), _value);
        }

    private:
        value_type _value;
    };

    class fSetting : ISetting
    {
    private:
        using super = ISetting;

    public:
        using value_type = float;

        fSetting() = delete;
        fSetting(const fSetting&) = delete;
        fSetting(fSetting&&) = delete;

        inline fSetting(std::string a_group, std::string a_key, value_type a_value)
            : super(std::move(a_group), std::move(a_key)),
              _value(a_value)
        {}

        ~fSetting() = default;

        fSetting& operator=(const fSetting&) = delete;
        fSetting& operator=(fSetting&&) = delete;

        [[nodiscard]] constexpr value_type& operator*() noexcept { return _value; }
        [[nodiscard]] constexpr const value_type& operator*() const noexcept { return _value; }

    protected:
        void do_load(const INIReader& a_ini) override
        {
            _value = a_ini.GetReal(group(), key(), _value);
        }

    private:
        value_type _value;
    };

    class iSetting : ISetting
    {
    private:
        using super = ISetting;

    public:
        using value_type = long;

        iSetting() = delete;
        iSetting(const iSetting&) = delete;
        iSetting(iSetting&&) = delete;

        inline iSetting(std::string a_group, std::string a_key, value_type a_value)
            : super(std::move(a_group), std::move(a_key)),
              _value(a_value)
        {}

        ~iSetting() = default;

        iSetting& operator=(const iSetting&) = delete;
        iSetting& operator=(iSetting&&) = delete;

        [[nodiscard]] constexpr value_type& operator*() noexcept { return _value; }
        [[nodiscard]] constexpr const value_type& operator*() const noexcept { return _value; }

    protected:
        void do_load(const INIReader& a_ini) override
        {
            _value = a_ini.GetInteger(group(), key(), _value);
        }

    private:
        value_type _value;
    };

    class sSetting : ISetting
    {
    private:
        using super = ISetting;

    public:
        using value_type = std::string;

        sSetting() = delete;
        sSetting(const sSetting&) = delete;
        sSetting(sSetting&&) = delete;

        inline sSetting(std::string a_group, std::string a_key, value_type a_value)
            : super(std::move(a_group), std::move(a_key)),
              _value(std::move(a_value))
        {}

        ~sSetting() = default;

        sSetting& operator=(const sSetting&) = delete;
        sSetting& operator=(sSetting&&) = delete;

        [[nodiscard]] constexpr value_type& operator*() noexcept { return _value; }
        [[nodiscard]] constexpr const value_type& operator*() const noexcept { return _value; }

        [[nodiscard]] constexpr value_type* operator->() noexcept { return std::addressof(_value); }
        [[nodiscard]] constexpr const value_type* operator->() const noexcept { return std::addressof(_value); }

    protected:
        void do_load(const INIReader& a_ini) override
        {
            _value = a_ini.Get(group(), key(), _value);
        }

    private:
        value_type _value;
    };

public:
    // EngineFixes
    static inline bSetting verboseLogging{ "EngineFixes", "VerboseLogging", false };
    static inline bSetting cleanSKSECosaves{ "EngineFixes", "CleanSKSECosaves", true };

    // Patches
    static inline bSetting patchDisableChargenPrecache{ "Patches", "DisableChargenPrecache", false };
    static inline bSetting patchEnableAchievementsWithMods{ "Patches", "EnableAchievementsWithMods", true };
    static inline bSetting patchFormCaching{ "Patches", "FormCaching", true };
    static inline bSetting patchMaxStdio{ "Patches", "MaxStdio", true };
    static inline bSetting patchRegularQuicksaves{ "Patches", "RegularQuicksaves", false };
    static inline bSetting patchSaveAddedSoundCategories{ "Patches", "SaveAddedSoundCategories", true };
    static inline bSetting patchScrollingDoesntSwitchPOV{ "Patches", "ScrollingDoesntSwitchPOV", false };
    static inline bSetting patchSleepWaitTime{ "Patches", "SleepWaitTime", false };
    static inline fSetting sleepWaitTimeModifier{ "Patches", "SleepWaitTimeModifier", 0.3 };
    static inline bSetting patchTreeLODReferenceCaching{ "Patches", "TreeLODReferenceCaching", true };
    static inline bSetting patchWaterflowAnimation{ "Patches", "WaterflowAnimation", true };
    static inline fSetting waterflowSpeed{ "Patches", "WaterflowSpeed", 20.0 };

    // Fixes
    static inline bSetting fixAnimationLoadSignedCrash{ "Fixes", "AnimationLoadSignedCrash", true };
    static inline bSetting fixArcheryDownwardAiming{ "Fixes", "ArcheryDownwardAiming", true };
    static inline bSetting fixBethesdaNetCrash{ "Fixes", "BethesdaNetCrash", true };
    static inline bSetting fixBSLightingAmbientSpecular{ "Fixes", "BSLightingAmbientSpecular", true };
    static inline bSetting fixBSLightingShaderForceAlphaTest{ "Fixes", "BSLightingShaderForceAlphaTest", true };
    static inline bSetting fixBSLightingShaderGeometryParallaxBug{ "Fixes", "BSLightingShaderParallaxBug", true };
    static inline bSetting fixBSTempEffectNiRTTI{ "Fixes", "BSTempEffectNiRTTI", true };
    static inline bSetting fixCalendarSkipping{ "Fixes", "CalendarSkipping", true };
    static inline bSetting fixCellInit{ "Fixes", "CellInit", true };
    static inline bSetting fixCreateArmorNodeNullptrCrash{ "Fixes", "CreateArmorNodeNullptrCrash", true };
    static inline bSetting fixConjurationEnchantAbsorbs{ "Fixes", "ConjurationEnchantAbsorbs", true };
    static inline bSetting fixDoublePerkApply{ "Fixes", "DoublePerkApply", true };
    static inline bSetting fixEquipShoutEventSpam{ "Fixes", "EquipShoutEventSpam", true };
    static inline bSetting fixGetKeywordItemCount{ "Fixes", "GetKeywordItemCount", true };
    static inline bSetting fixGHeapLeakDetectionCrash{ "Fixes", "GHeapLeakDetectionCrash", true };
    static inline bSetting fixLipSync{ "Fixes", "LipSync", true };
    static inline bSetting fixMemoryAccessErrors{ "Fixes", "MemoryAccessErrors", true };
    static inline bSetting fixMO5STypo{ "Fixes", "MO5STypo", true };
    static inline bSetting fixNullProcessCrash{ "Fixes", "NullProcessCrash", true };
    static inline bSetting fixPerkFragmentIsRunning{ "Fixes", "PerkFragmentIsRunning", true };
    static inline bSetting fixRemovedSpellBook{ "Fixes", "RemovedSpellBook", true };
    static inline bSetting fixSaveScreenshots{ "Fixes", "SaveScreenshots", true };
    static inline bSetting fixSlowTimeCameraMovement{ "Fixes", "SlowTimeCameraMovement", true };
    static inline bSetting fixTorchLandscape{ "Fixes", "TorchLandscape", true };
    static inline bSetting fixTreeReflections{ "Fixes", "TreeReflections", true };
    static inline bSetting fixVerticalLookSensitivity{ "Fixes", "VerticalLookSensitivity", true };
    static inline bSetting fixWeaponBlockScaling{ "Fixes", "WeaponBlockScaling", true };

    // Warnings
    static inline bSetting warnDupeAddonNodes{ "Warnings", "DupeAddonNodes", true };
    static inline bSetting warnRefHandleLimit{ "Warnings", "RefHandleLimit", true };
    static inline iSetting warnRefrMainMenuLimit{ "Warnings", "RefrMainMenuLimit", 800000 };
    static inline iSetting warnRefrLoadedGameLimit{ "Warnings", "RefrLoadedGameLimit", 1000000 };

    // Experimental
    static inline bSetting experimentalMemoryManager{ "Experimental", "MemoryManager", false };
    static inline bSetting experimentalUseTBBMalloc{ "Experimental", "UseTBBMalloc", true };
    static inline bSetting experimentalSaveGameMaxSize{ "Experimental", "SaveGameMaxSize", false };
    static inline bSetting experimentalTreatAllModsAsMasters{ "Experimental", "TreatAllModsAsMasters", false };

    static inline bool LoadConfig(const std::string& path)
    {
        const INIReader ini(path);

        if (const auto parse = ini.ParseError(); parse == 0)
        {
            ISetting::load_all(ini);
            return true;
        }
        else if (parse == -1)
        {
            _MESSAGE("unable to read ini file at path %s", path.c_str());
            return false;
        }
        else
        {
            _MESSAGE("error while reading ini file at position: %i", parse);
            return false;
        }
    }
};
