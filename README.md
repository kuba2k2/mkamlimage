# mkamlimage

Amlogic Upgrade Package creation/extraction tool

## Usage

This tool can create and extract Amlogic upgrade images (.img), used by the Meson6, Meson8b, GX* families (and probably
others).

Extraction of Version 1 and 2 packages is supported, but created packages are always Version 2.

### Building

Simply, use Make:

```shell
make all
```

### Create package

Example using mainline U-Boot, encrypted with tools from `amlogic-boot-fip`:

```shell
./mkamlimage \
	"u-boot-live.img" \
	"normal,USB,DDR=u-boot.bin.usb.bl2" \
	"normal,USB,UBOOT=u-boot.bin.usb.tpl" \
	"normal,conf,platform=platform.conf"
```

The first argument is the output file name. Each following argument adds a new image to the package, and follows the
format:

`<file_type>,<main_type>,<sub_type>=<filename>`

For more details, refer to [
`pyamlboot/AML-IMAGE-FORMAT.md`](https://github.com/superna9999/pyamlboot/blob/master/AML-IMAGE-FORMAT.md).

Example of `platform.conf` for S905W - it's best to extract this from a known-working package:

```
Platform:0x0811
DDRLoad:0xd9000000
DDRRun:0xd9000000
UbootLoad:0x200c000
UbootRun:0xd9000000
Control0=0xd9000000:0x000000b1
Control1=0xd9000000:0x00005183
Encrypt_reg:0xc8100228
bl2ParaAddr=0xd900c000
```

When using binaries extracted from a vendor-provided package, the resulting image can be 100% byte-identical to the
original one.

### Listing contents

Use `-l` with `unamlimage` to list package contents without extracting.

```shell
$ ./unamlimage -l MXQ-S805-0720.img
Found version 1 image with 18 file(s)
File 'normal,USB,DDR' - offset: 0x00000940, size: 15092, verify: no
File 'normal,USB,UBOOT_COMP' - offset: 0x00004434, size: 343724, verify: no
File 'normal,conf,platform' - offset: 0x000582e0, size: 116, verify: no
File 'normal,dtd,meson8b_m201_1G' - offset: 0x00058354, size: 67956, verify: no
File 'normal,dtd,meson8b_skykirin_m201c_512M' - offset: 0x00068cc8, size: 65574, verify: no
File 'normal,ini,aml_sdc_burn' - offset: 0x00078cf0, size: 624, verify: no
File 'normal,dtb,meson' - offset: 0x00078f60, size: 49152, verify: no
File 'normal,conf,keys' - offset: 0x00084f60, size: 5, verify: no
File 'normal,PARTITION,logo' - offset: 0x00084f68, size: 11567664, verify: no
File 'normal,VERIFY,logo' - offset: 0x00b8d198, size: 48, verify: yes
File 'normal,PARTITION,boot' - offset: 0x00b8d1c8, size: 7953816, verify: no
File 'normal,VERIFY,boot' - offset: 0x01322f60, size: 48, verify: yes
File 'sparse,PARTITION,system' - offset: 0x01322f90, size: 1096306204, verify: no
File 'normal,VERIFY,system' - offset: 0x428a7dac, size: 48, verify: yes
File 'normal,PARTITION,recovery' - offset: 0x428a7ddc, size: 7994426, verify: no
File 'normal,VERIFY,recovery' - offset: 0x43047a18, size: 48, verify: yes
File 'normal,PARTITION,bootloader' - offset: 0x43047a48, size: 376496, verify: no
File 'normal,VERIFY,bootloader' - offset: 0x430a38f8, size: 48, verify: yes
```

### Extracting contents

Without the `-l` flag, the default action is to extract the entire package to its directory. The extracted file names
are in the format:

`<image name>.<file type>.<main type>.<sub type>.img`

For example:

```shell
$ ./unamlimage d:\Dev\tvbox\mistick\_nobackup\MXQ-S805-0720.img
Found version 1 image with 18 file(s)
Extracting 'USB,DDR' to MXQ-S805-0720.img.normal.USB.DDR.img
Extracting 'USB,UBOOT_COMP' to MXQ-S805-0720.img.normal.USB.UBOOT_COMP.img
Extracting 'conf,platform' to MXQ-S805-0720.img.normal.conf.platform.img
Extracting 'dtd,meson8b_m201_1G' to MXQ-S805-0720.img.normal.dtd.meson8b_m201_1G.img
Extracting 'dtd,meson8b_skykirin_m201c_512M' to MXQ-S805-0720.img.normal.dtd.meson8b_skykirin_m201c_512M.img
Extracting 'ini,aml_sdc_burn' to MXQ-S805-0720.img.normal.ini.aml_sdc_burn.img
Extracting 'dtb,meson' to MXQ-S805-0720.img.normal.dtb.meson.img
Extracting 'conf,keys' to MXQ-S805-0720.img.normal.conf.keys.img
Extracting 'PARTITION,logo' to MXQ-S805-0720.img.normal.PARTITION.logo.img
Extracting 'VERIFY,logo' to MXQ-S805-0720.img.normal.VERIFY.logo.img
Extracting 'PARTITION,boot' to MXQ-S805-0720.img.normal.PARTITION.boot.img
Extracting 'VERIFY,boot' to MXQ-S805-0720.img.normal.VERIFY.boot.img
Extracting 'PARTITION,system' to MXQ-S805-0720.img.sparse.PARTITION.system.img
Extracting 'VERIFY,system' to MXQ-S805-0720.img.normal.VERIFY.system.img
Extracting 'PARTITION,recovery' to MXQ-S805-0720.img.normal.PARTITION.recovery.img
Extracting 'VERIFY,recovery' to MXQ-S805-0720.img.normal.VERIFY.recovery.img
Extracting 'PARTITION,bootloader' to MXQ-S805-0720.img.normal.PARTITION.bootloader.img
Extracting 'VERIFY,bootloader' to MXQ-S805-0720.img.normal.VERIFY.bootloader.img
```

## License

```
MIT License

Copyright (c) 2026 Kuba Szczodrzyński

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
