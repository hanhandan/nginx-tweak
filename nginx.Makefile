all: ${CURDIR}/sbin/nginx

build/nginx-1.6.2.tar.gz:
	-mkdir build && \
	wget http://nginx.org/download/nginx-1.6.2.tar.gz -O build/nginx-1.6.2.tar.gz

build/nginx-1.6.2: build/nginx-1.6.2.tar.gz
	cd build && \
	tar xzvf nginx-1.6.2.tar.gz

build/nginx-module-vts:
	cd build &&\
	git clone git://github.com/vozlt/nginx-module-vts.git

${CURDIR}/sbin/nginx: build/nginx-1.6.2 build/nginx-module-vts
	cd build/nginx-1.6.2 && \
	./configure --prefix=$(CURDIR)/nginx --add-module=$(CURDIR)/build/nginx-module-vts && \
	make && \
	make install && \
	ln -sf $(CURDIR)/nginx.conf $(CURDIR)/nginx/conf/
