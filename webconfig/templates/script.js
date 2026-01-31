const mgmtReportId = 6;
var device;

/* Screen Position validation: both outputs must use same orientation */
const SCREEN_POS_KEY_A = 17;  /* Output A: base 10 + offset 7 */
const SCREEN_POS_KEY_B = 47;  /* Output B: base 40 + offset 7 */

/* Conditional visibility: Monitor Layout and Border Monitor Index only shown when Screen Count > 1 */
const SCREEN_COUNT_KEY_A = 11;      /* Output A: base 10 + offset 1 */
const SCREEN_COUNT_KEY_B = 41;      /* Output B: base 40 + offset 1 */
const MONITOR_LAYOUT_KEY_A = 14;    /* Output A: base 10 + offset 4 */
const MONITOR_LAYOUT_KEY_B = 44;    /* Output B: base 40 + offset 4 */
const BORDER_MON_IDX_KEY_A = 15;    /* Output A: base 10 + offset 5 */
const BORDER_MON_IDX_KEY_B = 45;    /* Output B: base 40 + offset 5 */

const packetType = {
  keyboardReportMsg: 1, mouseReportMsg: 2, outputSelectMsg: 3, firmwareUpgradeMsg: 4, switchLockMsg: 7,
  syncBordersMsg: 8, flashLedMsg: 9, wipeConfigMsg: 10, readConfigMsg: 16, writeConfigMsg: 17, saveConfigMsg: 18,
  rebootMsg: 19, getValMsg: 20, setValMsg: 21, getValAllMsg: 22, proxyPacketMsg: 23
};

function calcChecksum(report) {
  let checksum = 0;
  for (let i = 3; i < 11; i++)
    checksum ^= report[i];

  return checksum;
}

async function sendReport(type, payload = [], sendBoth = false) {
  if (!device || !device.opened)
    return;

  /* First send this one, if the first one gets e.g. rebooted */
  if (sendBoth) {
    var reportProxy = makeReport(type, payload, true);
    await device.sendReport(mgmtReportId, reportProxy);
    }

    var report = makeReport(type, payload, false);
    await device.sendReport(mgmtReportId, report);
}

function makeReport(type, payload, proxy=false) {
  var dataOffset = proxy ? 4 : 3;
  report = new Uint8Array([0xaa, 0x55, type, ...new Array(9).fill(0)]);

  if (proxy)
    report = new Uint8Array([0xaa, 0x55, packetType.proxyPacketMsg, type, ...new Array(7).fill(0), type]);

  if (payload) {
    report.set([...payload], dataOffset);
    report[report.length - 1] = calcChecksum(report);
  }
  return report;
}

function packValue(element, key, dataType, buffer) {
  const dataOffset = 1;
  var buffer = new ArrayBuffer(8);
  var view = new DataView(buffer);

  const methods = {
    "uint32": view.setUint32,
    "uint64": view.setUint32, /* Yes, I know. :-| */
    "int32": view.setInt32,
    "uint16": view.setUint16,
    "uint8": view.setUint8,
    "int16": view.setInt16,
    "int8": view.setInt8
  };

  if (dataType in methods) {
    const method = methods[dataType];
    if (element.type === 'checkbox')
      view.setUint8(dataOffset, element.checked ? 1 : 0, true);
    else
      method.call(view, dataOffset, element.value, true);
  }

  view.setUint8(0, key);
  return new Uint8Array(buffer);
}

window.addEventListener('load', async function () {
  if (!("hid" in navigator)) {
    document.getElementById('warning').style.display = 'block';
    return;
  }

  this.document.getElementById('menu-buttons').addEventListener('click', function (event) {
    window[event.target.dataset.handler]();
  });

  /* Hide Border Monitor Index by default (shown when Screen Count > 1) */
  updateAllMultiMonitorFieldsVisibility();

  // Try to auto-connect to a previously authorized device
  await autoConnectHandler();
});

async function autoConnectHandler() {
  // Get previously authorized devices (no user interaction required)
  const devices = await navigator.hid.getDevices();

  // Find a DeskHop device
  const deskhopDevice = devices.find(d =>
    d.vendorId === 0x2e8a &&
    d.productId === 0x107c &&
    d.collections.some(c => c.usagePage === 0xff00 && c.usage === 0x10)
  );

  if (deskhopDevice) {
    device = deskhopDevice;
    if (!device.opened) {
      await device.open();
    }
    device.addEventListener('inputreport', handleInputReport);
    document.querySelectorAll('.online').forEach(element => { element.style.opacity = 1.0; });
    await readHandler();
  }
}

document.getElementById('submitButton').addEventListener('click', async () => { await saveHandler(); });

async function connectHandler() {
  if (device && device.opened)
    return;

  var devices = await navigator.hid.requestDevice({
    filters: [{ vendorId: 0x2e8a, productId: 0x107c, usagePage: 0xff00, usage: 0x10 }]
  });

  device = devices[0];
  device.open().then(async () => {
    device.addEventListener('inputreport', handleInputReport);
    document.querySelectorAll('.online').forEach(element => { element.style.opacity = 1.0; });
    await readHandler();
  });
}

async function blinkHandler() {
  await sendReport(packetType.flashLedMsg, []);
}

async function blinkBothHandler() {
  await sendReport(packetType.flashLedMsg, [], true);
}

function getValue(element) {
  if (element.type === 'checkbox')
    return element.checked ? 1 : 0;
  else
    return element.value;
}

function setValue(element, value, updateFetchedValue = true) {
  if (updateFetchedValue)
    element.setAttribute('fetched-value', value);

  if (element.type === 'checkbox')
    element.checked = value;
  else
    element.value = value;
  element.dispatchEvent(new Event('input', { bubbles: true }));

  /* Update Border Monitor Index visibility when Screen Count changes */
  const key = parseInt(element.getAttribute('data-key'));
  if (key === SCREEN_COUNT_KEY_A || key === SCREEN_COUNT_KEY_B)
    updateMultiMonitorFieldsVisibility(key);
}


function updateElement(key, event) {
  var dataOffset = 4;
  var element = document.querySelector(`[data-key="${key}"]`);

  if (!element)
    return;

  const methods = {
    "uint32": event.data.getUint32,
    "uint64": event.data.getUint32, /* Yes, I know. :-| */
    "int32": event.data.getInt32,
    "uint16": event.data.getUint16,
    "uint8": event.data.getUint8,
    "int16": event.data.getInt16,
    "int8": event.data.getInt8
  };

  dataType = element.getAttribute('data-type');

  if (dataType in methods) {
    var value = methods[dataType].call(event.data, dataOffset, true);
    setValue(element, value);

    if (element.hasAttribute('data-hex'))
      setValue(element, parseInt(value).toString(16));

    if (element.hasAttribute('data-fw-ver')) {
      /* u16 version = major * 1000 + minor + 100; */
      const major = Math.floor((value - 100) / 1000);
      const minor = (value - 100) % 1000;
      setValue(element, `v${major}.${minor}`);
    }
  }
}

async function readHandler() {
  if (!device || !device.opened)
    await connectHandler();

  await sendReport(packetType.getValAllMsg);
}

async function handleInputReport(event) {
  var data = new Uint8Array(event.data.buffer);
  var key = data[3];

  updateElement(key, event);
}

async function rebootHandler() {
  await sendReport(packetType.rebootMsg);
}

async function enterBootloaderHandler() {
  await sendReport(packetType.firmwareUpgradeMsg, true, true);
}

function setFieldVisibility(fieldKey, show) {
  const element = document.querySelector(`[data-key="${fieldKey}"]`);
  if (!element)
    return;

  const display = show ? '' : 'none';

  /* Hide the element and its preceding label */
  element.style.display = display;
  const label = element.previousElementSibling;
  if (label && label.tagName === 'LABEL')
    label.style.display = display;
  /* Also hide the <br> after the element */
  const br = element.nextElementSibling;
  if (br && br.tagName === 'BR')
    br.style.display = display;
}

function updateMultiMonitorFieldsVisibility(screenCountKey) {
  const isOutputA = (screenCountKey === SCREEN_COUNT_KEY_A);
  const monitorLayoutKey = isOutputA ? MONITOR_LAYOUT_KEY_A : MONITOR_LAYOUT_KEY_B;
  const borderMonKey = isOutputA ? BORDER_MON_IDX_KEY_A : BORDER_MON_IDX_KEY_B;

  const screenCountEl = document.querySelector(`[data-key="${screenCountKey}"]`);

  /* Hide by default if Screen Count not set or <= 1 */
  const screenCount = screenCountEl ? parseInt(screenCountEl.value) : 0;
  const show = screenCount > 1;

  setFieldVisibility(monitorLayoutKey, show);
  setFieldVisibility(borderMonKey, show);
}

function updateAllMultiMonitorFieldsVisibility() {
  updateMultiMonitorFieldsVisibility(SCREEN_COUNT_KEY_A);
  updateMultiMonitorFieldsVisibility(SCREEN_COUNT_KEY_B);
}

function isVerticalPosition(value) {
  /* TOP=4, BOTTOM=5 are vertical; LEFT=1, RIGHT=2 are horizontal */
  return value == 4 || value == 5;
}

function validateScreenPositions() {
  const elementA = document.querySelector(`[data-key="${SCREEN_POS_KEY_A}"]`);
  const elementB = document.querySelector(`[data-key="${SCREEN_POS_KEY_B}"]`);

  if (!elementA || !elementB || !elementA.value || !elementB.value)
    return true;  /* Not both set yet, allow */

  const aVertical = isVerticalPosition(elementA.value);
  const bVertical = isVerticalPosition(elementB.value);

  if (aVertical !== bVertical) {
    const aOrientation = aVertical ? "vertical (Top/Bottom)" : "horizontal (Left/Right)";
    const bOrientation = bVertical ? "vertical (Top/Bottom)" : "horizontal (Left/Right)";

    alert(`Screen Position mismatch!\n\nOutput A is set to ${aOrientation}, but Output B is set to ${bOrientation}.\n\nBoth outputs must use the same orientation (both horizontal or both vertical).`);
    return false;
  }

  return true;
}

async function valueChangedHandler(element) {
  var key = element.getAttribute('data-key');
  var dataType = element.getAttribute('data-type');
  var keyInt = parseInt(key);

  var origValue = element.getAttribute('fetched-value');
  var newValue = getValue(element);

  /* Update Border Monitor Index visibility when Screen Count changes */
  if (keyInt === SCREEN_COUNT_KEY_A || keyInt === SCREEN_COUNT_KEY_B)
    updateMultiMonitorFieldsVisibility(keyInt);

  if (origValue != newValue) {
    uintBuffer = packValue(element, key, dataType);

    /* Send to both devices */
    await sendReport(packetType.setValMsg, uintBuffer, true);

    /* Set this as the current value */
    element.setAttribute('fetched-value', newValue);
  }
}

async function saveHandler() {
  const elements = document.querySelectorAll('.api');

  if (!device || !device.opened)
    return;

  /* Validate Screen Position orientation before saving */
  if (!validateScreenPositions())
    return;

  for (const element of elements) {
    var origValue = element.getAttribute('fetched-value')

    if (element.hasAttribute('readonly'))
      continue;

    if (origValue != getValue(element))
      await valueChangedHandler(element);
  }
  await sendReport(packetType.saveConfigMsg, [], true);
}

async function wipeConfigHandler() {
  await sendReport(packetType.wipeConfigMsg, [], true);
}

function setNestedValue(obj, path, value) {
  const parts = path.split('.');
  let current = obj;
  for (let i = 0; i < parts.length - 1; i++) {
    if (!(parts[i] in current)) current[parts[i]] = {};
    current = current[parts[i]];
  }
  current[parts[parts.length - 1]] = value;
}

function getNestedValue(obj, path) {
  const parts = path.split('.');
  let current = obj;
  for (const part of parts) {
    if (current === undefined || current === null) return undefined;
    current = current[part];
  }
  return current;
}

const configVersion = {{ config_version }};

function exportConfigHandler() {
  const config = { version: configVersion, output_a: {}, output_b: {}, common: {} };

  document.querySelectorAll('.api').forEach(el => {
    if (el.hasAttribute('readonly')) return;

    const name = el.getAttribute('data-name');
    const section = el.getAttribute('data-section');

    if (name && section && config[section]) {
      setNestedValue(config[section], name, getValue(el));
    }
  });

  const blob = new Blob([JSON.stringify(config, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = 'deskhop-config.json';
  a.click();
  URL.revokeObjectURL(url);
}

function importConfigHandler() {
  const input = document.createElement('input');
  input.type = 'file';
  input.accept = '.json';
  input.onchange = async (e) => {
    const file = e.target.files[0];
    if (!file) return;

    try {
      const text = await file.text();
      const config = JSON.parse(text);

      document.querySelectorAll('.api').forEach(el => {
        if (el.hasAttribute('readonly')) return;

        const name = el.getAttribute('data-name');
        const section = el.getAttribute('data-section');

        if (name && section && config[section]) {
          const value = getNestedValue(config[section], name);
          if (value !== undefined) {
            setValue(el, value, false);
          }
        }
      });
    } catch (err) {
      alert('Failed to import config: ' + err.message);
    }
  };
  input.click();
}
