#include "efs.h"

static inline struct efs_super_block *EFS_SB(struct super_block *sb){
        return sb->s_fs_info;
}

static inline struct efs_inode *EFS_INODE(struct inode *inode){
        return inode->i_private;
}


