#pragma once

#include "Forms.h"

// Menu thumbnails of recovered items.
//
// The workshop menu shows the thumbnail of an item with the item's own model,
// framed by its preview transform (TRNS: position, rotation, scale, zoom).
// Items that were never meant for the build menu often have none, so their
// thumbnail stays empty. Lost Objects gives such an item a transform from its
// pool in LostObjects.esp and frames the item by its OWN size: the camera of
// the icon of the first sub menu of the main menu, scaled by the ratio of that
// icon's size to the item's size (its bounds). The same is done for the Lost
// Objects icon. Nothing is taken from other items.

namespace LO::Preview
{
	// The icon (art object) of the first sub menu of the main menu that has one
	// with a preview transform. nullptr if there is none.
	[[nodiscard]] RE::BGSArtObject* FindReference(const OwnedForms& a_forms);

	// Largest side of the object's bounds (0 if it has none).
	[[nodiscard]] float Extent(const RE::TESBoundObject* a_object);

	// Sets a_transform so that a_object is shown in the size of a_reference's
	// icon. False (and a_transform unchanged) if a size is missing.
	bool Frame(RE::BGSTransform* a_transform, const RE::TESBoundObject* a_object, const RE::BGSArtObject* a_reference);
}
