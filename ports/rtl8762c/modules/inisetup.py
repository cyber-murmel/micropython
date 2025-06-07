import os
from flashbdev import bdev

boot_py = """\
import os
from micropython import const
from bluetooth import BLE
from ble_advertising import advertising_payload
from ble_uart_peripheral import BLEUART
from ble_uart_repl import BLEUARTStream

# org.bluetooth.characteristic.gap.appearance.xml
ADV_APPEARANCE_GENERIC_COMPUTER = const(128)

IRQ_CENTRAL_CONNECT = const(1)
IRQ_CENTRAL_DISCONNECT = const(2)

payload = advertising_payload(name="uPython", appearance=ADV_APPEARANCE_GENERIC_COMPUTER)

# Activate bluetooth
ble = BLE()
ble.active(True)
connections = set()

# Initialize UART REPL
uart = BLEUART(ble, connections)
stream = BLEUARTStream(uart)
os.dupterm(stream)

def ble_irq_cb(event, data):
    # Track connections so we can send notifications.
    if event == IRQ_CENTRAL_CONNECT:
        conn_handle, _, _ = data
        connections.add(conn_handle)
    elif event == IRQ_CENTRAL_DISCONNECT:
        conn_handle, _, _ = data
        if conn_handle in connections:
            connections.remove(conn_handle)
        # Start advertising again to allow a new connection.
        ble.gap_advertise(500000, adv_data=payload)
    else:
        conn_handle, value_handle = data
        if conn_handle in connections:
            if value_handle in uart.handles:
                uart.irq_cb(event, data)

ble.irq(ble_irq_cb)

# All BLE services must be registered at once
uart.handles, = ble.gatts_register_services((uart.service, ))

ble.gap_advertise(500000, adv_data=payload)
"""

def setup():
    print("Formating flash block device...")
    os.VfsLfs1.mkfs(bdev)
    print("Formated")

    print("Mounting...")
    vfs = os.VfsLfs1(bdev)
    os.mount(vfs, "/")
    print("Mounted")

    print("Writing boot.py...")
    chunk_size = 64
    with open("/boot.py", "w") as f:
        for i in range(0, len(boot_py), chunk_size):
            f.write(boot_py[i:i+chunk_size])
    print("Written")

    return vfs
