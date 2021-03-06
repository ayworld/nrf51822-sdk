#include "lx_heap.h"

#include "../lx_nrf51Kit.h"


__align(4) static uint8_t heap[HEAP_SIZE];													
					
static uint16_t heapAllocTable[HEAP_ALLOC_TABLE_SIZE];

static volatile bool heapReady = false;

void *lx_malloc(uint32_t size)
{   
    if (size <= 0)
    {
        return NULL;
    }

    LX_ENTER_CRITICAL();

    if (!heapReady)
    {
        memset(heap, 0, HEAP_SIZE);
        memset(heapAllocTable, 0, HEAP_ALLOC_TABLE_SIZE);

        heapReady = true;
    }

    int blockNum = (size - 1) / HEAP_BLOCK_SIZE + 1;
    
    int blockCount = 0;
    for (int i = 0; i < HEAP_ALLOC_TABLE_SIZE; i++)  
    {     
        /* find a row of empty blocks large enough to hold the size required */
        if (heapAllocTable[i] != 0)
        {
            blockCount = 0;
            continue;
        }
        
        blockCount++;

        if (blockCount == blockNum)
        {
            /*
             * Note: This line of code is the key to heap management.
             * Every time a piece of memory is allocated to an user, the block number shall be
             * saved in the corresponding position of heapAllocTable. The value blockNum has two
             * functions: 1. mark the memory block as occupied; 2. record the number of block used.
             * Once we want to free a pre-allocated memory by calling void lx_free(void *p), the 
             * function will find the pointer of the memory in heap by parameter *p, and retrieve
             * the block number which *p is pointing to by reading heapAllocTable, which value shall 
             * be blockNum.
             * See lx_free for more information. 
             */
            int blockIndex = i + 1 - blockNum;
            for (int j = 0; j < blockNum; j++)
            {
                heapAllocTable[blockIndex + j] = blockNum;
            }

            uint8_t *p = heap + HEAP_BLOCK_SIZE * (i + 1 - blockNum);

            LX_EXIT_CRITICAL();

            return p;
        }
    }  

    LX_EXIT_CRITICAL();
    return NULL;
}  

void lx_free(void *p)
{   
    if (p == NULL)
    {
        return;
    }

    LX_ENTER_CRITICAL();

    if (!heapReady)
    {
        LX_EXIT_CRITICAL();
        return;
    }

    uint32_t offset = (uint8_t *)p - heap;
    if (offset >= HEAP_SIZE)
    {
        LX_EXIT_CRITICAL();
        return;
    }

    uint32_t blockIndex = offset / HEAP_BLOCK_SIZE;
    uint16_t blockNum = heapAllocTable[blockIndex];

    for (int i = 0; i < blockNum; i++)
    {
        heapAllocTable[blockIndex + i] = 0;
    }

    LX_EXIT_CRITICAL();
}   

uint32_t lx_memusage()  
{  
    int used = 0;

    if (!heapReady)
    {
        return 0;
    }
    
    for (int i = 0; i < HEAP_ALLOC_TABLE_SIZE; i++)  
    {  
        if (heapAllocTable[i] != 0)
        {
            used++;
        } 
    } 

    return (used * 100) / HEAP_ALLOC_TABLE_SIZE;  
}  
          

