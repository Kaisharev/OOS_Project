#include <gtest/gtest.h>

#include "../src/agent/Operations/Operations.hpp"

TEST (OperationTest, ConstructorAndGetters) {
    Operation think_op (OperationType::THINK, 3, "", "", "", "");
    EXPECT_EQ (think_op.getType (), OperationType::THINK);
    EXPECT_EQ (think_op.getThinkingDuration (), 3);

    Operation open_op (OperationType::OPEN, 0, "/path/to/file.txt", "", "read", "f");
    EXPECT_EQ (open_op.getType (), OperationType::OPEN);
    EXPECT_EQ (open_op.getPath (), "/path/to/file.txt");
    EXPECT_EQ (open_op.getMode (), "read");
    EXPECT_EQ (open_op.getHandle (), "f");

    Operation read_op (OperationType::READ, 0, "", "", "", "f");
    EXPECT_EQ (read_op.getType (), OperationType::READ);
    EXPECT_EQ (read_op.getHandle (), "f");

    Operation write_op (OperationType::WRITE, 0, "", "test data", "", "f");
    EXPECT_EQ (write_op.getType (), OperationType::WRITE);
    EXPECT_EQ (write_op.getData (), "test data");
    EXPECT_EQ (write_op.getHandle (), "f");
}

TEST (OperationTest, DefaultConstructor) {
    Operation default_op;
    EXPECT_EQ (default_op.getType (), OperationType::THINK);
    EXPECT_EQ (default_op.getThinkingDuration (), 0);
    EXPECT_EQ (default_op.getPath (), "");
    EXPECT_EQ (default_op.getHandle (), "");
}