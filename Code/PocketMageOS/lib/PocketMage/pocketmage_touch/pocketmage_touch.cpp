#include <pocketmage.h> 
#include <Adafruit_MPR121.h>

Adafruit_MPR121 cap =  Adafruit_MPR121(); // Touch slider

// Initialization of capacative touch class
static PocketmageTOUCH pm_touch(cap);

static constexpr const char* TAG = "TOUCH";

// Setup for Touch Class
void setupTouch(){
  // Production Device Setup
  #if POCKETMAGE_HW_VERSION == 2
    if (!cap.begin(MPR121_ADDR, &Wire1)) {
      ESP_LOGE(TAG, "TouchPad Failed");
      OLED().sysMessage("Touchpad Failed",1000);
    }
    cap.setAutoconfig(true);
  #else
    // MPR121 / SLIDER
    if (!cap.begin(MPR121_ADDR)) {
      ESP_LOGE(TAG, "TouchPad Failed");
      OLED().sysMessage("Touchpad Failed",1000);
    }
    cap.setAutoconfig(true);
  #endif
}

// Access for other apps
PocketmageTOUCH& TOUCH() { return pm_touch; }

void PocketmageTOUCH::updateScrollFromTouch() {
  uint16_t touched = cap_.touched();
  int newTouch = -1;

  // Prioritize lowest pad index to cleanly handle physical finger overlap
  for (int i = 0; i < 9; ++i) {
    if (touched & (1 << i)) { 
      newTouch = i; 
      break; 
    }
  }

  unsigned long now = millis();

  if (newTouch != -1) {
    // reset timeout
    CLOCK().setPrevTimeMillis(millis());

    if (lastTouch_ != -1) {
      int d = abs(newTouch - lastTouch_);
      if (d <= 2) {
        int maxScroll = max(0, (int)allLines.size() - EINK().maxLines());
        if (newTouch > lastTouch_) {
          dynamicScroll_ = min((long)(dynamicScroll_ + 1), (long)maxScroll);
        } else if (newTouch < lastTouch_) {
          dynamicScroll_ = max((long)(dynamicScroll_ - 1), 0L);
        }
      }
    }
    lastTouch_ = newTouch;
    lastTouchTime_ = now;
  } else if (lastTouch_ != -1 && (now - lastTouchTime_ > TOUCH_TIMEOUT_MS)) {
    lastTouch_ = -1;
    if (prev_dynamicScroll_ != dynamicScroll_)
      newLineAdded = true;
  }
}

int PocketmageTOUCH::readTouchPad() {
  uint16_t touched = cap_.touched();
  for (int i = 0; i < 9; i++) {
    if (touched & (1 << i)) return i;
  }
  return -1;
}

bool PocketmageTOUCH::updateScroll(int maxScroll, ulong& lineScroll, int stepSize) {

  bool updateScreen = false;
  unsigned long currentTime = millis();
  int touchPos = readTouchPad();
  ulong scrollCap = maxScroll > 0 ? (ulong)maxScroll : 0;

  if (touchPos != -1) {
    CLOCK().setPrevTimeMillis(millis());

    if (lastTouchPos_ != -1) {
      int touchDelta = abs(touchPos - lastTouchPos_);
      if (touchDelta <= 2) {
        if (touchPos < lastTouchPos_ && lineScroll < scrollCap) {
          prev_lineScroll_ = lineScroll;
          lineScroll = min(lineScroll + stepSize, scrollCap);
        } else if (touchPos > lastTouchPos_ && lineScroll > 0) {
          prev_lineScroll_ = lineScroll;
          lineScroll = (lineScroll >= (ulong)stepSize) ? lineScroll - stepSize : 0;
        }
      }
    }

    lastTouchPos_ = touchPos;
    lastTouch_ = touchPos;
    lastTouchPosTime_ = currentTime;
  } else if (lastTouchPos_ != -1 && (currentTime - lastTouchPosTime_ > TOUCH_TIMEOUT_MS)) {
    lastTouchPos_ = -1;
    lastTouch_ = -1;

    if (prev_lineScroll_ != lineScroll) {
      updateScreen = true;
    }

    prev_lineScroll_ = lineScroll;
  }
  return updateScreen;
}

int PocketmageTOUCH::getScrollVector() {
  int scrollVector = 0;
  unsigned long currentTime = millis();
  int touchPos = readTouchPad();

  if (touchPos != -1) {
    CLOCK().setPrevTimeMillis(currentTime);

    if (lastTouchPos_ != -1) {
      int touchDelta = abs(touchPos - lastTouchPos_);
      if (touchDelta > 0 && touchDelta <= 2) {
        scrollVector = lastTouchPos_ - touchPos;
      }
    }

    lastTouchPos_ = touchPos;
    lastTouch_ = touchPos;
    lastTouchPosTime_ = currentTime;
  } else if (lastTouchPos_ != -1 && (currentTime - lastTouchPosTime_ > TOUCH_TIMEOUT_MS)) {
    lastTouchPos_ = -1;
    lastTouch_ = -1;
  }

  return scrollVector;
}