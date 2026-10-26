In this directory you can find ansible playbooks - right now we have 3 files. One inventory file - you can create your own `inventory.ini` - use the example one as guide. 2 others are the actual playbooks - one is supposed to be run over a fresh pi (right after you install the OS, to prepare it), it's called `setup.yml`, and the other one is called `deploy.yml`. The other one you would run every time after you deploy fresh software. Right now they do the following:

`setup.yml`
- enables i2c
- installs some packages :
- - i2c-tools - that's useful for debugging i2c over ssh
- - python3-smbus2 - useful if you want to quickly test some i2c sensors, adds the i2c library as a python package
- - rpi-update - we need that to enable the rt kernel 
- - lsyncd - that's used to replicate the data we will save to a different sd card
- install the rt kernel
- mount the 2nd sd card under a known address - so that we have the paths nicely defined
- create the backup folder for the data products, logs (and in the future camers)
- create the config for lsyncd, so that it will replicate the data. 
- in the end, reboot so that the rt kernel kicks in
*Important thing* - if you have the sd card, define the sd2_uuid. That should mount it and allow the replication. But maybe the mount task should be kicked out? You can apparently get the uuid by running `sudo blkid /dev/sda1`. 


`deploy.yml` - after you copy the MARA executalbe into the mara directory
- gives the binary elevated thread priorities 
- adds the binary to the systemd, so that it resets after a failure, and starts the binary with a very high priority using `/usr/bin/chrt -f 80`
- enables the service - it should now be turned on every time the pi boots up. 
- it also saves the logs to the mara folder. You can also access the system logs by using journalctl, like in the examples below
```bash
# Live tail
journalctl -u mara -f

# Last 200 lines
journalctl -u mara -n 200

# Since last boot
journalctl -u mara -b
```

Here's some example usage of the playbook :
- `ansible-playbook -i Mara/ansible/inventory.ini Mara/ansible/setup.yml`
- `ansible-playbook -i Mara/ansible/inventory.ini Mara/ansible/deploy.yml`
- `ansible-playbook -i inventory.ini setup.yml -e "sd2_uuid=xxxx-xxxx mara_user=fsowa"`

Also, for a very fresh pi - you need an OS first. Use the Raspberry pi imager (with the usb gadget for easy ssh access over usb. But feel free to go over network, especially later in the project, this is just easy for developement). Raspberry Pi Os lite 64 bit, configure ssh, no rpi connect. 
