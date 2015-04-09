Tool for Atheros ART partition
==============================

Dump, remove regulatory limits and fixes the Atheros ART partition with ar9300 layout, compatible with WDR3600, WDR4300, WDR4310 and more.

* *I have to thank some (many) people who provided tips and information on forums that saved weeks of hard work.*


Development status
------------------

* Dump of the ART partition with ar9300 layout that enables the analysis of settings.
* Have removes the regulatory domain specific (puts all the power in 60dB as suggested by source), leaving it to the drive and configuration apply the appropriate regulatory domain.
* Fix the table 'Calibration 5GHz' that is poorly formatted / incomplete (worked correct only between channels 149 and 165) in WDR4310 v1.0 board 2050500271 r1.3. The values are interpolated, but until I have in hand a WDR4300 ART partition with the same board revision is that I can do.


Compiling
---------

* Compile for Linux is only run the command 'make' the project root directory.
* The source codes are prepared to be compiled in Visual Studio.

Take a backup of your ART partition
-----------------------------------

```
		(This command will show you all MTD partitions)

		cat /proc/mtd


		dev:    size   erasesize  name
		mtd0: 00020000 00010000 "u-boot"
		mtd1: 000f1808 00010000 "kernel"
		mtd2: 006de7f8 00010000 "rootfs"
		mtd3: 002e0000 00010000 "rootfs_data"
		mtd4: 00010000 00010000 "art"
		mtd5: 007d0000 00010000 "firmware"


		(To backup ART partition in this example, run)

		cat /dev/mtd4 > /tmp/art.bin
```


How can you contribute to the project
-------------------------------------

* Help identify if your router is compatible with the program. The routers WDR3600, WDR4300 and WDR4310 are compatible, they use the AR9344 and AR9580 chips, routers using other Atheros chips can also use ar9300 layout. It is only possible to identify the compatibility taking a backup of ART partition and run the program. Send the result of the dump or compare with [this result](https://github.com/zeptoZB/ar9300_eeprom/blob/master/dump/wdr4310_v1.0_2050500271_rev1.3_cn.txt). Usually the MAC address of the dump is not used but not ownership claim to be true in your router model, suggest that, in doubt, wipe this field for your security before sending the dump.

* Contributing its ART partition. Help the program to automatically identify your router (model, version, board number e revision). Send your ART partition with the following information:

  * Model and version found in the router label.

	<p align=center><img src="http://www.tp-link.com/Resources/UploadFiles/Snap4.jpg" alt="Router label" width="487" height="161"><br>source: tp-link.com</p>

  * Board number and revision normally found together in one of the board corners.

	<p align=center><img src="http://wiki.openwrt.org/_media/toh/tp-link/20401912k2aaig4c2l9ici.jpg" alt="Board number and revision" width="400" height="300"><br>source: openwrt.org</p>
							
  * Where the router was marketed, as for example a router marketed in the European Union usually has a regulatory domain different from marketed in the USA.

  * Report if you know any abnormal behavior of the your wireless router. May be due to the configuration of the ART partition, if so is likely to be fixed.

* Your contribution as a developer is welcome. The ath9k drive uses other layouts beyond the ar9300, interprets them increases the number of compatible routers.

* Help improve this documentation, your suggestion is welcome, unfortunately my English is not good and the google translate not do miracles.


How everything started
----------------------

I purchased a router WDR4310 (Chinese version of WDR4300) when installed DD-WRT was detected that the 5GHz worked correct only between channels 149 (5745MHz) and 165 (5825MHz). I tried the OpenWrt and problem persisted, with search the web the conclusion was that the ART partition imposed some restriction, but I not found a solution.

The other day I was looking for how to overclock into WDR4310, I found a [great article](http://aspiregemstone.blogspot.com.br/2014/12/overclocking-tp-link-wdr4300.html) that made everything very simple and yet clarified several points, making it much easier in trying to solve the problem of 5GHz restriction.

* In WDR4300 the MAC address is not on ART partition but in the upper u-boot partition (64KB U-Boot + 64KB settings).
* The modified U-Boot allows you to update the ART partition easily and safely.
* ART partition has the calibration of the wireless then change the partition WDR4310 by the WDR4300 should remove the restrictions.
* With the [backup of WDR4300](https://github.com/gwlim/Openwrt_Firmware/tree/master/TP-Link_TL-WDR3500-3600-43XX-WM4350R/TL-WDR4300-BackUp_Image_Only) closed everything needed to test.

The test was successful, the WDR4310 worked properly in 5GHz. But I thought the solution was too simplistic, because the ART partition contain the calibration of the wireless so even though all work good, suspected that the calibration of WDR4310 should be close to WDR4300 but not equal. I decided the one looking in the source code of OpenWrt, everything is there and only find the correct sources, was lucky to find them.


Looking the source code of OpenWrt
----------------------------------

The starting point was 'linux-(version)/arch/mips/ath79/mach-tl-wdr4300.c' that is for routers WDR3600, WDR4300 and WDR4310 that has the same hardware (WDR3600 has the same board but the components were suppressed a chain). Analyzing this source has confirmation that the data is in 0x1000 (the offset in ART partition) to 2,4GHz and the data is in 0x5000 to 5GHz and the MAC address that is in u-boot partition.

The Atheros wireless drive for this router is in 'linux-(version)/drivers/net/wireless/ath/ath9k'. Analyzing the sources, I found that the ART partition data is loaded directly into the struct ar9300_eeprom (definition in the source 'ar9003_eeprom.h'). The starting point for a dump were to ath9k_hw_ar9003_dump_eeprom and ar9003_dump_modal_eeprom functions in source 'ar9003_eeprom.c'. After a long analysis field-by-field, mainly in the source 'ar9003_eeprom.c', have had the first dump version.


Analysis, conclusions and speculations
--------------------------------------

Based on the limited information obtained so far (ART of one WDR4310 and of two WDR3400) the result for now is:
* Most likely, that the calibration data contained in ART partition are not unique per router, but vary according to the revision of the board.
* The data are loaded directly without any CRC consistency facilitating the changes.
* The MAC address is not on the partition ART allowing test and distribute modified versions without bothering to adjust or be touting the MAC address.
* Various fields presented identical in the three ART images, some are relevant to the correct operation of wireless, others are obsolete or not used by the drive. I did not spend much time with them as it is not the focus.
* The fields 'Caps Tuning' (2.4GHz and 5GHz) are to the accuracy of the crystal clock. Different revisions of board normally will have these different parameters. A good indication that change the ART with another router without looking at the revision of the board may be bad idea.
* The tables 'Calibration' (2.4GHz and 5GHz) are to calibrate / compensate asymmetries between the chains and deviations from expected. Has significant variation between board revisions. Confirming that change the ART with another router without looking at the revision of the board is bad idea.
* The tables 'Target 2.4GHz (11B, 11G, HT20, HT40)', 'Target 5GHz (11A, HT20, HT40)' specify the maximum power due to the speed and fequency of the connection. For the analysis of the sources these tables should (and seem) have only the hardware power limitations, but can be used to impose a regulatory domain specific (I hope they are not used to this). These tables are vital to the health of your router and the wireless vicinity, because they limit the hardware to secure powers for the proper functioning. Change these values or even use a drive that ignores them has consequences. Power up the hardware limit can have a similar effect to a stereo at full volume, a lot of noise, a lot of distortion, it works worse than the more moderate powers, not to mention all that noise interferes in the wireless vicinity operation. Overheating generate thermal erosion deteriorates the power stage, further increasing noise and distortion and over time leads to burning of it. The power may be as high (remembering +3dB that is twice the power) the power stage burning occurs in a short time. Now assume that the hardware has protection from excess power, then none of the above is valid, all table values are merely informative, is useless exceed, are fakes values because the hardware limits the power with supposed to. And last, for much power to the router is a few meters and with little obstacle walls.
* The tables 'Regulatory (2.4GHz and 5GHz)' are to impose regulatory domain specific, limiting the maximum power, vary with where routers are marketed.


License
-------

This project is Free Software, licensed under version 2 of the **GNU General Public License**.


Credits
-------

* .
