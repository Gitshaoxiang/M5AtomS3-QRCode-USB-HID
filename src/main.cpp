#include <M5AtomS3.h>

#include "M5UnitQRCode.h"
#include "manual_scan.h"
#include "auto_scan.h"

#include "USB.h"
#include "USBHIDKeyboard.h"
#include "Keyboard_def.h"

USBHIDKeyboard Keyboard;

M5Canvas canvas(&AtomS3.Display);
M5UnitQRCodeUART qrcode;
#define TRIG_OUTPUT_IO GPIO_NUM_7

qrcode_scan_mode_t scan_trig_mode = MANUAL_SCAN_MODE;

int needs_modifier(char ch) {
    // 根据ASCII码范围判断是否需要修饰符
    if (ch >= 'A' && ch <= 'Z') {
        return 1;  // 大写字母需要修饰符
    } else if (ch >= '!' && ch <= '/') {
        return 1;  // 一些特殊字符需要修饰符
    } else if (ch >= ':' && ch <= '@') {
        return 1;  // 更多特殊字符需要修饰符
    } else {
        return 0;  // 其他情况不需要修饰符
    }
}

void setup() {
    AtomS3.begin();

    canvas.setColorDepth(8);
    canvas.createSprite(116, 74);
    canvas.setTextScroll(true);
    canvas.pushSprite(6, 48);
    canvas.setTextFont(&fonts::FreeMonoBold9pt7b);

    gpio_pad_select_gpio(TRIG_OUTPUT_IO);
    gpio_set_direction(TRIG_OUTPUT_IO, GPIO_MODE_OUTPUT);
    gpio_set_level(TRIG_OUTPUT_IO, 1);

    while (!qrcode.begin(&Serial2, UNIT_QRCODE_UART_BAUD, 5, 6)) {
        delay(200);
        Serial.println("Unit QRCode UART Init Fail");
    }

    Serial.println("Unit QRCode UART Init Success");
    AtomS3.update();
    if (AtomS3.BtnA.isPressed()) {
        qrcode.setTriggerMode(AUTO_SCAN_MODE);
        Serial.println("Unit QRCode UART Auto Scan Mode");
        scan_trig_mode = AUTO_SCAN_MODE;
    } else {
        Serial.println("Unit QRCode UART Manual Scan Mode");
        qrcode.setTriggerMode(MANUAL_SCAN_MODE);
        scan_trig_mode = MANUAL_SCAN_MODE;
    }
    if (scan_trig_mode == MANUAL_SCAN_MODE) {
        AtomS3.Display.pushImage(0, 0, 128, 128, image_data_manual_scan);
    } else {
        AtomS3.Display.pushImage(0, 0, 128, 128, image_data_auto_scan);
    }
    Keyboard.begin();
    USB.begin();
}

void loop() {
    if (qrcode.available()) {
        String data     = qrcode.getDecodeData();
        uint16_t length = data.length();
        // Serial.printf("len:%d\r\n", length);
        Serial.printf("decode data:");
        Serial.println(data);
        Serial.println();
        canvas.println(data);
        canvas.pushSprite(6, 48);

        for (auto i : data) {
            KeyReport report  = {0};
            uint8_t modifiers = _kb_asciimap[i] & SHIFT ? 0x02 : 0;
            report.modifiers  = modifiers;
            report.keys[0]    = _kb_asciimap[i] & 0x7F;
            Keyboard.sendReport(&report);
            Keyboard.releaseAll();
        }
        KeyReport report = {0};
        report.keys[0]   = KEY_ENTER;
        Keyboard.sendReport(&report);
        Keyboard.releaseAll();
    }
    if (scan_trig_mode == MANUAL_SCAN_MODE) {
        AtomS3.update();
        if (AtomS3.BtnA.isPressed()) {
            // start scan
            gpio_set_level(TRIG_OUTPUT_IO, 0);
        } else {
            gpio_set_level(TRIG_OUTPUT_IO, 1);
        }
    }
}