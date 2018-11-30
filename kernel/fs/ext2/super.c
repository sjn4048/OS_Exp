//
// Created by zengk on 2018/11/30.
//

#include "ext2.h"

int sb_fill(struct ext2_super_block *sb, void *data) {
    *sb = *(struct ext2_super_block *) data;
}
