# Encrypting-File-System
```
1) $ mkdir mount
```
```
2) $ touch image
```
```
3) $ sudo insmod ./encrypt_fs.ko 
```
```
4) $ sudo mount -o loop -t efs image ./mount
```
```
5) $ sudo umount ./mount
```
```
6) $ sudo /sbin/rmmod encrypt_fs.ko
```
