#include <gtest/gtest.h>
#include <memory>
#include "trans_table/TransTable.h"

namespace dds_test {

// Test fixture for TransTable base class
class TransTableBaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create mock derived class for testing base functionality
        baseTable = std::make_unique<MockTransTable>();
    }

    void TearDown() override {
        baseTable.reset();
    }

    // Mock concrete implementation for testing base class
    class MockTransTable : public TransTable {
    public:
        MockTransTable() : TransTable() {}
        
        ~MockTransTable() override = default;

        void init(const int handLookup[][15]) override {
            initCalled = true;
        }

        void set_memory_default(int megabytes) override {
            defaultMemory = megabytes;
        }

        void set_memory_maximum(int megabytes) override {
            maximumMemory = megabytes;
        }

        void make_tt() override {
            ttMade = true;
        }

        void reset_memory(ResetReason reason) override {
            lastResetReason = reason;
        }

        void return_all_memory() override {
            memoryReturned = true;
        }

        double memory_in_use() const override {
            return testMemoryUsage;
        }

        NodeCards const * lookup(
            int trick,
            int hand,
            const unsigned short aggr_target[],
            const int hand_dist[],
            int limit,
            bool& lowerFlag) override {
            
            lookupCalled = true;
            lastTrick = trick;
            lastHand = hand;
            lastLimit = limit;
            lowerFlag = testLowerFlag;
            return testLookupResult;
        }

        void add(
            int trick,
            int hand,
            const unsigned short aggr_target[],
            const unsigned short win_ranks_arg[],
            const NodeCards& first,
            bool flag) override {
            
            addCalled = true;
            addTrick = trick;
            addHand = hand;
            addFlag = flag;
        }

        void print_suits(
            std::ofstream& /*fout*/,
            int /*trick*/,
            int /*hand*/) const override {
            
            printSuitsCalled = true;
        }

        void print_all_suits(std::ofstream& /*fout*/) const override {
            printAllSuitsCalled = true;
        }

        // No-op implementations for remaining pure-virtual printers
        void print_suit_stats(std::ofstream& /*fout*/, int /*trick*/, int /*hand*/) const override {}
        void print_all_suit_stats(std::ofstream& /*fout*/) const override {}
        void print_summary_suit_stats(std::ofstream& /*fout*/) const override {}
        void print_entries_dist(std::ofstream& /*fout*/, int /*trick*/, int /*hand*/, const int /*hand_dist*/[]) const override {}
        void print_entries_dist_and_cards(std::ofstream& /*fout*/, int /*trick*/, int /*hand*/, const unsigned short /*aggr_target*/[], const int /*hand_dist*/[]) const override {}
        void print_entries(std::ofstream& /*fout*/, int /*trick*/, int /*hand*/) const override {}
        void print_all_entries(std::ofstream& /*fout*/) const override {}
        void print_entry_stats(std::ofstream& /*fout*/, int /*trick*/, int /*hand*/) const override {}
        void print_all_entry_stats(std::ofstream& /*fout*/) const override {}
        void print_summary_entry_stats(std::ofstream& /*fout*/) const override {}

        // Test state variables (mutable for const methods)
        mutable bool initCalled = false;
        mutable bool ttMade = false;
        mutable bool memoryReturned = false;
        mutable bool lookupCalled = false;
        mutable bool addCalled = false;
        mutable bool printSuitsCalled = false;
        mutable bool printAllSuitsCalled = false;
        
        int defaultMemory = 0;
        int maximumMemory = 0;
    ResetReason lastResetReason = ResetReason::Unknown;
        
        // Lookup test state
        int lastTrick = -1;
        int lastHand = -1;
        int lastLimit = -1;
        bool testLowerFlag = false;
    NodeCards const* testLookupResult = nullptr;
        
        // Add test state
        int addTrick = -1;
        int addHand = -1;
        bool addFlag = false;
        
        // Memory test state
        double testMemoryUsage = 42.5;
    };

    std::unique_ptr<MockTransTable> baseTable;
};

// Construction/Destruction Tests
TEST_F(TransTableBaseTest, ConstructorCreatesValidObject) {
    EXPECT_NE(baseTable.get(), nullptr);
    
    // Verify initial state
    EXPECT_FALSE(baseTable->initCalled);
    EXPECT_FALSE(baseTable->ttMade);
    EXPECT_FALSE(baseTable->memoryReturned);
    EXPECT_EQ(baseTable->defaultMemory, 0);
    EXPECT_EQ(baseTable->maximumMemory, 0);
}

TEST_F(TransTableBaseTest, VirtualDestructorWorks) {
    // Create through base pointer to verify virtual destructor
    std::unique_ptr<TransTable> basePtr = std::make_unique<MockTransTable>();
    EXPECT_NE(basePtr.get(), nullptr);
    
    // Destructor should work properly when called through base pointer
    // This test verifies that destructor is virtual
    basePtr.reset(); // Should call derived destructor properly
    EXPECT_EQ(basePtr.get(), nullptr);
}

// Interface Verification Tests
TEST_F(TransTableBaseTest, InitMethodCallsOverride) {
    int handLookup[15][15]; // Mock lookup table
    
    EXPECT_FALSE(baseTable->initCalled);
    baseTable->init(handLookup);
    EXPECT_TRUE(baseTable->initCalled);
}

TEST_F(TransTableBaseTest, SetMemoryMethodsWork) {
    baseTable->set_memory_default(64);
    EXPECT_EQ(baseTable->defaultMemory, 64);
    
    baseTable->set_memory_maximum(128);
    EXPECT_EQ(baseTable->maximumMemory, 128);
}

TEST_F(TransTableBaseTest, MakeTTMethodCallsOverride) {
    EXPECT_FALSE(baseTable->ttMade);
    baseTable->make_tt();
    EXPECT_TRUE(baseTable->ttMade);
}

TEST_F(TransTableBaseTest, ResetMemoryMethodCallsOverride) {
    EXPECT_EQ(baseTable->lastResetReason, ResetReason::Unknown);
    
    baseTable->reset_memory(ResetReason::NewDeal);
    EXPECT_EQ(baseTable->lastResetReason, ResetReason::NewDeal);
    
    baseTable->reset_memory(ResetReason::MemoryExhausted);
    EXPECT_EQ(baseTable->lastResetReason, ResetReason::MemoryExhausted);
}

TEST_F(TransTableBaseTest, ReturnAllMemoryMethodCallsOverride) {
    EXPECT_FALSE(baseTable->memoryReturned);
    baseTable->return_all_memory();
    EXPECT_TRUE(baseTable->memoryReturned);
}

TEST_F(TransTableBaseTest, MemoryInUseMethodCallsOverride) {
    // Test that virtual method calls override implementation
    double memUsage = baseTable->memory_in_use();
    EXPECT_EQ(memUsage, 42.5); // Should return mock value
}

TEST_F(TransTableBaseTest, LookupMethodCallsOverride) {
    unsigned short aggrTarget[DDS_SUITS] = {0x1111, 0x2222, 0x3333, 0x4444};
    int handDist[4] = {13, 13, 13, 13};
    bool lowerFlag = true;
    
    EXPECT_FALSE(baseTable->lookupCalled);
    
    NodeCards const* result = baseTable->lookup(
        10, // trick
        2,  // hand
        aggrTarget,
        handDist,
        8,  // limit
        lowerFlag
    );
    
    EXPECT_TRUE(baseTable->lookupCalled);
    EXPECT_EQ(baseTable->lastTrick, 10);
    EXPECT_EQ(baseTable->lastHand, 2);
    EXPECT_EQ(baseTable->lastLimit, 8);
    EXPECT_EQ(lowerFlag, baseTable->testLowerFlag); // Should be modified by override
    EXPECT_EQ(result, baseTable->testLookupResult);
}

TEST_F(TransTableBaseTest, AddMethodCallsOverride) {
    unsigned short aggrTarget[DDS_SUITS] = {0x1111, 0x2222, 0x3333, 0x4444};
    unsigned short winRanks[DDS_SUITS] = {0x5555, 0x6666, 0x7777, 0x8888};
    NodeCards nodeData;
    
    EXPECT_FALSE(baseTable->addCalled);
    
    baseTable->add(
        8,  // trick
        1,  // hand
        aggrTarget,
        winRanks,
        nodeData,
        true // flag
    );
    
    EXPECT_TRUE(baseTable->addCalled);
    EXPECT_EQ(baseTable->addTrick, 8);
    EXPECT_EQ(baseTable->addHand, 1);
    EXPECT_TRUE(baseTable->addFlag);
}

TEST_F(TransTableBaseTest, PrintMethodsCallOverride) {
    std::ofstream testFile("test_output.txt");
    
    EXPECT_FALSE(baseTable->printSuitsCalled);
    EXPECT_FALSE(baseTable->printAllSuitsCalled);
    
    baseTable->print_suits(testFile, 5, 2); // trick=5, hand=2
    EXPECT_TRUE(baseTable->printSuitsCalled);
    
    baseTable->print_all_suits(testFile);
    EXPECT_TRUE(baseTable->printAllSuitsCalled);
    
    testFile.close();
    std::remove("test_output.txt"); // Cleanup
}

// Interface Contract Tests
TEST_F(TransTableBaseTest, AllVirtualMethodsHaveExpectedSignatures) {
    // This test verifies that all expected virtual methods exist
    // and have the correct signatures by calling them through base pointer
    
    std::unique_ptr<TransTable> basePtr = std::make_unique<MockTransTable>();
    
    // Test that we can call all virtual methods through base pointer
    int handLookup[15][15];
    basePtr->init(handLookup);
    
    basePtr->set_memory_default(64);
    basePtr->set_memory_maximum(128);
    basePtr->make_tt();
    basePtr->reset_memory(ResetReason::NewDeal);
    basePtr->return_all_memory();
    
    double mem = basePtr->memory_in_use();
    EXPECT_GE(mem, 0.0); // Should return non-negative value
    
    unsigned short aggrTarget[DDS_SUITS] = {0, 0, 0, 0};
    unsigned short winRanks[DDS_SUITS] = {0, 0, 0, 0};
    int handDist[4] = {0, 0, 0, 0};
    bool lowerFlag = false;
    NodeCards nodeData;
    
    // Should not crash when called through base pointer
    basePtr->lookup(0, 0, aggrTarget, handDist, 0, lowerFlag);
    basePtr->add(0, 0, aggrTarget, winRanks, nodeData, false);
    
    std::ofstream nullFile("/dev/null");
    basePtr->print_suits(nullFile, 0, 1); // trick=0, hand=1
    basePtr->print_all_suits(nullFile);
    nullFile.close();
}

TEST_F(TransTableBaseTest, PolymorphicBehaviorWorks) {
    // Verify that virtual dispatch works correctly
    TransTable* basePtr = baseTable.get();
    
    // Calls should go to derived class implementations
    basePtr->make_tt();
    EXPECT_TRUE(baseTable->ttMade);
    
    basePtr->return_all_memory();
    EXPECT_TRUE(baseTable->memoryReturned);
    
    double memUsage = basePtr->memory_in_use();
    EXPECT_EQ(memUsage, 42.5); // Should call derived implementation
}

} // namespace dds_test
