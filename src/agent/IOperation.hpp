#include "Agent.hpp"
#pragma once
class IOperation {
    public:
        virtual void execute (Agent& agent, VFS& vfs, Context& ctx) = 0;
        virtual ~IOperation () = default;
};