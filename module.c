#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/version.h>

#include "module.h"

static DEFINE_MUTEX(efs_sb_lock);
static DEFINE_MUTEX(efs_inodes_mgmt_lock);
static DEFINE_MUTEX(efs_directory_children_update_lock);

void efs_sb_sync(struct super_block *vsb){
        struct buffer_head *bh;
        struct efs_super_block *sb = EFS_SB(vsb);
        bh = (struct buffer_head *)sb_bread(vsb, EFS_SUPERBLOCK_BLOCK_NUMBER);
        bh->b_data = (char *)sb;
        mark_buffer_dirty(bh);
        sync_dirty_buffer(bh);
        brelse(bh);
}

void efs_inode_add(struct super_block *vsb, struct efs_inode *inode){
        struct efs_super_block *sb = EFS_SB(vsb);
        struct buffer_head *bh;
        struct efs_inode *inode_iterator;

        if (mutex_lock_interruptible(&efs_inodes_mgmt_lock)) {
                printk(KERN_ERR "Failed to acquire mutex lock %s +%d\n",
                       __FILE__, __LINE__);
                return;
        }

        bh = (struct buffer_head *)sb_bread(vsb,
                                            EFS_INODESTORE_BLOCK_NUMBER);

        inode_iterator = (struct efs_inode *)bh->b_data;

        if (mutex_lock_interruptible(&efs_sb_lock)) {
                printk(KERN_ERR "Failed to acquire mutex lock %s +%d\n",
                       __FILE__, __LINE__);
                return;
        }

        /* Append the new inode in the end in the inode store */
        inode_iterator += sb->inodes_count;

        memcpy(inode_iterator, inode, sizeof(struct efs_inode));
        sb->inodes_count++;

        mark_buffer_dirty(bh);
        efs_sb_sync(vsb);
        brelse(bh);

        mutex_unlock(&efs_sb_lock);
        mutex_unlock(&efs_inodes_mgmt_lock);
}

int efs_sb_get_a_freeblock(struct super_block *vsb, uint64_t * out){
        struct efs_super_block *sb = EFS_SB(vsb);
        int i;
        int ret = 0;

        if (mutex_lock_interruptible(&efs_sb_lock)) {
                printk(KERN_ERR "Failed to acquire mutex lock %s +%d\n",
                       __FILE__, __LINE__);
                ret = -EINTR;
                goto end;
        }

        for (i = 3; i < EFS_MAX_FILESYSTEM_OBJECTS_SUPPORTED; i++) {
                if (sb->free_blocks & (1 << i)) {
                        break;
                }
        }

 		if (unlikely(i == EFS_MAX_FILESYSTEM_OBJECTS_SUPPORTED)) {
                printk(KERN_ERR "No more free blocks available");
                ret = -ENOSPC;
                goto end;
        }

        *out = i;

        sb->free_blocks &= ~(1 << i);

        efs_sb_sync(vsb);

end:
        mutex_unlock(&efs_sb_lock);
        return ret;
}

static int efs_sb_get_objects_count(struct super_block *vsb, uint64_t * out){
        struct efs_super_block *sb = EFS_SB(vsb);

        if (mutex_lock_interruptible(&efs_inodes_mgmt_lock)) {
                printk(KERN_ERR "Failed to acquire mutex lock %s +%d\n",
                       __FILE__, __LINE__);
                return -EINTR;
        }
        *out = sb->inodes_count;
        mutex_unlock(&efs_inodes_mgmt_lock);

        return 0;
}

static int efs_iterate(struct file *filp, struct dir_context *ctx) {

	loff_t pos;
	struct inode *inode;
	struct super_block *sb;
	struct buffer_head *bh;
	struct efs_inode *efs_inode;
	struct efs_dir_record *record;
	int i;

	pos = ctx->pos;

	inode = filp->f_path.dentry->d_inode;
	sb = inode->i_sb;

	if(pos){
		return 0;
	}	

	efs_inode = EFS_INODE(inode);

	if(unlikely(!S_ISDIR(efs_inode->mode))) {
                printk(KERN_ERR
                       "inode [%llu][%lu] for fs object [%s] not a directory\n",
                       efs_inode->inode_no, inode->i_ino,
                       filp->f_path.dentry->d_name.name);
                return -ENOTDIR;
	}

	bh = (struct buffer_head *)sb_bread(sb, efs_inode->data_block_number);

	record = (struct efs_dir_record *)bh->b_data;
	for (i = 0; i < efs_inode->dir_children_count; i++) {
                dir_emit(ctx, record->filename, EFS_FILENAME_MAXLEN,
                        record->inode_no, DT_UNKNOWN);
                ctx->pos += sizeof(struct efs_dir_record);
                filp->f_pos += sizeof(struct efs_dir_record);
                pos += sizeof(struct efs_dir_record);
                record++;
        }
        brelse(bh);

        return 0;
}


struct efs_inode *efs_get_inode(struct super_block *sb,
                                          uint64_t inode_no){
        
	struct efs_super_block *efs_sb = EFS_SB(sb);
	struct efs_inode *efs_inode = NULL;

	int i;
	struct buffer_head *bh;

	bh = (struct buffer_head *)sb_bread(sb,
                                        EFS_INODESTORE_BLOCK_NUMBER);
	efs_inode = (struct efs_inode *)bh->b_data;

	for (i = 0; i < efs_sb->inodes_count; i++) {
		if (efs_inode->inode_no == inode_no) {
			return efs_inode;
		}
		efs_inode++;
	}
	return NULL;
}


ssize_t efs_read(struct file * filp, char __user * buf, size_t len,
                      loff_t * ppos){
       
	static int done = 0;
	struct efs_inode *inode = EFS_INODE(filp->f_path.dentry->d_inode);
	struct buffer_head *bh;

	char *buffer;
	int nbytes;

	if (done) {
		done = 0;
		return 0;
	}

	if (*ppos >= inode->file_size) {
		return 0;
	}

	bh = (struct buffer_head *)sb_bread(filp->f_path.dentry->d_inode->i_sb,
                                        		inode->data_block_number);

	if (!bh) {
		printk(KERN_ERR "Reading the block number [%llu] failed.",
						inode->data_block_number);
		return 0;
	}

	buffer = (char *)bh->b_data;
	nbytes = min((size_t) inode->file_size, len);

	if (copy_to_user(buf, buffer, nbytes)) {
		brelse(bh);
		printk(KERN_ERR "Error copying file contents to the userspace buffer\n");
		return -EFAULT;
	}

	brelse(bh);

	*ppos += nbytes;

	done = 1;
	return nbytes;
}


ssize_t efs_write(struct file * filp, const char __user * buf, size_t len,
                       loff_t * ppos){
        
	struct inode *inode;
	struct efs_inode *efs_inode;
	struct efs_inode *inode_iterator;
	struct buffer_head *bh;
	struct super_block *sb;

	char *buffer;
	int count;

	inode = filp->f_path.dentry->d_inode;
	efs_inode = EFS_INODE(inode);
    sb = inode->i_sb;

	if (*ppos + len >= EFS_DEFAULT_BLOCK_SIZE) {
		printk(KERN_ERR "File size write will exceed a block");
		return -ENOSPC;
	}

	bh = (struct buffer_head *)sb_bread(filp->f_path.dentry->d_inode->i_sb,
                                            efs_inode->data_block_number);


	if (!bh) {
		printk(KERN_ERR "Reading the block number [%llu] failed.",
                       efs_inode->data_block_number);
		return 0;
	}
       
	buffer = (char *)bh->b_data;
	buffer += *ppos;

	if (copy_from_user(buffer, buf, len)) {
		brelse(bh);
		printk(KERN_ERR
                       "Error copying file contents from the userspace buffer to the kernel space\n");
		return -EFAULT;
	}
	*ppos += len;

	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);

	if (mutex_lock_interruptible(&efs_inodes_mgmt_lock)) {
		printk(KERN_ERR "Failed to acquire mutex lock %s +%d\n",
                       __FILE__, __LINE__);
		return -EINTR;
	}
    
	bh = (struct buffer_head *)sb_bread(sb,
                                            EFS_INODESTORE_BLOCK_NUMBER);

	efs_inode->file_size = *ppos;

	inode_iterator = (struct efs_inode *)bh->b_data;

	if (mutex_lock_interruptible(&efs_sb_lock)) {
		printk(KERN_ERR "Failed to acquire mutex lock %s +%d\n",
                       __FILE__, __LINE__);
        	return -EINTR;
	}

	count = 0;
	while (inode_iterator->inode_no != efs_inode->inode_no
               && count < EFS_SB(sb)->inodes_count) {
		count++;
		inode_iterator++;
	}

	if (likely(count < EFS_SB(sb)->inodes_count)) {
		inode_iterator->file_size = efs_inode->file_size;
		printk(KERN_INFO
                       "The new filesize that is written is: [%llu] and len was: [%lu]\n",
		efs_inode->file_size, len);

		mark_buffer_dirty(bh);
		sync_dirty_buffer(bh);
	} else {
		printk(KERN_ERR
                       "The new filesize could not be stored to the inode.");
		len = -EIO;
	}

	brelse(bh);

	mutex_unlock(&efs_sb_lock);
	mutex_unlock(&efs_inodes_mgmt_lock);

	return len;
}

const struct file_operations efs_file_operations = {
 	.read = efs_read,
	.write = efs_write,
};

const struct file_operations efs_dir_operations = { 
	.owner = THIS_MODULE, 
	.iterate = efs_iterate, 
}; 

struct dentry *efs_lookup(struct inode *parent_inode,
                               struct dentry *child_dentry, unsigned int flags);

static int efs_create(struct inode *dir, struct dentry *dentry,
                           umode_t mode, bool excl);

static int efs_mkdir(struct inode *dir, struct dentry *dentry,
                          umode_t mode);

static struct inode_operations efs_inode_ops = {
	.create = efs_create,
	.lookup = efs_lookup,
	.mkdir = efs_mkdir, 
}; 


static int efs_create_fs_object(struct inode *dir, struct dentry *dentry,
                                     umode_t mode){
	struct inode *inode;
	struct efs_inode *efs_inode;
	struct efs_inode *inode_iterator;
	struct super_block *sb;
	struct efs_dir_record *record;
	struct efs_inode *parent_dir_inode;
	struct buffer_head *bh;
	struct efs_dir_record *dir_contents_datablock;
	uint64_t count;
	int ret;

	if (mutex_lock_interruptible(&efs_directory_children_update_lock)) {
		printk(KERN_ERR "Failed to acquire mutex lock %s +%d\n",
                       __FILE__, __LINE__);
		return -EINTR;
	}
	sb = dir->i_sb;

	ret = efs_sb_get_objects_count(sb, &count);
	if (ret < 0) {
		mutex_unlock(&efs_directory_children_update_lock);
		return ret;
	}

	if (unlikely(count >= EFS_MAX_FILESYSTEM_OBJECTS_SUPPORTED)) {
		printk(KERN_ERR
                    	"Maximum number of objects supported by efs is already reached");
		mutex_unlock(&efs_directory_children_update_lock);
		return -ENOSPC;
	}

	if (!S_ISDIR(mode) && !S_ISREG(mode)) {
		printk(KERN_ERR
                       "Creation request but for neither a file nor a directory");
		mutex_unlock(&efs_directory_children_update_lock);
		return -EINVAL;
	}

	inode = new_inode(sb);
	if (!inode) {
		mutex_unlock(&efs_directory_children_update_lock);
		return -ENOMEM;
	}

	inode->i_sb = sb;
	inode->i_op = &efs_inode_ops;
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_ino = 10;

	while (efs_get_inode(sb, inode->i_ino)) {
		inode->i_ino++;
	}

	efs_inode = kmalloc(sizeof(struct efs_inode), GFP_KERNEL);
	efs_inode->inode_no = inode->i_ino;
	inode->i_private = efs_inode;
	efs_inode->mode = mode;

	if (S_ISDIR(mode)) {
		printk(KERN_INFO "New directory creation request\n");
		efs_inode->dir_children_count = 0;
		inode->i_fop = &efs_dir_operations;
	} else if (S_ISREG(mode)) {
		printk(KERN_INFO "New file creation request\n");
		efs_inode->file_size = 0;
		inode->i_fop = &efs_file_operations;
	}

	ret = efs_sb_get_a_freeblock(sb, &efs_inode->data_block_number);
	if(ret < 0){
		printk(KERN_ERR "efs could not get a freeblock");
		mutex_unlock(&efs_directory_children_update_lock);
		return ret;
	}

	efs_inode_add(sb, efs_inode);

	record = kmalloc(sizeof(struct efs_dir_record), GFP_KERNEL);
	record->inode_no = efs_inode->inode_no;
	strcpy(record->filename, dentry->d_name.name);

	parent_dir_inode = EFS_INODE(dir);
	bh = sb_bread(sb, parent_dir_inode->data_block_number);
	dir_contents_datablock = (struct efs_dir_record *)bh->b_data;

	dir_contents_datablock += parent_dir_inode->dir_children_count;

	memcpy(dir_contents_datablock, record,
               sizeof(struct efs_dir_record));

	kfree(record);

	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);

	if (mutex_lock_interruptible(&efs_inodes_mgmt_lock)) {
		mutex_unlock(&efs_directory_children_update_lock);
		printk(KERN_ERR "Failed to acquire mutex lock %s +%d\n",
                       __FILE__, __LINE__);
		return -EINTR;
	}

	bh = (struct buffer_head *)sb_bread(sb, EFS_INODESTORE_BLOCK_NUMBER);

	inode_iterator = (struct efs_inode *)bh->b_data;

	if (mutex_lock_interruptible(&efs_sb_lock)) {
		printk(KERN_ERR "Failed to acquire mutex lock %s +%d\n",
                       __FILE__, __LINE__);
		return -EINTR;
	}

	count = 0;
	while (inode_iterator->inode_no != parent_dir_inode->inode_no
               && count < EFS_SB(sb)->inodes_count) {
		count++;
		inode_iterator++;
	}

	if (likely(inode_iterator->inode_no == parent_dir_inode->inode_no)) {
		parent_dir_inode->dir_children_count++;
		inode_iterator->dir_children_count =
		parent_dir_inode->dir_children_count;
		mark_buffer_dirty(bh);
		sync_dirty_buffer(bh);
	} else {
		printk(KERN_ERR
                       "The updated childcount could not be stored to the dir inode.");
	}

	brelse(bh);

	mutex_unlock(&efs_sb_lock);
	mutex_unlock(&efs_inodes_mgmt_lock);
	mutex_unlock(&efs_directory_children_update_lock);

	inode_init_owner(inode, dir, mode);
	d_add(dentry, inode);

	return 0;
}

static int efs_mkdir(struct inode *dir, struct dentry *dentry,
                          umode_t mode){
	return efs_create_fs_object(dir, dentry, S_IFDIR | mode);
}

static int efs_create(struct inode *dir, struct dentry *dentry,
                           umode_t mode, bool excl){
	return efs_create_fs_object(dir, dentry, mode);
}



struct dentry *efs_lookup(struct inode *parent_inode,
                               struct dentry *child_dentry, unsigned int flags){
	struct efs_inode *parent = EFS_INODE(parent_inode);
	struct super_block *sb = parent_inode->i_sb;
	struct buffer_head *bh;
	struct efs_dir_record *record;
	int i;

	bh = (struct buffer_head *)sb_bread(sb, parent->data_block_number);
	record = (struct efs_dir_record *)bh->b_data;
	for (i = 0; i < parent->dir_children_count; i++) {
		if (!strcmp(record->filename, child_dentry->d_name.name)) {

			struct inode *inode;
			struct efs_inode *efs_inode;

			efs_inode = efs_get_inode(sb, record->inode_no);

			inode = new_inode(sb);
			inode->i_ino=record->inode_no;
			inode_init_owner(inode,parent_inode,efs_inode->mode);
			inode->i_sb=sb;
			inode->i_op=&efs_inode_ops;

			if(S_ISDIR(inode->i_mode))
				inode->i_fop=&efs_dir_operations;
				
			else if(S_ISREG(inode->i_mode))
					inode->i_fop = &efs_file_operations;
			else
				printk(KERN_ERR "Unknown inode type. Neither a directory nor a file");

			inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;

			inode->i_private = efs_inode;

			d_add(child_dentry,inode);
			return NULL;
		}
		record++;
	}

	printk(KERN_ERR	"No inode found for the filename [%s]\n",
	child_dentry->d_name.name);

	return NULL;
}

int efs_fill_super(struct super_block *sb, void *data, int silent){
	struct inode *root_inode;
	struct buffer_head *bh;
	struct efs_super_block *sb_disk;

	bh = (struct buffer_head *)sb_bread(sb,
                                            EFS_SUPERBLOCK_BLOCK_NUMBER);

	sb_disk = (struct efs_super_block *)bh->b_data;

	printk(KERN_INFO "The magic number obtained in disk is: [%llu]\n", sb_disk->magic);

	if (unlikely(sb_disk->magic != EFS_MAGIC_NUMBER)) {
		printk(KERN_ERR
                       "The filesystem that you try to mount is not of type efs. Magicnumber mismatch.");
		return -EPERM;
	}

	if (unlikely(sb_disk->block_size != EFS_DEFAULT_BLOCK_SIZE)) {
		printk(KERN_ERR
                       "efs seem to be formatted using a non-standard block size.");
		return -EPERM;
	}

	printk(KERN_INFO
          			"efs filesystem of version [%llu] formatted with a block size of [%llu] detected in the device.\n",
               		sb_disk->version, sb_disk->block_size);

	sb->s_magic = EFS_MAGIC_NUMBER;
	sb->s_fs_info = sb_disk;

	root_inode = new_inode(sb);
	root_inode->i_ino = EFS_ROOTDIR_INODE_NUMBER;
	inode_init_owner(root_inode, NULL, S_IFDIR);
	root_inode->i_sb = sb;
	root_inode->i_op = &efs_inode_ops;
	root_inode->i_fop = &efs_dir_operations;
	root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime = CURRENT_TIME;

	root_inode->i_private = efs_get_inode(sb, EFS_ROOTDIR_INODE_NUMBER);

	sb->s_root = d_make_root(root_inode);
	if (!sb->s_root)
		return -ENOMEM;

	return 0;
}

static struct dentry *efs_mount(struct file_system_type *fs_type,
                                     int flags, const char *dev_name,
                                     void *data)
{
        struct dentry *ret;

        ret = mount_bdev(fs_type, flags, dev_name, data, efs_fill_super);

        if (unlikely(IS_ERR(ret)))
                printk(KERN_ERR "Error mounting efs");
        else
                printk(KERN_INFO "efs is succesfully mounted on [%s]\n",
                       dev_name);
	return ret;
}

static void efs_kill_superblock(struct super_block *s){
	printk(KERN_INFO
               "efs superblock is destroyed. Unmount succesful.\n");
	return;
}

struct file_system_type efs = {
 	.owner = THIS_MODULE,
	.name = "encryptfs",
	.mount = efs_mount,
	.kill_sb = efs_kill_superblock,
};

static int efs_init(void){
	int ret;
	ret = register_filesystem(&efs);
	
	if(likely(!ret))
		printk(KERN_ALERT "Registering filesystem name\n");
	else
		printk(KERN_ERR "Failed with registering filesystem\n");
	
	return ret;
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
