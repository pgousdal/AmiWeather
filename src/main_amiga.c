#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <proto/exec.h>

int amiweather_gui_run(void);

static const char stack_cookie[] __attribute__((used)) = "$STACK:32768";
#define AMIWEATHER_STACK_SIZE 32768UL

static int amiweather_main_on_private_stack(void)
{
    return amiweather_gui_run();
}

int main(void)
{
    APTR stack;
    struct StackSwapStruct swap;
    int result;

    stack = AllocMem(AMIWEATHER_STACK_SIZE, MEMF_PUBLIC | MEMF_CLEAR);
    if (!stack)
        return 20;
    swap.stk_Lower = stack;
    swap.stk_Upper = (ULONG)stack + AMIWEATHER_STACK_SIZE;
    swap.stk_Pointer = (APTR)(swap.stk_Upper & ~3UL);
    StackSwap(&swap);
    result = amiweather_main_on_private_stack();
    StackSwap(&swap);
    FreeMem(stack, AMIWEATHER_STACK_SIZE);
    return result;
}
