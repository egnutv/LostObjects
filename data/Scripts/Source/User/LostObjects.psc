Scriptname LostObjects Native
{Native functions of the Lost Objects F4SE plugin (LostObjects.dll).
Vibecoded by AI (Claude, Anthropic). Project owner and tester: egnutv.
License: PolyForm Noncommercial 1.0.0 with additional permissions, see NOTICE.txt.}

; Called by the settings holotape when a menu item is selected.
; The DLL does the work on the game's main thread and shows a message.
Function HolotapeItem(int aiItemID) global native
