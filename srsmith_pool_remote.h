#include "esphome.h"
#undef yield
#undef delay
#undef delayMicroseconds
#undef millis
#undef micros
#include "RadioLib.h"

// custom changes for srsmith radio
#define SYNC_WORD                                                              \
  { 0xd3, 0x91, 0xd3, 0x91 }
#define PACKET_FIRST_PART                                                      \
  { 0x01, 0xff, 0xff, 0xf5 }
#define BUTTON_ID_ONE 0x0d
#define BUTTON_ID_TWO 0x1f
#define BUTTON_ID_S 0x07
#define BUTTON_ID_M 0x0b

class PoolButtonRadio {
public:
  virtual void init_radio() = 0;
  virtual int xmit_bytes(uint8_t *data_to_send, int len) = 0;
};

class PoolButtonSender {
public:
  PoolButtonRadio *pbr;
  uint8_t pin;

  PoolButtonSender(uint8_t pin_parameter, PoolButtonRadio *pbr_parameter) {
    pin = pin_parameter;
    pbr = pbr_parameter;
    pbr->init_radio();
  }
  
  // useful methods
  //
  //
  // shamlessly stolen from RTL-433
  uint8_t crc8(uint8_t const message[], unsigned nBytes, uint8_t polynomial,
               uint8_t init) {
    uint8_t remainder = init;
    unsigned byte, bit;

    for (byte = 0; byte < nBytes; ++byte) {
      remainder ^= message[byte];
      for (bit = 0; bit < 8; ++bit) {
        if (remainder & 0x80) {
          remainder = (remainder << 1) ^ polynomial;
        } else {
          remainder = (remainder << 1);
        }
      }
    }
    return remainder;
  }

  // constructs command bytes packet (at least the part we're responsible for )
  void construct_srsmith_pool_command(uint8_t *command_bytes, uint8_t pin,
                                      uint8_t button_id) {
    int offset = 0;
    uint8_t const packet_first_part[] = PACKET_FIRST_PART;

    // should not be necessary since we're overwriting the whole thing anyways,
    // but why not.
    bzero(command_bytes, 7);
    memcpy(command_bytes, packet_first_part, sizeof(packet_first_part));
    offset += sizeof(packet_first_part);

    command_bytes[offset] = pin;
    offset++;

    command_bytes[offset] = button_id;
    offset++;

    uint8_t parity = crc8(command_bytes, 6, 1, 1);
    command_bytes[offset] = parity;
  }

  int send_command(uint8_t button_id) {
    uint8_t radio_command[7];
    construct_srsmith_pool_command(radio_command, pin, button_id);
    int radio_state = pbr->xmit_bytes(radio_command, sizeof(radio_command));
    return radio_state;
  }
};
