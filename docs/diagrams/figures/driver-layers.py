"""The three layers of the driver: hardware-agnostic core, communication interface, platform port.

Reads: From top to bottom. Your application talks to one PCA9685<I2cType> object, whose public calls (listed
from inc/pca9685.hpp) set the frequency, the channels and the output stage. Each call turns into register reads
and writes with retries. The communication interface (I2cInterface in inc/pca9685_i2c_interface.hpp) is a CRTP
base that forwards them to your class. The platform port is the only code you write: a class that brings the
I2C bus up and moves register bytes over it. Below it is the hardware.
"""

import re
from pathlib import Path

from hardfoc_mkdocs_theme.figkit import Fig

ROOT = Path(__file__).resolve().parents[3]
DRIVER = "inc/pca9685.hpp"
COMM = "inc/pca9685_i2c_interface.hpp"
SKIP = {"IsInitialized", "HasError", "HasAnyError", "ClearError", "GetLastError", "SetRetries",
        "SetRetryDelay"}


def data():
    drv = (ROOT / DRIVER).read_text()
    public = drv.split("public:", 1)[1].split("private:", 1)[0]
    calls = []
    for name in re.findall(r"^  (?:\[\[nodiscard\]\] )?bool (\w+)\(", public, re.M):
        if name not in SKIP and name not in calls:
            calls.append(name)
    comm = (ROOT / COMM).read_text()
    hooks = [n for n in ("EnsureInitialized", "Write", "Read") if re.search(rf"bool {n}\(", comm)]
    return {
        "calls": calls,
        "port_hooks": hooks,
        "sources": [f"{DRIVER}:PCA9685 public calls", f"{COMM}:I2cInterface"],
    }


def build(d):
    f = Fig(
        "driver-layers",
        "Three layers, one of them yours",
        alt="Layer diagram: the application calls the hardware-agnostic PCA9685 driver, which uses the CRTP I2C "
        "interface, which calls the platform port you write, which drives the I2C bus and the 16 PWM outputs "
        "of the PCA9685.",
        width=1200,
        kicker="Architecture",
        subtitle="The driver and the interface are the same on every MCU. The port is a small class with three methods.",
        profile="overview",
        legend=[("brand", "this library"), ("teal", "you write"), ("slate", "hardware")],
    )
    W = f.body_w
    app = f.card(0, 0, W, "Your application", ["servo control, LED dimming, motor speed ..."], tone="slate")
    top = app.b + 40
    chips_h = f.chips(16, f.region_inner_top(top), W - 32, [c + "()" for c in d["calls"]], tone="brand")
    reg = f.region(0, top, W, f.region_inner_top(top) - top + chips_h + 18, "Hardware-agnostic core", tone="brand",
                   sub="PCA9685<I2cType>, inc/pca9685.hpp")
    comm = f.card(0, reg.b + 40, W, "Communication interface",
                  ["I2cInterface<Derived>: CRTP base, register reads and writes resolved at compile time,",
                   "retries and error flags in the driver above it"],
                  tone="brand", accent=True)
    port = f.card(0, comm.b + 40, W, "Platform port (your class)", ["  ".join(h + "()" for h in d["port_hooks"])],
                  tone="teal", accent=True, mono=True)
    hw = f.card(0, port.b + 40, W, "PCA9685", ["I2C up to 1 MHz (Fm+), address 0x40 to 0x7F  /  LED0 to LED15, OE"],
                tone="slate")
    for a, b in ((app, reg), (reg, comm), (comm, port), (port, hw)):
        f.connect(a, b, "bottom", "top", tone="rail")
    return f
