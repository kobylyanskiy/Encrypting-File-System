#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include "module.h"
#include <linux/buffer_head.h>

static int efs_iterate(struct file *filp, struct dir_context *ctx) { 
	return 0; 
} 
 
const struct file_operations efs_dir_operations = { 
  .owner = THIS_MODULE, 
  .iterate = efs_iterate, 
}; 
 
struct dentry *efs_lookup(struct inode *parent_inode, 
             struct dentry *child_dentry, unsigned int flags) { 
  return NULL; 
} 
 
static struct inode_operations efs_inode_ops = { 
  .lookup = efs_lookup, 
}; 

static void efs_put_super(struct super_block *sb){
	printk(KERN_ALERT "efs super block have been destroyed\n");
}

static struct super_operations const efs_super_ops = {
	.put_super = efs_put_super,
};

int efs_fill_super(struct super_block *sb, void *data, int silent){
	
	struct inode *root = NULL;

	struct buffer_head *bh; 
	struct efs_super_block *sb_disk; 
	
	bh = (struct buffer_head *)sb_bread(sb, 0); 
	
	sb_disk = (struct efs_super_block *)bh->b_data; 
	
	printk(KERN_INFO "The magic number obtained in disk is: [%d]\n", sb_disk->magic); 
	
	if (unlikely(sb_disk->magic != EFS_MAGIC_NUMBER)) { 
	  printk(KERN_ERR 
	         "The filesystem that you try to mount is not of type efs. Magicnumber mismatch."); 
	  return -EPERM; 
	} 
	
	if (unlikely(sb_disk->block_size != EFS_DEFAULT_BLOCK_SIZE)) { 
	  printk(KERN_ERR "efs seem to be formatted using a non-standard block size."); 
	  return -EPERM; 
	} 
	
	printk(KERN_INFO 
		"efs filesystem of version [%d] formatted with a block size of [%d] detected in the device.\n", 
		sb_disk->version, sb_disk->block_size); 

	sb->s_magic = EFS_MAGIC_NUMBER;		        
	sb->s_op = &efs_super_ops;

	root = new_inode(sb);

	root->i_op = &efs_inode_ops; 
  	root->i_fop = &efs_dir_operations; 

	if (!root){
		printk(KERN_ERR "Inode allocation failed\n");
		return -ENOMEM;
	}

	root->i_ino = 0;
	root->i_sb = sb;

	root->i_atime = root->i_mtime = root->i_ctime = CURRENT_TIME;

	inode_init_owner(root, NULL, S_IFDIR);
	sb->s_root = d_make_root(root);

	if (!sb->s_root){
		printk(KERN_ERR "Root creation failed\n");
		return -ENOMEM;
	}
	
	return 0;
}

static struct dentry *efs_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data) {

	struct dentry *ret;
	ret = mount_bdev(fs_type, flags, dev_name, data, efs_fill_super);

	if (IS_ERR(ret))
		printk(KERN_ALERT "Mounting failed\n");
	else
		printk(KERN_ALERT "Mounting filesystem on [%s]\n", dev_name);
	
	return ret; 
}

struct file_system_type efs = {
 	.owner = THIS_MODULE,
	.name = "efs",
	.mount = efs_mount,
	.kill_sb = kill_block_super
};

static int efs_init(void){
	int ret;
	ret = register_filesystem(&efs);
	
	if(likely(!ret))
		printk(KERN_ALERT "Registering filesystem name\n");
	else
		printk(KERN_ERR "Failed with refistering filesystem\n");
	
	return 0;
}

static void efs_exit(void){
	int ret;
	ret = unregister_filesystem(&efs);
	if(unlikely(!ret))
		printk(KERN_ALERT "Unregistering filesystem\n");
}

module_init(efs_init);
module_exit(efs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kirill Kobylyanskiy");
MODULE_DESCRIPTION("Encrypting filesystem");
