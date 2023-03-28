#pragma once

#include <optional>
#include <memory>
#include <vector>
#include <list>

template <typename T>
using Optional = std::optional<T>;
template <typename T>
using Result = std::optional<T>;
template <typename T>
using Vec = std::vector<T>;
template <typename T>
using List = std::list<T>;
template <typename T>
using Ptr = std::unique_ptr<T>;
template <typename T>
using RefPtr = std::shared_ptr<T>;