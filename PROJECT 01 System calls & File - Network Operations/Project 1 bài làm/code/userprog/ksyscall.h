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
#include "syscall.h"
#include "socketSys.h"

Socket socketSystem;

#define MAXLENGTH 256
#define INT_MIN (-2147483647 - 1)
#define FILENAME_MAXLEN 32
#define MaxBufferLength 32


// Input: - User space address (int)
// - Limit of buffer (int)
// Output:- Buffer (char*)
// Purpose: Copy buffer from User memory space to System memory space
char *User2System(int virtAddr, int limit) {
    int i; // index
    int oneChar;
    char *kernelBuf = NULL;
    kernelBuf = new char[limit + 1];

    if (kernelBuf == NULL)
        return kernelBuf;
    memset(kernelBuf, 0, limit + 1);

    for (i = 0; i < limit; i++) {
        kernel->machine->ReadMem(virtAddr + i, 1, &oneChar);
        kernelBuf[i] = (char)oneChar;
        if (oneChar == 0)
            break;
    }
    return kernelBuf;
}

// Input: - User space address (int)
// - Limit of buffer (int)
// - Buffer (char[])
// Output:- Number of bytes copied (int)
// Purpose: Copy buffer from System memory space to User memory space
int System2User(int virtAddr, char *buffer, int len) {
    if (len <= 0)
        return len;
    int i = 0;
    int oneChar = 0;
    do {
        oneChar = (int)buffer[i];
        kernel->machine->WriteMem(virtAddr + i, 1, oneChar);
        i++;
    } while (i < len && oneChar != 0);
    return i;
}

void IncreasePC()
/* Modify return point */
{
    /* set previous programm counter (debugging only)*/
    kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

    /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
    kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

    /* set next programm counter for brach execution */
    kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
}
// -------------------------------------------------------------------

void SysHalt() {
    kernel->interrupt->Halt();
}

int SysAdd(int op1, int op2) {
    return op1 + op2;
}



// Các hàm xử lý trong Project ------------------------------------------------

void SysReadChar() {
    // Lấy kí tự từ người dùng nhập vào
    char ch = kernel->synchConsoleIn->GetChar();
    // Nhận lệnh xuống dòng (enter)
    while (ch != '\n' && kernel->synchConsoleIn->GetChar() != '\n') {
        // Do nothing !!
    }
    // Ghi dữ liệu ký tự vào reg2
    kernel->machine->WriteRegister(2, ch);
}

void SysPrintChar() {
    // Lấy dữ liệu từ reg4
    char ch = (char)kernel->machine->ReadRegister(4);
    // Đưa lên system
    kernel->synchConsoleOut->PutChar(ch);
}

void SysReadString(int length) {
    // Tạo chuỗi ký tự buffer
    char *buffer = new char[length + 1];
    // Nếu buffer == NULL thì kết thúc (Do không đủ vùng nhớ để tạo)
    if (!buffer) {
        printf("\n NULL pointer");
        return;
    }

    // Vị trí i (index) của từng ký tự
    int i = 0;
    // Biến ch có kiểu char
    char ch;

    // Dùng vòng lặp để đọc chuỗi ký tự nhập vào
    while (i < length) {
        ch = kernel->synchConsoleIn->GetChar();
        // Khi nhập: Enter -> ket thuc nhap
        if (ch == '\0' || ch == '\n') {
            buffer[i] = '\0';
            break;
        }

        // Lưu ký tự vào chuỗi ký tự buffer
        buffer[i++] = ch;
    }

    // kiểm soát trường hợp tràn buffer;
    if (ch != '\0' && ch != '\n') {
        while (true) {
            ch = kernel->synchConsoleIn->GetChar();
            if (ch == '\0' || ch == '\n')
                break;
        }
    }

    // Gọi ptr là value của reg4
    int ptr = kernel->machine->ReadRegister(4);
    // Chuyển dữ liệu vùng nhớ buffer từ hệ thống (system) sang giao diện người dùng (user)
    // Với virAdd = ptr (giá trị reg4)
    //     buffer = buffer (dữ liệu của buffer từ system sẽ chuyển thành buffer của user)
    //     len = MAXLENGTH (Độ dài của buffer là MAXLENGTH)
    System2User(ptr, buffer, length);
    delete[] buffer;
}

void SysPrintString() {
    // Vị trí (index) của ký tự
    int i = 0;
    // Lấy giá trị (value) từ reg4
    int ptr = kernel->machine->ReadRegister(4);

    // Đọc dữ liệu vùng nhớ từ giao diện người dùng (user)
    // Với virAdd = ptr (giá trị reg4)
    //     len = MAXLENGTH (độ dài của buffer người dùng)
    // Trả về giá trị char* (chuỗi dữ liệu) từ người dùng (user) gán vào char* buffer của hệ thống (system)
    char *buffer = User2System(ptr, MAXLENGTH);

    // Dùng vòng lặp để đưa lên hệ thống (system)
    while (buffer[i] != '\0') {
        kernel->synchConsoleOut->PutChar(buffer[i++]);
    }
    delete[] buffer;
}



void SysOpen() {
    int filenameAddr = kernel->machine->ReadRegister(4);
    int fileType = kernel->machine->ReadRegister(5);

    char *filename;
    // Copy file name vào system space
    // Độ dài tối đa của filename: 32 bytes
    filename = User2System(filenameAddr, MAXLENGTH + 1);
    if (filename == NULL || strlen(filename) < 1) {
        kernel->machine->WriteRegister(2, -1);
        if (filename)
            delete[] filename;
        return;
    }

    int openfile = kernel->fileSystem->GetFileDescriptorID(filename);
    if (openfile != -1) {
        kernel->machine->WriteRegister(2, openfile);
        delete[] filename;
        return;
    }

    int fileIDfree = kernel->fileSystem->FileDescriptorFree();
    if (fileIDfree == -1) {
        kernel->machine->WriteRegister(2, -1);
        delete[] filename;
        return;
    }

    switch (fileType) {
    // 0: read-write, 1: read-only
    case 0: {
        // Mở file thành công
        int flagsocket = -1;

        if (kernel->fileSystem->AssignFileDescriptor(fileIDfree, filename, fileType, flagsocket) != NULL) {
            kernel->machine->WriteRegister(2, fileIDfree);
        }
        // Mở file không thành công
        else {
            kernel->machine->WriteRegister(2, -1);
        }
        break;
    }
    case 1: {
        // Mở file thành công
        int flagsocket = -1;
        if (kernel->fileSystem->AssignFileDescriptor(fileIDfree, filename, fileType, flagsocket) != NULL) {
            kernel->machine->WriteRegister(2, fileIDfree);
        }
        // Mở file không thành công
        else {
            kernel->machine->WriteRegister(2, -1);
        }
        break;
    }
    default: {
        // Filetype không hợp lệ
        kernel->machine->WriteRegister(2, -1);
        break;
    }
    }
    delete[] filename;
}

void SysClose() {
    OpenFileId fileId = kernel->machine->ReadRegister(4);
    if (fileId >= 0 && fileId < MAX_FILE_NUM) {
        if (kernel->fileSystem->RemoveFileDescriptor(fileId)) {
            kernel->machine->WriteRegister(2, 0);
        }
        else {
            kernel->machine->WriteRegister(2, -1);
        }
    }
    else {
        kernel->machine->WriteRegister(2, -1);
    }
}

int SysRead(int bufferAddress, int lenBuffer, OpenFileId fileId) {
    // Lưu kết quả trả về 
    int cntChar = -1;
    // Xét những trường hợp fileID đặc biệt Console IO
    switch (fileId) {
    // consoleIN
    case 0: {
        // Làm tương tự như SC_ReadString: Đọc từng kí tự từ console cho đến khi gặp EOF
        char *buffer = new char[lenBuffer + 1];
        char ch;
        int i;
        if (!buffer)
            return -1;
        for (i = 0; i < lenBuffer; ++i) {
            ch = kernel->synchConsoleIn->GetChar();
            if (ch == '\0' || ch == '\n') {
                buffer[i] = '\0';
                break;
            }
            buffer[i] = ch;
        }
        System2User(bufferAddress, buffer, i);
        // Kiểm soát trường hợp tràn buffer;
        if (ch != '\0' && ch != '\n') {
            while (true) {
                ch = kernel->synchConsoleIn->GetChar();
                if (ch == '\0' || ch == '\n')
                    break;
            }
        }
        // Trả về số byte (kí tự thực sự đọc được)
        cntChar = i;
        delete[] buffer;
        break;
    }
    // consoleOUT
    case 1: {
        DEBUG(dbgFile, "stdout can't be read!\n");
        break;
    }
    default: {
        char *buffer = new char[lenBuffer + 1];
        // Lấy con trỏ vùng nhớ của file cần đọc
        // Đọc nội dung file vào buffer và lưu số byte đọc được vào cntChar
        cntChar = kernel->fileSystem->GetFileDescriptor(fileId)->Read(buffer, lenBuffer);
        if (cntChar > 0) {
            buffer[cntChar] = 0;
            System2User(bufferAddress, buffer, cntChar);
        }
        else {
            DEBUG(dbgFile, "File empty\n");
            printf("\nFile empty");
        }
        delete[] buffer;
        break;
    }
    }
    return cntChar;
}

int SysWrite(int bufferAddress, int lenBuffer, OpenFileId fileId) {
    // lưu kết quả trả về
    int cntChar = -1;
    // xét những trường hợp fileID đặc biệt Console IO
    switch (fileId) {
    // consoleIN
    case 0: {
        DEBUG(dbgFile, "<ERROR> stdout can't be written !!\n");
        break;
    }
    // consoleOUT
    case 1: {
        // tương tự như print string
        char *buffer = User2System(bufferAddress, lenBuffer);
        int i = 0;

        if (!buffer) {
            DEBUG(dbgAddr, "<ERROR> Allocate failed !!\n");
            SysHalt();
            return -1;
        }

        // Dùng vòng lặp để đưa lên console
        while (buffer[i] != '\0') {
            kernel->synchConsoleOut->PutChar(buffer[i++]);
        }
        // return the actual size
        cntChar = i;
        delete[] buffer;
        break;
    }
    default: {
        char *buffer = User2System(bufferAddress, lenBuffer);
        OpenFile *file2write = kernel->fileSystem->GetFileDescriptor(fileId);
        if (!buffer) {
            DEBUG(dbgAddr, "<ERROR> Cap phat khong thanh cong !!\n");
            SysHalt();
            return -1;
        }
        if (file2write->GetFileType() != 0) {
            DEBUG(dbgFile, "<ERROR> Read-only files can't be written !!\n");
        }
        else {
        }
        cntChar = file2write->Write(buffer, lenBuffer);
        if (cntChar > 0)
            buffer[cntChar] = 0;
        delete[] buffer;
    }
    }
    return cntChar;
}

int SysSeek(int id, int pos) {
    OpenFile *fileSpace = kernel->fileSystem->GetFileDescriptor(id);
    if (fileSpace == NULL) {
        printf("\n<ERROR> This file is not exist !!");
        kernel->machine->WriteRegister(2, -1);
        return -1;
    }
    if (pos == -1)
        pos = fileSpace->Length();
    if (pos > fileSpace->Length() || pos < 0) {
        printf("\n<ERROR> Can not seek !!");
        pos = -1;
    }
    else {
        fileSpace->Seek(pos);
    }
    return pos;
}

//========================================================================
int SysSocketTCP()
{
  DEBUG(dbgAll, "Goes to SocketTCP");

  int socketID = socketSystem.SocketTCP();
  if (socketID == -1)
  {
    DEBUG(dbgSys, "\nFailed to open socket!");
    return -1;
  }

  DEBUG(dbgSys, "\nOpen socket successfully!");
  return socketID;
}

int SysConnect(int socketid, char *ip, int port)
{
  int connected = socketSystem.Connect(socketid, ip, port);

  if (connected == -1)
  {
    DEBUG(dbgSys, "\nFailed to connect socket!");
    return -1;
  }

  DEBUG(dbgSys, "\nConnect to socket successfully!");
  return 0;
}

int SysSend(int socketid, char *buffer, int len, int connection)
{
  if (connection == -1)
  {
    DEBUG(dbgSys, "\nConnection is closed!");
    return 0;
  }

  int sent = socketSystem.Send(socketid, buffer, len);
  if (sent == -1)
  {
    DEBUG(dbgSys, "\nFailed to send data to socket!");
    return -1;
  }

  DEBUG(dbgSys, "\nSend data to socket successfully!");
  return sent;
}

int SysReceive(int socketid, char *buffer, int len, int connection)
{
  if (connection == -1)
  {
    DEBUG(dbgSys, "\nConnection is closed!");
    return 0;
  }

  int received = socketSystem.Receive(socketid, buffer, len);
  if (received == -1)
  {
    DEBUG(dbgSys, "\nFailed to receive data from socket!");
    return -1;
  }

  DEBUG(dbgSys, "\nReceive data from socket successfully!");
  return received;
}

int SysCloseSocket(int socketid)
{
  socketid = socketSystem.CloseSocket(socketid);  
  if (socketid == -1)
  {
    DEBUG(dbgSys, "\nFailed to close socket!");
    return -1;
  }
  DEBUG(dbgSys, "\nClose socket successfully!");
  return 0;
}

///======================================================================
void handle_SC_SocketTCP()
{
	kernel->machine->WriteRegister(2, SysSocketTCP());
}

void handle_SC_Connect()
{
	int indexsocketID, virtAdr, port;
	char *ip;
	indexsocketID = kernel->machine->ReadRegister(4);
	virtAdr = kernel->machine->ReadRegister(5);
	port = kernel->machine->ReadRegister(6);
	ip = User2System(virtAdr, MaxBufferLength + 1);
	kernel->machine->WriteRegister(2, SysConnect(indexsocketID, ip, port));
}

void handle_SC_Send()
{
	int connection, indexsocketID, virtAdr, len;
	
	connection = kernel->machine->ReadRegister(2);
	indexsocketID = kernel->machine->ReadRegister(4);
	virtAdr = kernel->machine->ReadRegister(5);
	len = kernel->machine->ReadRegister(6);
	char *buffer = User2System(virtAdr, MAXLENGTH);
    //printf(buffer,"\n");
	kernel->machine->WriteRegister(2, SysSend(indexsocketID, buffer, len, connection));
	delete[] buffer;
}

void handle_SC_Receive()
{
	int connection, indexsocketID, virtAdr, len;
	char *buffer;
	connection = kernel->machine->ReadRegister(2);
	indexsocketID = kernel->machine->ReadRegister(4);
	virtAdr = kernel->machine->ReadRegister(5);
	len = kernel->machine->ReadRegister(6);
	buffer = User2System(virtAdr, MAXLENGTH);
	kernel->machine->WriteRegister(2, SysReceive(indexsocketID, buffer, len, connection));
    System2User((int)virtAdr, buffer, len);
	delete[] buffer;
}

void handle_SC_CloseSocket()
{
	int indexsocketID;

	indexsocketID = kernel->machine->ReadRegister(4);
	kernel->machine->WriteRegister(2, SysCloseSocket(indexsocketID));
}
#endif /* ! __USERPROG_KSYSCALL_H__ */