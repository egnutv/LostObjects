#pragma once

// Small helpers shared by the other source files.

#include "NameMap.h"

namespace LO::Util
{
	[[nodiscard]] inline std::string ToLower(std::string_view a_text)
	{
		std::string result(a_text);
		std::ranges::transform(result, result.begin(), [](unsigned char a_char) {
			return static_cast<char>(std::tolower(a_char));
		});
		return result;
	}

	[[nodiscard]] inline std::string Trim(std::string_view a_text)
	{
		const auto isSpace = [](unsigned char a_char) { return std::isspace(a_char) != 0; };

		std::size_t begin = 0;
		std::size_t end = a_text.size();
		while (begin < end && isSpace(static_cast<unsigned char>(a_text[begin]))) {
			++begin;
		}
		while (end > begin && isSpace(static_cast<unsigned char>(a_text[end - 1]))) {
			--end;
		}
		return std::string(a_text.substr(begin, end - begin));
	}

	[[nodiscard]] inline bool EqualsIgnoreCase(std::string_view a_lhs, std::string_view a_rhs)
	{
		return a_lhs.size() == a_rhs.size() &&
		       std::ranges::equal(a_lhs, a_rhs, [](unsigned char a_l, unsigned char a_r) {
				   return std::tolower(a_l) == std::tolower(a_r);
			   });
	}

	// "SomeMod.esp" -> "SomeMod"
	[[nodiscard]] inline std::string StripExtension(std::string_view a_fileName)
	{
		const auto dot = a_fileName.find_last_of('.');
		return std::string(dot == std::string_view::npos ? a_fileName : a_fileName.substr(0, dot));
	}

	[[nodiscard]] inline std::string FormIDToString(std::uint32_t a_formID)
	{
		return std::format("{:08X}", a_formID);
	}

	// Name of the file a form was defined in (a_index 0) or last overridden in (a_index -1).
	[[nodiscard]] inline std::string GetFileName(const RE::TESForm* a_form, std::int32_t a_index = 0)
	{
		if (!a_form) {
			return {};
		}

		const auto* file = a_form->GetFile(a_index);
		return file ? std::string(file->filename.data()) : std::string();
	}

	// Sort key that roughly follows the load order (full plugins first, then light plugins).
	[[nodiscard]] inline std::uint32_t GetLoadOrderKey(const RE::TESForm* a_form)
	{
		const auto* file = a_form ? a_form->GetFile(0) : nullptr;
		if (!file) {
			return std::numeric_limits<std::uint32_t>::max();
		}

		if (file->IsLight()) {
			return 0x100000u + file->smallFileCompileIndex;
		}

		return static_cast<std::uint32_t>(file->compileIndex) << 12;
	}

	// Display name of a form (see NameMap.h - never use
	// RE::TESFullName::GetFormFullName, its address is wrong on 1.11.x).
	[[nodiscard]] inline std::string GetFormName(const RE::TESForm* a_form)
	{
		return NameMap::GetName(a_form);
	}

	// Editor IDs are only kept at runtime for a few form types (keywords are one of them).
	[[nodiscard]] inline std::string GetEditorID(const RE::TESForm* a_form)
	{
		if (!a_form) {
			return {};
		}

		const auto* editorID = a_form->GetFormEditorID();
		return editorID ? std::string(editorID) : std::string();
	}

	// What a recipe really builds: the item itself, or - for a form list - every
	// variant in it (nested lists one level deep, at most 256 forms).
	inline void CollectVariants(const RE::TESForm* a_form, std::vector<const RE::TESForm*>& a_out, std::uint32_t a_depth = 0)
	{
		if (!a_form || a_out.size() >= 256) {
			return;
		}
		if (const auto* list = a_form->As<RE::BGSListForm>()) {
			if (a_depth >= 2) {
				return;
			}
			for (const auto* form : list->GetAllForms()) {
				CollectVariants(form, a_out, a_depth + 1);
			}
			return;
		}
		a_out.push_back(a_form);
	}
}
