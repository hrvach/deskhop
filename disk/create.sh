
#!/bin/bash

dd if=/dev/zero of=fat.img bs=2M count=1
<<<<<<< HEAD
# sudo mkdosfs -F 16 -R 1 -n DESKHOP fat.img

mkdosfs  -F12 -n DESKHOP -i 0 fat.img

sudo mount -o loop -t vfat fat.img /mnt/disk/
=======

mkdosfs  -F12 -n DESKHOP -i 0 fat.img

sudo mount -o loop,x-mount.mkdir -t vfat fat.img /mnt/disk/
>>>>>>> 7a0e7f31ffc3151dc5618b9b21a0303e9952df4b
sudo cp ../webconfig/config.htm /mnt/disk/config.htm
sudo umount /mnt/disk

dd if=fat.img of=disk.img bs=512 count=128
rm fat.img
