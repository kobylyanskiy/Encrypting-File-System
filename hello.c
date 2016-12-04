#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>


static struct dentry *_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data) {

	struct dentry *ret;
	printk(KERN_ALERT "Mounting filesystem\n");
	ret = NULL;
	return ret; 
}

struct file_system_type efs = {
 	.owner = THIS_MODULE,
	.name = "efs",
	.mount = _mount,
};


static int efs_init(void){
	int ret;
	ret = register_filesystem(&efs);
	printk(KERN_ALERT "Registering filesystem name\n");
	return 0;
}


static void efs_exit(void){
	int ret;
	ret = unregister_filesystem(&efs);
	printk(KERN_ALERT "Uninstalling filesystem\n");
}

module_init(efs_init);
module_exit(efs_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kirill Kobylyanskiy");
MODULE_DESCRIPTION("A Hello, World Module");
