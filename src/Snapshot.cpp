#include "Snapshot.h"

#include "Util.h"

namespace LO::Snapshot
{
	namespace
	{
		constexpr auto FILE_NAME = "LostObjects_Snapshot.txt"sv;
		constexpr auto FORMAT_VERSION = "3"sv;

		std::filesystem::path GetPath()
		{
			return F4SE::GetLogDirectoryPath() / FILE_NAME;
		}

		// FNV-1a, 64 bit: the same on every machine and every run.
		class Hasher
		{
		public:
			void Add(std::string_view a_text)
			{
				for (const unsigned char c : a_text) {
					_hash ^= c;
					_hash *= 0x100000001B3ull;
				}
				_hash ^= 0xFF;  // separator
				_hash *= 0x100000001B3ull;
			}

			[[nodiscard]] std::uint64_t Value() const noexcept { return _hash; }

		private:
			std::uint64_t _hash{ 0xCBF29CE484222325ull };
		};

		// "Fallout4.esm|106DA2" - independent of the load order.
		std::string Ref(const RE::TESForm* a_form)
		{
			const auto* file = a_form ? a_form->GetFile(0) : nullptr;
			if (!file) {
				return {};
			}
			const auto id = a_form->GetFormID();
			const auto local = file->IsLight() ? (id & 0xFFFu) : (id & 0xFFFFFFu);
			return std::format("{}|{:X}", file->filename.data(), local);
		}

		RE::TESForm* Resolve(std::string_view a_ref)
		{
			const auto bar = a_ref.rfind('|');
			if (bar == std::string_view::npos || bar == 0) {
				return nullptr;
			}
			const auto file = a_ref.substr(0, bar);
			const auto idText = a_ref.substr(bar + 1);
			std::uint32_t local = 0;
			const auto [ptr, ec] = std::from_chars(idText.data(), idText.data() + idText.size(), local, 16);
			if (ec != std::errc() || ptr != idText.data() + idText.size()) {
				return nullptr;
			}
			auto* dataHandler = RE::TESDataHandler::GetSingleton();
			return dataHandler ? dataHandler->FindForm(local, file) : nullptr;  // not FindForm<TESForm>: that one only accepts the type "TESForm" itself
		}

		std::vector<std::string_view> SplitTabs(std::string_view a_line)
		{
			std::vector<std::string_view> parts;
			std::size_t start = 0;
			while (start <= a_line.size()) {
				const auto tab = a_line.find('\t', start);
				const auto end = tab == std::string_view::npos ? a_line.size() : tab;
				parts.push_back(a_line.substr(start, end - start));
				if (tab == std::string_view::npos) {
					break;
				}
				start = tab + 1;
			}
			return parts;
		}

		std::string Clean(std::string_view a_text)
		{
			std::string result(a_text);
			for (auto& c : result) {
				if (c == '\t' || c == '\r' || c == '\n') {
					c = ' ';
				}
			}
			return result;
		}

		std::uint64_t ParseHex(std::string_view a_text)
		{
			std::uint64_t value = 0;
			std::from_chars(a_text.data(), a_text.data() + a_text.size(), value, 16);
			return value;
		}

		std::uint32_t ParseUInt(std::string_view a_text)
		{
			std::uint32_t value = 0;
			std::from_chars(a_text.data(), a_text.data() + a_text.size(), value);
			return value;
		}
	}

	std::uint64_t Fingerprint(const Settings& a_settings)
	{
		Hasher hash;
		hash.Add(FORMAT_VERSION);
		hash.Add(F4SE::GetPluginVersion().ToString<char>());

		// Every loaded plugin, in load order.
		if (const auto* dataHandler = RE::TESDataHandler::GetSingleton()) {
			for (const auto* file : dataHandler->compiledFileCollection.files) {
				hash.Add(file ? Util::ToLower(file->filename.data()) : std::string());
			}
			hash.Add("light");
			for (const auto* file : dataHandler->compiledFileCollection.smallFiles) {
				hash.Add(file ? Util::ToLower(file->filename.data()) : std::string());
			}
		}

		// The settings that decide what the menus look like.
		hash.Add(a_settings.restoreOriginalPlaces ? "restore" : "no-restore");
		hash.Add(a_settings.mergeSimilarMenus ? "merge" : "no-merge");
		hash.Add(a_settings.onlyOwnTab ? "only-own-tab" : "other-menus");
		hash.Add(std::format("{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}",
			a_settings.placement == Placement::kTab ? "tab" : "home",
			a_settings.sortMode == SortMode::kFlat ? "flat" : "bymod",
			a_settings.splitThreshold,
			a_settings.maxGroups,
			a_settings.minGroupSize,
			a_settings.skipUntouchedBaseGame,
			a_settings.skipWithoutCategory,
			a_settings.skipAlreadyBuildable,
			a_settings.skipMissingModels,
			a_settings.sortIntoCategories,
			a_settings.minConfidence));
		for (const auto& plugin : a_settings.ignoredPlugins) {
			hash.Add(plugin);
		}
		return hash.Value();
	}

	std::optional<Data> Load(const OwnedForms& a_forms, std::uint64_t a_fingerprint)
	{
		std::ifstream file(GetPath());
		if (!file.is_open()) {
			REX::LogInformation("Snapshot: none yet (it is written after the first build)");
			return std::nullopt;
		}

		Data data;
		bool versionOk = false;
		bool fingerprintOk = false;
		std::uint32_t lineNumber = 0;
		std::string line;

		const auto broken = [&lineNumber](std::string_view a_why) {
			REX::LogInformation("Snapshot: not used ({} in line {}) - the menus are built as usual", a_why, lineNumber);
			return std::nullopt;
		};

		while (std::getline(file, line)) {
			++lineNumber;
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}
			if (line.empty() || line.front() == ';') {
				continue;
			}
			const auto parts = SplitTabs(line);
			const auto key = parts.front();

			if (key == "version") {
				versionOk = parts.size() == 2 && parts[1] == FORMAT_VERSION;
				if (!versionOk) {
					return broken("other format version");
				}
			} else if (key == "fingerprint") {
				fingerprintOk = parts.size() == 2 && ParseHex(parts[1]) == a_fingerprint;
				if (!fingerprintOk) {
					REX::LogInformation("Snapshot: not used - plugins or settings changed since it was written. The menus are built as usual.");
					return std::nullopt;
				}
			} else if (!versionOk || !fingerprintOk) {
				return broken("header missing");
			} else if (key == "signature" && parts.size() == 2) {
				data.signature = static_cast<std::size_t>(ParseHex(parts[1]));
			} else if (key == "summary" && parts.size() == 5) {
				data.summary.recipeCount = ParseUInt(parts[1]);
				data.summary.sortedIn = ParseUInt(parts[2]);
				data.summary.inModFolder = ParseUInt(parts[3]);
				data.summary.inTab = ParseUInt(parts[4]);
			} else if (key == "tab" && parts.size() == 2) {
				data.state.tabVisible = parts[1] == "1";
			} else if (key == "name" && parts.size() == 3) {
				auto* form = Resolve(parts[1]);
				if (!a_forms.IsOwned(form)) {
					return broken("unknown name entry");
				}
				data.state.names.emplace_back(form, std::string(parts[2]));
			} else if (key == "list" && parts.size() >= 2) {
				auto* list = Resolve(parts[1]);
				auto* menu = list ? list->As<RE::BGSListForm>() : nullptr;
				if (!menu || !a_forms.IsOwned(menu)) {
					return broken("unknown menu");
				}
				std::vector<RE::TESForm*> children;
				for (std::size_t i = 2; i < parts.size(); ++i) {
					auto* child = Resolve(parts[i]);
					if (!a_forms.IsOwned(child)) {
						return broken("unknown menu entry");
					}
					children.push_back(child);
				}
				data.state.contents.emplace_back(menu, std::move(children));
			} else if (key == "inject" && parts.size() == 3) {
				auto* target = Resolve(parts[1]);
				auto* targetList = target ? target->As<RE::BGSListForm>() : nullptr;
				auto* node = Resolve(parts[2]);
				if (!targetList || a_forms.IsOwned(targetList) || !a_forms.IsOwned(node)) {
					return broken("unknown folder");
				}
				data.state.injections.emplace_back(targetList, node);
			} else if (key == "restore" && parts.size() == 4) {
				// A category or menu of another mod, put back where its plugin had it.
				auto* listForm = Resolve(parts[1]);
				auto* list = listForm ? listForm->As<RE::BGSListForm>() : nullptr;
				auto* node = Resolve(parts[2]);
				auto* after = parts[3] == "-" ? nullptr : Resolve(parts[3]);
				if (!list || !node || a_forms.IsOwned(list) || a_forms.IsOwned(node) || (parts[3] != "-" && !after)) {
					return broken("unknown restored entry");
				}
				data.state.restores.push_back({ list, node, after });
			} else if (key == "merge" && parts.size() == 4) {
				// A step of merging menus of another mod with the same name.
				auto* listForm = Resolve(parts[2]);
				auto* list = listForm ? listForm->As<RE::BGSListForm>() : nullptr;
				auto* node = Resolve(parts[3]);
				if (!list || !node || a_forms.IsOwned(list) || a_forms.IsOwned(node) || (parts[1] != "insert" && parts[1] != "remove")) {
					return broken("unknown merged entry");
				}
				data.state.merges.push_back({ parts[1] == "insert" ? MergeOp::Kind::kInsert : MergeOp::Kind::kRemove, list, node });
			} else if (key == "tag" && parts.size() == 3) {
				auto* recipeForm = Resolve(parts[1]);
				auto* keywordForm = Resolve(parts[2]);
				auto* recipe = recipeForm ? recipeForm->As<RE::BGSConstructibleObject>() : nullptr;
				auto* keyword = keywordForm ? keywordForm->As<RE::BGSKeyword>() : nullptr;
				if (!recipe || !keyword) {
					return broken("unknown recipe or category");
				}
				data.state.tags.emplace_back(recipe, keyword);
			} else if (key == "preview" && parts.size() == 3) {
				auto* item = RE::DynamicCast<RE::TESBoundObject*>(Resolve(parts[1]));
				auto* transformForm = Resolve(parts[2]);
				auto* transform = transformForm ? transformForm->As<RE::BGSTransform>() : nullptr;
				if (item && transform) {
					data.previews.emplace(item, transform);  // a missing item only loses its thumbnail
				}
			} else {
				return broken("unknown entry");
			}
		}

		if (!fingerprintOk || data.signature == 0) {
			return broken("incomplete file");
		}

		REX::LogInformation("Snapshot: loaded ({} menus, {} folders in other menus, {} put back where their mod had them, {} merge steps, {} recipes)", data.state.contents.size(), data.state.injections.size(), data.state.restores.size(), data.state.merges.size(), data.state.tags.size());
		return data;
	}

	void Save(const MenuBuilder& a_builder, const ApplyResult& a_summary, std::uint64_t a_fingerprint)
	{
		const auto& state = a_builder.State();
		const auto path = GetPath();
		auto temp = path;
		temp += ".tmp";

		{
			std::ofstream file(temp, std::ios::out | std::ios::trunc);
			if (!file.is_open()) {
				REX::LogWarning("Snapshot: could not write {}", temp.filename().string());
				return;
			}

			file << "; Lost Objects - snapshot of the workshop menus it built.\n";
			file << "; Written by the game. It is used at the next game start so the menus are\n";
			file << "; complete right away. Safe to delete: the menus are then built as usual.\n";
			file << "version\t" << FORMAT_VERSION << "\n";
			file << std::format("fingerprint\t{:016X}\n", a_fingerprint);
			file << std::format("signature\t{:016X}\n", static_cast<std::uint64_t>(a_builder.Signature()));
			file << std::format("summary\t{}\t{}\t{}\t{}\n", a_summary.recipeCount, a_summary.sortedIn, a_summary.inModFolder, a_summary.inTab);
			file << "tab\t" << (state.tabVisible ? "1" : "0") << "\n";

			for (const auto& [form, name] : state.names) {
				file << "name\t" << Ref(form) << "\t" << Clean(name) << "\n";
			}
			for (const auto& [menu, children] : state.contents) {
				file << "list\t" << Ref(menu);
				for (const auto* child : children) {
					file << "\t" << Ref(child);
				}
				file << "\n";
			}
			for (const auto& [target, node] : state.injections) {
				file << "inject\t" << Ref(target) << "\t" << Ref(node) << "\n";
			}
			for (const auto& restored : state.restores) {
				file << "restore\t" << Ref(restored.list) << "\t" << Ref(restored.node) << "\t" << (restored.after ? Ref(restored.after) : "-") << "\n";
			}
			for (const auto& merge : state.merges) {
				file << "merge\t" << (merge.kind == MergeOp::Kind::kInsert ? "insert" : "remove") << "\t" << Ref(merge.list) << "\t" << Ref(merge.node) << "\n";
			}
			for (const auto& [recipe, keyword] : state.tags) {
				file << "tag\t" << Ref(recipe) << "\t" << Ref(keyword) << "\n";
			}

			std::vector<std::pair<std::string, std::string>> previews;
			for (const auto& [item, transform] : a_builder.Previews()) {
				previews.emplace_back(Ref(item), Ref(transform));
			}
			std::ranges::sort(previews);
			for (const auto& [item, transform] : previews) {
				file << "preview\t" << item << "\t" << transform << "\n";
			}

			if (!file.good()) {
				REX::LogWarning("Snapshot: writing {} failed", temp.filename().string());
				return;
			}
		}

		std::error_code error;
		std::filesystem::rename(temp, path, error);
		if (error) {
			REX::LogWarning("Snapshot: could not replace {} ({})", path.filename().string(), error.message());
			return;
		}
		REX::LogInformation("Snapshot: saved ({} menus, {} folders in other menus, {} recipes)", state.contents.size(), state.injections.size(), state.tags.size());
	}
}
