#include "PCH.h"

namespace JM::Poison
{
    struct PerkRule
    {
        std::string plugin;
        RE::FormID localFormID{ 0 };
        std::int32_t bonus{ 0 };
        RE::BGSPerk* perk{ nullptr };
    };

    struct Settings
    {
        bool enabled{ true };
        bool debugNotifications{ false };
        bool includeVanillaDose{ true };
        std::int32_t minimumDoses{ 1 };
        std::int32_t maximumFiniteDoses{ 20 };

        // Alchemy skill thresholds. Highest matching threshold wins.
        std::vector<std::pair<float, std::int32_t>> alchemyBonuses{
            { 25.0f, 1 },
            { 50.0f, 2 },
            { 75.0f, 3 },
            { 100.0f, 4 }
        };

        std::vector<PerkRule> perkRules;
        std::string infinitePlugin{ "JM_Sithis_Overhaul.esp" };
        RE::FormID infiniteLocalFormID{ 0x00000804 };
        RE::BGSPerk* infinitePerk{ nullptr };
    };

    Settings g_settings;

    std::string Trim(std::string value)
    {
        auto notSpace = [](unsigned char c) { return !std::isspace(c); };
        value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
        value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
        return value;
    }

    std::string Lower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }

    bool ParseBool(const std::string& text, bool fallback)
    {
        auto value = Lower(Trim(text));
        if (value == "1" || value == "true" || value == "yes" || value == "on") {
            return true;
        }
        if (value == "0" || value == "false" || value == "no" || value == "off") {
            return false;
        }
        return fallback;
    }

    std::optional<std::uint32_t> ParseUInt(const std::string& text)
    {
        try {
            std::size_t consumed = 0;
            const auto trimmed = Trim(text);
            const auto value = std::stoul(trimmed, &consumed, 0);
            if (consumed != trimmed.size()) {
                return std::nullopt;
            }
            return static_cast<std::uint32_t>(value);
        } catch (...) {
            return std::nullopt;
        }
    }

    std::optional<std::int32_t> ParseInt(const std::string& text)
    {
        try {
            std::size_t consumed = 0;
            const auto trimmed = Trim(text);
            const auto value = std::stol(trimmed, &consumed, 0);
            if (consumed != trimmed.size()) {
                return std::nullopt;
            }
            return static_cast<std::int32_t>(value);
        } catch (...) {
            return std::nullopt;
        }
    }

    std::optional<float> ParseFloat(const std::string& text)
    {
        try {
            std::size_t consumed = 0;
            const auto trimmed = Trim(text);
            const auto value = std::stof(trimmed, &consumed);
            if (consumed != trimmed.size()) {
                return std::nullopt;
            }
            return value;
        } catch (...) {
            return std::nullopt;
        }
    }

    using IniMap = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

    IniMap ReadIni(const std::filesystem::path& path)
    {
        IniMap result;
        std::ifstream input(path);
        if (!input.is_open()) {
            return result;
        }

        std::string section;
        std::string line;
        while (std::getline(input, line)) {
            line = Trim(line);
            if (line.empty() || line[0] == ';' || line[0] == '#') {
                continue;
            }
            if (line.front() == '[' && line.back() == ']') {
                section = Lower(Trim(line.substr(1, line.size() - 2)));
                continue;
            }

            const auto equals = line.find('=');
            if (equals == std::string::npos) {
                continue;
            }

            auto key = Lower(Trim(line.substr(0, equals)));
            auto value = Trim(line.substr(equals + 1));

            const auto semicolon = value.find(';');
            if (semicolon != std::string::npos) {
                value = Trim(value.substr(0, semicolon));
            }
            result[section][key] = value;
        }
        return result;
    }

    std::optional<std::string> IniValue(const IniMap& ini, std::string_view section, std::string_view key)
    {
        const auto sectionIt = ini.find(Lower(std::string(section)));
        if (sectionIt == ini.end()) {
            return std::nullopt;
        }
        const auto keyIt = sectionIt->second.find(Lower(std::string(key)));
        if (keyIt == sectionIt->second.end()) {
            return std::nullopt;
        }
        return keyIt->second;
    }

    void LoadSettings()
    {
        const auto path = std::filesystem::path("Data/SKSE/Plugins/JM_Poison.ini");
        const auto ini = ReadIni(path);

        if (auto v = IniValue(ini, "General", "Enabled")) {
            g_settings.enabled = ParseBool(*v, g_settings.enabled);
        }
        if (auto v = IniValue(ini, "General", "DebugNotifications")) {
            g_settings.debugNotifications = ParseBool(*v, g_settings.debugNotifications);
        }
        if (auto v = IniValue(ini, "General", "IncludeVanillaDose")) {
            g_settings.includeVanillaDose = ParseBool(*v, g_settings.includeVanillaDose);
        }
        if (auto v = IniValue(ini, "General", "MinimumDoses")) {
            if (auto parsed = ParseInt(*v)) {
                g_settings.minimumDoses = (std::max)(1, *parsed);
            }
        }
        if (auto v = IniValue(ini, "General", "MaximumFiniteDoses")) {
            if (auto parsed = ParseInt(*v)) {
                g_settings.maximumFiniteDoses = (std::max)(g_settings.minimumDoses, *parsed);
            }
        }

        g_settings.alchemyBonuses.clear();
        for (int i = 1; i <= 8; ++i) {
            const auto thresholdKey = "Threshold" + std::to_string(i);
            const auto bonusKey = "Bonus" + std::to_string(i);
            const auto threshold = IniValue(ini, "Alchemy", thresholdKey);
            const auto bonus = IniValue(ini, "Alchemy", bonusKey);
            if (!threshold || !bonus) {
                continue;
            }
            const auto parsedThreshold = ParseFloat(*threshold);
            const auto parsedBonus = ParseInt(*bonus);
            if (parsedThreshold && parsedBonus) {
                g_settings.alchemyBonuses.emplace_back(*parsedThreshold, *parsedBonus);
            }
        }
        if (g_settings.alchemyBonuses.empty()) {
            g_settings.alchemyBonuses = {
                { 25.0f, 1 }, { 50.0f, 2 }, { 75.0f, 3 }, { 100.0f, 4 }
            };
        }
        std::sort(g_settings.alchemyBonuses.begin(), g_settings.alchemyBonuses.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

        g_settings.perkRules.clear();
        for (int i = 1; i <= 8; ++i) {
            const auto prefix = "Tier" + std::to_string(i);
            const auto plugin = IniValue(ini, "Perks", prefix + "Plugin");
            const auto formID = IniValue(ini, "Perks", prefix + "FormID");
            const auto bonus = IniValue(ini, "Perks", prefix + "Bonus");
            if (!plugin || !formID || !bonus) {
                continue;
            }
            const auto parsedForm = ParseUInt(*formID);
            const auto parsedBonus = ParseInt(*bonus);
            if (parsedForm && parsedBonus) {
                g_settings.perkRules.push_back(PerkRule{ *plugin, *parsedForm, *parsedBonus, nullptr });
            }
        }

        if (auto v = IniValue(ini, "Infinite", "Plugin")) {
            g_settings.infinitePlugin = *v;
        }
        if (auto v = IniValue(ini, "Infinite", "FormID")) {
            if (auto parsed = ParseUInt(*v)) {
                g_settings.infiniteLocalFormID = *parsed;
            }
        }

        logger::info("JM_Poison settings loaded: enabled={}, includeVanillaDose={}, finite cap={}",
            g_settings.enabled, g_settings.includeVanillaDose, g_settings.maximumFiniteDoses);
    }

    void ResolvePerks()
    {
        auto* data = RE::TESDataHandler::GetSingleton();
        if (!data) {
            logger::error("TESDataHandler unavailable; perk resolution failed");
            return;
        }

        for (auto& rule : g_settings.perkRules) {
            rule.perk = data->LookupForm<RE::BGSPerk>(rule.localFormID, rule.plugin);
            logger::info("Perk tier {}:{:08X} => {}", rule.plugin, rule.localFormID,
                rule.perk ? "resolved" : "NOT FOUND");
        }

        g_settings.infinitePerk = data->LookupForm<RE::BGSPerk>(
            g_settings.infiniteLocalFormID, g_settings.infinitePlugin);
        logger::info("Infinite perk {}:{:08X} => {}", g_settings.infinitePlugin,
            g_settings.infiniteLocalFormID, g_settings.infinitePerk ? "resolved" : "NOT FOUND");
    }

    bool HasInfinite(RE::PlayerCharacter* player)
    {
        return g_settings.enabled && player && g_settings.infinitePerk && player->HasPerk(g_settings.infinitePerk);
    }

    std::int32_t AlchemyBonus(RE::PlayerCharacter* player)
    {
        if (!player) {
            return 0;
        }
        const float skill = player->GetActorValue(RE::ActorValue::kAlchemy);
        std::int32_t bonus = 0;
        for (const auto& [threshold, value] : g_settings.alchemyBonuses) {
            if (skill >= threshold) {
                bonus = value;
            } else {
                break;
            }
        }
        return bonus;
    }

    std::int32_t PerkBonus(RE::PlayerCharacter* player)
    {
        if (!player) {
            return 0;
        }

        // Sithis currently grants exactly one rank-state perk, but this is intentionally
        // generic. If another setup uses cumulative perks, the largest configured bonus wins.
        std::int32_t best = 0;
        for (const auto& rule : g_settings.perkRules) {
            if (rule.perk && player->HasPerk(rule.perk)) {
                best = (std::max)(best, rule.bonus);
            }
        }
        return best;
    }

    std::int32_t CalculateFiniteDoses(RE::PlayerCharacter* player, float vanillaDose)
    {
        std::int32_t base = g_settings.includeVanillaDose ?
            (std::max)(1, static_cast<std::int32_t>(std::lround(vanillaDose))) : 1;

        const auto result = base + AlchemyBonus(player) + PerkBonus(player);
        return (std::clamp)(result, g_settings.minimumDoses, g_settings.maximumFiniteDoses);
    }

    RE::ExtraPoison* GetExtraPoison(RE::InventoryEntryData* item)
    {
        if (!item || !item->extraLists || item->extraLists->empty()) {
            return nullptr;
        }
        auto* list = item->extraLists->front();
        return list ? list->GetByType<RE::ExtraPoison>() : nullptr;
    }

    struct ApplyDoseHook
    {
        static void Install()
        {
            // Skyrim SE 1.5.97 path used by the poison-application callback.
            // Same relocation family used by Poisoner's Aid; this hook runs after
            // the game's own Mod Poison Dose Count perk evaluation.
            REL::Relocation<std::uintptr_t> target{ REL::RelocationID(39407, 40482), REL::VariantOffset(0xB0, 0x9D, 0xB0) };
            auto& trampoline = SKSE::GetTrampoline();
            _original = trampoline.write_call<5>(target.address(), Thunk);
            logger::info("ApplyDoseHook installed");
        }

        static void Thunk(RE::BGSEntryPoint::ENTRY_POINT entryPoint, RE::PlayerCharacter* player,
            RE::TESObjectWEAP* weapon, RE::AlchemyItem* poison, float& out)
        {
            _original(entryPoint, player, weapon, poison, out);

            if (!g_settings.enabled || !player || !player->IsPlayerRef() || !poison) {
                return;
            }

            if (HasInfinite(player)) {
                // Keep application conventional. The consumption hook below makes it permanent.
                out = static_cast<float>(g_settings.minimumDoses);
                if (g_settings.debugNotifications) {
                    RE::DebugNotification("JM Poison: infinite coating");
                }
                return;
            }

            const auto calculated = CalculateFiniteDoses(player, out);
            out = static_cast<float>(calculated);

            if (g_settings.debugNotifications) {
                std::string message = "JM Poison: " + std::to_string(calculated) + " doses";
                RE::DebugNotification(message.c_str());
            }
        }

        static inline REL::Relocation<decltype(Thunk)> _original;
    };

    struct ConsumePoisonHook
    {
        template <std::size_t Index>
        static RE::AlchemyItem* Thunk(RE::InventoryEntryData* item, RE::Character* aggressor)
        {
            auto* poison = _original[Index](item);

            if (!g_settings.enabled || !poison || !aggressor || !aggressor->IsPlayerRef()) {
                return poison;
            }

            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!HasInfinite(player)) {
                return poison;
            }

            // The game decrements ExtraPoison immediately after this lookup. Refreshing the
            // count to 2 here means vanilla decrements it to 1, never reaching zero/removal.
            if (auto* extraPoison = GetExtraPoison(item)) {
                if (extraPoison->count < 2) {
                    extraPoison->count = 2;
                }
            }

            return poison;
        }

        static void Install()
        {
            auto& trampoline = SKSE::GetTrampoline();

            // The original call being replaced only receives InventoryEntryData*.
            // On SE 1.5.97 the current aggressor is still held in a register at these
            // two call sites. Tiny stubs move that register into RDX so Thunk receives
            // both (item, aggressor), matching the proven Poisoner's Aid pattern.
            struct BowStub : Xbyak::CodeGenerator
            {
                explicit BowStub(std::uintptr_t target)
                {
                    mov(rdx, rsi);
                    mov(rax, target);
                    jmp(rax);
                }
            };

            struct MeleeStub : Xbyak::CodeGenerator
            {
                explicit MeleeStub(std::uintptr_t target)
                {
                    mov(rdx, rbx);
                    mov(rax, target);
                    jmp(rax);
                }
            };

            static BowStub bowStub{ reinterpret_cast<std::uintptr_t>(Thunk<0>) };
            static MeleeStub meleeStub{ reinterpret_cast<std::uintptr_t>(Thunk<1>) };

            // Bow/arrow poison-consumption path.
            REL::Relocation<std::uintptr_t> bow{ REL::RelocationID(41778, 42859), REL::VariantOffset(0x11A, 0x11F, 0x11A) };
            _original[0] = trampoline.write_call<5>(bow.address(), reinterpret_cast<std::uintptr_t>(bowStub.getCode()));

            // Melee poison-consumption path.
            REL::Relocation<std::uintptr_t> melee{ REL::RelocationID(37799, 38748), REL::VariantOffset(0x148, 0x153, 0x148) };
            _original[1] = trampoline.write_call<5>(melee.address(), reinterpret_cast<std::uintptr_t>(meleeStub.getCode()));

            logger::info("ConsumePoisonHook installed for bow and melee paths");
        }

        static inline REL::Relocation<RE::AlchemyItem*(RE::InventoryEntryData*)> _original[2];
    };

    void Install()
    {
        LoadSettings();
        ResolvePerks();

        if (!g_settings.enabled) {
            logger::info("JM_Poison disabled by INI");
            return;
        }

        SKSE::AllocTrampoline(128);
        ApplyDoseHook::Install();
        ConsumePoisonHook::Install();
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    SKSE::Init(skse);

    if (const auto logDir = SKSE::log::log_directory()) {
        auto path = *logDir / "JM_Poison.log";
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.string(), true);
        auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));
        spdlog::set_default_logger(std::move(log));
        spdlog::set_level(spdlog::level::info);
        spdlog::flush_on(spdlog::level::info);
    }

    logger::info("JM_Poison loading");

    const auto runtime = REL::Module::get().version();
    if (runtime != SKSE::RUNTIME_SSE_1_5_97) {
        logger::critical("Unsupported Skyrim runtime {}. JM_Poison is intentionally built for 1.5.97.", runtime.string());
        return false;
    }

    const auto* messaging = SKSE::GetMessagingInterface();
    if (!messaging) {
        logger::critical("SKSE messaging interface unavailable");
        return false;
    }

    messaging->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        if (message && message->type == SKSE::MessagingInterface::kDataLoaded) {
            JM::Poison::Install();
        }
    });

    return true;
}