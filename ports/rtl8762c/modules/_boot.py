import os
from flashbdev import bdev

if bdev:
    try:
        os.mount(bdev, "/")
    except:
        import inisetup 
        inisetup.setup()