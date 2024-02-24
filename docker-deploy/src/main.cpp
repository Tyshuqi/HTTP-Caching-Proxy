#include "proxy.h"

#include <stdio.h>
#include <stdlib.h>

int main()
{
    const char *port = "12345";
    Proxy *proxy = new Proxy();
    proxy->processRequest(port);
    return 1;
}