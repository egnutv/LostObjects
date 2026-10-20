Scriptname LostObjects_Holotape extends Terminal
{Script of the Lost Objects settings holotape (terminal LO_HolotapeTerminal).
It only passes the selected menu item to the DLL. Nothing runs in the
background and nothing is stored by this script.
Vibecoded by AI (Claude, Anthropic). Project owner and tester: egnutv.}

Event OnMenuItemRun(int auiMenuItemID, ObjectReference akTerminalRef)
	LostObjects.HolotapeItem(auiMenuItemID)
EndEvent
