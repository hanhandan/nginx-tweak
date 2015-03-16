# nginx-tweak #
Tweak nginx for serve stream media files.

# build #

make -f nginx.Makefile

# start nginx #

./nginx.start

# generate media files #

./files.create

## storage ##

total file size: 4TB
size per file: 60MB
total files: 70000
directories: 100
files per directory: 700

# run tests #
