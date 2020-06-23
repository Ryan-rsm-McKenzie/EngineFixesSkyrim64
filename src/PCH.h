#pragma once

#include "RE/Skyrim.h"
#include "REL/Relocation.h"
#include "SKSE/SKSE.h"

#include <INIReader.h>
#include <Simpleini.h>

#include <tbb/concurrent_hash_map.h>
#include <tbb/scalable_allocator.h>

#include <xbyak/xbyak.h>

#include <intrin.h>

#include <array>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <system_error>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#define DLLEXPORT __declspec(dllexport)
