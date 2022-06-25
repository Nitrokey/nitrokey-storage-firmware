# Firmware update methods

## Regular firmware update

Note: running the firmware update will remove some user data:
   - Password Safe
   - OTP slots (including Nitropad's Secure Start settings)
   - Hidden Volumes

Data expected to remain:
  - Encrypted Volume content
  - all OpenPGP smart card data - keys, data files

To complete the update process please follow our guide at [2]. If you are using Windows or macOS, a GUI tool is available under the name "Nitrokey Update Tool" [1].

For Linux we offer either manual update (as described under [3]), or a guided update using our CLI tool - nitropy [4]. Please make sure the latest Udev rules are installed first [5].
Please download and use the latest firmware file is available at [6] and [8].

After downloading firmware files please follow the verification steps, as in the example:
```
$ wget https://github.com/Nitrokey/nitrokey-storage-firmware/releases/download/V0.57/nitrokey-storage-V0.57-reproducible.hex
$ wget https://github.com/Nitrokey/nitrokey-storage-firmware/releases/download/V0.57/nitrokey-storage-V0.57.hex
$ wget https://github.com/Nitrokey/nitrokey-storage-firmware/releases/download/V0.57/sha256sum
$ wget https://github.com/Nitrokey/nitrokey-storage-firmware/releases/download/V0.57/sha256sum.sig
$ gpg2 --fetch "https://keys.openpgp.org/vks/v1/by-fingerprint/868184069239FF65DE0BCD7DD9BAE35991DE5B22"
$ gpg2 --verify ./sha256sum.sig
$ sha256sum -c ./sha256sum --ignore-missing
```
Actual update commands through pynitrokey, including its installation:
```
# Check the current state of the device
$ lsusb | grep -ie atmel -e nitrokey
# Install pynitrokey
$ python3 -m pip install --user pipx -U
$ python3 -m pipx ensurepath
# Execute actual update
$ pipx run pynitrokey storage update nitrokey-storage-V0.57.hex --experimental
```
The default firmware passphrase is `12345678`.

Example update process using nitropy can be seen at [7].
Please make sure the `dfu-programmer` tool and `libnitrokey` are installed:
- Fedora: `sudo dnf install dfu-programmer libnitrokey`
- Ubuntu: `sudo apt install dfu-programmer libnitrokey3`

Please note, that running the update under Qubes OS might need additional configuration regarding the VM-USB setup.

Alternatively, it should be possible to update the Nitrokey Storage using bootable image, as described here: [9]


## Exiting firmware update mode (experimental)

Aborting the firmware update can result in a non-working device in case the MCU's flash was overwritten with some other data, or the update process was incomplete.
   Due to that it is important to check the flash content with the following command:
```
$ dfu-programmer at32uc3a3256s read --bin --force > nitrokey-storage-memory.bin
```

This command will show, whether the dumped binary image contains the user data:
```
$ nitropy storage user-data check nitrokey-storage-memory.bin
```

With the following we can verify the currently flashed firmware with the local copy:
```
$ nitropy storage user-data compare nitrokey-storage-V0.57.hex nitrokey-storage-memory.bin application
# in case the previous would fail, please try this one with another firmware image:
$ nitropy storage user-data compare nitrokey-storage-V0.57-reproducible.hex nitrokey-storage-memory.bin application
```

If there are any errors reported, it would be best to update to the latest firmware (by choosing either the lossy regular firmware update, or trying the experimental method of transporting the user data in the firmware, as explained in the next point).
Otherwise, if everything seems correct, we can launch the firmware on the device:
```
$ dfu-programmer at32uc3a3256s launch
```
Note: for the older `dfu-programmer` the command is `start`, instead of `launch`.

## Transporting user data between firmware updates (experimental)

Here is an experimental method for transporting the user data between the firmware upgrades. Please follow the steps from the point (2), and then execute:
```
$ nitropy storage user-data merge nitrokey-storage-memory.bin nitrokey-storage-V0.57-reproducible.hex nitrokey-storage-V0.57-with-user-data.hex
$ pipx run pynitrokey storage update nitrokey-storage-V0.57-with-user-data.hex --experimental
```

This method is currently not handled over our help/support channels, and was introduced only lately.
During the tests the OTP slots were transported correctly.


[1]: https://github.com/Nitrokey/nitrokey-update-tool/releases/tag/v0.5
[2]: https://docs.nitrokey.com/storage/windows/firmware-update.html
[3]: https://docs.nitrokey.com/storage/linux/firmware-update.html
[4]: https://docs.nitrokey.com/software/nitropy/index.html
[5]: https://docs.nitrokey.com/software/nitropy/linux/udev.html
[6]: https://github.com/Nitrokey/nitrokey-storage-firmware/releases/download/V0.57/nitrokey-storage-V0.57.hex
[7]: https://asciinema.org/a/ezebrXBdwLOETPyE8axCMvpkb
[8]: https://github.com/Nitrokey/nitrokey-storage-firmware/releases/download/V0.57/nitrokey-storage-V0.57-reproducible.hex
[9]: https://docs.nitrokey.com/storage/windows/firmware-update-manually.html
