Ongoing notes for the developement that's not related to code
You have to enable I2C on the pi. At elast on mine, it was done by
1. `sudo nano /boot/firmware/config.txt`
2. uncomment and set to on the `dtparam=i2c_arm=off` line in `/boot/firmware/config.txt` and then `sudo reboot`
3. It's good to install `i2c-tools`. Then, you can use i2cdetect -y 1 (or 2 for a different bus, but you should not need that)

We might have to think about having better clock on the pi. Apparently as easy as
# Install rpi-update if not present
sudo apt install rpi-update

# Get the RT kernel from the 'next' branch
sudo BRANCH=next rpi-update

# Reboot
sudo reboot

# Verify — should show "-rt+" in the kernel string
uname -r

Then do a 
sudo chrt -f 80 ./bin/Linux/Mara_MaraRPiIP
