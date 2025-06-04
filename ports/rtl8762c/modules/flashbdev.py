import rtl

class FlashBdev:
    SEC_SIZE = rtl.SEC_SIZE

    def __init__(self, start_sec, blocks):
        self.start_sec = start_sec
        self.blocks = blocks

    def readblocks(self, n, buf, off=0):
        # print("readblocks(%s, %x(%d), %d)" % (n, id(buf), len(buf), off))
        return rtl.flash_read((n + self.start_sec) * self.SEC_SIZE + off, buf)

    def writeblocks(self, n, buf, off=None):
        # print("writeblocks(%s, %x(%d), %d)" % (n, id(buf), len(buf), off))
        assert len(buf) <= self.SEC_SIZE, len(buf)
        if off is None:
            rtl.flash_erase(n + self.start_sec)
            off = 0
        rtl.flash_write((n + self.start_sec) * self.SEC_SIZE + off, buf)

    def ioctl(self, op, arg):
        # print("ioctl(%d, %r)" % (op, arg))
        if op == 4:  # MP_BLOCKDEV_IOCTL_BLOCK_COUNT
            return self.blocks
        if op == 5:  # MP_BLOCKDEV_IOCTL_BLOCK_SIZE
            return self.SEC_SIZE
        if op == 6:  # MP_BLOCKDEV_IOCTL_BLOCK_ERASE
            rtl.flash_erase(arg + self.start_sec)
            return 0

start_sec = rtl.flash_user_start // FlashBdev.SEC_SIZE
bdev = FlashBdev(start_sec, (rtl.flash_size // FlashBdev.SEC_SIZE) - start_sec)
