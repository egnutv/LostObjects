#include "ModelCheck.h"

#include "Util.h"

namespace LO::ModelCheck
{
	namespace
	{
		constexpr double MIN_FOUND_RATIO = 0.7;  // self-test: at least 70 % of the visible models must be found
		constexpr std::size_t MIN_SAMPLE = 20;

		enum class State
		{
			kUntested,
			kActive,
			kOff
		};

		State g_state{ State::kUntested };
		std::string g_status{ "not run yet" };
		std::unordered_map<std::string, bool> g_cache;  // lower-case path -> exists

		// "SetDressing\Foo.nif" -> "Meshes\SetDressing\Foo.nif"
		std::string FullPath(std::string_view a_model)
		{
			std::string path(a_model);
			std::ranges::replace(path, '/', '\\');
			while (path.starts_with('\\')) {
				path.erase(0, 1);
			}
			if (!Util::ToLower(path).starts_with("meshes\\")) {
				path.insert(0, "Meshes\\");
			}
			return path;
		}

		bool OpenUnguarded(const char* a_path)
		{
			RE::BSTSmartPointer<RE::BSResource::Stream> stream;
			const auto error = RE::BSResource::GetOrCreateStream(a_path, stream, false, nullptr);
			return error == RE::BSResource::ErrorCode::kNone && static_cast<bool>(stream);
		}

		// 1 = exists, 0 = not found, -1 = the game function crashed
		int OpenGuarded(const char* a_path)
		{
			__try {
				return OpenUnguarded(a_path) ? 1 : 0;
			} __except (SEH_EXECUTE_HANDLER) {
				return -1;
			}
		}

		// -1 = check failed (turn off), else 0/1
		int Exists(std::string_view a_model)
		{
			const auto path = FullPath(a_model);
			auto key = Util::ToLower(path);
			if (const auto it = g_cache.find(key); it != g_cache.end()) {
				return it->second ? 1 : 0;
			}
			const auto result = OpenGuarded(path.c_str());
			if (result < 0) {
				return -1;
			}
			g_cache.emplace(std::move(key), result == 1);
			return result;
		}

		void TurnOff(std::string a_reason)
		{
			g_state = State::kOff;
			g_status = std::move(a_reason);
			REX::LogWarning("ModelCheck: {} - ghost objects are not hidden", g_status);
		}
	}

	void SelfTest(const std::vector<std::string>& a_visibleModels)
	{
		if (g_state != State::kUntested) {
			return;
		}

		if (a_visibleModels.size() < MIN_SAMPLE) {
			g_status = std::format("not run yet (only {} test files)", a_visibleModels.size());
			return;  // try again on the next scan
		}

		std::size_t found = 0;
		for (const auto& model : a_visibleModels) {
			const auto result = Exists(model);
			if (result < 0) {
				TurnOff("off: the game's file lookup failed");
				return;
			}
			found += static_cast<std::size_t>(result);
		}

		const auto ratio = static_cast<double>(found) / static_cast<double>(a_visibleModels.size());
		if (ratio < MIN_FOUND_RATIO) {
			TurnOff(std::format("off: self-test found only {} of {} model files of visible items", found, a_visibleModels.size()));
			return;
		}

		g_state = State::kActive;
		g_status = std::format("active (self-test: {} of {} model files of visible items found)", found, a_visibleModels.size());
		REX::LogInformation("ModelCheck: {}", g_status);
	}

	int FileExists(std::string_view a_modelPath)
	{
		return Exists(a_modelPath);
	}

	bool IsActive() noexcept
	{
		return g_state == State::kActive;
	}

	const std::string& Status() noexcept
	{
		return g_status;
	}

	std::string FindProblem(const RE::BGSConstructibleObject* a_recipe)
	{
		if (!IsActive() || !a_recipe || !a_recipe->createdItem) {
			return {};
		}

		std::vector<const RE::TESForm*> variants;
		Util::CollectVariants(a_recipe->createdItem, variants);
		if (variants.empty()) {
			return a_recipe->createdItem->As<RE::BGSListForm>() ? "empty variant list, nothing to build"s : std::string();
		}

		std::vector<std::string> missing;
		for (const auto* variant : variants) {
			const auto* model = RE::DynamicCast<const RE::TESModel*>(variant);
			if (!model) {
				return {};  // a form without a model (an NPC, ...): cannot be judged, keep it
			}
			const char* path = model->GetModel();
			if (!path || !*path) {
				missing.emplace_back(std::format("{} has no model", Util::FormIDToString(variant->GetFormID())));
				continue;
			}
			const auto result = Exists(path);
			if (result < 0) {
				TurnOff("off: the game's file lookup failed");
				return {};
			}
			if (result == 1) {
				return {};  // at least one variant can be seen
			}
			missing.push_back(FullPath(path));
		}

		std::string reason = "model file not found: " + missing.front();
		if (missing.size() > 1) {
			reason += std::format(" (+{} more variants)", missing.size() - 1);
		}
		return reason;
	}
}
