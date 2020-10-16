[![noswpatv3](http://zoobab.wdfiles.com/local--files/start/noupcv3.jpg)](https://ffii.org/donate-now-to-save-europe-from-software-patents-says-ffii/)
[![noswpatv3](http://zoobab.wdfiles.com/local--files/start/noupcv3.jpg)](https://ffii.org/donate-now-to-save-europe-from-software-patents-says-ffii/)
# About

Mkfirm is producing firmwares for the BRN (Broad Net Technology) bootloader, such as the one present on the BBOX1, based on a SOC TI AR7200.

# Usage

```
zoobab@sabayonx86-64 /home/zoobab/soft/brn [122]$ ./mkfirm 
Usage: ./mkfirm [-h|-?]
  or:  ./mkfirm [-o <outfile>] [-l <blocksize>] -m <magic> <zipfile>
  or:  ./mkfirm [-o <outfile>] [-l <blocksize>] -m <magic> <pfsfile> <sohofile>
<magic>:     any of the following
        BRNABR    SMC7004ABR V2
        BRN???    SMC7004VBR
        BRNAW     SMC7004VWBR
        BRN???    SMC7004FW
        BRN???    SMC7004WFW
        BRN2804W  SMC2804WBR V1
        BRN6104V2 NorthQ9100
        BRN154BAS Sinus 154 DSL Basic SE
        BRNDTBAS3 Sinus 154 DSL Basic 3
        BRN154DSL Sinus 154 DSL
        BRN154KOM Sinus 154 DSL Komfort
<zipfile>:   zipped file with code or user interface
<pfsfile>:   zipped file with user interface (pfs.img)
<sohofile>:  zipped file with code (soho.bin)
<outfile>:   write result into this file
<blocksize>: size of flash blocks (default: 65536)
```

# Links

* http://www.zoobab.com/bbox1
* https://github.com/rvalles/brntool
* https://wiki.openwrt.org/doc/techref/bootloader/brnboot
* http://hri.sourceforge.net/hw/northq9100/bootloader.html
* https://en.wikipedia.org/wiki/TI-AR7
* https://wikidevi.com/wiki/Arcadyan_ARV7519
* https://plus.google.com/+TobiasDiedrich/posts/RhpfU8SSN2c
