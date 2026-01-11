#!/usr/bin/python3

from dataclasses import dataclass, field

def to_json_name(name: str) -> str:
    """Convert a human-readable name to a JSON-friendly key."""
    return (name.lower()
            .replace(" ↔ ", "_")
            .replace("↔", "_")
            .replace(" ", "_")
            .replace("(", "")
            .replace(")", "")
            .replace("μ", "u"))

@dataclass
class FormField:
    offset: int
    name: str
    default: int | None = None
    values: dict[int, str] = field(default_factory=dict)
    data_type: str = "int32"
    elem: str | None = None
    json_name: str | None = None
    help: str | None = None

    def __post_init__(self):
        if self.json_name is None and self.name:
            self.json_name = to_json_name(self.name)

@dataclass
class TableRow:
    """A table row with a label and four input fields (from top/bottom, to top/bottom)."""
    label: str
    from_top_offset: int
    from_bottom_offset: int
    to_top_offset: int
    to_bottom_offset: int
    data_type: str = "int32"
    elem: str = "table_row"
    json_prefix: str | None = None

    def __post_init__(self):
        if self.json_prefix is None:
            # Convert "1 ↔ 2" to "1_2", "A ↔ B" to "a_b"
            self.json_prefix = self.label.lower().replace(" ↔ ", "_")

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
    FormField(71, "Force Mouse Boot Mode", None, {}, "uint8", "checkbox",
              help="Not recommended for most users. Uses a simpler protocol that may fix compatibility issues, but the mouse may lose some features."),
    FormField(75, "Enable Acceleration", None, {}, "uint8", "checkbox",
              help="Makes the cursor move faster the quicker you move the mouse. Disable for precise 1:1 movement, e.g. for gaming or design work."),
    FormField(77, "Jump Threshold", 0, {"min": 0, "max": 3000}, "uint16", "range"),

    FormField(1002, "Keyboard", elem="label"),
    FormField(72, "Force KBD Boot Protocol", None, {}, "uint8", "checkbox",
              help="Enable if your keyboard isn't working correctly. Some keyboards with media keys or macro features need this simpler protocol."),
    FormField(73, "KBD LED as Indicator", None, {}, "uint8", "checkbox",
              help="Briefly flashes Caps/Scroll/Num Lock LEDs when switching outputs, giving visual feedback of which computer is active."),

    FormField(76, "Enforce Ports", None, {}, "uint8", "checkbox",
              help="Restricts keyboard to board A and mouse to board B. Enable if your devices are being detected on the wrong board."),

    FormField(1003, "Computer Border (A ↔ B)", elem="table_start", json_name="border"),
    TableRow("A ↔ B", 83, 84, 85, 86, json_prefix=""),
    FormField(1004, "", elem="table_end"),
]

OUTPUT_ = [
    FormField(1, "Screen Count", 1, {1: "1", 2: "2", 3: "3"}, "uint32"),
    FormField(2, "Speed X", 16, {"min": 1, "max": 100}, "int32", "range"),
    FormField(3, "Speed Y", 16, {"min": 1, "max": 100}, "int32", "range"),
    FormField(6, "Operating System", 1, {1: "Linux", 2: "MacOS", 3: "Windows", 4: "Android", 255: "Other"}, "uint8"),
    FormField(7, "Screen Position", 1, {1: "Left", 2: "Right"}, "uint8"),
    FormField(8, "Cursor Park Position", 0, {0: "Top", 1: "Bottom", 3: "Previous"}, "uint8",
              help="Where the cursor appears when switching to this computer. Use 'Previous' to remember the last position, or set Top/Bottom for a consistent starting point."),
    FormField(1003, "Screensaver", elem="label"),
    FormField(9, "Mode", 0, {0: "Disabled", 1: "Pong", 2: "Jitter"}, "uint8",
              help="Moves the cursor automatically to prevent screen sleep. Pong bounces smoothly across the screen; Jitter makes small random movements."),
    FormField(10, "Only If Inactive", None, {}, "uint8", "checkbox",
              help="Only activate screensaver when no mouse/keyboard input is detected. Disable to run screensaver continuously regardless of activity."),
    FormField(11, "Idle Time (μs)", None, {}, "uint64",
              help="How long to wait (in microseconds) after last input before starting the screensaver. Example: 5000000 = 5 seconds."),
    FormField(12, "Max Time (μs)", None, {}, "uint64",
              help="Maximum duration (in microseconds) the screensaver runs before stopping. Set to 0 for unlimited. Example: 60000000 = 1 minute."),

    FormField(1004, "Screen Transitions", elem="table_start",
              help="Defines where the cursor exits and enters each screen. Use RightShift + F12 + Y hotkey; see manual for more details."),
    TableRow("1 ↔ 2", 13, 14, 15, 16),
    TableRow("2 ↔ 3", 17, 18, 19, 20),
    FormField(1005, "", elem="table_end"),
]

def generate_output(base, data, table_context=""):
    output = []
    current_table_name = ""
    for item in data:
        if isinstance(item, TableRow):
            # Build prefix: table_context + current_table_name + row_prefix (if any)
            parts = [p for p in [table_context.rstrip('.'), current_table_name, item.json_prefix] if p]
            prefix = ".".join(parts) if parts else ""
            output.append({
                "label": item.label,
                "from_top_key": base + item.from_top_offset,
                "from_bottom_key": base + item.from_bottom_offset,
                "to_top_key": base + item.to_top_offset,
                "to_bottom_key": base + item.to_bottom_offset,
                "type": item.data_type,
                "elem": item.elem,
                "json_names": {
                    "from_top": f"{prefix}.from_top" if prefix else "from_top",
                    "from_bottom": f"{prefix}.from_bottom" if prefix else "from_bottom",
                    "to_top": f"{prefix}.to_top" if prefix else "to_top",
                    "to_bottom": f"{prefix}.to_bottom" if prefix else "to_bottom",
                },
            })
        else:
            # Track current table name for table rows
            if item.elem == "table_start":
                current_table_name = item.json_name if item.json_name else (to_json_name(item.name) if item.name else "")
            elif item.elem == "table_end":
                current_table_name = ""

            output.append({
                "name": item.name,
                "key": base + item.offset,
                "default": item.default,
                "values": item.values,
                "type": item.data_type,
                "elem": item.elem,
                "json_name": f"{table_context}{item.json_name}" if item.json_name else None,
                "help": item.help,
            })
    return output

def output_A(base=10):
    return generate_output(base, data=OUTPUT_)

def output_B(base=40):
    return generate_output(base, data=OUTPUT_)

def output_status():
    return generate_output(0, data=STATUS_)

def output_config():
    return generate_output(0, data=CONFIG_)
