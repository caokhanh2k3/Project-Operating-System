#include "syscall.h"
#include "copyright.h"
#define maxlen 255


int main() {
    int idsocket;
    char buffer[512];
    int idfile;
    int len, pos, temp;
    char filename[256];
    char s[256];

    
    idsocket = SocketTCP();
        
    temp = Connect(idsocket, "127.0.0.1", 8080);
     
    PrintString("Nhap ten file ban muon doc: ");
    ReadString(filename, maxlen);
    
    idfile = Open(filename, 1);
    if (idfile != -1) {
        len = Seek(-1, idfile);
        Seek(0, idfile);
        Read(s, len, idfile);
        Close(idfile);
    }
    else
    {
        PrintString("< ERROR > Khong the mo file de doc !!"); 
    }
        
    temp = Send(idsocket, s, 1024);
    //PrintString("4");
    temp = Receive(idsocket, buffer, sizeof(buffer));
   
    temp = SocketClose(idsocket);
   

// Tạo file lưu kết quả
    idfile = Open("resultTran.txt", 0);
    if (idfile != -1)
    {
        Close(idfile);
        Remove("resultTran.txt");
    }
    
    idfile = Create("resultTran.txt");
    
    Close(idfile);
    if (idfile == -1) {
        PrintString("\n<ERROR> Tao file that bai !!\n");
        Halt();
        return 0;
    }
    
    idfile = Open("resultTran.txt", 0);
    // Xác định kích thước
    pos = 0;  
    while(buffer[pos] != '\0') {
        pos++;
    }
    Write(buffer, pos, idfile);  
    if (Close(idfile) == 0)
    {
        PrintString("\nGhi file thanh cong\n");
    }
    else
    {
        PrintString("\n<ERROR> Ghi file khong thanh cong !!");
    }




    Halt();
    return 0;
}