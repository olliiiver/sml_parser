[![PlatformIO Registry](https://badges.registry.platformio.org/packages/olliiiver/library/SML%20Parser.svg)](https://registry.platformio.org/libraries/olliiiver/SML%20Parser) [![Test](https://github.com/olliiiver/sml_parser/workflows/Test/badge.svg)](https://github.com/olliiiver/sml_parser/actions)

# Smart Message Language (SML) parser

Easy to use C library with a low memory footprint to parse SML messages (based on BSI TR-03109-1) byte by byte from smart meters. It's designed to be lightweight and efficient and has a small memory footprint, making it suitable for use on embedded systems or other devices with limited memory resources.

The library will control the last CRC value to check if the received data is correct. On any error, the parser will reset and wait for valid data. This allows to start parsing at any time. For example, a half received message is discarded. The library allows you to register handlers to process the received information. This feature allows you to easily process the data in your application without having to handle the low-level details of parsing the SML messages.

It's actively maintained and has already been used in various projects with meters from EMH, EFR, EasyMeter, etc.

## Examples

| Directory                                              | Description                                                    |
| ------------------------------------------------------ | -------------------------------------------------------------- |
| [arduino](examples/arduino/)                           | Loops through static data and outputs debug messages to serial |
| [arduino_serial](examples/arduino_serial/)             | Reads data from Arduino Pin 8 and outputs debug to serial      |
| [esp32_lora](examples/esp32_lora/)                     | Forward energy usage to LoraWAN (The Things Network)           |
| [esp32_m5stack_sender](examples/esp32_m5stack_sender/) | Use m5stack to produce a message for testing                   |
| [esp32_receiver](examples/esp32_receiver/)             | Receive messages and show infos on a display                   |
| [native](examples/native/)                             | Test library locally                                           |

The easiest way to test the library would be over the [native](examples/native/) example.

## Example usage

```cpp
void Manufacturer() {
  smlOBISManufacturer(manuf, MAX_STR_MANUF);
}

void PowerT1() {
  smlOBISWh(T1Wh);
}

void PowerSum() {
  smlOBISWh(SumWh);
}

SMLHandler SMLHandlers[] = {
  {{ 0x81, 0x81, 0xc7, 0x82, 0x03, 0xff }, &Manufacturer}, /* 129-129:199.130.3*255 */
  {{ 0x01, 0x00, 0x01, 0x08, 0x01, 0xff }, &PowerT1},      /*   1-  0:  1.  8.1*255 (T1) */
  {{ 0x01, 0x00, 0x01, 0x08, 0x00, 0xff }, &PowerSum},     /*   1-  0:  1.  8.0*255 (T1 + T2) */
  {{ 0, 0 }}
};

int main () {
  int i = 0, iHandler = 0;
  unsigned char c;
  sml_states_t s;
  for (i = 0; i < ehz_bin_len; ++i) {
    c = ehz_bin[i];
    s = smlState(c);
    if (s == SML_START) {
      /* reset local vars */
      manuf[0] = 0; T1Wh = -3; SumWh = -3;
    }
    if (s == SML_LISTEND) {
      /* check handlers on last received list */
      for (iHandler=0; SMLHandlers[iHandler].Handler != 0 &&
            !(smlOBISCheck(SMLHandlers[iHandler].OBIS)); iHandler++);
      if (SMLHandlers[iHandler].Handler != 0) {
        SMLHandlers[iHandler].Handler();
      }
    }
    if (s == SML_UNEXPECTED) {
      printf(">>> Unexpected byte! <<<\n");
    }
    if (s == SML_FINAL) {
      printf(">>> FINAL! Checksum OK\n");
      printf(">>> Manufacturer.............: %s\n", manuf);
      printf(">>> Power T1    (1-0:1.8.1)..: %.3f kWh\n", T1Wh);
      printf(">>> Power T1+T2 (1-0:1.8.0)..: %.3f kWh\n\n", SumWh);
    }
  }
}
```

## Debug mode

If debug mode via `SML_DEBUG` (see examples/native/platformio.ini) is enabled, the SML data is displayed in a tree like structure.

```
START
 LISTSTART on level 1 with 6 nodes
  Data 6 (length = 6, octet string): 00 0C 04 08 87 2D
  Data 5 (length = 1, unsigned int): 00
  Data 4 (length = 1, unsigned int): 00
  LISTSTART on level 2 with 2 nodes
   Data 2 (length = 2, unsigned int): 01 01
   LISTSTART on level 3 with 6 nodes
    Data 6 (empty)
    Data 5 (empty)
    Data 4 (length = 6, octet string): 00 0C 06 9E 2D 0F
    Data 3 (length = 10, octet string): 06 45 4D 48 01 00 1D 46 15 CA
    Data 2 (empty)
    Data 1 (empty)
   LISTEND
   back to previous list
  back to previous list
  Data 2 (length = 2, unsigned int): 2B 8E
 End of block at level 1
 back to previous list
 LISTSTART on level 1 with 6 nodes
  Data 6 (length = 6, octet string): 00 0C 04 08 87 2E
  Data 5 (length = 1, unsigned int): 00
  Data 4 (length = 1, unsigned int): 00
  LISTSTART on level 2 with 2 nodes
   Data 2 (length = 2, unsigned int): 07 01
   LISTSTART on level 3 with 7 nodes
    Data 7 (empty)
    Data 6 (length = 10, octet string): 06 45 4D 48 01 00 1D 46 15 CA
    Data 5 (empty)
    LISTSTART on level 4 with 2 nodes
     Data 2 (length = 1, unsigned int): 01
     Data 1 (length = 4, unsigned int): 06 9E FA 83
    LISTEND on level 4
    back to previous list
    LISTSTART on level 4 with 7 nodes
     LISTSTART on level 5 with 7 nodes
      Data 7 (length = 6, octet string): 81 81 C7 82 03 FF
      Data 6 (empty)
      Data 5 (empty)
      Data 4 (empty)
      Data 3 (empty)
      Data 2 (length = 3, octet string): 45 4D 48
      Data 1 (empty)
     LISTEND
     back to previous list
     LISTSTART on level 5 with 7 nodes
      Data 7 (length = 6, octet string): 01 00 00 00 09 FF
      Data 6 (empty)
      Data 5 (empty)
      Data 4 (empty)
      Data 3 (empty)
      Data 2 (length = 10, octet string): 06 45 4D 48 01 00 1D 46 15 CA
      Data 1 (empty)
     LISTEND
     back to previous list
     LISTSTART on level 5 with 7 nodes
      Data 7 (length = 6, octet string): 01 00 01 08 00 FF
      Data 6 (length = 2, unsigned int): 01 82
      Data 5 (empty)
      Data 4 (length = 1, unsigned int): 1E
      Data 3 (length = 1, signed int): 03
      Data 2 (length = 5, signed int): 00 00 00 1C 46
      Data 1 (empty)
     LISTEND
     back to previous list
     LISTSTART on level 5 with 7 nodes
      Data 7 (length = 6, octet string): 01 00 01 08 01 FF
      Data 6 (empty)
      Data 5 (empty)
      Data 4 (length = 1, unsigned int): 1E
      Data 3 (length = 1, signed int): FF
      Data 2 (length = 8, signed int): 00 00 00 00 07 5B CD 15
      Data 1 (empty)
     LISTEND
     back to previous list
     LISTSTART on level 5 with 7 nodes
      Data 7 (length = 6, octet string): 01 00 01 08 02 FF
      Data 6 (empty)
      Data 5 (empty)
      Data 4 (length = 1, unsigned int): 1E
      Data 3 (length = 1, signed int): 03
      Data 2 (length = 5, signed int): 00 00 00 1C 46
      Data 1 (empty)
     LISTEND
     back to previous list
     LISTSTART on level 5 with 7 nodes
      Data 7 (length = 6, octet string): 01 00 0F 07 00 FF
      Data 6 (empty)
      Data 5 (empty)
      Data 4 (length = 1, unsigned int): 1B
      Data 3 (length = 1, signed int): FF
      Data 2 (length = 4, signed int): 00 00 2F 65
      Data 1 (empty)
     LISTEND
     back to previous list
     LISTSTART on level 5 with 7 nodes
      Data 7 (length = 6, octet string): 81 81 C7 82 05 FF
      Data 6 (empty)
      Data 5 (empty)
      Data 4 (empty)
      Data 3 (empty)
      Data (length = 48): FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
      Data 2 (empty)
      Data 1 (empty)
     LISTEND
     back to previous list
    back to previous list
    Data 2 (empty)
    Data 1 (length = 2, unsigned int): B9 3F
   LISTEND on level 3
   back to previous list
  back to previous list
 End of block at level 1
  LISTSTART on level 2 with 6 nodes
   Data 6 (length = 6, octet string): 00 0C 04 08 87 31
   Data 5 (length = 1, unsigned int): 00
   Data 4 (length = 1, unsigned int): 00
   LISTSTART on level 3 with 2 nodes
    Data 2 (length = 2, unsigned int): 02 01
    LISTSTART on level 4 with 1 nodes
     Data 1 (empty)
    LISTEND
    back to previous list
   back to previous list
   Data 2 (length = 2, unsigned int): 6A 53
  End of block at level 2
  back to previous list
 back to previous list
End of block at level 0
Received checksum: C6E8
Calculated checksum: C6E8
>>> FINAL! Checksum OK
>>> Manufacturer.............: EMH
>>> Power T1    (1-0:1.8.1)..: 12345678.900 Wh
>>> Power T1+T2 (1-0:1.8.0)..: 7238000.000 Wh
```

## Links

The following sites provided a lot of helpful information to me.

- https://wiki.volkszaehler.org/software/obis
- https://wiki.volkszaehler.org/software/sml
- https://www.stefan-weigert.de/php_loader/sml.php
- https://github.com/devZer0/libsml-testing

## License

GNU LGPL v2.1
