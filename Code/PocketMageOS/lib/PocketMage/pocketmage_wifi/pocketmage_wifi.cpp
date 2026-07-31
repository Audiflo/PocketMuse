#include "pocketmage_wifi.h"

#include <esp_log.h>
#include <esp_task_wdt.h>

#include <cstring>

static const char* TAG = "PocketMageWifi";

PocketMageWifi& PocketMageWifi::getInstance() {
  static PocketMageWifi instance;
  return instance;
}

PocketMageWifi& P_WIFI = PocketMageWifi::getInstance();

PocketMageWifi::PocketMageWifi()
    : _mutex(xSemaphoreCreateRecursiveMutex()),
      _state(WifiRadioState::Off),
      _scanResults(nullptr),
      _scanResultCount(0),
      _taskHandle(nullptr),
      _commandQueue(nullptr),
      _shutdownSem(xSemaphoreCreateBinary()),
      _staNetif(nullptr),
      _wifiEventHandler(nullptr),
      _ipEventHandler(nullptr),
      _initialized(false),
      _autoConnectEnabled(true),
      _retryCount(0),
      _retryAt(0),
      _eventCallback(nullptr),
      _eventPending(false) {
  _statusMessage[0] = 0;
  _connectedSSID[0] = 0;
  _ipAddress[0] = 0;
  _pendingSSID[0] = 0;
  _pendingPassword[0] = 0;
  _pendingSave = false;
  _retrySSID[0] = 0;
  _retryPassword[0] = 0;
  _connectError[0] = 0;
}

PocketMageWifi::~PocketMageWifi() {
  stop();
  if (_mutex)
    vSemaphoreDelete(_mutex);
  if (_shutdownSem)
    vSemaphoreDelete(_shutdownSem);
}

void PocketMageWifi::begin() {
  if (_initialized)
    return;
  _commandQueue = xQueueCreate(8, sizeof(Command));
  xTaskCreatePinnedToCore(wifiTaskFunc, "pmwifi", 4096, this, 2, &_taskHandle, 0);  // Pin to core 0
  _initialized = true;
}

void PocketMageWifi::stop() {
  if (_taskHandle) {
    Command cmd = Command::Shutdown;
    xQueueSend(_commandQueue, &cmd, portMAX_DELAY);
    xSemaphoreTake(_shutdownSem, pdMS_TO_TICKS(5000));
    _taskHandle = nullptr;
  }
  if (_commandQueue) {
    vQueueDelete(_commandQueue);
    _commandQueue = nullptr;
  }
  if (_scanResults) {
    free(_scanResults);
    _scanResults = nullptr;
  }
  _initialized = false;
}

void PocketMageWifi::enable() {
  Command cmd = Command::Enable;
  xQueueSend(_commandQueue, &cmd, 0);
}

void PocketMageWifi::disable() {
  Command cmd = Command::Disable;
  xQueueSend(_commandQueue, &cmd, 0);
}

void PocketMageWifi::scan() {
  Command cmd = Command::Scan;
  xQueueSend(_commandQueue, &cmd, 0);
}

void PocketMageWifi::connect(const char* ssid, const char* password, bool save) {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  strncpy(_pendingSSID, ssid, sizeof(_pendingSSID));
  _pendingSSID[sizeof(_pendingSSID) - 1] = 0;
  strncpy(_pendingPassword, password, sizeof(_pendingPassword));
  _pendingPassword[sizeof(_pendingPassword) - 1] = 0;
  _pendingSave = save;
  xSemaphoreGiveRecursive(_mutex);
  Command cmd = Command::Connect;
  xQueueSend(_commandQueue, &cmd, 0);
}

void PocketMageWifi::disconnect() {
  Command cmd = Command::Disconnect;
  xQueueSend(_commandQueue, &cmd, 0);
}

void PocketMageWifi::reconnect() {
  Command cmd = Command::Reconnect;
  xQueueSend(_commandQueue, &cmd, 0);
}

WifiRadioState PocketMageWifi::getState() const {
  return _state;
}

bool PocketMageWifi::isConnected() const {
  return _state == WifiRadioState::Connected;
}

bool PocketMageWifi::isScanning() const {
  return _state == WifiRadioState::Scanning;
}

String PocketMageWifi::getStatusMessage() const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  String msg = String(_statusMessage);
  xSemaphoreGiveRecursive(_mutex);
  return msg;
}

String PocketMageWifi::getConnectedSSID() const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  String ssid = String(_connectedSSID);
  xSemaphoreGiveRecursive(_mutex);
  return ssid;
}

String PocketMageWifi::getIpAddress() const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  String ip = String(_ipAddress);
  xSemaphoreGiveRecursive(_mutex);
  return ip;
}

int PocketMageWifi::getRssi() const {
  wifi_ap_record_t info;
  if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
    return info.rssi;
  }
  return 0;
}

String PocketMageWifi::getLastError() const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  String err = String(_connectError);
  xSemaphoreGiveRecursive(_mutex);
  return err;
}

uint16_t PocketMageWifi::getScanResultCount() const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  uint16_t count = _scanResultCount;
  xSemaphoreGiveRecursive(_mutex);
  return count;
}

bool PocketMageWifi::getScanResult(uint16_t index, WifiApInfo& out) const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  bool ok = false;
  if (index < _scanResultCount && _scanResults) {
    strncpy(out.ssid, (const char*)_scanResults[index].ssid, sizeof(out.ssid));
    out.ssid[sizeof(out.ssid) - 1] = 0;
    out.rssi = _scanResults[index].rssi;
    out.channel = _scanResults[index].primary;
    out.authmode = _scanResults[index].authmode;
    ok = true;
  }
  xSemaphoreGiveRecursive(_mutex);
  return ok;
}

bool PocketMageWifi::hasSavedCredentials(const char* ssid) const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  bool found = false;
  if (_prefs.begin(PREFS_NAMESPACE, true)) {
    found = _prefs.isKey(ssid);
    _prefs.end();
  }
  xSemaphoreGiveRecursive(_mutex);
  return found;
}

bool PocketMageWifi::loadSavedCredentials(const char* ssid, char* password, size_t maxLen) const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  bool ok = false;
  if (_prefs.begin(PREFS_NAMESPACE, true)) {
    String pass = _prefs.getString(ssid, "");
    _prefs.end();
    if (pass.length() > 0) {
      strncpy(password, pass.c_str(), maxLen);
      password[maxLen - 1] = 0;
      ok = true;
    }
  }
  xSemaphoreGiveRecursive(_mutex);
  return ok;
}

void PocketMageWifi::clearSavedCredentials(const char* ssid) {
  if (_prefs.begin(PREFS_NAMESPACE, false)) {
    _prefs.remove(ssid);
    _prefs.end();
  }
}

void PocketMageWifi::setEventCallback(WifiEventCallback cb) {
  _eventCallback = cb;
}

void PocketMageWifi::wifiTaskFunc(void* param) {
  static_cast<PocketMageWifi*>(param)->taskLoop();
}

void PocketMageWifi::taskLoop() {
  Command cmd = Command::None;
  unsigned long lastAutoScan = 0;
  setStatus("WiFi idle");
  while (true) {
    esp_task_wdt_reset();  // Reset watchdog
    // Wait for command or periodic auto-scan
    if (xQueueReceive(_commandQueue, &cmd, pdMS_TO_TICKS(200)) == pdTRUE) {
      switch (cmd) {
        case Command::Enable:
          doEnable();
          break;
        case Command::Disable:
          doDisable();
          break;
        case Command::Scan:
          doScan();
          break;
        case Command::Connect:
          doConnect();
          break;
        case Command::Disconnect:
          doDisconnect();
          break;
        case Command::Reconnect:
          doAutoConnect();
          break;
        case Command::CheckAutoConnect:
          doAutoConnect();
          break;
        case Command::Shutdown:
          if (_state != WifiRadioState::Off && _state != WifiRadioState::TurningOff) {
            esp_wifi_disconnect();
            esp_wifi_stop();
            esp_wifi_deinit();
            if (_staNetif) {
              esp_netif_destroy(_staNetif);
              _staNetif = nullptr;
            }
            if (_wifiEventHandler) {
              esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, _wifiEventHandler);
              _wifiEventHandler = nullptr;
            }
            if (_ipEventHandler) {
              esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, _ipEventHandler);
              _ipEventHandler = nullptr;
            }
            _state = WifiRadioState::Off;
          }
          xSemaphoreGive(_shutdownSem);
          vTaskDelete(NULL);
          return;
        default:
          break;
      }
    }
    // Auto-scan/auto-connect if enabled
    if (_autoConnectEnabled && _state == WifiRadioState::On) {
      unsigned long now = millis();
      if (now - lastAutoScan > AUTO_SCAN_INTERVAL) {
        lastAutoScan = now;
        doScan();
        doAutoConnect();
      }
    }
    // Check for pending retry
    if (_retryCount > 0 && _retryCount <= MAX_RETRIES && _retryAt > 0) {
      unsigned long now = millis();
      if (now >= _retryAt) {
        _retryAt = 0;
        xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
        strncpy(_pendingSSID, _retrySSID, sizeof(_pendingSSID));
        _pendingSSID[sizeof(_pendingSSID) - 1] = 0;
        strncpy(_pendingPassword, _retryPassword, sizeof(_pendingPassword));
        _pendingPassword[sizeof(_pendingPassword) - 1] = 0;
        _pendingSave = false;
        xSemaphoreGiveRecursive(_mutex);
        doConnect();
      }
    }
    vTaskDelay(10);
  }
}

void PocketMageWifi::espEventHandler(void* arg, esp_event_base_t base, int32_t id, void* data) {
  PocketMageWifi* self = static_cast<PocketMageWifi*>(arg);
  if (base == WIFI_EVENT)
    self->handleWifiEvent(id, data);
  else if (base == IP_EVENT)
    self->handleIpEvent(id, data);
}

void PocketMageWifi::handleWifiEvent(int32_t id, void* data) {
  switch (id) {
    case WIFI_EVENT_STA_START:
      setStatus("WiFi started");
      break;
    case WIFI_EVENT_STA_CONNECTED: {
      auto* connected = static_cast<wifi_event_sta_connected_t*>(data);
      setStatus("WiFi connected");
      _connectError[0] = 0;
      _retryCount = 0;
      size_t len = connected->ssid_len;
      if (len > sizeof(_connectedSSID) - 1)
        len = sizeof(_connectedSSID) - 1;
      memcpy(_connectedSSID, connected->ssid, len);
      _connectedSSID[len] = 0;
      _state = WifiRadioState::Connected;
      publishEvent();
      break;
    }
    case WIFI_EVENT_STA_DISCONNECTED: {
      auto* disconn = static_cast<wifi_event_sta_disconnected_t*>(data);
      _state = WifiRadioState::On;
      switch (disconn->reason) {
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
        case WIFI_REASON_AUTH_FAIL:
        case WIFI_REASON_ASSOC_FAIL:
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
          _retryCount = MAX_RETRIES;
          strncpy(_connectError, "Authentication failed", sizeof(_connectError) - 1);
          _connectError[sizeof(_connectError) - 1] = 0;
          setStatus("Authentication failed");
          break;
        case WIFI_REASON_NO_AP_FOUND:
          _retryCount = MAX_RETRIES;
          strncpy(_connectError, "Network not found", sizeof(_connectError) - 1);
          _connectError[sizeof(_connectError) - 1] = 0;
          setStatus("Network not found");
          break;
        default:
          if (_retryCount < MAX_RETRIES) {
            unsigned long delay = RETRY_BASE_DELAY_MS << _retryCount;
            if (delay > RETRY_MAX_DELAY_MS)
              delay = RETRY_MAX_DELAY_MS;
            _retryAt = millis() + delay;
            _retryCount++;
            char buf[48];
            snprintf(buf, sizeof(buf), "Reconnecting in %lums...", delay);
            setStatus(buf);
          } else {
            strncpy(_connectError, "Max retries reached", sizeof(_connectError) - 1);
            _connectError[sizeof(_connectError) - 1] = 0;
            setStatus("Connection failed");
          }
          break;
      }
      publishEvent();
      break;
    }
    case WIFI_EVENT_SCAN_DONE:
      setStatus("Scan done");
      {
        uint16_t num = 0;
        esp_wifi_scan_get_ap_num(&num);
        xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
        if (_scanResults) {
          free(_scanResults);
          _scanResults = nullptr;
        }
        if (num > MAX_SCAN_RESULTS)
          num = MAX_SCAN_RESULTS;
        _scanResults = (wifi_ap_record_t*)malloc(sizeof(wifi_ap_record_t) * MAX_SCAN_RESULTS);
        if (_scanResults) {
          esp_wifi_scan_get_ap_records(&num, _scanResults);
          _scanResultCount = num;
        } else {
          _scanResultCount = 0;
        }
        xSemaphoreGiveRecursive(_mutex);
      }
      _state = WifiRadioState::On;
      publishEvent();
      break;
    default:
      break;
  }
}

void PocketMageWifi::handleIpEvent(int32_t id, void* data) {
  if (id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*)data;
    snprintf(_ipAddress, sizeof(_ipAddress), "%d.%d.%d.%d", IP2STR(&event->ip_info.ip));
    setStatus("Got IP");
    _state = WifiRadioState::Connected;
    publishEvent();
  }
}

void PocketMageWifi::doEnable() {
  if (_state == WifiRadioState::Off || _state == WifiRadioState::TurningOff) {
    _state = WifiRadioState::TurningOn;
    setStatus("Enabling WiFi...");
    if (_staNetif)
      esp_netif_destroy(_staNetif);
    _staNetif = esp_netif_create_default_wifi_sta();
    if (!_staNetif) {
      setStatus("Failed to create netif");
      _state = WifiRadioState::Off;
      return;
    }
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK) {
      setStatus("Failed to init WiFi");
      _state = WifiRadioState::Off;
      return;
    }
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &PocketMageWifi::espEventHandler, this, &_wifiEventHandler);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        &PocketMageWifi::espEventHandler, this, &_ipEventHandler);
    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) {
      setStatus("Failed to set mode");
      _state = WifiRadioState::Off;
      return;
    }
    if (esp_wifi_start() != ESP_OK) {
      setStatus("Failed to start WiFi");
      _state = WifiRadioState::Off;
      return;
    }
    _state = WifiRadioState::On;
    setStatus("WiFi enabled");
    publishEvent();
  }
}

void PocketMageWifi::doDisable() {
  if (_state != WifiRadioState::Off && _state != WifiRadioState::TurningOff) {
    _state = WifiRadioState::TurningOff;
    setStatus("Disabling WiFi...");
    esp_wifi_stop();
    esp_wifi_deinit();
    if (_staNetif) {
      esp_netif_destroy(_staNetif);
      _staNetif = nullptr;
    }
    _state = WifiRadioState::Off;
    setStatus("WiFi disabled");
    publishEvent();
  }
}

void PocketMageWifi::doScan() {
  if (_state == WifiRadioState::On) {
    _state = WifiRadioState::Scanning;
    setStatus("Scanning...");
    wifi_scan_config_t scanConf = {};
    scanConf.ssid = nullptr;
    scanConf.bssid = nullptr;
    scanConf.channel = 0;
    scanConf.show_hidden = true;
    esp_wifi_scan_start(&scanConf, false);
    publishEvent();
  }
}

void PocketMageWifi::doConnect() {
  char ssid[33] = {0};
  char password[65] = {0};
  bool save = false;
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  if (_pendingSSID[0] == 0) {
    xSemaphoreGiveRecursive(_mutex);
    setStatus("No SSID");
    return;
  }
  strncpy(ssid, _pendingSSID, sizeof(ssid));
  ssid[sizeof(ssid) - 1] = 0;
  strncpy(password, _pendingPassword, sizeof(password));
  password[sizeof(password) - 1] = 0;
  save = _pendingSave;
  xSemaphoreGiveRecursive(_mutex);
  setStatus("Connecting...");
  wifi_config_t config = {};
  strncpy((char*)config.sta.ssid, ssid, sizeof(config.sta.ssid));
  config.sta.ssid[sizeof(config.sta.ssid) - 1] = 0;
  strncpy((char*)config.sta.password, password, sizeof(config.sta.password));
  config.sta.password[sizeof(config.sta.password) - 1] = 0;
  config.sta.threshold.authmode = (password[0] != 0) ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
  config.sta.pmf_cfg.capable = true;
  esp_wifi_set_config(WIFI_IF_STA, &config);
  esp_wifi_connect();
  if (save)
    saveCredentials(ssid, password);
  _state = WifiRadioState::Connecting;
  _retryCount = 0;
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  strncpy(_retrySSID, ssid, sizeof(_retrySSID));
  _retrySSID[sizeof(_retrySSID) - 1] = 0;
  strncpy(_retryPassword, password, sizeof(_retryPassword));
  _retryPassword[sizeof(_retryPassword) - 1] = 0;
  xSemaphoreGiveRecursive(_mutex);
  publishEvent();
}

void PocketMageWifi::doDisconnect() {
  esp_wifi_disconnect();
  setStatus("Disconnecting...");
  _state = WifiRadioState::On;
  publishEvent();
}

void PocketMageWifi::doAutoConnect() {
  // Try to find a saved network in scan results
  char ssid[33] = {0};
  char password[65] = {0};
  if (findSavedNetwork(ssid, password)) {
    xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
    strncpy(_pendingSSID, ssid, sizeof(_pendingSSID));
    _pendingSSID[sizeof(_pendingSSID) - 1] = 0;
    strncpy(_pendingPassword, password, sizeof(_pendingPassword));
    _pendingPassword[sizeof(_pendingPassword) - 1] = 0;
    _pendingSave = false;
    xSemaphoreGiveRecursive(_mutex);
    doConnect();
  }
}

void PocketMageWifi::setStatus(const char* msg) {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  strncpy(_statusMessage, msg, sizeof(_statusMessage));
  _statusMessage[sizeof(_statusMessage) - 1] = 0;
  xSemaphoreGiveRecursive(_mutex);
  publishEvent();
}

void PocketMageWifi::publishEvent() {
  _eventPending = true;
}

void PocketMageWifi::dispatchEvents() {
  if (_eventPending) {
    _eventPending = false;
    if (_eventCallback)
      _eventCallback();
  }
}

void PocketMageWifi::saveCredentials(const char* ssid, const char* password) {
  if (_prefs.begin(PREFS_NAMESPACE, false)) {
    _prefs.putString(ssid, password);
    _prefs.end();
  }
}

bool PocketMageWifi::findSavedNetwork(char* ssid, char* password) {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  bool found = false;
  if (_scanResults && _scanResultCount > 0) {
    for (uint16_t i = 0; i < _scanResultCount; ++i) {
      if (hasSavedCredentials((const char*)_scanResults[i].ssid)) {
        strncpy(ssid, (const char*)_scanResults[i].ssid, 33);
        ssid[32] = 0;
        loadSavedCredentials(ssid, password, 65);
        found = true;
        break;
      }
    }
  }
  xSemaphoreGiveRecursive(_mutex);
  return found;
}
