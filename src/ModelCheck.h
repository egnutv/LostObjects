#pragma once

// Finds "ghost objects": recipes whose item has no model file the game can
// load, so the build menu shows an empty slot and nothing can be seen.
//
// The check asks the game's own file system (loose files in Data and all
// loaded .ba2 archives) whether the model file exists. It only runs after a
// self-test: the models of items that are visible in the menu right now must
// be found. If the self-test fails, the check stays off and nothing is hidden.

namespace LO::ModelCheck
{
	// Runs the self-test once per game session with model paths of visible items.
	void SelfTest(const std::vector<std::string>& a_visibleModels);

	[[nodiscard]] bool IsActive() noexcept;

	// One line for the report ("active (298 of 300 test files found)").
	[[nodiscard]] const std::string& Status() noexcept;

	// Asks the game's file system directly (no self-test needed).
	// 1 = the model file exists, 0 = it does not, -1 = the lookup failed.
	[[nodiscard]] int FileExists(std::string_view a_modelPath);

	// Empty if the recipe's item (or at least one of its variants) has a model
	// file, or cannot be judged (for example an NPC, which has no model).
	// Otherwise the reason, e.g. "model file not found: Meshes\Foo\Bar.nif".
	[[nodiscard]] std::string FindProblem(const RE::BGSConstructibleObject* a_recipe);
}
