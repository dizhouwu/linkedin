#include <gtest/gtest.h>
#include "../wub_shared_ptr.cpp"

// Test fixture class
class SharedPtrTest : public ::testing::Test {};

// Test basic construction and destruction
TEST_F(SharedPtrTest, BasicConstruction) {
    SharedPtr<int> ptr(new int(10));
    EXPECT_EQ(*ptr, 10);
    EXPECT_EQ(ptr.use_count(), 1);
}

// Test copy constructor and reference counting
TEST_F(SharedPtrTest, CopyConstructor) {
    SharedPtr<int> ptr1(new int(20));
    SharedPtr<int> ptr2(ptr1);

    EXPECT_EQ(ptr1.use_count(), 2);
    EXPECT_EQ(ptr2.use_count(), 2);
    EXPECT_EQ(*ptr1, 20);
    EXPECT_EQ(*ptr2, 20);
}

// Test move constructor
TEST_F(SharedPtrTest, MoveConstructor) {
    SharedPtr<int> ptr1(new int(30));
    EXPECT_EQ(ptr1.use_count(), 1);

    SharedPtr<int> ptr2(std::move(ptr1));
    EXPECT_EQ(ptr1.get(), nullptr); // ptr1 should be null after move
    EXPECT_EQ(ptr2.use_count(), 1);
    EXPECT_EQ(*ptr2, 30);
}

// Test copy assignment operator and reference counting
TEST_F(SharedPtrTest, CopyAssignment) {
    SharedPtr<int> ptr1(new int(40));
    SharedPtr<int> ptr2;
    ptr2 = ptr1;

    EXPECT_EQ(ptr1.use_count(), 2);
    EXPECT_EQ(ptr2.use_count(), 2);
    EXPECT_EQ(*ptr1, 40);
    EXPECT_EQ(*ptr2, 40);
}

// Test move assignment operator
TEST_F(SharedPtrTest, MoveAssignment) {
    SharedPtr<int> ptr1(new int(50));
    SharedPtr<int> ptr2;
    ptr2 = std::move(ptr1);

    EXPECT_EQ(ptr1.get(), nullptr); // ptr1 should be null after move
    EXPECT_EQ(ptr2.use_count(), 1);
    EXPECT_EQ(*ptr2, 50);
}

// Test unique ownership
TEST_F(SharedPtrTest, Unique) {
    SharedPtr<int> ptr(new int(60));
    EXPECT_TRUE(ptr.unique());

    SharedPtr<int> ptr_copy(ptr);
    EXPECT_FALSE(ptr.unique());
    EXPECT_FALSE(ptr_copy.unique());
}

// Test releasing ownership
TEST_F(SharedPtrTest, ReleaseOwnership) {
    SharedPtr<int> ptr1(new int(70));
    SharedPtr<int> ptr2(ptr1);
    EXPECT_EQ(ptr1.use_count(), 2);

    ptr1.release();
    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_EQ(ptr1.use_count(), 0);
    EXPECT_EQ(ptr2.use_count(), 1);
}

// Test dereference operators
TEST_F(SharedPtrTest, DereferenceOperators) {
    SharedPtr<int> ptr(new int(80));
    EXPECT_EQ(*ptr, 80);
}

// Test nullptr behavior
TEST_F(SharedPtrTest, NullptrBehavior) {
    SharedPtr<int> ptr;
    EXPECT_EQ(ptr.get(), nullptr);
    EXPECT_EQ(ptr.use_count(), 0);
    EXPECT_FALSE(ptr.unique());
}

// Test my_make_shared with basic construction
TEST_F(SharedPtrTest, MyMakeSharedBasic) {
    auto ptr = my_make_shared<int>(100); // Create an int with value 100
    EXPECT_EQ(*ptr, 100);
    EXPECT_EQ(ptr.use_count(), 1);
}

// Test my_make_shared with a complex object
TEST_F(SharedPtrTest, MyMakeSharedComplexObject) {
    struct TestStruct {
        int a;
        double b;
        TestStruct(int x, double y) : a(x), b(y) {}
    };

    auto ptr = my_make_shared<TestStruct>(42, 3.14); // Create TestStruct with args
    EXPECT_EQ(ptr->a, 42);
    EXPECT_EQ(ptr->b, 3.14);
    EXPECT_EQ(ptr.use_count(), 1);
}

// Test my_make_shared with default-constructible type
TEST_F(SharedPtrTest, MyMakeSharedDefaultConstructible) {
    struct DefaultConstructible {
        int x = 99;
    };

    auto ptr = my_make_shared<DefaultConstructible>();
    EXPECT_EQ(ptr->x, 99);
    EXPECT_EQ(ptr.use_count(), 1);
}

// Test my_make_shared reference counting
TEST_F(SharedPtrTest, MyMakeSharedReferenceCounting) {
    auto ptr1 = my_make_shared<int>(10);
    EXPECT_EQ(ptr1.use_count(), 1);

    SharedPtr<int> ptr2 = ptr1; // Copy construction
    EXPECT_EQ(ptr1.use_count(), 2);
    EXPECT_EQ(ptr2.use_count(), 2);

    ptr2.release(); // Release ownership of ptr2
    EXPECT_EQ(ptr1.use_count(), 1);
    EXPECT_EQ(ptr2.use_count(), 0);
}

// Test my_make_shared move semantics
TEST_F(SharedPtrTest, MyMakeSharedMoveSemantics) {
    auto ptr1 = my_make_shared<int>(50);
    EXPECT_EQ(ptr1.use_count(), 1);

    SharedPtr<int> ptr2 = std::move(ptr1); // Move construction
    EXPECT_EQ(ptr1.get(), nullptr);       // ptr1 should be null after move
    EXPECT_EQ(ptr2.use_count(), 1);
    EXPECT_EQ(*ptr2, 50);
}

// Test my_make_shared with nullptr behavior
TEST_F(SharedPtrTest, MyMakeSharedNullptrBehavior) {
    auto ptr = my_make_shared<int>(); // No arguments passed; creates a default-initialized int
    EXPECT_EQ(*ptr, 0);               // Default initialization for int
    EXPECT_EQ(ptr.use_count(), 1);
}

// Test releasing ownership of a my_make_shared object
TEST_F(SharedPtrTest, MyMakeSharedReleaseOwnership) {
    auto ptr1 = my_make_shared<int>(70);
    SharedPtr<int> ptr2(ptr1);
    EXPECT_EQ(ptr1.use_count(), 2);

    ptr1.release();
    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_EQ(ptr1.use_count(), 0);
    EXPECT_EQ(ptr2.use_count(), 1);
}