#include <gtest/gtest.h>
#include <memory>
#include "library/src/trans_table/TransTable.h"

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

        void Init(const int handLookup[][15]) override {
            initCalled = true;
        }

        void SetMemoryDefault(const int megabytes) override {
            defaultMemory = megabytes;
        }

        void SetMemoryMaximum(const int megabytes) override {
            maximumMemory = megabytes;
        }

        void MakeTT() override {
            ttMade = true;
        }

        void ResetMemory(const TTresetReason reason) override {
            lastResetReason = reason;
        }

        void ReturnAllMemory() override {
            memoryReturned = true;
        }

        double MemoryInUse() const override {
            return testMemoryUsage;
        }

        nodeCardsType const * Lookup(
            const int trick,
            const int hand,
            const unsigned short aggrTarget[],
            const int handDist[],
            const int limit,
            bool& lowerFlag) override {
            
            lookupCalled = true;
            lastTrick = trick;
            lastHand = hand;
            lastLimit = limit;
            lowerFlag = testLowerFlag;
            return testLookupResult;
        }

        void Add(
            const int trick,
            const int hand,
            const unsigned short aggrTarget[],
            const unsigned short winRanksArg[],
            const nodeCardsType& first,
            const bool flag) override {
            
            addCalled = true;
            addTrick = trick;
            addHand = hand;
            addFlag = flag;
        }

        void PrintSuits(
            std::ofstream& fout,
            const int trick,
            const int hand) const override {
            
            printSuitsCalled = true;
        }

        void PrintAllSuits(std::ofstream& fout) const override {
            printAllSuitsCalled = true;
        }

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
        TTresetReason lastResetReason = TT_RESET_UNKNOWN;
        
        // Lookup test state
        int lastTrick = -1;
        int lastHand = -1;
        int lastLimit = -1;
        bool testLowerFlag = false;
        nodeCardsType* testLookupResult = nullptr;
        
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
    baseTable->Init(handLookup);
    EXPECT_TRUE(baseTable->initCalled);
}

TEST_F(TransTableBaseTest, SetMemoryMethodsWork) {
    baseTable->SetMemoryDefault(64);
    EXPECT_EQ(baseTable->defaultMemory, 64);
    
    baseTable->SetMemoryMaximum(128);
    EXPECT_EQ(baseTable->maximumMemory, 128);
}

TEST_F(TransTableBaseTest, MakeTTMethodCallsOverride) {
    EXPECT_FALSE(baseTable->ttMade);
    baseTable->MakeTT();
    EXPECT_TRUE(baseTable->ttMade);
}

TEST_F(TransTableBaseTest, ResetMemoryMethodCallsOverride) {
    EXPECT_EQ(baseTable->lastResetReason, TT_RESET_UNKNOWN);
    
    baseTable->ResetMemory(TT_RESET_NEW_DEAL);
    EXPECT_EQ(baseTable->lastResetReason, TT_RESET_NEW_DEAL);
    
    baseTable->ResetMemory(TT_RESET_MEMORY_EXHAUSTED);
    EXPECT_EQ(baseTable->lastResetReason, TT_RESET_MEMORY_EXHAUSTED);
}

TEST_F(TransTableBaseTest, ReturnAllMemoryMethodCallsOverride) {
    EXPECT_FALSE(baseTable->memoryReturned);
    baseTable->ReturnAllMemory();
    EXPECT_TRUE(baseTable->memoryReturned);
}

TEST_F(TransTableBaseTest, MemoryInUseMethodCallsOverride) {
    // Test that virtual method calls override implementation
    double memUsage = baseTable->MemoryInUse();
    EXPECT_EQ(memUsage, 42.5); // Should return mock value
}

TEST_F(TransTableBaseTest, LookupMethodCallsOverride) {
    unsigned short aggrTarget[DDS_SUITS] = {0x1111, 0x2222, 0x3333, 0x4444};
    int handDist[4] = {13, 13, 13, 13};
    bool lowerFlag = true;
    
    EXPECT_FALSE(baseTable->lookupCalled);
    
    nodeCardsType const* result = baseTable->Lookup(
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
    nodeCardsType nodeData;
    
    EXPECT_FALSE(baseTable->addCalled);
    
    baseTable->Add(
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
    
    baseTable->PrintSuits(testFile, 5, 2); // trick=5, hand=2
    EXPECT_TRUE(baseTable->printSuitsCalled);
    
    baseTable->PrintAllSuits(testFile);
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
    basePtr->Init(handLookup);
    
    basePtr->SetMemoryDefault(64);
    basePtr->SetMemoryMaximum(128);
    basePtr->MakeTT();
    basePtr->ResetMemory(TT_RESET_NEW_DEAL);
    basePtr->ReturnAllMemory();
    
    double mem = basePtr->MemoryInUse();
    EXPECT_GE(mem, 0.0); // Should return non-negative value
    
    unsigned short aggrTarget[DDS_SUITS] = {0, 0, 0, 0};
    unsigned short winRanks[DDS_SUITS] = {0, 0, 0, 0};
    int handDist[4] = {0, 0, 0, 0};
    bool lowerFlag = false;
    nodeCardsType nodeData;
    
    // Should not crash when called through base pointer
    basePtr->Lookup(0, 0, aggrTarget, handDist, 0, lowerFlag);
    basePtr->Add(0, 0, aggrTarget, winRanks, nodeData, false);
    
    std::ofstream nullFile("/dev/null");
    basePtr->PrintSuits(nullFile, 0, 1); // trick=0, hand=1
    basePtr->PrintAllSuits(nullFile);
    nullFile.close();
}

TEST_F(TransTableBaseTest, PolymorphicBehaviorWorks) {
    // Verify that virtual dispatch works correctly
    TransTable* basePtr = baseTable.get();
    
    // Calls should go to derived class implementations
    basePtr->MakeTT();
    EXPECT_TRUE(baseTable->ttMade);
    
    basePtr->ReturnAllMemory();
    EXPECT_TRUE(baseTable->memoryReturned);
    
    double memUsage = basePtr->MemoryInUse();
    EXPECT_EQ(memUsage, 42.5); // Should call derived implementation
}

} // namespace dds_test
