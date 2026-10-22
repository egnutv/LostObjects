#include "PluginHistory.h"

#include "Util.h"

#include <zlib.h>

namespace LO::PluginHistory
{
	namespace
	{
		constexpr std::uint32_t RECORD_COMPRESSED = 0x00040000;
		constexpr std::uint32_t RECORD_DELETED = 0x00000020;
		constexpr std::size_t HEADER_SIZE = 24;  // records and groups

		struct Header
		{
			char type[4];
			std::uint32_t size;  // records: data size; groups: total size including the header
			std::uint32_t flags;  // records: flags; groups: label
			std::uint32_t formID;
			std::uint32_t versionControl;
			std::uint16_t formVersion;
			std::uint16_t unknown;
		};
		static_assert(sizeof(Header) == HEADER_SIZE);

		template <class T>
		T ReadAt(std::span<const std::uint8_t> a_data, std::size_t a_offset)
		{
			T value{};
			std::memcpy(&value, a_data.data() + a_offset, sizeof(T));
			return value;
		}

		template <class Visit>
		void ForEachSubrecord(std::span<const std::uint8_t> a_data, Visit&& a_visit)
		{
			std::size_t pos = 0;
			std::uint32_t largeSize = 0;
			while (pos + 6 <= a_data.size()) {
				const std::string_view type(reinterpret_cast<const char*>(a_data.data() + pos), 4);
				std::size_t size = ReadAt<std::uint16_t>(a_data, pos + 4);
				pos += 6;
				if (largeSize != 0) {
					size = largeSize;
					largeSize = 0;
				}
				if (pos + size > a_data.size()) {
					return;
				}
				if (type == "XXXX" && size == 4) {
					largeSize = ReadAt<std::uint32_t>(a_data, pos);
				} else {
					a_visit(type, a_data.subspan(pos, size));
				}
				pos += size;
			}
		}

		class FileReader
		{
		public:
			FileReader(const RE::TESFile* a_file, std::uint32_t a_order, std::unordered_map<RE::TESFormID, std::vector<Version>>& a_lists) :
				_file(a_file),
				_order(a_order),
				_name(a_file->filename.data()),
				_lists(a_lists)
			{}

			// Returns an empty string if all went well, else what went wrong.
			std::string Run()
			{
				_stream.open(std::filesystem::path("Data") / _name, std::ios::binary);
				if (!_stream.is_open()) {
					return "could not be opened";
				}
				Header header{};
				std::vector<std::uint8_t> data;
				if (!ReadHeader(header) || std::string_view(header.type, 4) != "TES4" || !ReadData(header, data)) {
					return "is not a plugin file";
				}
				ReadMasters(data);

				while (ReadHeader(header)) {
					if (std::string_view(header.type, 4) != "GRUP" || header.size < HEADER_SIZE) {
						return "has a broken top-level group";
					}
					const std::string_view label(reinterpret_cast<const char*>(&header.flags), 4);
					const auto end = static_cast<std::uint64_t>(_stream.tellg()) + header.size - HEADER_SIZE;
					if (label == "FLST" && !ReadLists(end)) {
						return "has a broken FLST group";
					}
					_stream.clear();
					_stream.seekg(static_cast<std::streamoff>(end));
				}
				return {};
			}

		private:
			bool ReadHeader(Header& a_header)
			{
				return static_cast<bool>(_stream.read(reinterpret_cast<char*>(&a_header), HEADER_SIZE));
			}

			bool ReadData(const Header& a_header, std::vector<std::uint8_t>& a_data)
			{
				_raw.resize(a_header.size);
				if (a_header.size > 0 && !_stream.read(reinterpret_cast<char*>(_raw.data()), a_header.size)) {
					return false;
				}
				if ((a_header.flags & RECORD_COMPRESSED) == 0) {
					a_data.swap(_raw);
					return true;
				}
				if (_raw.size() < 4) {
					return false;
				}
				uLongf size = ReadAt<std::uint32_t>(_raw, 0);
				a_data.resize(size);
				return uncompress(a_data.data(), &size, _raw.data() + 4, static_cast<uLong>(_raw.size() - 4)) == Z_OK && size == a_data.size();
			}

			void ReadMasters(std::span<const std::uint8_t> a_data)
			{
				auto* dataHandler = RE::TESDataHandler::GetSingleton();
				ForEachSubrecord(a_data, [&](std::string_view a_type, std::span<const std::uint8_t> a_value) {
					if (a_type == "MAST" && !a_value.empty()) {
						const std::string master(reinterpret_cast<const char*>(a_value.data()), strnlen(reinterpret_cast<const char*>(a_value.data()), a_value.size()));
						_owners.push_back(dataHandler ? dataHandler->FindLoadedFileByName(master) : nullptr);
					}
				});
				_owners.push_back(_file);  // index = master count: the file itself
			}

			// Form ID inside the file -> form ID in the running game (0 if its file is not loaded).
			RE::TESFormID ToRuntime(std::uint32_t a_raw) const
			{
				const auto* owner = _owners[std::min<std::size_t>(a_raw >> 24, _owners.size() - 1)];
				if (!owner) {
					return 0;
				}
				if (owner->IsLight()) {
					return 0xFE000000u | (static_cast<std::uint32_t>(owner->smallFileCompileIndex) << 12) | (a_raw & 0xFFFu);
				}
				return (static_cast<std::uint32_t>(owner->compileIndex) << 24) | (a_raw & 0xFFFFFFu);
			}

			bool ReadLists(std::uint64_t a_end)
			{
				Header header{};
				std::vector<std::uint8_t> data;
				while (static_cast<std::uint64_t>(_stream.tellg()) + HEADER_SIZE <= a_end) {
					if (!ReadHeader(header)) {
						return false;
					}
					if (std::string_view(header.type, 4) == "GRUP") {
						if (header.size < HEADER_SIZE) {
							return false;
						}
						_stream.seekg(static_cast<std::streamoff>(header.size - HEADER_SIZE), std::ios::cur);
						continue;
					}
					if (!ReadData(header, data)) {
						return false;
					}
					const auto list = ToRuntime(header.formID);
					if (list == 0 || (header.flags & RECORD_DELETED) != 0) {
						continue;
					}
					Version version{ _order, _name, {} };
					ForEachSubrecord(data, [&](std::string_view a_type, std::span<const std::uint8_t> a_value) {
						if (a_type == "LNAM" && a_value.size() >= 4) {
							version.entries.push_back(ToRuntime(ReadAt<std::uint32_t>(a_value, 0)));  // 0 = not loaded, keeps the positions
						}
					});
					_lists[list].push_back(std::move(version));
				}
				return true;
			}

			const RE::TESFile* _file;
			std::uint32_t _order;
			std::string _name;
			std::unordered_map<RE::TESFormID, std::vector<Version>>& _lists;
			std::ifstream _stream;
			std::vector<const RE::TESFile*> _owners;
			std::vector<std::uint8_t> _raw;
		};

		const std::vector<Place> NO_PLACES;
		const std::vector<Version> NO_VERSIONS;
	}

	const std::vector<Place>& History::PlacesOf(RE::TESFormID a_form) const
	{
		const auto it = _places.find(a_form);
		return it != _places.end() ? it->second : NO_PLACES;
	}

	const std::vector<Version>& History::VersionsOf(RE::TESFormID a_list) const
	{
		const auto it = _lists.find(a_list);
		return it != _lists.end() ? it->second : NO_VERSIONS;
	}

	const History& Get()
	{
		static History history;
		static bool read = false;
		if (read) {
			return history;
		}
		read = true;

		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			return history;
		}
		const auto start = std::chrono::steady_clock::now();

		std::vector<const RE::TESFile*> files;
		for (const auto* file : dataHandler->compiledFileCollection.files) {
			files.push_back(file);
		}
		for (const auto* file : dataHandler->compiledFileCollection.smallFiles) {
			files.push_back(file);
		}
		std::erase(files, nullptr);

		std::uint32_t problems = 0;
		for (std::uint32_t i = 0; i < files.size(); ++i) {
			if (const auto problem = FileReader(files[i], i, history._lists).Run(); !problem.empty()) {
				REX::LogWarning("PluginHistory: {} {}", files[i]->filename.data(), problem);
				++problems;
			}
		}

		// Reverse index: form -> every place it was listed in, newest first.
		std::size_t places = 0;
		for (const auto& [list, versions] : history._lists) {
			for (const auto& version : versions) {
				for (std::size_t index = 0; index < version.entries.size(); ++index) {
					if (version.entries[index] != 0) {
						history._places[version.entries[index]].push_back({ list, &version, index });
						++places;
					}
				}
			}
		}
		for (auto& [form, list] : history._places) {
			std::ranges::sort(list, [](const Place& a_lhs, const Place& a_rhs) { return a_lhs.version->order > a_rhs.version->order; });
		}

		const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
		REX::LogInformation("PluginHistory: {} plugin files read in {} ms: {} form lists, {} entries{}", files.size(), elapsed.count(), history._lists.size(), places,
			problems ? std::format(", {} file(s) could not be read", problems) : "");
		return history;
	}
}
