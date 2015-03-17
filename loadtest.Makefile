all: loadtest/node_modules/.bin/loadtest

loadtest/node_modules/.bin/loadtest:
	cd loadtest &&\
	npm install &&\
	wget https://raw.githubusercontent.com/tangxinfa/loadtest/master/bin/loadtest.js -O node_modules/loadtest/bin/loadtest.js
