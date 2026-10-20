#pragma once

#include "Forms.h"
#include "MenuBuilder.h"

namespace LO
{
	// Ties everything together: reacts to game events, runs the scan and keeps
	// the "Lost Objects" tab up to date. All game changes happen on the
	// main thread.
	class Recovery
	{
	public:
		[[nodiscard]] static Recovery& Get() noexcept;

		// F4SE: game data finished loading (also after a data reload).
		void OnDataReady();

		// F4SE: a save was loaded or a new game was started.
		void OnGameLoaded(std::string_view a_reason);

		// Scans and - if the result is ready - builds or keeps the menus. Must run
		// on the main thread. a_force: build now even if the menu has not settled
		// yet (the player is about to see it).
		void RequestScan(std::string_view a_reason, bool a_force = false);

		// Event hooks (called by the event sinks).
		void OnWorkshopModeChanged(bool a_entering);
		void OnWorkshopMenuClosed();

	private:
		Recovery() = default;

		void ScanNow(std::string_view a_reason, bool a_force);
		void OnScanFailed(std::string_view a_reason);
		static bool ScanGuarded(Recovery& a_self, std::string_view a_reason, bool a_force);

		void RegisterEvents();
		void BorrowMenuIcon();
		void ScheduleFollowUpScans();
		void ShowSummary(const ApplyResult& a_apply) const;
		[[nodiscard]] static bool IsWorkshopMenuOpen();

		OwnedForms _forms;
		MenuBuilder _builder;
		ApplyResult _lastApply;

		bool _ready{ false };
		bool _eventsRegistered{ false };
		bool _iconBorrowed{ false };
		bool _scanPending{ false };
		bool _pendingForce{ false };

		// The menus are built when the workshop menu has settled after loading
		// (two equal scans) and updated whenever a later scan finds something new.
		std::size_t _builtSignature{ 0 };
		std::size_t _previousSignature{ 0 };
		bool _built{ false };
		std::chrono::steady_clock::time_point _loadTime{};
		bool _summaryShown{ false };
		bool _hasLoggedOnce{ false };
		std::size_t _lastSignature{ 0 };
		std::uint32_t _scanCount{ 0 };

		std::atomic<std::uint32_t> _loadGeneration{ 0 };
	};
}
