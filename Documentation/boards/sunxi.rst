Allwinner sunxi
===

Because of size constraints Barebox proper cannot boot directly, the uses
of :doc:`PBL` allows to compress the Barebox image and it's device-tree.
However this is not enough, and two images are acctually needed. The first
image is suffixed *_xload* and it only consist of the PBL with a special
entry point that looks for ``barebox.bin`` the root of a FAT partition.
Only the SD card is currently searched, but this could also be in eMMC.
The second image is your standard Barebox plus PBL image (suffixed ``.pblb``).

Boot process


On power-up Allwinner SoC starts in boot ROM, aka BROM, which will search
for an eGON image: first from the SD card, then from eMMC. If no image is
found then the BROM will enter into FEL mode that can be used for initial
programming and recovery of devices using USB.

Some board may have a button to enter FEL mode at startup. If not, another
way to enter FEL mode is to not have a valid image eGON image, this can be
achived by erasing existing eGON image headers.

eGON header
---

The eGON header structure is described in the file ``include/mach/sunxi/egon.h``.
This is also documented on https://linux-sunxi.org/EGON .

The eGON header, followed by the actual image, must be located at a fixed
offset of 8192 bytes (4K) from the start of the disk, either SD; or eMMC.

.. code-block:: sh

  # copy the "pine64_xload" eGON image into disk sdd
  dd if=images/start_pine64_pine64_xload.pblb.egonimg of=/dev/sdd bs=1024 seek=8

The above will write the entire "pine64_xload" Barebox PBL plus the eGON
header into the disk "/dev/sdd".

BROM will load, at most, the first 32KB of the image into SRAM, including
the header itself! The jump instruction in the header needs to be patched
accordingly with the image size.

Note that on on sunxi platforms the boot ROM will load the entire image
**including** the eGON header. The actual load address will be offset by
the eGON header (currently 96 bytes), this bad because arm instructions
used for relocation expect the base address to be aligned on 4K boundary.
As a workaround, a egon header is included and linked into the Barebox
pbl image, this dummy header will be filled later by egon_mkimage.

Board images are defined in ``images/Makefile.sunxi``, here is an example::

.. code-block:: none

  pblb-$(CONFIG_MACH_PINE64_PINE64) += start_pine64_pine64_xload
  MAX_PBL_IMAGE_SIZE_start_pine64_pine64_xload = 0x8000
  FILE_barebox-pine64-pine64_xload.img = start_pine64_pine64_xload.pblb.egonimg
  image-$(CONFIG_MACH_PINE64_PINE64) += barebox-pine64-pine64_xload.img


RMR aarch64 switch
--

Aarch64 capable SoC (A64/sun50i) boot by default in 32-bit mode. A special header
is added to the start of the PBL image in order to switch to aarch64 mode as soon
as possible. This must be done very early in the boot process since both ISA are
not compatible. The code to switch mode is already assembled (mostly arm 32bit)
and is documented in the header file ``include/mach/sunxi/rmr_switch.h``.

FEL
---

The ``sunxi-fel`` tool is used to interact, through USB, with sunxi devices
in FEL mode. ``sunxi-fel`` is part of the sunxi-tools_.

.. _sunxi-tools: https://github.com/linux-sunxi/sunxi-tools

More documentation about FEL_ and how to use the sunxi-fel tool can be
found on https://linux-sunxi.org/FEL/USBBoot .

**Note:** ``sunxi-fel`` has a commands dedicated to boot u-boot images but theses
commands require a valid eGON header, if not more. This can be easily bypassed.

The ``sunxi-fel`` tool can be used to load any arbitrary image at a given address
and can also request the processor to jump and start executing at any address.
This can be achieved by the following two commands::

.. code-block:: sh

  sunxi-fel write-with-progress 0x00018000 images/start_pine64_pinephone.pblb
  sunxi-fel exe 0x00018000

These two commands allows the use of a different and bigger SRAM than the
default 32KB used by the boot ROM.
