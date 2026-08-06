#define HW_VENDOR meshtastic_HardwareModel_PRIVATE_HW
#define USE_SX1262

// SPI bus
#define LORA_SCK   4
#define LORA_MISO  5
#define LORA_MOSI  6
#define LORA_CS    7

// SX1262 control
#define SX126X_CS     7
#define SX126X_RESET  3
#define SX126X_DIO1   10
#define SX126X_BUSY   0    // BODGE: was GPIO9. GPIO9 is a strapping pin, moved to GPIO0
#define SX126X_RXEN   21   // RF switch RX (external E22 PA/LNA)
#define SX126X_TXEN   20   // RF switch TX

// E22: TCXO on DIO3, required or radio init fails
#define SX126X_DIO3_TCXO_VOLTAGE 1.8
#define TCXO_OPTIONAL              // fall back to XTAL if TCXO init fails

// Super Mini builtin LED (active LOW)
#define LED_PIN 8

// Do NOT define SX126X_DIO2_AS_RF_SWITCH: TXEN/RXEN handle the switch here.
// If the LED is inverted after flashing, add: #define LED_STATE_ON 0
