#include "NameMap.h"

namespace LO::NameMap
{
	namespace
	{
		using Map = RE::BSTHashMap<const RE::TESForm*, RE::BGSLocalizedString>;

		Map* g_map = nullptr;

		// The two candidates for the address of the table. CommonLibF4 AV adds
		// 0x00 for Next-Gen; on the Anniversary Edition the table was 0x08 lower
		// (as on Old-Gen). Which one is right is not guessed: Initialize uses the
		// one whose lookup of our own keyword gives the name from LostObjects.esp.
		struct Candidate
		{
			Map* map;
			std::string_view name;
		};

		std::array<Candidate, 2> ResolveCandidates()
		{
			// Old-Gen, Next-Gen, Anniversary Edition (address used by libxse/commonlibf4 for 1.11.x).
			const auto id = F4SE::CreateVariantId(226372, 2533149, 2661402);
			const REL::Relocation<Map*> first{ id, F4SE::CreateVariantOffset(-0x08, 0x00, -0x08) };
			const REL::Relocation<Map*> second{ id, F4SE::CreateVariantOffset(0x00, -0x08, 0x00) };
			return { Candidate{ first.get(), "1 (usual)"sv }, Candidate{ second.get(), "2 (0x08 apart)"sv } };
		}

		// Plain lookup, no error handling (called through the guarded wrapper).
		const char* LookupUnguarded(Map* a_map, const RE::TESForm* a_form)
		{
			const auto it = a_map->find(a_form);
			if (it == a_map->end()) {
				return nullptr;
			}
			return it->second.c_str();
		}

		// Looks up a_probe and compares its name with a_expected. Copies what was
		// found into a_found (for the log). No error handling.
		bool ProbeUnguarded(Map* a_map, const RE::TESForm* a_probe, std::string_view a_expected, char* a_found, std::size_t a_foundSize)
		{
			a_found[0] = '\0';
			const char* name = LookupUnguarded(a_map, a_probe);
			if (!name) {
				return a_expected.empty();
			}
			std::size_t i = 0;
			for (; i + 1 < a_foundSize && name[i] != '\0'; ++i) {
				a_found[i] = name[i];
			}
			a_found[i] = '\0';
			return std::string_view(name) == a_expected;
		}

		// Reading through a wrong address must never crash the game, so the
		// check runs under structured exception handling - including reading
		// the name itself. 1 = right table, 0 = wrong name, -1 = reading failed.
		int ProbeGuarded(Map* a_map, const RE::TESForm* a_probe, std::string_view a_expected, char* a_found, std::size_t a_foundSize)
		{
			__try {
				return ProbeUnguarded(a_map, a_probe, a_expected, a_found, a_foundSize) ? 1 : 0;
			} __except (SEH_EXECUTE_HANDLER) {
				return -1;
			}
		}
	}

	bool Initialize(const RE::BGSKeyword* a_probe, std::string_view a_expectedName)
	{
		g_map = nullptr;

		if (!a_probe) {
			REX::LogWarning("NameMap: no probe keyword, names disabled");
			return false;
		}

		for (const auto& candidate : ResolveCandidates()) {
			if (!candidate.map) {
				REX::LogWarning("NameMap: address {} not found", candidate.name);
				continue;
			}

			char found[128];
			const int result = ProbeGuarded(candidate.map, a_probe, a_expectedName, found, sizeof(found));
			if (result < 0) {
				REX::LogWarning("NameMap: reading the name table at address {} failed", candidate.name);
				continue;
			}
			if (result == 0) {
				REX::LogWarning("NameMap: check at address {} failed (expected '{}', found '{}')", candidate.name, a_expectedName, std::string_view(found));
				continue;
			}

			g_map = candidate.map;
			REX::LogInformation("NameMap: name table verified (address {})", candidate.name);
			return true;
		}

		REX::LogWarning("NameMap: no address passed the check, names disabled");
		return false;
	}

	bool IsAvailable() noexcept
	{
		return g_map != nullptr;
	}

	std::string GetName(const RE::TESForm* a_form)
	{
		if (!a_form) {
			return {};
		}

		// Forms with their own name field (furniture, containers, lights, ...).
		if (const auto* fullName = RE::DynamicCast<const RE::TESFullName*>(a_form)) {
			const char* name = fullName->fullName.c_str();
			return name ? std::string(name) : std::string();
		}

		if (!g_map) {
			return {};
		}

		const char* name = LookupUnguarded(g_map, a_form);
		return name ? std::string(name) : std::string();
	}

	bool SetName(const RE::TESForm* a_form, std::string_view a_name)
	{
		if (!g_map || !a_form) {
			return false;
		}

		const auto it = g_map->find(a_form);
		if (it == g_map->end()) {
			return false;
		}

		const char* current = it->second.c_str();
		if (current && std::string_view(current) == a_name) {
			return true;
		}

		const std::string copy(a_name);
		it->second = RE::BGSLocalizedString(copy.c_str());
		return true;
	}
}
