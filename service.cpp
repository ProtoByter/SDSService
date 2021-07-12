#include "ServiceMain.h"

#include "SDSService.h"

int __cdecl _tmain(int argc, TCHAR *argv[])
{
    char* service_name = (char*)"SDSService";
    const char* display_name = "SchoolDigitalSignage Server";
    char* description = (char*)"Acts as the server for SchoolDigitalSignage Clients";

    ServiceMain<SDSService>(service_name, display_name, description, argv);
    return 0;
}
