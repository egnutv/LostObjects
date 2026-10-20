#include "Notice.h"

#include "Util.h"

namespace LO::Notice
{
	namespace
	{
		constexpr auto NOTICE_TEXT =
			"LOST MOD OBJECTS\n\n"
			"This mod was vibecoded by AI (written by Claude, Anthropic) and tested by a human.\n"
			"AI-generated code can contain mistakes. Please keep a backup save.\n\n"
			"New in the workshop menu: the tab \"Lost Objects\". It collects every "
			"craftable workshop item that cannot be reached in the build menu.\n\n"
			"This message is shown once per version."sv;

		constexpr auto STATE_FILE = "LostObjects_State.ini"sv;
		constexpr auto STATE_KEY = "AcknowledgedVersion"sv;

		std::filesystem::path GetStatePath()
		{
			return F4SE::GetLogDirectoryPath() / STATE_FILE;
		}

		std::string ReadAcknowledgedVersion()
		{
			std::ifstream file(GetStatePath());
			std::string line;
			while (std::getline(file, line)) {
				const auto equals = line.find('=');
				if (equals == std::string::npos) {
					continue;
				}
				if (Util::EqualsIgnoreCase(Util::Trim(line.substr(0, equals)), STATE_KEY)) {
					return Util::Trim(line.substr(equals + 1));
				}
			}
			return {};
		}

		void WriteAcknowledgedVersion(std::string_view a_version)
		{
			std::ofstream file(GetStatePath(), std::ios::out | std::ios::trunc);
			if (!file.is_open()) {
				REX::LogWarning("Notice: could not write {}", GetStatePath().string());
				return;
			}
			file << "; Written by Lost Objects. Delete this file to see the AI notice again.\n";
			file << "[State]\n";
			file << STATE_KEY << "=" << a_version << "\n";
		}
	}

	std::string_view GetText() noexcept
	{
		return NOTICE_TEXT;
	}

	void ShowIfNeeded()
	{
		const auto version = F4SE::GetPluginVersion().ToString<char>();
		if (ReadAcknowledgedVersion() == version) {
			return;
		}

		auto* messageMenu = RE::MessageMenuManager::GetSingleton();
		if (!messageMenu) {
			REX::LogWarning("Notice: message menu not available yet");
			return;
		}

		// NOTICE_TEXT is a string literal: static storage and null-terminated.
		messageMenu->CreateOKMessage(NOTICE_TEXT.data());
		WriteAcknowledgedVersion(version);

		REX::LogInformation("Notice: AI disclosure shown for version {}", version);
	}
}
