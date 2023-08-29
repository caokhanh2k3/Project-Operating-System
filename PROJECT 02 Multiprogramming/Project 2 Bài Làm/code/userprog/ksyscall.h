/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__
#define __USERPROG_KSYSCALL_H__

#include "kernel.h"
#include "synchconsole.h"
#include <stdlib.h>

void SysHalt() { kernel->interrupt->Halt(); }

char SysReadChar() { return kernel->synchConsoleIn->GetChar(); }

void SysPrintChar(char character) {
    kernel->synchConsoleOut->PutChar(character);
}

char* SysReadString(int length) {
    char* buffer = new char[length + 1];
    for (int i = 0; i < length; i++) {
        buffer[i] = SysReadChar();
    }
    buffer[length] = '\0';
    return buffer;
}

void SysPrintString(char* buffer, int length) {
    for (int i = 0; i < length; i++) {
        kernel->synchConsoleOut->PutChar(buffer[i]);
    }
}

bool SysCreate(char* fileName) {
    bool success;
    int fileNameLength = strlen(fileName);

    if (fileNameLength == 0) {
        DEBUG(dbgSys, "\nFile name can't be empty");
        success = false;

    } else if (fileName == NULL) {
        DEBUG(dbgSys, "\nNot enough memory in system");
        success = false;

    } else {
        DEBUG(dbgSys, "\nFile's name read successfully");
        if (!kernel->fileSystem->Create(fileName)) {
            DEBUG(dbgSys, "\nError creating file");
            success = false;
        } else {
            success = true;
        }
    }

    return success;
}

int SysOpen(char* fileName, int type) {
    if (type != 0 && type != 1) return -1;

    int id = kernel->fileSystem->Open(fileName, type);
    if (id == -1) return -1;
    DEBUG(dbgSys, "\nOpened file");
    return id;
}

int SysClose(int id) { return kernel->fileSystem->Close(id); }

int SysRead(char* buffer, int charCount, int fileId) {
    if (fileId == 0) {
        return kernel->synchConsoleIn->GetString(buffer, charCount);
    }
    return kernel->fileSystem->Read(buffer, charCount, fileId);
}

int SysWrite(char* buffer, int charCount, int fileId) {
    if (fileId == 1) {
        return kernel->synchConsoleOut->PutString(buffer, charCount);
    }
    return kernel->fileSystem->Write(buffer, charCount, fileId);
}

int SysSeek(int seekPos, int fileId) {
    if (fileId <= 1) {
        DEBUG(dbgSys, "\nCan't seek in console");
        return -1;
    }
    return kernel->fileSystem->Seek(seekPos, fileId);
}

int SysExec(char* name) {
    // cerr << "call: `" << name  << "`"<< endl;
    OpenFile* oFile = kernel->fileSystem->Open(name);
    if (oFile == NULL) {
        DEBUG(dbgSys, "\nExec:: Can't open this file.");
        return -1;
    }

    delete oFile;

    // Return child process id
    return kernel->pTab->ExecUpdate(name);
}

int SysJoin(int id) { return kernel->pTab->JoinUpdate(id); }

int SysExit(int id) { return kernel->pTab->ExitUpdate(id); }

int SysCreateSemaphore(char* name, int initialValue) {
    int res = kernel->semTab->Create(name, initialValue);

    if (res == -1) {
        DEBUG('a', "\nError creating semaphore");
        delete[] name;
        return -1;
    }

    return 0;
}

int SysWait(char* name) {
    int res = kernel->semTab->Wait(name);

    if (res == -1) {
        DEBUG('a', "\nSemaphore not found");
        delete[] name;
        return -1;
    }

    return res;
}

int SysSignal(char* name) {
    int res = kernel->semTab->Signal(name);

    if (res == -1) {
        DEBUG('a', "\nSemaphore not found");
        delete[] name;
        return -1;
    }

    return 0;
}

int SysGetPid() { return kernel->currentThread->processID; }

#endif /* ! __USERPROG_KSYSCALL_H__ */
