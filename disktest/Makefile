all: disktest

disktest: *.c
	gcc -D__STDC_FORMAT_MACROS -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -g -o disktest *.c -lpthread
