#pragma once

#include "Scanner.h"

namespace LO
{
	enum class PlacedIn : std::uint8_t
	{
		kOriginalPlace,  // its category is back where its plugin had it (PluginHistory)
		kExistingCategory,  // sorted into a category the mod author made
		kModFolder,         // "Lost Objects" folder inside the mod's own menu
		kTab                // the "Lost Objects" tab in the main menu
	};

	struct Placed
	{
		std::string path;      // "Snappy Builds > Lost Objects > Stairs"
		PlacedIn where{ PlacedIn::kTab };
		std::string evidence;  // why it was (or was not) sorted into a category
	};

	struct ApplyResult
	{
		bool changed{ false };  // menu content differs from before
		bool tabVisible{ false };  // the "Lost Objects" tab is part of WorkshopMenuMain
		std::uint32_t folderCount{ 0 };  // folders in the tab
		std::uint32_t entryCount{ 0 };  // menus + categories created
		std::uint32_t recipeCount{ 0 };  // recovered recipes
		std::uint32_t restoredInPlace{ 0 };  // recipes whose category is back where its plugin had it
		std::uint32_t sortedIn{ 0 };  // recipes sorted into existing categories
		std::uint32_t inModFolder{ 0 };  // recipes in a "Lost Objects" folder of their mod
		std::uint32_t inTab{ 0 };  // recipes in the "Lost Objects" tab
		std::uint32_t overflow{ 0 };  // recipes that did not fit into the pool
		std::uint32_t previewsAdded{ 0 };  // items that got a preview transform (menu thumbnail)

		std::unordered_map<const RE::BGSConstructibleObject*, Placed> placement;
		// Where things were added to existing menus (for the report).
		std::vector<std::string> injections;
		// What was put back where its plugin had it (for the report).
		std::vector<std::string> restores;
	};

	// Everything Lost Objects changes in the game's menus. Built by Apply from
	// a scan, or read back from the snapshot file at the next game start.
	struct MenuState
	{
		std::vector<std::pair<RE::TESForm*, std::string>> names;  // our pool forms -> display name
		std::vector<std::pair<RE::BGSListForm*, std::vector<RE::TESForm*>>> contents;  // our menus (incl. the tab) -> entries
		std::vector<std::pair<RE::BGSListForm*, RE::TESForm*>> injections;  // foreign menu -> our folder added to it

		struct Restored
		{
			RE::BGSListForm* list{ nullptr };
			RE::TESForm* node{ nullptr };  // a category or sub menu of another mod, put back into `list`
			RE::TESForm* after{ nullptr };  // right behind this entry (nullptr: at the end)
			bool operator==(const Restored&) const = default;
		};
		std::vector<Restored> restores;
		std::vector<MergeOp> merges;  // menus of another mod with the same name and similar items, merged (in this order)
		std::vector<std::pair<RE::BGSConstructibleObject*, RE::BGSKeyword*>> tags;  // recipe -> category we add
		bool tabVisible{ false };  // the tab is added to WorkshopMenuMain

		// Same content (the order of names, folders and tags does not matter,
		// the order inside a menu does).
		[[nodiscard]] bool SameAs(const MenuState& a_other) const;
	};

	using PreviewMap = std::unordered_map<RE::TESBoundObject*, RE::BGSTransform*>;

	// Turns a ScanResult into menus.
	//
	// Every recovered recipe gets exactly one extra category:
	//   1. an existing category of the menu that fits it (Matcher), so it
	//      shows up right next to the items it belongs to,
	//   2. otherwise a category in a "Lost Objects" folder inside the plugin's
	//      own top menu ("Snappy Builds > Lost Objects"),
	//   3. otherwise a category in the "Lost Objects" tab, one folder per plugin.
	// The categories of 2. and 3. come from the pool in LostObjects.esp, are
	// named after the original category and, if large, split into groups.
	//
	// Everything happens in memory only. Nothing is stored in the save game;
	// the forms of other mods are only extended with references to our forms,
	// and those references are removed again on every rebuild.
	class MenuBuilder
	{
	public:
		ApplyResult Apply(const OwnedForms& a_forms, const ScanResult& a_scan, const Settings& a_settings);

		// Restores a state saved earlier (snapshot) without a scan. a_signature
		// is the signature Apply computed for it, so the next Apply with the
		// same result changes nothing.
		void Restore(const OwnedForms& a_forms, const MenuState& a_state, const PreviewMap& a_previews, std::size_t a_signature, const ApplyResult& a_summary);

		// What is in the menus right now (for the snapshot).
		[[nodiscard]] const MenuState& State() const noexcept { return _state; }
		[[nodiscard]] const PreviewMap& Previews() const noexcept { return _previewGiven; }
		[[nodiscard]] std::size_t Signature() const noexcept { return _lastSignature; }
		[[nodiscard]] bool HasState() const noexcept { return _lastSignature != 0; }

		// Recipes we tagged, with their original categories.
		[[nodiscard]] const KeywordMap& OriginalKeywords() const noexcept { return _originalKeywords; }

		// What we put back where its plugin had it (the scan does not count it).
		[[nodiscard]] RestoreSet RestoredPairs() const;

		// The scan must see the menus of other mods as their plugins and scripts
		// made them: the merges are taken out before it and put in again after it.
		void SuspendMerges();
		void ResumeMerges();

		// Forget all state (used when the game data is reloaded).
		void Reset();

	private:
		using TagMap = std::unordered_map<RE::BGSConstructibleObject*, RE::BGSKeyword*>;

		void UpdateTags(const TagMap& a_wanted);
		std::uint32_t AddPreviewTransforms(const ScanResult& a_scan, const ApplyResult& a_result);
		void RemoveInjections(const OwnedForms& a_forms);
		void RemoveRestores();
		void ApplyMerges(const std::vector<MergeOp>& a_merges);
		void RemoveMerges();
		void Execute(const OwnedForms& a_forms, const MenuState& a_state);

		KeywordMap _originalKeywords;  // recipe -> categories before we tagged it
		PreviewMap _previewGiven;  // items we gave a preview transform, with that transform
		MenuState _state;  // what Execute put into the menus last
		TagMap _currentTags;  // recipe -> the keyword we added
		std::unordered_set<RE::BGSListForm*> _injectedInto;  // foreign menus we added entries to
		std::vector<MenuState::Restored> _insertedRestores;  // what we really put back (not there before)

		struct AppliedMerge
		{
			MergeOp op;
			std::size_t index{ 0 };  // kRemove: where the node was in the list
		};
		std::vector<AppliedMerge> _appliedMerges;  // what the merges really changed, in order
		std::size_t _lastSignature{ 0 };
		ApplyResult _lastResult;
	};
}
