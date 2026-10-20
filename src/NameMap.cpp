#include "NameMap.h"

namespace LO::NameMap
{
	namespace
	{
		using Map = RE::BSTHashMap<const RE::TESForm*, RE::BGSLocalizedString>;

		Map* g_map = nullptr;

		Map* ResolveAddress()
		{
			// Old-Gen and Next-Gen: same as CommonLibF4 AV.
			// Anniversary Edition: address used by libxse/commonlibf4 for 1.11.x.
			const auto id = F4SE::CreateVariantId(226372, 2533149, 2661402);
			const auto offset = F4SE::CreateVariantOffset(-0x08, 0x00, -0x08);
			const REL::Relocation<Map*> relocation{ id, offset };
			return relocation.get();
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

		// Reading through a wrong address must never crash the game, so the
		// first lookup runs under structured exception handling.
		const char* LookupGuarded(Map* a_map, const RE::TESForm* a_form, bool& a_failed)
		{
			a_failed = false;
			__try {
				return LookupUnguarded(a_map, a_form);
			} __except (SEH_EXECUTE_HANDLER) {
				a_failed = true;
				return nullptr;
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

		auto* candidate = ResolveAddress();
		if (!candidate) {
			REX::LogWarning("NameMap: address not found, names disabled");
			return false;
		}

		bool failed = false;
		const char* name = LookupGuarded(candidate, a_probe, failed);
		if (failed) {
			REX::LogWarning("NameMap: reading the name table failed, names disabled");
			return false;
		}

		const std::string_view found = name ? std::string_view(name) : std::string_view();
		if (found != a_expectedName) {
			REX::LogWarning("NameMap: check failed (expected '{}', found '{}'), names disabled", a_expectedName, found);
			return false;
		}

		g_map = candidate;
		REX::LogInformation("NameMap: name table verified");
		return true;
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
