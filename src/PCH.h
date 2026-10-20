#pragma once

// Precompiled header for Lost Objects.
// F4SE.hpp pulls in the standard library, REX/REL helpers and all RE:: game types.

#include <F4SE/F4SE.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std::literals;

// Value of EXCEPTION_EXECUTE_HANDLER from <excpt.h>, used with __try/__except.
// Defined here because CommonLibF4 does not include the Windows headers.
inline constexpr int SEH_EXECUTE_HANDLER = 1;
