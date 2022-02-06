# Smart Message Language (SML) parser

Easy to use C++ library with a low memory foodprint to parse SML messages byte by byte. 

The library will control the last CRC value to check if the received data is correct. On any error the parser will reset and wait for valid data. 

Outsite the library handlers can be registered to work on received information. 

## Example and usage

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

If debug mode (see examples/native) is enabled, the SML data is displayed in a tree like structure.

```
START
 LISTSTART with 6 nodes
  Data 6 (length = 6): 00 0C 04 08 87 2D
  Data 5 (length = 1): 00
  Data 4 (length = 1): 00
  LISTSTART with 2 nodes
   Data 2 (length = 2): 01 01
   LISTSTART with 6 nodes
    Data 6 (empty)
    Data 5 (empty)
    Data 4 (length = 6): 00 0C 06 9E 2D 0F
    Data 3 (length = 10): 06 45 4D 48 01 00 1D 46 15 CA
    Data 2 (empty)
    Data 1 (empty)
   LISTEND
   back to previous list
  back to previous list
  Data 2 (length = 2): 2B 8E
 End of block 1
 back to previous list
 LISTSTART with 6 nodes
  Data 6 (length = 6): 00 0C 04 08 87 2E
  Data 5 (length = 1): 00
  Data 4 (length = 1): 00
  LISTSTART with 2 nodes
   Data 2 (length = 2): 07 01
   LISTSTART with 7 nodes
    Data 7 (empty)
    Data 6 (length = 10): 06 45 4D 48 01 00 1D 46 15 CA
    Data 5 (empty)
    LISTSTART with 2 nodes
     Data 2 (length = 1): 01
     Data 1 (length = 4): 06 9E FA 83
    LISTEND
    back to previous list
    LISTSTART with 7 nodes
     LISTSTART with 7 nodes
      Data 7 (length = 6): 81 81 C7 82 03 FF
      Data 6 (empty)
      Data 5 (empty)
      Data 4 (empty)
      Data 3 (empty)
      Data 2 (length = 3): 45 4D 48
      Data 1 (empty)
     LISTEND
     back to previous list
     LISTSTART with 7 nodes
      Data 7 (length = 6): 01 00 00 00 09 FF
      Data 6 (empty)
      Data 5 (empty)
      Data 4 (empty)
      Data 3 (empty)
      Data 2 (length = 10): 06 45 4D 48 01 00 1D 46 15 CA
      Data 1 (empty)
     LISTEND
     back to previous list
     LISTSTART with 7 nodes
      Data 7 (length = 6): 01 00 01 08 00 FF
      Data 6 (length = 2): 01 82
      Data 5 (empty)
      Data 4 (length = 1): 1E
      Data 3 (length = 1): FF
      Data 2 (length = 5): 00 09 06 74 EA
      Data 1 (empty)
     LISTEND
     back to previous list
     LISTSTART with 7 nodes
      Data 7 (length = 6): 01 00 01 08 01 FF
      Data 6 (empty)
      Data 5 (empty)
      Data 4 (length = 1): 1E
      Data 3 (length = 1): FF
      Data 2 (length = 8): 00 00 00 00 07 5B CD 15
      Data 1 (empty)
     LISTEND
     back to previous list
     LISTSTART with 7 nodes
      Data 7 (length = 6): 01 00 01 08 02 FF
      Data 6 (empty)
      Data 5 (empty)
      Data 4 (length = 1): 1E
      Data 3 (length = 1): FF
      Data 2 (length = 5): 00 00 00 00 00
      Data 1 (empty)
     LISTEND
     back to previous list
     LISTSTART with 7 nodes
      Data 7 (length = 6): 01 00 0F 07 00 FF
      Data 6 (empty)
      Data 5 (empty)
      Data 4 (length = 1): 1B
      Data 3 (length = 1): FF
      Data 2 (length = 4): 00 00 2F 65
      Data 1 (empty)
     LISTEND
     back to previous list
     LISTSTART with 7 nodes
      Data 7 (length = 6): 81 81 C7 82 05 FF
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
    Data 1 (length = 2): B9 3F
   LISTEND
   back to previous list
  back to previous list
 End of block 1
  LISTSTART with 6 nodes
   Data 6 (length = 6): 00 0C 04 08 87 31
   Data 5 (length = 1): 00
   Data 4 (length = 1): 00
   LISTSTART with 2 nodes
    Data 2 (length = 2): 02 01
    LISTSTART with 1 nodes
     Data 1 (empty)
    LISTEND
    back to previous list
   back to previous list
   Data 2 (length = 2): 6A 53
  End of block 2
  back to previous list
 back to previous list
End of block 0
Received checksum: 1B70
Calculated checksum: 1B70
>>> FINAL! Checksum OK
>>> Manufacturer.............: EMH
>>> Power T1    (1-0:1.8.1)..: 12345678.900 kWh
>>> Power T1+T2 (1-0:1.8.0)..: 15141809.000 kWh
```
## Links

The following sites provided a lot of helpful information to me.

- https://wiki.volkszaehler.org/software/obis
- https://wiki.volkszaehler.org/software/sml
- https://www.stefan-weigert.de/php_loader/sml.php

## License

GNU LGPL v2.1


