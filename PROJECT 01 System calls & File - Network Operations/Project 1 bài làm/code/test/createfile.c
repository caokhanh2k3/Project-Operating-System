#include"syscall.h"
#define maxlen 255


int main()
{

    char filename[256];
    int idfile; 

    PrintString("Nhap ten file ban muon tao: ");
    ReadString(filename, maxlen);
    
    idfile = Create(filename);
    Halt();
    return 0;
}
