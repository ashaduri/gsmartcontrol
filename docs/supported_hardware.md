---
title: "Supported Hardware"
permalink: /supported-hardware
---

# Supported Hardware

GSmartControl supports a wide range of hard disk and solid-state drives:
- ATA (both SATA and PATA) drives
- NVMe drives
- ATA and NVMe drives behind some USB bridges. See
[USB Devices and Smartmontools](https://www.smartmontools.org/wiki/USB)
for more information.
- ATA drives behind some RAID controllers:
  - Adaptec (Linux, selected models only)
  - Areca (Linux, Windows)
  - HP CCISS (Linux)
  - HP hpsa / hpahcisr (Linux)
  - Intel Matrix Storage (CSMI) (Linux, Windows, FreeBSD)
  - LSI 3ware (Linux, Windows)
  - LSI MegaRAID (Windows)

**Note:** Smartmontools supports even
[more RAID Controllers](https://www.smartmontools.org/wiki/Supported_RAID-Controllers).
The drives behind such controllers can be manually entered in GSmartControl using
<tt>Add Device...</tt> functionality or `--add-device` command-line option.

SCSI / SAS drives are **not** supported by GSmartControl, but the application
may still display some information about them.
