# Encrypting-File-System
```
1) $ mkdir mount
```
```
2) $ touch image
```
```
3) $ ./mkfs ./image 
```
```
4) $ sudo insmod ./encrypt_fs.ko 
```
```
5) $ sudo mount -o loop -t efs image ./mount
```
```
6) $ sudo umount ./mount
```
```
7) $ sudo /sbin/rmmod encrypt_fs.ko
```
