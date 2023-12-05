#include <unistd.h>
#include "webserver.h"

int main() {
    /* 守护进程 后台运行 */
    //daemon(1, 0); 

    WebServer server(1316, 3, 60000, false, 2);            
    server.Start();
} 