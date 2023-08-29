#include"syscall.h"
#define maxlen 255


int main()
{
    char filename[256];
    int removefile; 

    PrintString("Nhap ten file ban muon xoa: ");
    ReadString(filename, maxlen);
    removefile = Remove(filename);
    if(removefile != -1)
    {
        PrintString("Xoa file thanh cong ");
    }
    Halt();
    return 0;
}
