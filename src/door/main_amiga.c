#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <proto/exec.h>

int amiweather_door_amiga_run(void);

#define AMIWEATHER_DOOR_STACK_SIZE 32768UL

int main(void)
{
    APTR stack;
    struct StackSwapStruct swap;
    int result;

    stack = AllocMem(AMIWEATHER_DOOR_STACK_SIZE, MEMF_PUBLIC | MEMF_CLEAR);
    if (!stack) return 20;
    swap.stk_Lower = stack;
    swap.stk_Upper = (ULONG)stack + AMIWEATHER_DOOR_STACK_SIZE;
    swap.stk_Pointer = (APTR)(swap.stk_Upper & ~3UL);
    StackSwap(&swap);
    result = amiweather_door_amiga_run();
    StackSwap(&swap);
    FreeMem(stack, AMIWEATHER_DOOR_STACK_SIZE);
    return result;
}
