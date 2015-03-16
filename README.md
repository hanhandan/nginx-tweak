# nginx-tweak #
Tweak nginx for serve stream media files.

# build #

make -f nginx.Makefile

# start nginx #

./nginx.start

# generate media files #

./files.create

## storage ##

disks: 10
total file size: 4TB
size per file: 60MB
total files: 70000
files per disk: 7000

# run tests #
