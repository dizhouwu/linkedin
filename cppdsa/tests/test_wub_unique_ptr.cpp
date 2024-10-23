#include <gtest/gtest.h>
#include "../wub_unique_ptr.cpp" // Adjust the path as needed

class MyClass {
public:
    MyClass() { }
    ~MyClass() { }
    void sayHello() const { /* ... */ }
};

TEST(UniquePtrTest, DefaultConstructor) {
    UniquePtr<MyClass> ptr;
    EXPECT_EQ(ptr.get(), nullptr);
}

TEST(UniquePtrTest, ConstructorWithPointer) {
    UniquePtr<MyClass> ptr(new MyClass());
    EXPECT_NE(ptr.get(), nullptr);
}

TEST(UniquePtrTest, MoveConstructor) {
    UniquePtr<MyClass> ptr1(new MyClass());
    UniquePtr<MyClass> ptr2(std::move(ptr1));
    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_NE(ptr2.get(), nullptr);
}

TEST(UniquePtrTest, MoveAssignment) {
    UniquePtr<MyClass> ptr1(new MyClass());
    UniquePtr<MyClass> ptr2;
    ptr2 = std::move(ptr1);
    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_NE(ptr2.get(), nullptr);
}

TEST(UniquePtrTest, Release) {
    UniquePtr<MyClass> ptr(new MyClass());
    MyClass* rawPtr = ptr.release();
    EXPECT_EQ(ptr.get(), nullptr);
    delete rawPtr; // Clean up manually
}

TEST(UniquePtrTest, Reset) {
    UniquePtr<MyClass> ptr(new MyClass());
    ptr.reset(new MyClass());
    EXPECT_NE(ptr.get(), nullptr);
}


TEST(UniquePtrTest, ResetWithNullptr) {
    UniquePtr<MyClass> ptr(new MyClass());
    ptr.reset(nullptr);
    EXPECT_EQ(ptr.get(), nullptr);
}

TEST(UniquePtrTest, SelfMoveAssignment) {
    UniquePtr<MyClass> ptr(new MyClass());
    ptr = std::move(ptr); // Self-move
    EXPECT_NE(ptr.get(), nullptr);
}

TEST(UniquePtrTest, CopyConstructor) {
    // Ensure that copying a UniquePtr fails to compile
    // Uncommenting the next line should result in a compile-time error
    // UniquePtr<MyClass> copy(ptr); // This should fail to compile if UniquePtr is non-copyable
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
