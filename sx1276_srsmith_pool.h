#include "esphome.h"
#undef yield
#undef delay
#undef delayMicroseconds
#undef millis
#undef micros
#include "RadioLib.h"

#define RADIO_SCLK_PIN 5
#define RADIO_MISO_PIN 19
#define RADIO_MOSI_PIN 27
#define RADIO_CS_PIN 18
#define RADIO_DI0_PIN 26
#define RADIO_RST_PIN 23
#define RADIO_DIO1_PIN 33
#define RADIO_BUSY_PIN 32

// custom changes for srsmith radio
#define SYNC_WORD                                                              \
  { 0xd3, 0x91, 0xd3, 0x91 }
#define PACKET_FIRST_PART                                                      \
  { 0x01, 0xff, 0xff, 0xf5 }
#define BUTTON_ID_ONE 0x0d
#define BUTTON_ID_TWO 0x1f
#define BUTTON_ID_S 0x07
#define BUTTON_ID_M 0x0b

class SX1276PoolButtonRadio : public PoolButtonRadio {
public:
  SX1276 radio =
      new Module(RADIO_CS_PIN, RADIO_DI0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
  bool radio_initted = false;
  bool configure_success = true;
  void init_radio() {
    ESP_LOGI("custom", "[SX1276] Initializing ... ");

    int state = radio.beginFSK(915.0, // freq
                               10,    // bitrrate
                               175,   // freq dev
                               250,   // bandwidth
                               17,    // power
                               32);   // preamble

    if (state == RADIOLIB_ERR_NONE) {
      ESP_LOGI("PoolRadio", "success configuring, onto next settings");
    } else {
      ESP_LOGE("PoolRadio", "FAILURE: could not configure radio.");
      configure_success = false;
    }

    // if the above step worked, i'm going to assume these steps will.
    state = radio.variablePacketLengthMode(RADIOLIB_SX126X_MAX_PACKET_LENGTH);
    uint8_t sync_word[] = {0xd3, 0x91, 0xd3, 0x91};
    radio.setSyncWord(sync_word, 4);
    radio.setDataShaping(RADIOLIB_SHAPING_NONE);
    radio.setEncoding(RADIOLIB_ENCODING_NRZ);
    radio.setCRC(
        true, true); // this is important: this tells the SX to generate an
                     // "IBM" style CRC for the packet and attach it to the end.
  }

  int xmit_bytes(uint8_t *data_to_send, int len) {
    if (configure_success == false) {
      ESP_LOGE(
          "PoolRadio",
          "not bothering to try and xmit packets because radio failed config.");
      return RADIOLIB_ERR_UNKNOWN;
    }
    ESP_LOGI("PoolRadio", "transmitting data");
    int radio_state = radio.transmit(data_to_send, len);
    if (radio_state == RADIOLIB_ERR_NONE) {
      // the packet was successfully transmitted
      ESP_LOGI("PoolRadio", " transmit success!");
    } else if (radio_state == RADIOLIB_ERR_PACKET_TOO_LONG) {
      // the supplied packet was longer than 256 bytes
      ESP_LOGE("PoolRadio", "packet too long!");
    } else if (radio_state == RADIOLIB_ERR_TX_TIMEOUT) {
      // timeout occurred while transmitting packet
      ESP_LOGE("PoolRadio", "ransmit error timeout!");
    } else {
      // some other error occurred
      ESP_LOGE("PoolRadio", "failed, for some other reason.");
    }

    return radio_state;
  }
};
