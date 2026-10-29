import createModule from "./sml.js";
import { samples } from "./samples.js";

/* sml_states_t values of the entry types, see src/sml.h */
const SML_DATA_SIGNED_INT = 16;
const SML_DATA_UNSIGNED_INT = 17;
const SML_DATA_OCTET_STRING = 18;

/* sml_units_t, see src/sml.h */
const UNITS = {
  1: "a", 2: "mo", 3: "wk", 4: "d", 5: "h", 6: "min", 7: "s", 8: "°",
  9: "°C", 10: "currency", 11: "m", 12: "m/s", 13: "m³", 14: "m³ (corr.)",
  15: "m³/h", 16: "m³/h (corr.)", 17: "m³/d", 18: "m³/d (corr.)", 19: "l",
  20: "kg", 21: "N", 22: "Nm", 23: "Pa", 24: "bar", 25: "J", 26: "J/h",
  27: "W", 28: "VA", 29: "var", 30: "Wh", 31: "VAh", 32: "varh", 33: "A",
  34: "C", 35: "V", 36: "V/m", 37: "F", 38: "Ω", 39: "Ωm", 40: "Wb", 41: "T",
  42: "A/m", 43: "H", 44: "Hz", 45: "1/(Wh)", 46: "1/(varh)", 47: "1/(VAh)",
  48: "V²h", 49: "A²h", 50: "kg/s", 52: "K", 53: "1/(V²h)", 54: "1/(A²h)",
  55: "1/m³", 56: "%", 57: "Ah", 60: "Wh/m³", 61: "J/m³", 62: "mol %",
  63: "g/m³", 64: "Pa·s", 253: "reserved", 254: "other", 255: "count",
};

/* Common OBIS codes, matched with and without the F group */
const OBIS_NAMES = {
  "129-129:199.130.3": "Manufacturer",
  "129-129:199.130.5": "Public key",
  "129-129:199.240.6": "Manufacturer specific",
  "1-0:0.0.0": "Device ID (property number)",
  "1-0:0.0.9": "Server ID",
  "1-0:96.1.0": "Device ID",
  "1-0:96.50.1*1": "Manufacturer",
  "1-0:96.50.1*4": "Hardware version",
  "1-0:96.50.4*4": "Parameter version",
  "1-0:96.90.2*1": "Firmware checksum",
  "1-0:97.97.0": "Status register",
  "1-0:0.2.0": "Firmware version",
  "1-0:1.8.0": "Energy import, total",
  "1-0:1.8.1": "Energy import, tariff 1",
  "1-0:1.8.2": "Energy import, tariff 2",
  "1-0:2.8.0": "Energy export, total",
  "1-0:2.8.1": "Energy export, tariff 1",
  "1-0:2.8.2": "Energy export, tariff 2",
  "1-0:15.7.0": "Active power, absolute",
  "1-0:16.7.0": "Active power, total",
  "1-0:21.7.0": "Active power L1",
  "1-0:41.7.0": "Active power L2",
  "1-0:61.7.0": "Active power L3",
  "1-0:36.7.0": "Active power L1",
  "1-0:56.7.0": "Active power L2",
  "1-0:76.7.0": "Active power L3",
  "1-0:31.7.0": "Current L1",
  "1-0:51.7.0": "Current L2",
  "1-0:71.7.0": "Current L3",
  "1-0:32.7.0": "Voltage L1",
  "1-0:52.7.0": "Voltage L2",
  "1-0:72.7.0": "Voltage L3",
  "1-0:14.7.0": "Grid frequency",
  "1-0:81.7.1": "Phase angle U-L2 to U-L1",
  "1-0:81.7.2": "Phase angle U-L3 to U-L1",
  "1-0:81.7.4": "Phase angle I-L1 to U-L1",
  "1-0:81.7.15": "Phase angle I-L2 to U-L2",
  "1-0:81.7.26": "Phase angle I-L3 to U-L3",
};

const $ = (id) => document.getElementById(id);
const input = $("input");
const output = $("output");

/* ---------- Theme ---------- */

function applyTheme(choice) {
  if (choice === "light" || choice === "dark") {
    document.documentElement.dataset.theme = choice;
  } else {
    delete document.documentElement.dataset.theme;
  }
  for (const button of document.querySelectorAll("[data-theme-choice]")) {
    button.setAttribute(
      "aria-checked",
      String(button.dataset.themeChoice === choice),
    );
  }
  try {
    if (choice === "auto") localStorage.removeItem("theme");
    else localStorage.setItem("theme", choice);
  } catch (e) {}
}

for (const button of document.querySelectorAll("[data-theme-choice]")) {
  button.addEventListener("click", () => applyTheme(button.dataset.themeChoice));
}
applyTheme(document.documentElement.dataset.theme || "auto");

/* ---------- Input ---------- */

/** Convert hex strings or C arrays into bytes, throws on invalid input. */
function parseInput(text) {
  const code = text
    .replace(/\/\*[\s\S]*?\*\//g, " ")
    .replace(/\/\/.*$/gm, " ");

  if (/0x[0-9a-f]/i.test(code)) {
    /* C array, e.g. a header from test/test/ */
    let body = code;
    const open = code.indexOf("{");
    const close = code.indexOf("}", open);
    if (open >= 0 && close > open) body = code.slice(open + 1, close);
    const tokens = body.split(/[\s,]+/).filter(Boolean);
    return Uint8Array.from(tokens, (token) => {
      const value = /^0x[0-9a-f]{1,2}$/i.test(token)
        ? parseInt(token, 16)
        : /^\d{1,3}$/.test(token)
          ? parseInt(token, 10)
          : NaN;
      if (!(value >= 0 && value <= 255)) {
        throw new Error(`"${token}" is not a byte value`);
      }
      return value;
    });
  }

  const hex = code.replace(/[\s:,;-]/g, "");
  const bad = hex.search(/[^0-9a-f]/i);
  if (bad >= 0) {
    throw new Error(`Unexpected character "${hex[bad]}", expected hex digits`);
  }
  if (hex.length % 2) {
    throw new Error(`Odd number of hex digits (${hex.length})`);
  }
  const bytes = new Uint8Array(hex.length / 2);
  for (let i = 0; i < bytes.length; i++) {
    bytes[i] = parseInt(hex.substr(i * 2, 2), 16);
  }
  return bytes;
}

function toHex(bytes) {
  return Array.from(bytes, (b) => b.toString(16).padStart(2, "0"))
    .join("")
    .toUpperCase();
}

/* ---------- Parser ---------- */

let logLines = [];
const sml = await createModule({
  print: (line) => logLines.push(line),
  printErr: (line) => logLines.push(line),
});

function runParser(bytes) {
  logLines = [];
  const ptr = sml._malloc(Math.max(bytes.length, 1));
  try {
    sml.HEAPU8.set(bytes, ptr);
    const result = sml.UTF8ToString(sml._web_parse(ptr, bytes.length));
    return {
      events: result.split("\n").filter(Boolean).map((line) => JSON.parse(line)),
      log: logLines.join("\n"),
    };
  } finally {
    sml._free(ptr);
  }
}

/** Group the parser events into messages from START to the final state. */
function toMessages(events, length) {
  const messages = [];
  let current = null;
  const close = (status, end, detail) => {
    if (!current) return;
    Object.assign(current, { status, end, detail });
    messages.push(current);
    current = null;
  };
  for (const event of events) {
    switch (event.event) {
      case "start":
        close("incomplete", event.offset - 1, "a new message started");
        current = { start: event.offset, entries: [] };
        break;
      case "entry":
        if (current) current.entries.push(event);
        break;
      case "final":
        close("ok", event.offset);
        break;
      case "checksum_error":
        close("checksum", event.offset);
        break;
      case "unexpected":
        close(
          "unexpected",
          event.offset,
          `unexpected byte 0x${event.byte.toString(16).padStart(2, "0").toUpperCase()} at offset ${event.offset}`,
        );
        break;
    }
  }
  close("incomplete", length - 1, "end of input");
  return messages;
}

/* ---------- Output ---------- */

/** value × 10^scaler as exact decimal string */
function scaled(value, scaler) {
  const negative = value < 0n;
  let digits = (negative ? -value : value).toString();
  if (scaler >= 0) {
    if (value !== 0n) digits += "0".repeat(scaler);
  } else {
    const decimals = -scaler;
    digits = digits.padStart(decimals + 1, "0");
    digits = digits.slice(0, -decimals) + "." + digits.slice(-decimals);
  }
  return (negative ? "-" : "") + digits;
}

function obisName(obis) {
  return OBIS_NAMES[obis] || OBIS_NAMES[obis.replace(/\*\d+$/, "")] || "";
}

function el(tag, attrs = {}, ...children) {
  const node = document.createElement(tag);
  for (const [key, value] of Object.entries(attrs)) {
    if (key === "class") node.className = value;
    else node.setAttribute(key, value);
  }
  for (const child of children) {
    if (child != null) node.append(child);
  }
  return node;
}

function renderValue(entry) {
  const unit = UNITS[entry.unit] || (entry.unit ? `unit ${entry.unit}` : "");
  if (entry.type === SML_DATA_SIGNED_INT || entry.type === SML_DATA_UNSIGNED_INT) {
    const value = BigInt(entry.value);
    const raw =
      entry.scaler !== 0 ? `${value} × 10^${entry.scaler}` : String(value);
    return [
      el("td", { class: "num" }, scaled(value, entry.scaler)),
      el("td", {}, unit),
      el("td", { class: "bytes" }, raw, el("span", { class: "sub" }, entry.bytes.toUpperCase())),
    ];
  }
  const bytes = entry.bytes.match(/../g) || [];
  const text = bytes.map((b) => parseInt(b, 16));
  const printable =
    text.length > 0 && text.every((c) => c >= 0x20 && c < 0x7f)
      ? String.fromCharCode(...text)
      : null;
  const type =
    entry.type === SML_DATA_OCTET_STRING
      ? "octet string"
      : bytes.length === 0
        ? "empty"
        : "data";
  return [
    el("td", { class: "num" }, printable != null ? `"${printable}"` : ""),
    el("td", {}, unit),
    el("td", { class: "bytes" }, entry.bytes.toUpperCase() || "–", el("span", { class: "sub" }, type)),
  ];
}

const STATUS = {
  ok: ["ok", "Checksum OK"],
  checksum: ["err", "Checksum error"],
  unexpected: ["err", "Parse error"],
  incomplete: ["warn", "Incomplete"],
};

function renderMessage(message, index) {
  const [kind, label] = STATUS[message.status];
  const head = el(
    "div",
    { class: "message-head" },
    el(
      "span",
      {},
      el("strong", {}, `Message ${index + 1}`),
      " ",
      el(
        "span",
        { class: "muted" },
        `· bytes ${message.start}–${message.end} · ${message.entries.length} entries`,
      ),
    ),
    el(
      "span",
      { class: `badge ${kind}`, title: message.detail || "" },
      message.detail ? `${label}: ${message.detail}` : label,
    ),
  );
  if (message.entries.length === 0) return el("div", { class: "message" }, head);

  const rows = message.entries.map((entry) =>
    el(
      "tr",
      {},
      el("td", { class: "obis" }, entry.obis),
      el("td", {}, obisName(entry.obis)),
      ...renderValue(entry),
    ),
  );
  const table = el(
    "table",
    {},
    el(
      "thead",
      {},
      el(
        "tr",
        {},
        el("th", {}, "OBIS"),
        el("th", {}, "Description"),
        el("th", { class: "num" }, "Value"),
        el("th", {}, "Unit"),
        el("th", {}, "Raw"),
      ),
    ),
    el("tbody", {}, ...rows),
  );
  return el("div", { class: "message" }, head, el("div", { class: "table-wrap" }, table));
}

function update() {
  const error = $("input-error");
  error.hidden = true;
  $("input-info").textContent = "";
  $("output-info").textContent = "";
  $("log").textContent = "";

  if (!input.value.trim()) {
    output.replaceChildren(
      el("div", { class: "empty" }, "Paste an SML message or pick a sample."),
    );
    return;
  }

  let bytes;
  try {
    bytes = parseInput(input.value);
  } catch (e) {
    error.textContent = e.message;
    error.hidden = false;
    return;
  }
  $("input-info").textContent = `${bytes.length} bytes`;

  const { events, log } = runParser(bytes);
  const messages = toMessages(events, bytes.length);
  $("log").textContent = log;

  if (messages.length === 0) {
    output.replaceChildren(
      el(
        "div",
        { class: "empty" },
        "No SML message found. A message starts with 1B 1B 1B 1B 01 01 01 01.",
      ),
    );
    return;
  }
  const ok = messages.filter((m) => m.status === "ok").length;
  $("output-info").textContent =
    `${messages.length} message${messages.length > 1 ? "s" : ""}, ${ok} valid`;
  output.replaceChildren(...messages.map(renderMessage));
}

let timer;
input.addEventListener("input", () => {
  clearTimeout(timer);
  timer = setTimeout(update, 120);
});

/* ---------- Samples, file drop ---------- */

for (const sample of samples) {
  const button = el("button", { type: "button" }, sample.name);
  button.addEventListener("click", () => {
    input.value = sample.data;
    update();
  });
  $("sample-buttons").append(button);
}

$("clear").addEventListener("click", () => {
  input.value = "";
  update();
  input.focus();
});

input.addEventListener("dragover", (e) => {
  if (!e.dataTransfer.types.includes("Files")) return;
  e.preventDefault();
  input.classList.add("dragging");
});
input.addEventListener("dragleave", () => input.classList.remove("dragging"));
input.addEventListener("drop", async (e) => {
  input.classList.remove("dragging");
  const file = e.dataTransfer.files[0];
  if (!file) return;
  e.preventDefault();
  const bytes = new Uint8Array(await file.arrayBuffer());
  /* text files (hex or C headers) are pasted as they are */
  const isText = bytes.every((b) => b === 9 || b === 10 || b === 13 || (b >= 32 && b < 127));
  input.value = isText ? new TextDecoder().decode(bytes) : toHex(bytes);
  update();
});

input.value = samples[0].data;
update();
