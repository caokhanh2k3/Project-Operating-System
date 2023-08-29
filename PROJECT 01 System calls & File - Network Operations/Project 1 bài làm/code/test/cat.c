#include "syscall.h"
#define maxlen 255

int main() 
{
    int id;
    int len;
    char filename[256];
    char s[256];

    PrintString("Nhap ten file ban muon doc: ");
    ReadString(filename, maxlen);
    
    id = Open(filename, 1);
    if (id != -1) {
        len = Seek(-1, id);
        Seek(0, id);
        Read(s, len, id);
        Close(id);
        PrintString("\nNoi dung file la : \n");
        PrintString(s);
    }
    else
        PrintString("< ERROR > Khong the mo file de doc !!");
    Halt();
   return  0;
}
