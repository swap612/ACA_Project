/*
 *  Author: Group-5 
 *  This file contains an ISA-portable PIN tool for ACA assignment 2-I.
 */

#include <stdio.h>
#include "pin.H"

//File and Pinlock variable declarations
FILE * trace;
PIN_LOCK pinLock;

//Prints the threadid and memory operand address in terms of 1, 2, 4 or 8 bytes machine access in trace file
VOID getMachineAddress(unsigned long long opAddr, UINT32 opSize, THREADID threadid, char operation)
{
    //Calculate the blockboundary by performing OR with last 6 bits. 
    unsigned long blockBoundary = opAddr | 0x000000000000003F;
    
    //Distribute the operand access in 1, 2, 4 and 8 bytes
    while (opSize > 0)
    {
        //Check if opsize >=8 and after allocating the 8 bytes the machine access is in the same block
        while ((opAddr + 7) <= blockBoundary && opSize >= 8)
        {
            fprintf(trace,"%d %llx %c \n",threadid, opAddr, operation);
            opAddr += 8;
            opSize -= 8;
           
        }
        
        //Check if opsize >=4 and after allocating the 4 bytes the machine access is in the same block
        while ((opAddr + 3) <= blockBoundary && opSize >= 4)
        {
            fprintf(trace,"%d %llx %c \n",threadid, opAddr, operation);
            opAddr += 4;
            opSize -= 4;
           
        }

        //Check if opsize >=2 and after allocating the 2 bytes the machine access is in the same block
        while ((opAddr + 1) <= blockBoundary && opSize >= 2)
        {
            fprintf(trace,"%d %llx %c \n",threadid, opAddr, operation);
            opAddr += 2;
            opSize -= 2;
           
        }

        //Check if opsize >=1 and after allocating the 1 bytes the machine access is in the same block
        while ((opAddr) <= blockBoundary && opSize >= 1)
        {
            fprintf(trace,"%d %llx %c \n",threadid, opAddr, operation);
            opAddr += 1;
            opSize -= 1;
        }

        //If operand size > 0, allocate the new block by adding 64 to block boundary  
        if (opSize > 0)
            blockBoundary += 64;
    }
   
}

//takes threadid, operand size and operand address and pass to the getMachineAddress function to machine access
VOID RecordReadMemAccess(VOID * ip, VOID * addr, UINT32 size, THREADID threadid)
{
    PIN_GetLock(&pinLock, threadid+1);
    unsigned long long addr1 = (unsigned long long)addr;
    getMachineAddress(addr1, size, threadid, 'R');
    fflush(trace);
    PIN_ReleaseLock(&pinLock);
}

//takes threadid, operand size and operand address and pass to the getMachineAddress function to machine access
VOID RecordWriteMemAccess(VOID * ip, VOID * addr, UINT32 size, THREADID threadid)
{
    PIN_GetLock(&pinLock, threadid+1);
    unsigned long long addr1 = (unsigned long long)addr;
    getMachineAddress(addr1, size, threadid, 'W');
    fflush(trace);
    PIN_ReleaseLock(&pinLock);
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
    //count the number of memory operands in the instruction
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        //calculate the size of the memory operand
        UINT32 opSize = INS_MemoryOperandSize(ins, memOp);

        //if Memory operand is read
        if (INS_MemoryOperandIsRead(ins, memOp))
        {   
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordReadMemAccess,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_UINT32, opSize,
                IARG_THREAD_ID,
                IARG_END);
        }
        //if Mem operand is write
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordWriteMemAccess,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_UINT32, opSize,
                IARG_THREAD_ID,
                IARG_END);
        }
    }

}

VOID Fini(INT32 code, VOID *v)
{
    fclose(trace);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */
   
INT32 Usage()
{
    PIN_ERROR( "This Pintool prints a trace of memory addresses\n" 
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main Function                                                         */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv)) return Usage();
    
    //open file to write the traces
    trace = fopen("addrtrace.out", "w");

    INS_AddInstrumentFunction(Instruction, 0);

    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}
