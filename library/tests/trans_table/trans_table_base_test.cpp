/// @file trans_table_base_test.cpp
/// @brief Unit tests for TransTable base class functionality.
/// @details Tests virtual methods, polymorphism, and base class interface.

// C++ standard library headers
#include <memory>

// Third-party headers
#include <gtest/gtest.h>

// Project headers
#include "trans_table/trans_table.hpp"

namespace dds_test {

// Test fixture for TransTable base class
class TransTableBaseTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create mock derived class for testing base functionality
        baseTable = std::make_unique<MockTransTable>();
    }

    void TearDown() override
    {
        baseTable.reset();
    }

    // Mock concrete implementation for testing base class
    class MockTransTable : public TransTable
    {
    public:
        MockTransTable() : TransTable() {}
        
        ~MockTransTable() override = default;

        void init(const int handLookup[][15]) override
        {
            init_called_ = true;
        }

        void set_memory_default(int megabytes) override
        {
            default_memory_ = megabytes;
        }

        void set_memory_maximum(int megabytes) override
        {
            maximum_memory_ = megabytes;
        }

        void make_tt() override
        {
            tt_made_ = true;
        }

        void reset_memory(ResetReason reason) override
        {
            last_reset_reason_ = reason;
        }

        void return_all_memory() override
        {
            memory_returned_ = true;
        }

        double memory_in_use() const override
        {
            return test_memory_usage_;
        }

        NodeCards const * lookup(
            int trick,
            int hand,
            const unsigned short aggr_target[],
            const int hand_dist[],
            int limit,
            bool& lowerFlag) override
        {

            lookup_called_ = true;
            last_trick_ = trick;
            last_hand_ = hand;
            last_limit_ = limit;
            lowerFlag = test_lower_flag_;
            return test_lookup_result_;
        }

        void add(
            int trick,
            int hand,
            const unsigned short aggr_target[],
            const unsigned short win_ranks_arg[],
            const NodeCards& first,
            bool flag) override
        {

            add_called_ = true;
            add_trick_ = trick;
            add_hand_ = hand;
            add_flag_ = flag;
        }

        void print_suits(
            std::ofstream& /*fout*/,
            int /*trick*/,
            int /*hand*/) const override
        {
            print_suits_called_ = true;
        }

        void print_all_suits(std::ofstream& /*fout*/) const override
        {
            print_all_suits_called_ = true;
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
        mutable bool init_called_ = false;
        mutable bool tt_made_ = false;
        mutable bool memory_returned_ = false;
        mutable bool lookup_called_ = false;
        mutable bool add_called_ = false;
        mutable bool print_suits_called_ = false;
        mutable bool print_all_suits_called_ = false;

        int default_memory_ = 0;
        int maximum_memory_ = 0;
        ResetReason last_reset_reason_ = ResetReason::Unknown;

        // Lookup test state
        int last_trick_ = -1;
        int last_hand_ = -1;
        int last_limit_ = -1;
        bool test_lower_flag_ = false;
        NodeCards const* test_lookup_result_ = nullptr;

        // Add test state
        int add_trick_ = -1;
        int add_hand_ = -1;
        bool add_flag_ = false;

        // Memory test state
        double test_memory_usage_ = 42.5;
    };

    std::unique_ptr<MockTransTable> baseTable;
};

// Construction/Destruction Tests
TEST_F(TransTableBaseTest, ConstructorCreatesValidObject)
{
    EXPECT_NE(baseTable.get(), nullptr);
    
    // Verify initial state
    EXPECT_FALSE(baseTable->init_called_);
    EXPECT_FALSE(baseTable->tt_made_);
    EXPECT_FALSE(baseTable->memory_returned_);
    EXPECT_EQ(baseTable->default_memory_, 0);
    EXPECT_EQ(baseTable->maximum_memory_, 0);
}

TEST_F(TransTableBaseTest, VirtualDestructorWorks)
{
    // Create through base pointer to verify virtual destructor
    std::unique_ptr<TransTable> basePtr = std::make_unique<MockTransTable>();
    EXPECT_NE(basePtr.get(), nullptr);
    
    // Destructor should work properly when called through base pointer
    // This test verifies that destructor is virtual
    basePtr.reset(); // Should call derived destructor properly
    EXPECT_EQ(basePtr.get(), nullptr);
}

// Interface Verification Tests
TEST_F(TransTableBaseTest, InitMethodCallsOverride)
{
    int handLookup[15][15] = {}; // Mock lookup table (zero-initialized)
    
    EXPECT_FALSE(baseTable->init_called_);
    baseTable->init(handLookup);
    EXPECT_TRUE(baseTable->init_called_);
}

TEST_F(TransTableBaseTest, SetMemoryMethodsWork)
{
    baseTable->set_memory_default(64);
    EXPECT_EQ(baseTable->default_memory_, 64);

    baseTable->set_memory_maximum(128);
    EXPECT_EQ(baseTable->maximum_memory_, 128);
}

TEST_F(TransTableBaseTest, MakeTTMethodCallsOverride)
{
    EXPECT_FALSE(baseTable->tt_made_);
    baseTable->make_tt();
    EXPECT_TRUE(baseTable->tt_made_);
}

TEST_F(TransTableBaseTest, ResetMemoryMethodCallsOverride)
{
    EXPECT_EQ(baseTable->last_reset_reason_, ResetReason::Unknown);

    baseTable->reset_memory(ResetReason::NewDeal);
    EXPECT_EQ(baseTable->last_reset_reason_, ResetReason::NewDeal);

    baseTable->reset_memory(ResetReason::MemoryExhausted);
    EXPECT_EQ(baseTable->last_reset_reason_, ResetReason::MemoryExhausted);
}

TEST_F(TransTableBaseTest, ReturnAllMemoryMethodCallsOverride)
{
    EXPECT_FALSE(baseTable->memory_returned_);
    baseTable->return_all_memory();
    EXPECT_TRUE(baseTable->memory_returned_);
}

TEST_F(TransTableBaseTest, MemoryInUseMethodCallsOverride)
{
    // Test that virtual method calls override implementation
    const double memUsage = baseTable->memory_in_use();
    EXPECT_EQ(memUsage, 42.5); // Should return mock value
}

TEST_F(TransTableBaseTest, LookupMethodCallsOverride)
{
    const unsigned short aggrTarget[DDS_SUITS] = {0x1111, 0x2222, 0x3333, 0x4444};
    const int hand_dist[4] = {13, 13, 13, 13};
    bool lowerFlag = true;
    
    EXPECT_FALSE(baseTable->lookup_called_);
    
    NodeCards const* result = baseTable->lookup(
        10, // trick
        2,  // hand
        aggrTarget,
        hand_dist,
        8,  // limit
        lowerFlag
    );
    
    EXPECT_TRUE(baseTable->lookup_called_);
    EXPECT_EQ(baseTable->last_trick_, 10);
    EXPECT_EQ(baseTable->last_hand_, 2);
    EXPECT_EQ(baseTable->last_limit_, 8);
    EXPECT_EQ(lowerFlag, baseTable->test_lower_flag_); // Should be modified by override
    EXPECT_EQ(result, baseTable->test_lookup_result_);
}

TEST_F(TransTableBaseTest, AddMethodCallsOverride)
{
    const unsigned short aggrTarget[DDS_SUITS] = {0x1111, 0x2222, 0x3333, 0x4444};
    const unsigned short win_ranks[DDS_SUITS] = {0x5555, 0x6666, 0x7777, 0x8888};
    NodeCards nodeData;
    
    EXPECT_FALSE(baseTable->add_called_);
    
    baseTable->add(
        8,  // trick
        1,  // hand
        aggrTarget,
        win_ranks,
        nodeData,
        true // flag
    );
    
    EXPECT_TRUE(baseTable->add_called_);
    EXPECT_EQ(baseTable->add_trick_, 8);
    EXPECT_EQ(baseTable->add_hand_, 1);
    EXPECT_TRUE(baseTable->add_flag_);
}

TEST_F(TransTableBaseTest, PrintMethodsCallOverride)
{
    std::ofstream testFile("test_output.txt");
    
    EXPECT_FALSE(baseTable->print_suits_called_);
    EXPECT_FALSE(baseTable->print_all_suits_called_);
    
    baseTable->print_suits(testFile, 5, 2); // trick=5, hand=2
    EXPECT_TRUE(baseTable->print_suits_called_);
    
    baseTable->print_all_suits(testFile);
    EXPECT_TRUE(baseTable->print_all_suits_called_);
    
    testFile.close();
    std::remove("test_output.txt"); // Cleanup
}

// Interface Contract Tests
TEST_F(TransTableBaseTest, AllVirtualMethodsHaveExpectedSignatures)
{
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
    unsigned short win_ranks[DDS_SUITS] = {0, 0, 0, 0};
    int hand_dist[4] = {0, 0, 0, 0};
    bool lowerFlag = false;
    NodeCards nodeData;
    
    // Should not crash when called through base pointer
    basePtr->lookup(0, 0, aggrTarget, hand_dist, 0, lowerFlag);
    basePtr->add(0, 0, aggrTarget, win_ranks, nodeData, false);
    
    std::ofstream nullFile("/dev/null");
    basePtr->print_suits(nullFile, 0, 1); // trick=0, hand=1
    basePtr->print_all_suits(nullFile);
    nullFile.close();
}

TEST_F(TransTableBaseTest, PolymorphicBehaviorWorks)
{
    // Verify that virtual dispatch works correctly
    TransTable* basePtr = baseTable.get();
    
    // Calls should go to derived class implementations
    basePtr->make_tt();
    EXPECT_TRUE(baseTable->tt_made_);
    
    basePtr->return_all_memory();
    EXPECT_TRUE(baseTable->memory_returned_);
    
    double memUsage = basePtr->memory_in_use();
    EXPECT_EQ(memUsage, 42.5); // Should call derived implementation
}

} // namespace dds_test
