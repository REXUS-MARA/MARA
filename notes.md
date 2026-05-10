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

About F' itself
Definitely needs a better setup on the fprime webpage. One thing is the old image (why????)
The other is the absolutely terrible setup for Macs, while it can actually be very nice
The F prime tools - maybe I'm not using them correctly, but when I add for example new type in one file
And then try to use that type in another file, the tool gets lost, it says the type does not exists. 
The build passes just fine - and then if I reset VSCode it works fine. Closing and opening the file does not help. And 
after I moved the whole file with state machine to a new directory - the fprime tools still point to the old directory.


Also, apparently it's good to modify the exec first with
sudo setcap 'cap_sys_nice=eip' MARA
and then run
./MARA -a 0.0.0.0 -p 50000

When we have data products enabled, the files will eb saved on the SD card (nice). 
We can also download them using the gds - for that, 2 commands need to be sent:
DataProducts.dpCat.BUILD_CATALOG
DataProducts.dpCat.START_XMIT_CATALOG, WAIT
In this order
First one will build the in RAM data catalog, the second one will wait for the first one to finish,
and then send stuff back home