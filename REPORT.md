## Phase 4

#### fs_read()
We tried to think of the most generic solution possible. The function is structured into a do..while() loop which handles one block per iteration. The function iterates until it reaches the final block of the file, or it runs out of bytes for the read. 
 
The startoffset and endoffset variables handle where within the block to read
from; intuitively, the read amount must be endoffset - startoffset. The logic
for their values is as follows:

##### Start Offset
The start offset denotes where to begin the read from. For the first block, the start offset is equal to the file's offset within the block (file offset % block size). For other blocks, we always start from the beginning, so start offset equals 0.

##### End Offset
The end offset is the part of the block where we cut off the read. It is equal to the block size for all middle blocks. For the last block in the read (The FAT table is FAT_EOC for this block, or the bytes remaining to be read are less than the block size), the function checks if the file has enough bytes to read. If it does, we read the amount of bytes remaining in count. Else, we read to the end of the file.

##### Bounce Buffer
The bounce buffer is only necessary for the first and last blocks being read, because the reads in the middle blocks are always the entirety of the block.

The function continues reading blocks until it reaches the end of chain or it reads up to the parameter count bytes. After finishing, the function increments the offset of the file descriptor by the amount of bytes which were read.

##### Total
After setting all of the above variables, the function reads the block from start offset to end offset. To prepare for the next block, the function decreases the remaining bytes by the amount of bytes read and increases the location of the buffer by the amount of bytes read. The function proceeds to the next block in the FAT table's chain. In this way, the loop handles all of the functionality by adjusting which parts of the block should be read according to the current read.

#### fs_write()
This function is very similar to fsread, except the logic within the loop varies
slightly. The first and last block must have the data outside of the write
location preserved; this also requires a bounce buffer, which we read the
block into and write from the file's offset. At the end of the loop, if the write
extends past the file's last data block, we extend the file with the file_extend
function.

##### file_extend()
The file_extend function extends the data blocks of a file, updating the FAT's
chain to contain the newest data blocks. It returns the amount of data blocks
allocated, up to the request amount. Note that if no more data blocks are available, this function will cease attempting to add more blocks and return how many blocks were added.

After returning the number of extended blocks from the file extend function, fswrite checks if the file has enough data blocks to complete the write. If it does not, fswrite updates the bytes remaining to fill up the extended blocks.

##### Initialization
Because files created in fscreate do not have data blocks allocated, the first fswrite call must allocate a first data block for these files (which is checked at the beginning of the function). If the file system runs out of data blocks and a file attempts a write, fswrite simply returns 0 bytes as the write amount.

#### Edge Cases
##### fs_read():

1: File offset is not a multiple of block size (only a problem in the first block)
2: Read exceeds the the file size (read to end of file)
3: Read ends at less than the entire block
4: Generic read (middle blocks, read entire block)

##### fs_write():

1: File needs to be initialized
2: File descriptor is out of bounds
3: File descriptor is not open
4: Write exceeds the file size (file must be extended)
5: Disk runs out of space for write (write as much as possible)
6: File offset is not a multiple of block size
7: Write ends at less than the entire block
8: Generic write (middle blocks, write entire block)

By handling these edge cases, we guarantee the functionality specified by the API.

## Testing
We created a test script from the professor's instructions in the discussion
section. The script creates two separate file system disks, one for the
reference program and one for our library's program; the script runs the same
commands on each respective disk, then compares the outputs.

We targeted edge cases for all of the available commands in the given program
test_fs.c:

##### add
We tested 'add' with files in different situations:

A file which fits in one block
A file which is empty
A file which is longer than the available size in the file system
A file perfectly fitted to the file system
A file when the file system is full
A file when there is only one data block (should fail, need at least two blocks to have a file)

In each of these cases, the reference file system defines different behavior and thus we must have separate test cases for each.

##### rm
We remove files which were added to ensure the function works as intended. We also tested removing empty files, and removing a file which does not exist.

##### info
We run info at the beginning and at points where we allocate or deallocate large files.

##### ls
We run ls at intermittent points in the tester to ensure that the files which are added/deleted are correctly changed.

##### stat
We run stat on each of the files which are added above, to ensure that their statistics align with the reference program.

##### cat
We run cat to ensure it works, and also on an empty file. We made sure to test the output when 'cat'ing a file which was added when the file system had no more available room.


#### Additional Tests
We realized that the implementation of testfs.c does not test the
library's lseek function, so we wrote testlseek.c which tests lseek and error
cases not handled by testfs.c.
