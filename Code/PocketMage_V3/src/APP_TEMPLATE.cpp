
#include <globals.h>
#if OTA_APP // OTA_APP build
// OTA_APP: entry point for OTA applications (OTA = Over The Air - 3rd party installed apps) 
// If building a 3rd party app, navigate to the platformio.ini file and set OTA_APP to true.

void APP_INIT() {
}

void processKB_APP() {
}
void einkHandler_APP() {
}
#endif
