#include "Adafruit_TinyUSB.h"
#if CFG_TUD_HID < 2
  #error "Requires two HID instances support. See https://github.com/adafruit/Adafruit_TinyUSB_Arduino/commit/b75604f794acdf88daad310dd75d3a0724129056"
#endif 

#define PIN_DATA0 0
#define PIN_LATCH 1
#define PIN_CLOCK 2
#define PIN_DATA1 3

#define TUD_HID_REPORT_DESC_SHORT_GAMEPAD(...) \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     )                 ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_GAMEPAD  )                 ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )                 ,\
    /* Report ID if any */\
    __VA_ARGS__ \
    /* 16 bit Button Map */ \
    HID_LOGICAL_MIN    ( 0                                      ) ,\
    HID_LOGICAL_MAX    ( 1                                      ) ,\
    HID_REPORT_SIZE    ( 1                                      ) ,\
    HID_REPORT_COUNT   ( 16                                     ) ,\
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_BUTTON                  ) ,\
    HID_USAGE_MIN      ( 1                                      ) ,\
    HID_USAGE_MAX      ( 16                                     ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 8 bit DPad/Hat */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_LOGICAL_MAX    ( 7                                      ) ,\
    HID_REPORT_SIZE    ( 4                                      ) ,\
    HID_REPORT_COUNT   ( 1                                      ) ,\
    HID_UNIT           ( 0x14                                   ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_HAT_SWITCH           ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE | HID_NULL_STATE ) ,\
    /* Padding */ \
    HID_UNIT           ( 0x00                                   ) ,\
    HID_REPORT_COUNT   ( 1                                      ) ,\
    HID_INPUT          ( HID_CONSTANT | HID_ARRAY | HID_ABSOLUTE ) ,\
    /* 8 bit analog stick axes, range 0-255 */ \
    HID_LOGICAL_MAX_N  ( 255, 2                                 ) ,\
    HID_PHYSICAL_MAX_N ( 255, 2                                 ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_X                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_Y                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_Z                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_RZ                   ) ,\
    HID_REPORT_SIZE    ( 8                                      ) ,\
    HID_REPORT_COUNT   ( 4                                      ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* wtf input */ \
    HID_USAGE_PAGE_N   ( HID_USAGE_PAGE_VENDOR, 2               ) ,\
    HID_USAGE          ( 0x20                                   ) ,\
    HID_REPORT_COUNT   ( 1                                      ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* wtf output */ \
    0x0A, 0x21, 0x26, \
    HID_REPORT_COUNT   ( 8                                      ) ,\
    HID_OUTPUT         ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
  HID_COLLECTION_END \

// NB NB!!! Select "Adafruit TinyUSB" for USB stack

// HID report descriptor using TinyUSB's template
uint8_t const desc_hid_report[] = {
  TUD_HID_REPORT_DESC_SHORT_GAMEPAD()
};

// USB HID object. For ESP32 these values cannot be changed after this declaration
// desc report, desc len, protocol, interval, use out endpoint
Adafruit_USBD_HID usb_hid[] {
  Adafruit_USBD_HID(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_NONE, 2, false), 
  Adafruit_USBD_HID(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_NONE, 2, false)
};

/// HID Gamepad Protocol Report.
typedef struct TU_ATTR_PACKED {
  uint16_t buttons;  ///< Buttons mask for currently pressed buttons
  uint8_t  hat;      ///< hat is required for the switch to recognize it. will always report "centered"
  uint8_t  lx;         ///< Delta x  movement of left analog-stick
  uint8_t  ly;         ///< Delta y  movement of left analog-stick
  uint8_t  rx;         ///< Delta x  movement of right analog-stick (always centered in our case)
  uint8_t  ry;         ///< Delta y  movement of right analog-stick (always centered in our case)
  uint8_t reserved;
} hid_short_gamepad_report_t;

hid_short_gamepad_report_t   gp[2]; // Two gamepad descriptors


inline int8_t axis_from_buttons(uint8_t lower, uint8_t upper) {
  return lower == LOW ? 0 : upper == LOW ? 255 : 127;
}

void setup() {
  
  TinyUSBDevice.setID(0x0f0d, 0x0092);
  
  // Manual begin() is required on core without built-in support e.g. mbed rp2040
  if (!TinyUSBDevice.isInitialized()) {
    TinyUSBDevice.begin(0);
  }

  Serial.begin(115200);
  usb_hid[0].begin();
  usb_hid[1].begin();

  // If already enumerated, additional class driverr begin() e.g msc, hid, midi won't take effect until re-enumeration
  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
  }
  
  Serial.println("PiPico NES joystick adapter");

  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_DATA0, INPUT);
  pinMode(PIN_DATA1, INPUT);

}

void read_pad(uint8_t pad, uint8_t pin_data) {
  uint8_t buttons[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

  digitalWrite(PIN_CLOCK, 1);
  digitalWrite(PIN_LATCH, 1);
  sleep_us(12);
  digitalWrite(PIN_LATCH, 0);

  sleep_us(6);

  buttons[2] = digitalRead(pin_data); // A

  digitalWrite(PIN_CLOCK, 0);
  sleep_us(6);
  digitalWrite(PIN_CLOCK, 1);
  sleep_us(6);

  buttons[1] = digitalRead(pin_data); // B

  digitalWrite(PIN_CLOCK, 0);
  sleep_us(6);
  digitalWrite(PIN_CLOCK, 1);
  sleep_us(6);

  buttons[8] = digitalRead(pin_data); // select

  digitalWrite(PIN_CLOCK, 0);
  sleep_us(6);
  digitalWrite(PIN_CLOCK, 1);
  sleep_us(6);

  buttons[9] = digitalRead(pin_data); // start

  digitalWrite(PIN_CLOCK, 0);
  sleep_us(6);
  digitalWrite(PIN_CLOCK, 1);
  sleep_us(6);

  uint8_t up = digitalRead(pin_data);

  digitalWrite(PIN_CLOCK, 0);
  sleep_us(6);
  digitalWrite(PIN_CLOCK, 1);
  sleep_us(6);

  uint8_t down = digitalRead(pin_data);

  digitalWrite(PIN_CLOCK, 0);
  sleep_us(6);
  digitalWrite(PIN_CLOCK, 1);
  sleep_us(6);

  uint8_t left = digitalRead(pin_data);

  digitalWrite(PIN_CLOCK, 0);
  sleep_us(6);
  digitalWrite(PIN_CLOCK, 1);
  sleep_us(6);

  uint8_t right = digitalRead(pin_data);

  digitalWrite(PIN_CLOCK, 0);

  gp[pad].lx = axis_from_buttons(left, right);
  gp[pad].ly = axis_from_buttons(up, down);


  gp[pad].buttons = 0;
  for (int i = 0; i<16; i++) {
    gp[pad].buttons |= (buttons[i] == LOW) << i;
  }

  gp[pad].rx = 127;
  gp[pad].ry = 127;
  gp[pad].hat = 8;
}

void loop() {
  #ifdef TINYUSB_NEED_POLLING_TASK
  // Manual call tud_task since it isn't called by Core's background
  TinyUSBDevice.task();
  #endif

  // not enumerated()/mounted() yet: nothing to do
  if (!TinyUSBDevice.mounted()) {
    return;
  }

// poll gpio once each 10 ms
  static uint32_t ms = 0;
  if (millis() - ms < 2) {
    return;
  }

  ms = millis();

  read_pad(0, PIN_DATA0);
  read_pad(1, PIN_DATA1);

  Serial.print("left x: ");
  Serial.print(gp[0].lx);
  Serial.print(" y: ");
  Serial.print(gp[0].ly);
  Serial.print(" buttons: 0x");
  Serial.print(gp[0].buttons, HEX);

  Serial.print(" - right x: ");
  Serial.print(gp[1].lx);
  Serial.print(" y: ");
  Serial.print(gp[1].ly);
  Serial.print(" buttons: 0x");
  Serial.print(gp[1].buttons, HEX);

  Serial.println("");

  if (usb_hid[0].ready()) {
    usb_hid[0].sendReport(0, &gp[0], sizeof(gp[0]));
  }
  
  if (usb_hid[1].ready()) {
    usb_hid[1].sendReport(0, &gp[1], sizeof(gp[1]));
  }

}