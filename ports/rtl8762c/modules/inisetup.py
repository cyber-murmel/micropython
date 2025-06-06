import os
from flashbdev import bdev

boot_py = """\
print("hello")
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
