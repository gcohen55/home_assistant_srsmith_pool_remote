# home_assistant_srsmith_pool_remote

## steps to use:
- implement a radio library, inherit from `PoolButtonRadio`, and implemenent two methods: `void init_radio()`, which inits the radio to send the packets out the "right" way (described below), and `int xmit_bytes(uint8_t *data_to_send, int len)`.

- in your yaml, you create the object, and then pass a pointer to the object to the pool button sender. The first argument is the encoded PIN for your home setup, and the second is the pointer to the radio object.
```C++
SX1276PoolButtonRadio *sx_pbr = new SX1276PoolButtonRadio();
PoolButtonSender *my_component = new PoolButtonSender(0xff, sx_pbr);
my_component->send_command(0x0d);
free(my_component);
free(sx_pbr);
```
### calculate your pin byte value
- unscrew battery compartment and remove the batteries.
- examine the dip switches, from left to right (where the top of the remote is left, the bottom is right). if the dip switch is down, write down 1, if it's up, write down 0. (**IMPORTANT: IT'S IN REVERSE**)
 - for example, if you have dip switches DOWN, DOWN, DOWN, UP, the values you write down are `1110`
- reverse it: `1110` becomes `0111`.
- prefix it with `0000` so the new byte becomes `0000 0111`
- convert `00000111` to hex: `0x07`
- `0x07` is your the encoded pin byte that you end to PoolButtonSender in the YAML file. In the above example, replace `0xff` with `0x07`

## how to send packets


### signal characteristics
- frequency `915MHz`
- modulation `FSK` or `GFSK`
- rate `10,000 bits per second`
- frequency deviation `175KHz`
- bandwidth `250KHz` bandwidth
- preamble: 32 bit

### packet characteristics
- use syncword `0xd391`, sent twice. how you define this is radio-specific, in some cases you just the two byte sync word and tell the radio to send it twice, with other radios you set the sync word to `0xd391d391`
- variable length packet
- tell the radio to use an IBM-style CRC-16 for the packet. hopefully this attaches the 16 bit CRC to the end of the radio packet. 
- if the radio doesn't support some of these packet characteristics, you can usually work around the limitations of the radio, but you'll need to be clever about it.
 - if the radio supports only 16 bit sync words, fixed length packets, and doesn't support the right CRC implementation, you can do the following:
	- begin your packet with D39107
	- perform the CRC-16 calculation (IBM style, to communicate with TI CCxxxx ICs) yourself starting from byte 07 all the way until the end of the packet that the PoolButtonSender hands to you.
 - if the radio supports 16 bit sync words, variable length packets, and has the right CRC implementation
 	- this is a trick question. you still need to configure the radio to send a fixed length packet because when you send the D391 sync word manually, the radio will prefix the packet with whatever the new size is, and the packet will actually begin with `D3 91 09 D3 91 01 FF FF F5`. you'll also need to calculate the CRC-16 manually because the radio would start the calculation from the the 09 which isn't right.
- packet looks something like:
```
Data Layout:
    PPPP WWWW S UUUU C B T PP
- P: 32 bit preamble (0xaaaaaaaa; 7 or 8 bits shifted left for analysis)
- W: 32 bit sync word (0xd391d391)
- S: 8 bit size (so far I've only seen 0x07)
- U: 32 bit unknown (I always see 0x01fffff5 here)
- C: 8 bit pin code is located in the bottom nibble of this byte, inverted and reversed.
- B: 8 bit contains the ID of the button that was pushed on the remote
- T: 8 bit CRC-8, poly 1, init 1 from the 16 bytes that we don't know (U) until the button that was pressed (B)
- P: 16 bit CRC-16, poly 0x8005, init 0xFFFF, of the packet from the size (S) until the CRC-8 (T)
Format String (in bitbench)
    PRE:32h SYNC: 32h SIZE: hh UNSURE:32h | UNSURE: 4b | PIN ~^4b |  BTN: hh | CRC-8: hh | CRC-16: hhhh
```

### other comments
I don't expect anyone else to ever use this.