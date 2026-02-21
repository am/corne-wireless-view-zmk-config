#!/usr/bin/env python3
"""
Generate ASCII visual reference of ZMK keymap layers from config/corne.keymap.
Output is markdown code blocks showing the Corne layout (6+6, 6+6, 6+6, 3+3) per layer.
"""

import json
import re
import sys
from pathlib import Path
from typing import Dict, Optional

# Corne: 3 rows of 6 keys per hand, then 1 row of 3 thumb keys (aligned to inner side)
ROW_SIZES = (6, 6, 6, 3)  # left half sizes; right half mirrors
# Cell width: more surrounding space around each keycode (7 chars = e.g. "   Q   ")
CELL_WIDTH = 7
BOX_H = "\u2500"  # ─ (box-drawing horizontal)
GAP = " "  # single space between left and right half
# Thumb row indent: 3 cells of the main block = 3 * (1 + CELL_WIDTH) chars
THUMB_INDENT = 3 * (1 + CELL_WIDTH)

_keycode_labels_cache: Optional[Dict[str, str]] = None


def _load_keycode_labels() -> Dict[str, str]:
    """Load keycode -> label mapping from keycode-labels.json if present; else return built-in default."""
    builtin = {
        "BACKSPACE": "BSp", "ESCAPE": "Esc", "RETURN": "Ret", "ENTER": "Ent",
        "SEMICOLON": "SEMI", "SEMI": "SEMI", "SQT": "'", "APOSTROPHE": "'",
        "BACKSLASH": "BSLH", "QUESTION": "?", "QMARK": "?", "PIPE": "|",
        "LEFT_PARENTHESIS": "(", "LPAR": "(", "RIGHT_PARENTHESIS": ")", "RPAR": ")",
        "LEFT_BRACKET": "[", "LBKT": "[", "RIGHT_BRACKET": "]", "RBKT": "]",
        "LEFT_BRACE": "{", "LBRC": "{", "RIGHT_BRACE": "}", "RBRC": "}",
        "EXCLAMATION": "!", "EXCL": "!", "AT": "@", "HASH": "#", "POUND": "#",
        "DOLLAR": "$", "DLLR": "$", "PERCENT": "%", "PRCNT": "%",
        "CARET": "^", "AMPERSAND": "&", "AMPS": "&", "STAR": "*", "ASTERISK": "*",
        "MINUS": "-", "PLUS": "+", "EQUAL": "=", "UNDERSCORE": "_", "UNDER": "_",
        "SLASH": "/", "FSLH": "/", "COMMA": ",", "DOT": ".", "PERIOD": ".",
        "GRAVE": "`", "TILDE": "~",
        "CAPS": "Caps", "CAPSLOCK": "Caps",
        "LCTRL": "LC", "RCTRL": "RC", "LSHFT": "LS", "RSHFT": "RS",
        "LALT": "LA", "RALT": "RA", "LGUI": "LG", "RGUI": "RG",
        "K_PREVIOUS": "Prev", "K_PREV": "Prev", "C_PLAY": "Play", "C_NEXT": "Next",
        "C_PLAY_PAUSE": "Play", "C_PP": "Play",
        "C_MUTE": "Mute", "C_VOL_DN": "VolDn", "C_VOL_UP": "VolUp",
        "C_VOLUME_DOWN": "VolDn", "C_VOLUME_UP": "VolUp",
        "LEFT": "←", "RIGHT": "→", "UP": "↑", "DOWN": "↓",
        "BT_SEL": "BT", "BT_CLR": "Clr",
        "N1": "1", "N2": "2", "N3": "3", "N4": "4", "N5": "5",
        "N6": "6", "N7": "7", "N8": "8", "N9": "9", "N0": "0",
        "SPACE": "Sp",
    }
    global _keycode_labels_cache
    if _keycode_labels_cache is not None:
        return _keycode_labels_cache
    script_dir = Path(__file__).resolve().parent
    path = script_dir / "keycode-labels.json"
    if path.exists():
        try:
            with open(path, encoding="utf-8") as f:
                custom = json.load(f)
            builtin.update(custom)
        except (json.JSONDecodeError, OSError):
            pass
    _keycode_labels_cache = builtin
    return builtin


def shorten_keycode(raw: str, abbrev: Optional[Dict[str, str]] = None) -> str:
    """Shorten keycode for display in ASCII cell. Uses keycode-labels.json if present."""
    if abbrev is None:
        abbrev = _load_keycode_labels()
    s = raw.strip()
    if not s:
        return "-"
    for full, short in abbrev.items():
        if s == full or s.endswith(" " + full):
            return short
    # Strip K_/C_ prefix for brevity when not in abbrev
    if s.startswith("K_") and len(s) > 6:
        s = s[2:]
    if s.startswith("C_") and len(s) > 6:
        s = s[2:]
    # Truncate if too long (display will re-truncate to CELL_WIDTH in template)
    if len(s) > 6:
        s = s[:5] + "."
    return s


def _mod_verbose(mod: str) -> str:
    """Map ZMK modifier token to verbose label: Ctl, Alt, Shf, Cmd."""
    u = mod.upper()
    if u in ("LCTRL", "RCTRL", "LC", "RC", "CTRL", "CONTROL"):
        return "Ctl"
    if u in ("LALT", "RALT", "LA", "RA", "ALT"):
        return "Alt"
    if u in ("LSHFT", "RSHFT", "LSHIFT", "RSHIFT", "LS", "RS", "SHIFT"):
        return "Shf"
    if u in ("LGUI", "RGUI", "LWIN", "RWIN", "LCMD", "RCMD", "GUI", "WIN", "CMD"):
        return "Cmd"
    return mod[:3] if len(mod) >= 3 else mod


def parse_binding(token: str) -> str:
    """Parse one binding token (e.g. 'kp TAB', 'mo 1', 'trans') into a short label."""
    token = token.strip()
    if not token or token == "trans":
        return "-"
    # &kp KEY or &kp MOD(KEY)
    m = re.match(r"kp\s+(.+)", token, re.IGNORECASE)
    if m:
        key = m.group(1).strip()
        # Handle LG(KEY) etc
        key = re.sub(r"L[A-Z]+\(([^)]+)\)", r"\1", key)
        return shorten_keycode(key)
    # &mo N -> Layer N
    m = re.match(r"mo\s+(\d+)", token, re.IGNORECASE)
    if m:
        return f"Layer {m.group(1)}"
    # &lt N KEY -> L3+Sp (layer-tap)
    m = re.match(r"lt\s+(\d+)\s+(\S+)", token, re.IGNORECASE)
    if m:
        layer, key = m.group(1), m.group(2)
        k = shorten_keycode(key)
        return f"L{layer}+{k}" if k and k != "-" else f"L{layer}"
    # &mt MOD KEY -> verbose e.g. Alt+S, Cmd+D, Shf+J, Ctl+Esc
    m = re.match(r"mt\s+(\S+)\s+(\S+)", token, re.IGNORECASE)
    if m:
        mod, key = m.group(1), m.group(2)
        mod_str = _mod_verbose(mod)
        key_str = shorten_keycode(key)
        return f"{mod_str}+{key_str}" if key_str and key_str != "-" else mod_str
    # &td0, &td1, ...
    m = re.match(r"td(\d+)", token, re.IGNORECASE)
    if m:
        return f"TD{m.group(1)}"
    # &bt BT_SEL N
    m = re.match(r"bt\s+BT_SEL\s+(\d+)", token, re.IGNORECASE)
    if m:
        return f"BT{m.group(1)}"
    # &bt BT_CLR
    m = re.match(r"bt\s+BT_CLR", token, re.IGNORECASE)
    if m:
        return "Clr"
    # &bt ...
    if token.lower().startswith("bt "):
        return "BT"
    return token[: CELL_WIDTH - 2] if len(token) > CELL_WIDTH else token


def extract_bindings(content: str) -> list[str]:
    """Extract binding tokens from bindings = < ... >; block."""
    inner = re.search(r"bindings\s*=\s*<\s*([\s\S]*?)\s*>", content)
    if not inner:
        return []
    text = inner.group(1)
    # Split by & to get tokens (then strip and drop empty)
    tokens = [t.strip() for t in re.split(r"\s*&\s*", text) if t.strip()]
    return tokens


def extract_layers(keymap_path: Path) -> list[tuple[str, list[str]]]:
    """Parse keymap file and return [(display_name, [binding_labels]), ...]."""
    text = keymap_path.read_text()
    layers = []
    # Find each layer block: optional display-name = "Name"; then bindings = < ... >;
    layer_blocks = re.split(
        r"(\w+_layer)\s*\{", text
    )
    for i in range(1, len(layer_blocks), 2):
        block_name = layer_blocks[i]
        block_content = layer_blocks[i + 1]
        display_name = "Layer"
        m = re.search(r'display-name\s*=\s*"([^"]+)"', block_content)
        if m:
            display_name = m.group(1)
        bindings_raw = extract_bindings(block_content)
        labels = [parse_binding(t) for t in bindings_raw]
        layers.append((display_name, labels))
    return layers


def _box_row(seg: str, left: str, mid: str, right: str, n: int) -> str:
    """One horizontal line: left + (seg+mid)*(n-1) + seg + right. Uses seg = BOX_H*CELL_WIDTH."""
    return left + (seg + mid) * (n - 1) + seg + right


def _cell(lbl: str) -> str:
    """Format label for one cell: exactly CELL_WIDTH chars, centered, truncated if needed. Transparent = empty."""
    if lbl.strip() == "-":
        return " " * CELL_WIDTH
    s = lbl[:CELL_WIDTH] if len(lbl) > CELL_WIDTH else lbl
    return s.center(CELL_WIDTH)


def draw_layer(name: str, labels: list[str]) -> str:
    """Produce ASCII diagram only (no Layer: name). For use with layer name outside code block."""
    seg = BOX_H * CELL_WIDTH  # "───"
    sep = "│"
    lines = []
    idx = 0
    row_size = 6

    # One continuous 6+6 block: top, row0 content, mid, row1 content, mid, row2 content, special bottom
    left_top = _box_row(seg, "┌", "┬", "┐", row_size)
    right_top = _box_row(seg, "┌", "┬", "┐", row_size)
    left_mid = _box_row(seg, "├", "┼", "┤", row_size)
    right_mid = _box_row(seg, "├", "┼", "┤", row_size)
    left_bot = "└" + seg + "┴" + seg + "┴" + seg + "┼" + seg + "┼" + seg + "┼" + seg + "┤"
    right_bot = "├" + seg + "┼" + seg + "┼" + seg + "┼" + seg + "┴" + seg + "┴" + seg + "┘"

    for row in range(3):
        left_labels = [labels[idx + i] if idx + i < len(labels) else "?" for i in range(row_size)]
        right_labels = [labels[idx + row_size + i] if idx + row_size + i < len(labels) else "?" for i in range(row_size)]
        idx += 2 * row_size

        if row == 0:
            lines.append(left_top + GAP + right_top)
        content_left = sep + sep.join([_cell(l) for l in left_labels]) + sep
        content_right = sep + sep.join([_cell(l) for l in right_labels]) + sep
        lines.append(content_left + GAP + content_right)
        if row == 2:
            lines.append(left_bot + GAP + right_bot)
        else:
            lines.append(left_mid + GAP + right_mid)

    # Thumb row: 3+3 aligned to inner side (indent = 12 spaces to match template)
    left_labels = [labels[idx + i] if idx + i < len(labels) else "?" for i in range(3)]
    right_labels = [labels[idx + 3 + i] if idx + 3 + i < len(labels) else "?" for i in range(3)]
    indent = " " * THUMB_INDENT
    content_left = indent + sep + sep.join([_cell(l) for l in left_labels]) + sep
    content_right = sep + sep.join([_cell(l) for l in right_labels]) + sep
    left_thumb_bot = _box_row(seg, "└", "┴", "┘", 3)
    right_thumb_bot = _box_row(seg, "└", "┴", "┘", 3)
    lines.append(content_left + GAP + content_right)
    lines.append(indent + left_thumb_bot + GAP + right_thumb_bot)
    return "\n".join(lines)


def main() -> None:
    import argparse
    parser = argparse.ArgumentParser(description="Generate ASCII keymap reference from ZMK keymap")
    parser.add_argument("--markdown", action="store_true", help="Wrap output in a markdown code block")
    parser.add_argument("--keymap", type=Path, default=None, help="Path to keymap file (default: config/corne.keymap)")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    keymap_path = args.keymap or (repo_root / "config" / "corne.keymap")
    if not keymap_path.exists():
        print(f"Keymap not found: {keymap_path}", file=sys.stderr)
        sys.exit(1)

    layers = extract_layers(keymap_path)
    blocks = []
    for name, labels in layers:
        diagram = draw_layer(name, labels)
        blocks.append(f"### Layer: {name}\n\n```\n{diagram}\n```")
    print("\n\n".join(blocks))


if __name__ == "__main__":
    main()
