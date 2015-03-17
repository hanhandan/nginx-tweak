var url = require('url');

var total_size = 500*1024*1024*1024;
var size_per_file = 60*1024*1024;
var directories = 100;
var files= Math.floor(total_size/size_per_file);

var request_range_size = 32*1024;
var ranges_per_file = Math.ceil(size_per_file/request_range_size);
var root_url;


function randomRange(options) {
    if (! root_url) {
        root_url = options.href;
    }

    var file_id = Math.floor(Math.random()*files + 1);
    var directory_id = Math.floor(file_id%directories);
    var request_url = url.parse(root_url + '/' + directory_id + '/' + file_id);
    for (var name in request_url) {
        if (typeof(request_url[name]) != 'function') {
            options[name] = request_url[name];
        }
    }

    var range_id = Math.floor(Math.random()*ranges_per_file);
    options.headers.Range = 'bytes=' + (range_id*request_range_size) + '-' + ((range_id+1)*request_range_size - 1);
}

module.exports = function(params, options, client, callback) {
    randomRange(options);

    request = client(options, callback);
    if (params.timeout) {
        request.setTimeout(params.timeout, function() {
            callback('Connection timed out');
        });
    }

    request.on('error', function(error) {
        callback('Connection error: ' + error.message);
    });

    request.end();
};
