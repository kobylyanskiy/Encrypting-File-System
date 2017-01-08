#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include "module.h"


static int efs_iterate(struct file *filp, struct dir_context *ctx) { 
  /* ls will list nothing as of now. 
   * Basic skeleton code to make ls (readdir) work for simplefs */ 
  
	/*loff_t pos;
	pos = ctx->pos;
	*/

	return 0; 
} 
 
const struct file_operations efs_dir_operations = { 
  .owner = THIS_MODULE, 
  .iterate = efs_iterate, 
}; 
 
struct dentry *efs_lookup(struct inode *parent_inode, 
             struct dentry *child_dentry, unsigned int flags) { 
  /* The lookup function is used for dentry association. 
   * As of now, we don't deal with dentries in simplefs. 
   * So we will keep this simple for now and revisit later */ 
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
