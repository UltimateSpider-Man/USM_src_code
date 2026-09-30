#include <gtest/gtest.h>

#include <vm_stack.h>

TEST(VmStack, Construct)
{
    vm_stack stack {nullptr};
    EXPECT_EQ(stack.size(), 0);
    EXPECT_EQ(((int *)stack.get_buffer())[0], UNINITIALIZED_SCRIPT_PARM);
}

TEST(VmStack, PushNum)
{
    vm_stack stack {nullptr};

    vm_num_t num {1};
    stack.push(num);

    EXPECT_EQ(stack.size(), sizeof(vm_num_t));
    EXPECT_EQ(stack.top_num(), vm_num_t {1});
}

TEST(VmStack, PopNum)
{
    vm_stack stack {nullptr};

    vm_num_t num {1};
    stack.push(num);
    vm_num_t num1 = stack.pop_num();

    EXPECT_EQ(num1, num);
    EXPECT_EQ(stack.size(), 0);
}

TEST(VmStack, MoveSP)
{
    vm_stack stack {nullptr};

    stack.move_SP(8u);
    EXPECT_EQ(stack.size(), 8u);

    stack.move_SP(-4);
    EXPECT_EQ(stack.size(), 4u);
}

TEST(VmStack, Push)
{
    vm_stack stack {nullptr};

    const char *str {"main"};
    auto len = strlen(str);
    stack.push(str, len);
    EXPECT_EQ(stack.size(), 4u);
}

TEST(VmStack, Pop)
{
    vm_stack stack {nullptr};
    stack.move_SP(8u);

    stack.pop(4u);
    EXPECT_EQ(stack.size(), 4u);
}
