#include "typedef.h"
#include "section.h"
#include "fs.h"
#include "ext4.h"

struct file_system ext4_fs _SECTION_(.array.fs) = 
{
    .name               = "ext4",
};

