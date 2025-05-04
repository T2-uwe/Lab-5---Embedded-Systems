#include "mbed.h"
#include <cctype>
#include <chrono>

//=====[Defines]===============================================================
#define TIME_INCREMENT_MS        10
#define DEBOUNCE_KEY_TIME_MS     40
#define KEYPAD_ROWS              4
#define KEYPAD_COLS              4
#define MAX_ALARM_LOGS           5

//=====[Keypad Pin Configuration]===============================================
DigitalOut keypadRowPins[KEYPAD_ROWS] = {PB_3, PB_5, PC_7, PA_15};
DigitalIn keypadColPins[KEYPAD_COLS]  = {PB_12, PB_13, PB_15, PC_6};
//=====[Serial]=================================================================
UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

//=====[Keypad State Machine]==================================================
typedef enum {
    KEYPAD_SCANNING,
    KEYPAD_DEBOUNCE,
    KEYPAD_HOLD_PRESSED
} keypadState_t;

keypadState_t keypadState;

char keypadLastKeyPressed = '\0';
int debounceTimer = 0;
char inputBuffer[5];           // To store 4-digit input + null terminator
int inputIndex = 0;
const char correctPin[] = "1805";  // Your PIN


// Key mapping (row-major order)
char keyMap[] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D'
};

time_t alarmLog[MAX_ALARM_LOGS];
int alarmLogIndex = 0;
//=====[Functions]=============================================================

void keypadInit() {
    keypadState = KEYPAD_SCANNING;
    for (int i = 0; i < KEYPAD_COLS; i++) {
        keypadColPins[i].mode(PullUp);
    }
}

char keypadScan() {
    for (int r = 0; r < KEYPAD_ROWS; r++) {
        for (int i = 0; i < KEYPAD_ROWS; i++) {
            keypadRowPins[i] = 1;
        }

        keypadRowPins[r] = 0;

        for (int c = 0; c < KEYPAD_COLS; c++) {
            if (keypadColPins[c] == 0) {
                return keyMap[r * KEYPAD_ROWS + c];
            }
        }
    }
    return '\0';
}

char keypadUpdate() {
    char keyDetected = '\0';
    char keyReleased = '\0';

    switch (keypadState) {
        case KEYPAD_SCANNING:
            keyDetected = keypadScan();
            if (keyDetected != '\0') {
                keypadLastKeyPressed = keyDetected;
                debounceTimer = 0;
                keypadState = KEYPAD_DEBOUNCE;
            }
            break;

        case KEYPAD_DEBOUNCE:
            if (debounceTimer >= DEBOUNCE_KEY_TIME_MS) {
                keyDetected = keypadScan();
                if (keyDetected == keypadLastKeyPressed) {
                    keypadState = KEYPAD_HOLD_PRESSED;
                } else {
                    keypadState = KEYPAD_SCANNING;
                }
            }
            debounceTimer += TIME_INCREMENT_MS;
            break;

        case KEYPAD_HOLD_PRESSED:
            keyDetected = keypadScan();
            if (keyDetected != keypadLastKeyPressed) {
                if (keyDetected == '\0') {
                    keyReleased = keypadLastKeyPressed;
                }
                keypadState = KEYPAD_SCANNING;
            }
            break;

        default:
            keypadInit();
            break;
    }

    return keyReleased;
}

void serialWriteChar(char chr) {
    uartUsb.write(&chr, 1);
}
void logAlarmEvent() {
    alarmLog[alarmLogIndex] = time(NULL);
    alarmLogIndex = (alarmLogIndex + 1) % MAX_ALARM_LOGS;  // Circular buffer

    // Print immediately to serial
    char buffer[64];
    sprintf(buffer, "\r\nAlarm triggered at: %s", ctime(&alarmLog[(alarmLogIndex + MAX_ALARM_LOGS - 1) % MAX_ALARM_LOGS]));
    uartUsb.write(buffer, strlen(buffer));
}
void pcSerialComStringWrite(const char* str) {
    uartUsb.write(str, strlen(str));
}
DigitalIn d2(D2);
DigitalOut led1(LED1);
//=====[Main]===================================================================

int main() {
d2.mode(PullDown);
 led1 = 0;
 set_time(1744732221);
 pcSerialComStringWrite("\r\nEnter 4-digit PIN and press # To Deactivate Alarm: ");
  keypadInit();

int alarmState = 0;
int lastAlarmState = 0;


    while (true) {
        char key = keypadUpdate();

if (d2 == 1) {
    led1 = 1;
    alarmState = 1;
} else {
    led1 = 0;
    alarmState = 0;
}

// Log the alarm only once when first triggered
if (alarmState == 1 && lastAlarmState == 0) {
    logAlarmEvent();
}
lastAlarmState = alarmState;

if (key == '#') {
        if (inputIndex == 0) {
        // Show alarm log if no PIN was being typed
        pcSerialComStringWrite("\r\nRecent Alarm Events:\r\n");
        for (int i = 0; i < MAX_ALARM_LOGS; i++) {
            if (alarmLog[i] != 0) {
                char buffer[64];
                sprintf(buffer, "• %s", ctime(&alarmLog[i]));
                pcSerialComStringWrite(buffer);
            }
        }
    } else {

        // PIN entered – check it
        inputBuffer[inputIndex] = '\0';
        pcSerialComStringWrite("\r\nYou entered: ");
        pcSerialComStringWrite(inputBuffer);
        pcSerialComStringWrite("\r\n");

        if (strcmp(inputBuffer, correctPin) == 0) {
            pcSerialComStringWrite("Correct PIN! Alarm cleared.\r\n");
        } else {
            pcSerialComStringWrite("Incorrect PIN.\r\n");
        }

        inputIndex = 0; // Always reset input
        }
    }
}
}