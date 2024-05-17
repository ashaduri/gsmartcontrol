---
title: "Supported Hardware"
permalink: /supported-hardware
---

# Supported Hardware

GSmartControl supports ATA drives (both PATA and SATA), NVMe drives,
various USB to ATA bridges, and drives behind some RAID controllers.

See [Smartmontools USB Device Support](https://www.smartmontools.org/wiki/Supported_USB-Devices)
page for an (incomplete) list of supported USB to ATA bridges.

GSmartControl supports the following RAID controllers:
- Adaptec (Linux, selected models only)
- Areca (Linux, Windows)
- HP CCISS (Linux)
- HP hpsa / hpahcisr (Linux)
- Intel Matrix Storage (CSMI) (Linux, Windows, FreeBSD)
- LSI 3ware (Linux, Windows)
- LSI MegaRAID (Windows)

Note: Smartmontools supports even
[more RAID Controllers](https://www.smartmontools.org/wiki/Supported_RAID-Controllers).
The drives behind such controllers can be manually entered in GSmartControl using
<tt>Add Device...</tt> functionality or `--add-device` command-line option.
