all: ${CURDIR}/sbin/nginx

build/nginx-1.6.2.tar.gz:
	-mkdir build && \
	wget http://nginx.org/download/nginx-1.6.2.tar.gz -O build/nginx-1.6.2.tar.gz

build/nginx-1.6.2: build/nginx-1.6.2.tar.gz
	cd build && \
	tar xzvf nginx-1.6.2.tar.gz

${CURDIR}/sbin/nginx: build/nginx-1.6.2
	cd build/nginx-1.6.2 && \
	./configure --prefix=$(CURDIR)/nginx && \
	make && \
	make install && \
	ln -sf $(CURDIR)/nginx.conf $(CURDIR)/nginx/conf/
