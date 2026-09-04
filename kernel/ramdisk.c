// Volatile xv6 file system built at boot from the embedded rootfs.tar.
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "stat.h"

#define FSNINODES 200
extern uchar _binary_rootfs_tar_start[];
extern uchar _binary_rootfs_tar_end[];
static struct superblock fs_sb;
static uint next_inode, next_block;

static uint octal(const uchar *p, int n) {
  uint v = 0;
  while(n-- && *p) { if(*p >= '0' && *p <= '7') v = v * 8 + *p - '0'; p++; }
  return v;
}
static struct dinode *dinode(uint n) {
  return (struct dinode *)(RAMDISK + IBLOCK(n, fs_sb) * BSIZE) + n % IPB;
}
static uint newinode(short type) {
  struct dinode *in = dinode(next_inode);
  memset(in, 0, sizeof(*in)); in->type = type; in->nlink = 1;
  return next_inode++;
}
static void append(uint inum, const uchar *src, uint n) {
  struct dinode *in = dinode(inum); uint off = in->size;
  while(n) {
    uint index = off / BSIZE, bn, take = BSIZE - off % BSIZE;
    if(index >= MAXFILE) panic("ramfs: file too large");
    if(off % BSIZE == 0) {
      bn = next_block++; if(bn >= FSSIZE) panic("ramfs: out of blocks");
      if(index < NDIRECT) in->addrs[index] = bn;
      else {
        uint *ind;
        if(index == NDIRECT) {
          in->addrs[NDIRECT] = next_block++;
          if(in->addrs[NDIRECT] >= FSSIZE) panic("ramfs: out of blocks");
          memset((void *)(RAMDISK + in->addrs[NDIRECT] * BSIZE), 0, BSIZE);
        }
        ind = (uint *)(RAMDISK + in->addrs[NDIRECT] * BSIZE);
        ind[index - NDIRECT] = bn;
      }
    }
    if(take > n) take = n;
    bn = index < NDIRECT ? in->addrs[index] :
      ((uint *)(RAMDISK + in->addrs[NDIRECT] * BSIZE))[index - NDIRECT];
    memmove((void *)(RAMDISK + bn * BSIZE + off % BSIZE), src, take);
    off += take; src += take; n -= take;
  }
  in->size = off;
}
static void format_ramfs(void) {
  uint nbitmap = FSSIZE / BPB + 1, ninodeblocks = FSNINODES / IPB + 1;
  uint nmeta = 2 + LOGSIZE + ninodeblocks + nbitmap; struct dirent de;
  memset((void *)RAMDISK, 0, FSSIZE * BSIZE);
  fs_sb.magic = FSMAGIC; fs_sb.size = FSSIZE; fs_sb.nblocks = FSSIZE - nmeta;
  fs_sb.ninodes = FSNINODES; fs_sb.nlog = LOGSIZE; fs_sb.logstart = 2;
  fs_sb.inodestart = 2 + LOGSIZE; fs_sb.bmapstart = fs_sb.inodestart + ninodeblocks;
  memmove((void *)(RAMDISK + BSIZE), &fs_sb, sizeof(fs_sb));
  next_inode = 1; next_block = nmeta; newinode(T_DIR);
  memset(&de, 0, sizeof(de)); de.inum = ROOTINO; safestrcpy(de.name, ".", DIRSIZ); append(ROOTINO, (uchar *)&de, sizeof(de));
  memset(&de, 0, sizeof(de)); de.inum = ROOTINO; safestrcpy(de.name, "..", DIRSIZ); append(ROOTINO, (uchar *)&de, sizeof(de));
}
void ramdiskinit(void) {
  const uchar *p = _binary_rootfs_tar_start, *end = _binary_rootfs_tar_end; struct dirent de;
  format_ramfs();
  while(p + 512 <= end && p[0]) {
    uint size = octal(p + 124, 12); const uchar *data = p + 512, *name = p; char shortname[DIRSIZ]; uint inum;
    if(size == 0 && p[156] == '5') { p += 512; continue; }
    if(name[0]=='u' && name[1]=='s' && name[2]=='e' && name[3]=='r' && name[4]=='/') name += 5;
    safestrcpy(shortname, (char *)name, DIRSIZ);
    if(shortname[0] == '_') memmove(shortname, shortname + 1, strlen(shortname));
    inum = newinode(T_FILE); memset(&de, 0, sizeof(de)); de.inum = inum; safestrcpy(de.name, shortname, DIRSIZ);
    append(ROOTINO, (uchar *)&de, sizeof(de)); append(inum, data, size);
    p += 512 + ((size + 511) / 512) * 512;
  }
  dinode(ROOTINO)->size = ((dinode(ROOTINO)->size + BSIZE - 1) / BSIZE) * BSIZE;
  for(uint b = 0; b < next_block; b++) ((uchar *)(RAMDISK + fs_sb.bmapstart * BSIZE))[b / 8] |= 1 << (b % 8);
}
void ramdiskrw(struct buf *b, int write) {
  if(!holdingsleep(&b->lock)) panic("ramdiskrw: buf not locked");
  if(b->blockno >= FSSIZE) panic("ramdiskrw: blockno too big");
  if(write) memmove((void *)(RAMDISK + b->blockno * BSIZE), b->data, BSIZE);
  else memmove(b->data, (void *)(RAMDISK + b->blockno * BSIZE), BSIZE);
}
