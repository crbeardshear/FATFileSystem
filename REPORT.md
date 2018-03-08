## Phase 4

#### fs_read()
We tried to think of the most generic solution possible. As such, we organized
the function into a do..while() loop which handles all cases by manipulating the
offsets and read amount. We decided to use a bounce buffer for every iteration,
allowing much simpler logic. However, this causes performance to suffer (because
we copy twice for middle blocks instead of once); if we were optimizing for
performance, we would certainly not use a bounce buffer for the middle blocks.
 
The startoffset and endoffset variables handle where within the block to read
from; intuitively, the read amount must be endoffset - startoffset. The logic
for their values is contained in the comments; we continue reading blocks until
we reach the end of chain or we read up to count bytes. As stated in the project
description, we implicitly increment the offset of the file descriptor after
finishing.

#### fs_write()
This function is very similar to fsread, except the logic within the loop varies
slightly. The first and last block must have the data outside of the write
location preserved. We implemented this with a bounce buffer, which we read the
block into and write only what is required. At the end of the loop, if the write
extends past the file's last data block, we extend the file with the file_extend
function.

The file_extend function extends the data blocks of a file, updating the FAT's
chain to contain the newest data blocks. It returns the amount of data blocks
allocated, up to the request amount.

Because files created in fscreate do not have data blocks allocated, fswrite
must allocate a first data block for these files (which is checked at the
beginning of the function). If the file system runs out of data blocks and a
file attempts a write, fswrite simply returns 0 bytes as the write amount.

## Testing
We created a test script from the professor's instructions in the discussion
section. The script creates two separate file system disks, one for the
reference program and one for our library's program; the script runs the same
commands on each respective disk, then compares the outputs.

We targeted edge cases for all of the available commands in the given program
test_fs.c. This includes 'add'ing a file which does not fit in the data blocks,
adding a file when the file system is full, reading an empty file, adding a file
which already exists, and many others. The test cases are commented in the test
script. We realized that the implementation of testfs.c does not test the
library's lseek function, so we wrote testlseek.c which tests lseek and error
cases not handled by testfs.c.
