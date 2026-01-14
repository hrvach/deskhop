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
    """A table row with a label and four input fields (from start/end, to start/end)."""
    label: str
    from_start_offset: int
    from_end_offset: int
    to_start_offset: int
    to_end_offset: int
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
    FormField(87, "Hold Threshold (ms)", 180, {"min": 0, "max": 1000}, "uint16", "range",
              help="Time in milliseconds to distinguish tap (permanent switch) from hold (temporary switch). Set to 0 to disable hold-to-switch."),

    FormField(76, "Enforce Ports", None, {}, "uint8", "checkbox",
              help="Restricts keyboard to board A and mouse to board B. Enable if your devices are being detected on the wrong board."),

    FormField(1003, "Computer Border (A ↔ B)", elem="table_start", json_name="border",
              help="Coordinate range for switching between computers. Y-range for horizontal layouts (Left/Right), X-range for vertical layouts (Top/Bottom)."),
    TableRow("A ↔ B", 83, 84, 85, 86, data_type="int16", json_prefix=""),
    FormField(1004, "", elem="table_end"),
]

OUTPUT_ = [
    FormField(1, "Screen Count", 1, {1: "1", 2: "2", 3: "3"}, "uint32"),
    FormField(4, "Monitor Layout", 0, {0: "Horizontal", 1: "Vertical"}, "uint8",
              help="How monitors are arranged on this computer: Horizontal (side-by-side) or Vertical (stacked top-to-bottom)."),
    FormField(5, "Border Monitor Index", 1, {1: "1", 2: "2", 3: "3"}, "uint8",
              help="Which monitor can switch to the other computer. Numbering: 1=left/top, 2=middle, 3=right/bottom. Only applies when monitor orientation differs from computer orientation (e.g., side-by-side monitors with a stacked computer arrangement)."),
    FormField(2, "Speed X", 16, {"min": 1, "max": 100}, "int32", "range"),
    FormField(3, "Speed Y", 16, {"min": 1, "max": 100}, "int32", "range"),
    FormField(6, "Operating System", 1, {1: "Linux", 2: "MacOS", 3: "Windows", 4: "Android", 255: "Other"}, "uint8"),
    FormField(7, "Screen Position", 1, {1: "Left", 2: "Right", 4: "Top", 5: "Bottom"}, "uint8",
              help="Position of this output in your setup. Use Left/Right for side-by-side setups, Top/Bottom for stacked setups. The inter-computer border is on the opposite side (e.g., Right position = border on left edge)."),
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

    FormField(1004, "Screen Transitions", elem="table_start", json_name="transitions",
              help="Coordinate ranges where cursor can move between monitors. Y-ranges for horizontal layouts, X-ranges for vertical layouts. Use RightShift + F12 + Y hotkey; see manual for more details."),
    TableRow("1 ↔ 2", 13, 14, 15, 16, data_type="int16"),
    TableRow("2 ↔ 3", 17, 18, 19, 20, data_type="int16"),
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
                "from_start_key": base + item.from_start_offset,
                "from_end_key": base + item.from_end_offset,
                "to_start_key": base + item.to_start_offset,
                "to_end_key": base + item.to_end_offset,
                "type": item.data_type,
                "elem": item.elem,
                "json_names": {
                    "from_start": f"{prefix}.from_start" if prefix else "from_start",
                    "from_end": f"{prefix}.from_end" if prefix else "from_end",
                    "to_start": f"{prefix}.to_start" if prefix else "to_start",
                    "to_end": f"{prefix}.to_end" if prefix else "to_end",
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
