#include "syscall.h"
#include "copyright.h"

int main() {
    int id;
    char buffer[1024];
    char *hi;

    //PrintString("1");
    id = SocketTCP();
    
    //hi = "Hello from the other sideeeeeee";
    //PrintString("2");
    PrintString("Nhap chuoi gui di: ");

    ReadString(hi, 255);
   
    Connect(id, "127.0.0.1", 8080);
    //PrintString("3");
    //PrintString(hi);
    Send(id, hi, 1024);
    //PrintString("4");
    Receive(id, buffer, sizeof(buffer));

    PrintString(buffer);
    SocketClose(id);
    Halt();
    return 0;
}