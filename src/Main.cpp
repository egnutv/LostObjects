// -----------------------------------------------------------------------------
// Lost Objects - F4SE plugin entry point
//
// Adds a "Lost Objects" tab to the workshop menu that collects every
// craftable workshop item which cannot be reached in the build menu.
//
// Vibecoded by AI (Claude, Anthropic). Project owner and tester: egnutv.
// License: PolyForm Noncommercial 1.0.0 - see LICENSE and NOTICE.md.
// -----------------------------------------------------------------------------

#include "Recovery.h"

namespace
{
	using MessageType = F4SE::MessagingInterface::MessageType;

	bool MessageFlag(const F4SE::MessagingInterface::Message* a_message)
	{
		// F4SE passes small values (bool) directly in the data pointer.
		return reinterpret_cast<std::uintptr_t>(a_message->GetData()) != 0;
	}

	void OnF4SEMessage(F4SE::MessagingInterface::Message* a_message)
	{
		if (!a_message) {
			return;
		}

		switch (a_message->GetType()) {
		case MessageType::kGameDataReady:
			if (MessageFlag(a_message)) {
				LO::Recovery::Get().OnDataReady();
			}
			break;
		case MessageType::kPostLoadGame:
			if (MessageFlag(a_message)) {
				LO::Recovery::Get().OnGameLoaded("save loaded");
			}
			break;
		case MessageType::kNewGame:
			LO::Recovery::Get().OnGameLoaded("new game");
			break;
		default:
			break;
		}
	}
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);

	if (auto logger = REX::GetDefaultLogger()) {
		logger->flush_on(spdlog::level::info);
	}

	REX::LogInformation(
		"{} {} loading (game {})",
		F4SE::GetPluginName(),
		F4SE::GetPluginVersion().ToString<char>(),
		F4SE::GetRuntimeVersion().ToString<char>());
	REX::LogInformation("Vibecoded by AI (Claude, Anthropic) - AI-generated code can contain mistakes.");

	F4SE::GetMessagingInterface()->RegisterListener(&OnF4SEMessage);

	return true;
}
