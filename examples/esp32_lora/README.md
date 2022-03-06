# Forward energy usage to LoraWAN (The Things Network)

- Rename keys.h.example to keys.h and insert your own DEVEUI and APPKEY
- Use the following uplink payload formatter:

```JavaScript
function decodeUplink(input) {
  var data = {};
  var str = String.fromCharCode.apply(null, input.bytes).trim();
  data.raw = str;
  if (str > 0) {
    data.kwh = str;
  }
  return {
    data: data,
    warnings: [],
    errors: []
  };
}
```
