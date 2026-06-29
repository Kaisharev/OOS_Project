#include <gtest/gtest.h>

#include <memory>

#include "../src/agent/Agent.hpp"
#include "../src/core/AgentState.hpp"
#include "../src/scheduler/PriorityPreemptiveScheduler.hpp"

static std::shared_ptr<Agent> make_agent (const std::string& id, int priority, int arrival_time) {
    auto a = std::make_shared<Agent> ();
    a->setId (id);
    a->setPriority (priority);
    a->setArrivalTime (arrival_time);
    a->setState (AgentState::READY);
    return a;
}

TEST (SchedulerTest, AgentStigneURightTick) {
    PriorityPreemptiveScheduler sched (2);
    auto a = make_agent ("A1", 1, 3);
    sched.add_agent (a);

    sched.tick (0);
    EXPECT_TRUE (sched.get_running ().empty ());

    sched.tick (3);
    EXPECT_EQ (sched.get_running ().size (), 1);
    EXPECT_EQ (sched.get_running ()[0]->getId (), "A1");
}

TEST (SchedulerTest, VisiPrioritetPreuzimaSlot) {
    PriorityPreemptiveScheduler sched (1);
    auto low = make_agent ("Low", 5, 0);
    auto high = make_agent ("High", 1, 3);

    sched.add_agent (low);
    sched.add_agent (high);

    sched.tick (0);
    EXPECT_EQ (sched.get_running ()[0]->getId (), "Low");

    sched.tick (3);
    EXPECT_EQ (sched.get_running ()[0]->getId (), "High");
    EXPECT_EQ (low->getPreemptions (), 1);
    EXPECT_EQ (low->getState (), AgentState::READY);
}

TEST (SchedulerTest, PopunjavanjeSloboodnihSlotova) {
    PriorityPreemptiveScheduler sched (2);
    auto a1 = make_agent ("A1", 1, 0);
    auto a2 = make_agent ("A2", 2, 0);
    sched.add_agent (a1);
    sched.add_agent (a2);

    sched.tick (0);
    EXPECT_EQ (sched.get_running ().size (), 2);
}

TEST (SchedulerTest, BlokiraniAgentNeZauzimaSlot) {
    PriorityPreemptiveScheduler sched (1);
    auto a1 = make_agent ("A1", 1, 0);
    auto a2 = make_agent ("A2", 2, 0);
    sched.add_agent (a1);
    sched.add_agent (a2);

    sched.tick (0);
    // A1 ima viši prioritet, zauzima slot
    EXPECT_EQ (sched.get_running ()[0]->getId (), "A1");

    // A1 se blokira
    a1->setState (AgentState::BLOCKED);
    sched.tick (1);

    // A2 treba preuzeti slot
    EXPECT_EQ (sched.get_running ()[0]->getId (), "A2");
}

TEST (SchedulerTest, UnblockAgentaVracaUReadyQueue) {
    PriorityPreemptiveScheduler sched (1);
    auto a1 = make_agent ("A1", 1, 0);
    sched.add_agent (a1);
    sched.tick (0);
    EXPECT_EQ (sched.get_running ().size (), 1);
    EXPECT_EQ (sched.get_running ()[0]->getId (), "A1");

    a1->setState (AgentState::BLOCKED);
    sched.tick (1);
    EXPECT_TRUE (sched.get_running ().empty ());

    sched.unblock_agent (a1->getId ());
    sched.tick (2);
    EXPECT_EQ (sched.get_running ().size (), 1);
    EXPECT_EQ (sched.get_running ()[0]->getId (), "A1");
}