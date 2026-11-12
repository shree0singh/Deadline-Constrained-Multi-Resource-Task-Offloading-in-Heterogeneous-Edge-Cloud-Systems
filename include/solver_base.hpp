#pragma once
#include "instance.hpp"
#include "solution.hpp"
#include <string>

namespace edge_cloud {

class ISolver {
public:
    virtual ~ISolver() = default;
    virtual std::string get_name() const = 0;
    virtual Solution solve(const TaskOffloadingInstance& instance) = 0;
};

} // namespace edge_cloud

