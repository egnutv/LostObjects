#include "Preview.h"

namespace LO::Preview
{
	RE::BGSArtObject* FindReference(const OwnedForms& a_forms)
	{
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler || !a_forms.workshopMenuMain) {
			return nullptr;
		}

		for (auto* form : a_forms.workshopMenuMain->GetAllForms()) {
			auto* subMenu = form ? form->As<RE::BGSListForm>() : nullptr;
			if (!subMenu || a_forms.IsOwned(subMenu)) {
				continue;
			}
			const auto* title = subMenu->GetNthForm(0);
			if (!title) {
				continue;
			}
			for (auto* art : dataHandler->GetFormArray<RE::BGSArtObject>()) {
				if (!art || !art->previewTransform.transform) {
					continue;
				}
				auto* keywordForm = static_cast<RE::BGSKeywordForm*>(art);
				for (auto* keyword : keywordForm->GetKeywords()) {
					if (keyword == title) {
						return art;
					}
				}
			}
		}
		return nullptr;
	}

	float Extent(const RE::TESBoundObject* a_object)
	{
		if (!a_object) {
			return 0.0f;
		}
		const auto& b = a_object->boundData;
		const auto side = [](auto a_min, auto a_max) { return std::abs(static_cast<float>(a_max) - static_cast<float>(a_min)); };
		return std::max({ side(b.boundMin.x, b.boundMax.x), side(b.boundMin.y, b.boundMax.y), side(b.boundMin.z, b.boundMax.z) });
	}

	bool Frame(RE::BGSTransform* a_transform, const RE::TESBoundObject* a_object, const RE::BGSArtObject* a_reference)
	{
		const auto* reference = a_reference ? a_reference->previewTransform.transform : nullptr;
		if (!a_transform || !reference) {
			return false;
		}
		const auto objectExtent = Extent(a_object);
		const auto referenceExtent = Extent(a_reference);
		if (objectExtent <= 0.0f || referenceExtent <= 0.0f) {
			return false;
		}

		a_transform->data = reference->data;
		a_transform->data.scale = reference->data.scale * referenceExtent / objectExtent;

		constexpr std::uint32_t aroundOrigin = RE::BGSTransform::RecordFlag::kAroundOrigin;
		auto& flags = *a_transform->formFlags.underlying_address();
		flags = (flags & ~aroundOrigin) | (reference->formFlags.underlying() & aroundOrigin);
		return true;
	}
}
