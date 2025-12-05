#!/usr/bin/python3

from dataclasses import dataclass, field

@dataclass
class FormField:
    offset: int
    name: str
    default: int | None = None
    values: dict[int, str] = field(default_factory=dict)
    data_type: str = "int32"
    elem: str | None = None

SHORTCUTS = {
    0x73: "None",
    0x2A: "Backspace",
    0x39: "Caps Lock",
    0x2B: "Tab",
    0x46: "Print Screen",
    0x47: "Scroll Lock",
    0x53: "Num Lock",
    }

STATUS_ = [
    FormField(78, "Running FW version", None, {}, "uint16", elem="fw_version"),
    FormField(79, "Running FW checksum", None, {}, "uint32", elem="hex_info"),
]

CONFIG_ = [
    FormField(1001, "Mouse", elem="label"),
    FormField(71, "Force Mouse Boot Mode", None, {}, "uint8", "checkbox"),
    FormField(75, "Enable Acceleration", None, {}, "uint8", "checkbox"),
    FormField(77, "Jump Threshold", 0, {"min": 0, "max": 3000}, "uint16", "range"),

    FormField(1002, "Keyboard", elem="label"),
    FormField(72, "Force KBD Boot Protocol", None, {}, "uint8", "checkbox"),
    FormField(73, "KBD LED as Indicator", None, {}, "uint8", "checkbox"),

    FormField(1004, "Gaming Mode", elem="label"),
    FormField(83, "Start in Gaming Mode", None, {}, "uint8", "checkbox"),
    FormField(84, "Enable Edge Switching in Gaming Mode", None, {}, "uint8", "checkbox"),

    FormField(76, "Enforce Ports", None, {}, "uint8", "checkbox"),
]

OUTPUT_ = [
    FormField(1, "Screen Count", 1, {1: "1", 2: "2", 3: "3"}, "uint32"),
    FormField(2, "Speed X", 16, {"min": 1, "max": 100}, "int32", "range"),
    FormField(3, "Speed Y", 16, {"min": 1, "max": 100}, "int32", "range"),
    FormField(4, "Border Top", None, {}, "int32"),
    FormField(5, "Border Bottom", None, {}, "int32"),
    FormField(6, "Operating System", 1, {1: "Linux", 2: "MacOS", 3: "Windows", 4: "Android", 255: "Other"}, "uint8"),
    FormField(7, "Screen Position", 1, {1: "Left", 2: "Right"}, "uint8"),
    FormField(8, "Cursor Park Position", 0, {0: "Top", 1: "Bottom", 3: "Previous"}, "uint8"),
    FormField(1003, "Screensaver", elem="label"),
    FormField(9, "Mode", 0, {0: "Disabled", 1: "Pong", 2: "Jitter"}, "uint8"),
    FormField(10, "Only If Inactive", None, {}, "uint8", "checkbox"),
    FormField(11, "Idle Time (μs)", None, {}, "uint64"),
    FormField(12, "Max Time (μs)", None, {}, "uint64"),
    FormField(1005, "Gaming Mode Edge Switching", elem="label"),
    FormField(13, "Movement Threshold", None, {}, "uint32", "number"),
    FormField(14, "Time Window (ms)", None, {}, "uint32", "number"),
    FormField(15, "Max Vertical Movement", None, {}, "uint32", "number"),
]

def generate_output(base, data):
    output = [
        {
            "name": field.name,
            "key": base + field.offset,
            "default": field.default,
            "values": field.values,
            "type": field.data_type,
            "elem": field.elem,
        }
        for field in data
    ]
    return output

def output_A(base=10):
    return generate_output(base, data=OUTPUT_)

def output_B(base=40):
    return generate_output(base, data=OUTPUT_)

def output_status():
    return generate_output(0, data=STATUS_)

def output_config():
    return generate_output(0, data=CONFIG_)
