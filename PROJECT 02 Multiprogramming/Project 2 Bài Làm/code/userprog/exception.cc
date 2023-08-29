// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// If you are handling a system call, don't forget to increment the pc
// before returning (by calling move_program_counter()). (Or else you'll loop
// making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions
//	is in machine.h.
//----------------------------------------------------------------------

/**
 * @brief Convert user string to system string
 *
 * @param addr addess of user string
 * @param convert_length set max length of string to convert, leave
 * blank to convert all characters of user string
 * @return char*
 */
char* User2Sys(int addr, int convert_len = -1) {
    int len = 0;
    bool stop = false;
    char* str;

    do {
        int oneChar;
        kernel->machine->ReadMem(addr + len, 1, &oneChar);
        len++;
        // if convert_length == -1, we use '\0' to terminate the process
        // otherwise, we use convert_length to terminate the process
        stop = ((oneChar == '\0' && convert_len == -1) ||
                len == convert_len);
    } while (!stop);

    str = new char[len];
    for (int i = 0; i < len; i++) {
        int oneChar;
        kernel->machine->ReadMem(addr + i, 1,
                                 &oneChar);  // copy characters to kernel space
        str[i] = (unsigned char)oneChar;
    }
    return str;
}

/**
 * @brief Convert system string to user string
 *
 * @param str string to convert
 * @param addr addess of user string
 * @param convert_length set max length of string to convert, leave
 * blank to convert all characters of system string
 * @return void
 */
void Sys2User(char* str, int addr, int convert_len = -1) {
    int len = (convert_len == -1 ? strlen(str) : convert_len);
    for (int i = 0; i < len; i++) {
        kernel->machine->WriteMem(addr + i, 1, str[i]);  // copy characters to user space
    }
    kernel->machine->WriteMem(addr + len, 1, '\0');
}

/**
 * Modify program counter
 * This code is adapted from `../machine/mipssim.cc`, line 667
 **/
void Up_PC() {
    kernel->machine->WriteRegister(PrevPCReg,kernel->machine->ReadRegister(PCReg));
    kernel->machine->WriteRegister(PCReg,kernel->machine->ReadRegister(NextPCReg));
    kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(NextPCReg) + 4);
}

void Treat_Not_Implemented(int type) {
    DEBUG(dbgSys, "Not yet implemented syscall " << type << "\n");
    return Up_PC();
}

void Halt_Handling() {
    DEBUG(dbgSys, "\nShutdown, initiated by user program.\n");
    SysHalt();
    ASSERTNOTREACHED();
}

void ReadChar_Handling() {
    char result = SysReadChar();
    kernel->machine->WriteRegister(2, (int)result);
    return Up_PC();
}

void PrintChar_Handling() {
    char character = (char)kernel->machine->ReadRegister(4);
    SysPrintChar(character);
    return Up_PC();
}

#define MAX_READ_STRING_LENGTH 255
void ReadString_Handling() {
    int memPtr = kernel->machine->ReadRegister(4);  // read address of C-string
    int length = kernel->machine->ReadRegister(5);  // read length of C-string
    if (length > MAX_READ_STRING_LENGTH) {  // avoid allocating large memory
        DEBUG(dbgSys, "String length exceeds " << MAX_READ_STRING_LENGTH);
        SysHalt();
    }
    char* buffer = SysReadString(length);
    Sys2User(buffer, memPtr);
    delete[] buffer;
    return Up_PC();
}

void PrintString_Handling() {
    int memPtr = kernel->machine->ReadRegister(4);  // read address of C-string
    char* buffer = User2Sys(memPtr);

    SysPrintString(buffer, strlen(buffer));
    delete[] buffer;
    return Up_PC();
}

void Create_Handling() {
    int virtAddr = kernel->machine->ReadRegister(4);
    char* fileName = User2Sys(virtAddr);

    if (SysCreate(fileName))
        kernel->machine->WriteRegister(2, 0);
    else
        kernel->machine->WriteRegister(2, -1);

    delete[] fileName;
    return Up_PC();
}

void Open_Handling() {
    int virtAddr = kernel->machine->ReadRegister(4);
    char* fileName = User2Sys(virtAddr);
    int type = kernel->machine->ReadRegister(5);

    kernel->machine->WriteRegister(2, SysOpen(fileName, type));

    delete fileName;
    return Up_PC();
}

void Close_Handling() {
    int id = kernel->machine->ReadRegister(4);
    kernel->machine->WriteRegister(2, SysClose(id));

    return Up_PC();
}

void Read_Handling() {
    int virtAddr = kernel->machine->ReadRegister(4);
    int charCount = kernel->machine->ReadRegister(5);
    char* buffer = User2Sys(virtAddr, charCount);
    int fileId = kernel->machine->ReadRegister(6);

    DEBUG(dbgFile,
          "Read " << charCount << " chars from file " << fileId << "\n");

    kernel->machine->WriteRegister(2, SysRead(buffer, charCount, fileId));
    Sys2User(buffer, virtAddr, charCount);

    delete[] buffer;
    return Up_PC();
}

void Write_Handling() {
    int virtAddr = kernel->machine->ReadRegister(4);
    int charCount = kernel->machine->ReadRegister(5);
    char* buffer = User2Sys(virtAddr, charCount);
    int fileId = kernel->machine->ReadRegister(6);

    DEBUG(dbgFile,
          "Write " << charCount << " chars to file " << fileId << "\n");

    kernel->machine->WriteRegister(2, SysWrite(buffer, charCount, fileId));
    Sys2User(buffer, virtAddr, charCount);

    delete[] buffer;
    return Up_PC();
}

void Seek_Handling() {
    int seekPos = kernel->machine->ReadRegister(4);
    int fileId = kernel->machine->ReadRegister(5);

    kernel->machine->WriteRegister(2, SysSeek(seekPos, fileId));

    return Up_PC();
}

void Exec_Handling() {
    int virtAddr;
    virtAddr = kernel->machine->ReadRegister(4);  // doc dia chi ten chuong trinh tu thanh ghi r4
    char* name;
    name = User2Sys(virtAddr);  // Lay ten chuong trinh, nap vao kernel
    if (name == NULL) {
        DEBUG(dbgSys, "\n Not enough memory in System");
        ASSERT(false);
        kernel->machine->WriteRegister(2, -1);
        return Up_PC();
    }

    kernel->machine->WriteRegister(2, SysExec(name));

    return Up_PC();
}

void Join_Handling() {
    int id = kernel->machine->ReadRegister(4);
    kernel->machine->WriteRegister(2, SysJoin(id));
    return Up_PC();
}

void Exit_Handling() {
    int id = kernel->machine->ReadRegister(4);
    kernel->machine->WriteRegister(2, SysExit(id));
    return Up_PC();
}

void CreateSemaphore_Handling() {
    int virtAddr = kernel->machine->ReadRegister(4);
    int semval = kernel->machine->ReadRegister(5);

    char* name = User2Sys(virtAddr);
    if (name == NULL) {
        DEBUG(dbgSys, "\n Not enough memory in System");
        ASSERT(false);
        kernel->machine->WriteRegister(2, -1);
        delete[] name;
        return Up_PC();
    }

    kernel->machine->WriteRegister(2, SysCreateSemaphore(name, semval));
    delete[] name;
    return Up_PC();
}

void Wait_Handling() {
    int virtAddr = kernel->machine->ReadRegister(4);

    char* name = User2Sys(virtAddr);
    if (name == NULL) {
        DEBUG(dbgSys, "\n Not enough memory in System");
        ASSERT(false);
        kernel->machine->WriteRegister(2, -1);
        delete[] name;
        return Up_PC();
    }

    kernel->machine->WriteRegister(2, SysWait(name));
    delete[] name;
    return Up_PC();
}

void Signal_Handling() {
    int virtAddr = kernel->machine->ReadRegister(4);

    char* name = User2Sys(virtAddr);
    if (name == NULL) {
        DEBUG(dbgSys, "\n Not enough memory in System");
        ASSERT(false);
        kernel->machine->WriteRegister(2, -1);
        delete[] name;
        return Up_PC();
    }

    kernel->machine->WriteRegister(2, SysSignal(name));
    delete[] name;
    return Up_PC();
}

void ExceptionHandler(ExceptionType which) {
    int type = kernel->machine->ReadRegister(2);

    DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");

    switch (which) {
        case NoException:  // return control to kernel
            kernel->interrupt->setStatus(SystemMode);
            DEBUG(dbgSys, "Switch to system mode\n");
            break;
        case PageFaultException:
        case ReadOnlyException:
        case BusErrorException:
        case AddressErrorException:
        case OverflowException:
        case IllegalInstrException:
        case NumExceptionTypes:
            cerr << "Error " << which << " occurs\n";
            SysHalt();
            ASSERTNOTREACHED();

        case SyscallException:
            switch (type) {
                case SC_Halt:
                    return Halt_Handling();
                case SC_ReadChar:
                    return ReadChar_Handling();
                case SC_PrintChar:
                    return PrintChar_Handling();
                case SC_ReadString:
                    return ReadString_Handling();
                case SC_PrintString:
                    return PrintString_Handling();
                case SC_Create:
                    return Create_Handling();
                case SC_Open:
                    return Open_Handling();
                case SC_Close:
                    return Close_Handling();
                case SC_Read:
                    return Read_Handling();
                case SC_Write:
                    return Write_Handling();
                case SC_Seek:
                    return Seek_Handling();
                case SC_Exec:
                    return Exec_Handling();
                case SC_Join:
                    return Join_Handling();
                case SC_Exit:
                    return Exit_Handling();
                case SC_CreateSemaphore:
                    return CreateSemaphore_Handling();
                case    SC_Wait:
                    return Wait_Handling();
                case SC_Signal:
                    return Signal_Handling();

                case SC_Remove:
                case SC_ThreadFork:
                case SC_ThreadYield:
                case SC_ExecV:
                case SC_ThreadExit:
                case SC_ThreadJoin:
                    return Treat_Not_Implemented(type);

                default:
                    cerr << "Unexpected system call " << type << "\n";
                    break;
            }
            break;
        default:
            cerr << "Unexpected user mode exception" << (int)which << "\n";
            break;
    }
    ASSERTNOTREACHED();
}
