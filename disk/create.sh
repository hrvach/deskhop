
#!/bin/bash

dd if=/dev/zero of=fat.img bs=2M count=1
# sudo mkdosfs -F 16 -R 1 -n DESKHOP fat.img

mkdosfs  -F12 -n DESKHOP -i 0 fat.img

sudo mount -o loop -t vfat fat.img /mnt/disk/
sudo cp ../webconfig/config.htm /mnt/disk/config.htm
sudo umount /mnt/disk

dd if=fat.img of=disk.img bs=512 count=128
rm fat.img
