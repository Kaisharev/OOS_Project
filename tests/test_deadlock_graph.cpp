#include <gtest/gtest.h>

#include "deadlock/DeadlockGraph.hpp"

TEST (DeadlockGraphTest, PrazanGrafNemaCiklusa) {
    DeadlockGraph g;
    EXPECT_FALSE (g.would_create_cycle ("A1", "A2"));
}

TEST (DeadlockGraphTest, JednaGranaNemaCiklusa) {
    DeadlockGraph g;
    g.add_edge ("A1", "A2");
    EXPECT_FALSE (g.would_create_cycle ("A3", "A1"));
}

TEST (DeadlockGraphTest, DirektniCiklus) {
    DeadlockGraph g;
    g.add_edge ("A1", "A2");
    EXPECT_TRUE (g.would_create_cycle ("A2", "A1"));
}

TEST (DeadlockGraphTest, IndirektniCiklusKrozTriAgenta) {
    DeadlockGraph g;
    g.add_edge ("A1", "A2");
    g.add_edge ("A2", "A3");
    EXPECT_TRUE (g.would_create_cycle ("A3", "A1"));
}

TEST (DeadlockGraphTest, IndirektniCiklusKrozCetiriAgenta) {
    DeadlockGraph g;
    g.add_edge ("A1", "A2");
    g.add_edge ("A2", "A3");
    g.add_edge ("A3", "A4");
    EXPECT_TRUE (g.would_create_cycle ("A4", "A1"));
}

TEST (DeadlockGraphTest, BezCiklusaULancu) {
    DeadlockGraph g;
    g.add_edge ("A1", "A2");
    g.add_edge ("A2", "A3");
    EXPECT_FALSE (g.would_create_cycle ("A4", "A3"));
}

TEST (DeadlockGraphTest, RemoveEdgesOslobadjaGranu) {
    DeadlockGraph g;
    g.add_edge ("A1", "A2");
    g.remove_edges_for ("A1");
    EXPECT_FALSE (g.would_create_cycle ("A2", "A1"));
}

TEST (DeadlockGraphTest, RemoveEdgesNeuticeNaDrugeGrane) {
    DeadlockGraph g;
    g.add_edge ("A1", "A2");
    g.add_edge ("A2", "A3");
    g.remove_edges_for ("A1");
    EXPECT_TRUE (g.would_create_cycle ("A3", "A2"));
}

TEST (DeadlockGraphTest, GetCyclePathDirektniCiklus) {
    DeadlockGraph g;
    g.add_edge ("A1", "A2");
    EXPECT_EQ (g.get_cycle_path ("A2", "A1"), "A2 -> A1 -> A2");
}

TEST (DeadlockGraphTest, GetCyclePathIndirektniCiklus) {
    DeadlockGraph g;
    g.add_edge ("A1", "A2");
    g.add_edge ("A2", "A3");
    EXPECT_EQ (g.get_cycle_path ("A3", "A1"), "A3 -> A1 -> A2 -> A3");
}

TEST (DeadlockGraphTest, AgentNeCekaSamSebe) {
    DeadlockGraph g;
    EXPECT_FALSE (g.would_create_cycle ("A1", "A1"));
}

TEST (DeadlockGraphTest, DodavanjeGraneNakonUklanjanja) {
    DeadlockGraph g;
    g.add_edge ("A1", "A2");
    g.remove_edges_for ("A1");
    g.add_edge ("A1", "A3");
    g.add_edge ("A3", "A2");
    EXPECT_TRUE (g.would_create_cycle ("A2", "A1"));
}