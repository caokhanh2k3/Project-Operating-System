#include "syscall.h"

int main() {
    SpaceId newProc1;
    SpaceId newProc2;

    newProc1 = Exec("ping");
    newProc2 = Exec("pong");

    Join(newProc1);
    Join(newProc2);
}
