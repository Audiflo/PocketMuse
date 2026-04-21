#include <globals.h>

#if !OTA_APP // PocketMage OS Only
#include <WiFi.h>
#include <vector>

extern "C" {
  #include "mesh_now.h"
}

static constexpr const char* TAG = "COMM";
static std::vector<String> chatMessages;

// Mesh callback execution occurs in the Wi-Fi task background context. 
// Do not draw to OLED/E-ink directly from here. Push to a buffer and set the redraw flag.
void onMeshMessageReceived(const mesh_message_t *message) {
  if (message->type == MSG_TYPE_CHAT) {
    String msg = String(message->message);
    chatMessages.push_back("> " + msg);
    
    // Keep buffer manageable
    if (chatMessages.size() > 12) {
      chatMessages.erase(chatMessages.begin());
    }
    
    if (CurrentAppState == COMM) {
      newState = true;
    }
  }
}


void MESSAGE_INIT() {

}

void processKB_MESSAGE() {

}

void einkHandler_MESSAGE() {

}

#endif