# Arduino Serial

Reads data from Arduino Pin 8.

Outputs some additional information if `SML_DEBUG` is enabled (see platformio.ini).

# Example output

```
START
LISTSTART on level 1 with 6 nodes
 Data 6 (length = 6): 00 19 03 77 8D E2
 Data 5 (length = 1): 00
 Data 4 (length = 1): 00
LISTSTART on level 2 with 2 nodes
 Data 2 (length = 2): 01 01
LISTSTART on level 3 with 6 nodes
 Data 6 (empty)
 Data 5 (empty)
 Data 4 (length = 6): 00 xx xx xx xx xx
 Data 3 (length = 10): xx xx xx xx xx 00 00 xx xx xx
 Data 2 (empty)
 Data 1 (empty)
LISTEND
back to previous list
back to previous list
 Data 2 (length = 2): 2F 90
End of block at level 1
back to previous list
LISTSTART on level 1 with 6 nodes
 Data 6 (length = 6): 00 19 03 77 8D E3
 Data 5 (length = 1): 00
 Data 4 (length = 1): 00
LISTSTART on level 2 with 2 nodes
 Data 2 (length = 2): 07 01
LISTSTART on level 3 with 7 nodes
 Data 7 (empty)
 Data 6 (length = 10): 09 01 45 4D 48 00 00 xx xx xx
 Data 5 (length = 6): 01 00 62 0A FF FF
LISTSTART on level 4 with 2 nodes
 Data 2 (length = 1): 01
 Data 1 (length = 4): 08 20 1F A9
LISTEND on level 4
back to previous list
LISTSTART on level 4 with 9 nodes
LISTSTART on level 5 with 7 nodes
 Data 7 (length = 6): 81 81 C7 82 03 FF
 Data 6 (empty)
 Data 5 (empty)
 Data 4 (empty)
 Data 3 (empty)
 Data 2 (length = 3): 45 4D 48
 Data 1 (empty)
LISTEND
back to previous list
LISTSTART on level 5 with 7 nodes
 Data 7 (length = 6): 01 00 00 00 09 FF
 Data 6 (empty)
 Data 5 (empty)
 Data 4 (empty)
 Data 3 (empty)
 Data 2 (length = 10): 09 01 xx xx xx 00 00 xx xx xx
 Data 1 (empty)
LISTEND
back to previous list
LISTSTART on level 5 with 7 nodes
 Data 7 (length = 6): 01 00 01 08 00 FF
 Data 6 (length = 3): 01 01 82
 Data 5 (empty)
 Data 4 (length = 1): 1E
 Data 3 (length = 1): 03
 Data 2 (length = 5): 00 00 00 5D 5D
 Data 1 (empty)
LISTEND
back to previous list
LISTSTART on level 5 with 7 nodes
 Data 7 (length = 6): 01 00 02 08 00 FF
 Data 6 (length = 3): 01 01 82
 Data 5 (empty)
 Data 4 (length = 1): 1E
 Data 3 (length = 1): 03
 Data 2 (length = 5): 00 00 00 00 00
 Data 1 (empty)
LISTEND
back to previous list
LISTSTART on level 5 with 7 nodes
 Data 7 (length = 6): 01 00 01 08 01 FF
 Data 6 (empty)
 Data 5 (empty)
 Data 4 (length = 1): 1E
 Data 3 (length = 1): 03
 Data 2 (length = 5): 00 00 00 5D 5D
 Data 1 (empty)
LISTEND
back to previous list
LISTSTART on level 5 with 7 nodes
 Data 7 (length = 6): 01 00 02 08 01 FF
 Data 6 (empty)
 Data 5 (empty)
 Data 4 (length = 1): 1E
 Data 3 (length = 1): 03
 Data 2 (length = 5): 00 00 00 00 00
 Data 1 (empty)
LISTEND
back to previous list
LISTSTART on level 5 with 7 nodes
 Data 7 (length = 6): 01 00 01 08 02 FF
 Data 6 (empty)
 Data 5 (empty)
 Data 4 (length = 1): 1E
 Data 3 (length = 1): 03
 Data 2 (length = 5): 00 00 00 00 00
 Data 1 (empty)
LISTEND
back to previous list
LISTSTART on level 5 with 7 nodes
 Data 7 (length = 6): 01 00 02 08 02 FF
 Data 6 (empty)
 Data 5 (empty)
 Data 4 (length = 1): 1E
 Data 3 (length = 1): 03
 Data 2 (length = 5): 00 00 00 00 00
 Data 1 (empty)
LISTEND
back to previous list
LISTSTART on level 5 with 7 nodes
 Data 7 (length = 6): 81 81 C7 82 05 FF
 Data 6 (empty)
 Data 5 (empty)
 Data 4 (empty)
 Data 3 (empty)
 Data (length = 48): xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx
 Data 2 (empty)
 Data 1 (empty)
LISTEND
back to previous list
back to previous list
 Data 2 (empty)
 Data 1 (length = 2): 8B D3
LISTEND on level 3
back to previous list
back to previous list
End of block at level 1
LISTSTART on level 2 with 6 nodes
 Data 6 (length = 6): 00 19 03 77 8D E4
 Data 5 (length = 1): 00
 Data 4 (length = 1): 00
LISTSTART on level 3 with 2 nodes
 Data 2 (length = 2): 02 01
LISTSTART on level 4 with 1 nodes
 Data 1 (empty)
LISTEND
back to previous list
back to previous list
 Data 2 (length = 2): 58 75
End of block at level 2
back to previous list
back to previous list
Received checksum: E0BE
Calculated checksum: E0BE
>>> Successfully received a complete message!
```