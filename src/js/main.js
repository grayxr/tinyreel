var CHUNK_SIZE = 1500;
var DOWNLOAD_TIMEOUT = 20000;

function sendBitmap(bitmap){
  var i = 0;
  var nextSize = bitmap.length-i > CHUNK_SIZE ? CHUNK_SIZE : bitmap.length-i;
  var sliced = bitmap.slice(i, i + nextSize);

  MessageQueue.sendAppMessage({"size": bitmap.length});

  var success = function(){
    if(i>=bitmap.length)
      return;
    i += nextSize;
    //console.log(i + "/" + bitmap.length);
    nextSize = bitmap.length-i > CHUNK_SIZE ? CHUNK_SIZE : bitmap.length-i;
    sliced = bitmap.slice(i, i + nextSize);
    MessageQueue.sendAppMessage(
      {
      "index":i,
      "chunk":sliced
      },
      success,
      null
      );
  };

  MessageQueue.sendAppMessage(
      {
      "index":i,
      "chunk":sliced
      },
      success,
      null
      );
}

function convertImage(rgbaPixels, numComponents, width, height){

  var watch_info;
  if(Pebble.getActiveWatchInfo) {
    watch_info = Pebble.getActiveWatchInfo() || { 'platform' : 'aplite'};
  } else {
    watch_info = { 'platform' : 'aplite'};
  }

  var ratio = Math.min(144 / width,168 / height);
  var ratio = Math.min(ratio,1);

  var final_width = Math.floor(width * ratio);
  var final_height = Math.floor(height * ratio);
  var final_pixels = [];
  var bitmap = [];

  if(watch_info.platform === 'aplite') {
    var grey_pixels = greyScale(rgbaPixels, width, height, numComponents);
    ScaleRect(final_pixels, grey_pixels, width, height, final_width, final_height, 1);
    floydSteinberg(final_pixels, final_width, final_height, pebble_nearest_color_to_black_white);
    bitmap = toPBI(final_pixels, final_width, final_height);
  }
  else {
    ScaleRect(final_pixels, rgbaPixels, width, height, final_width, final_height, numComponents);
    floydSteinberg(final_pixels, final_width, final_height, pebble_nearest_color_to_pebble_palette);
    var png = generatePngForPebble(final_width, final_height, final_pixels);
    for(var i=0; i<png.length; i++){
      bitmap.push(png.charCodeAt(i));
    }
  }

  return bitmap;
}

function getPbiImage(url){
  var xhr = new XMLHttpRequest();
  xhr.open("GET", url, true);
  xhr.responseType = "arraybuffer";
  xhr.onload = function() {
    if(xhr.status == 200 && xhr.response) {
      clearTimeout(xhrTimeout); // got response, no more need in timeout
      var data = new Uint8Array(xhr.response);
      var bitmap = [];
      for(var i=0; i<data.byteLength; i++) {
        bitmap.push(data[i]);
      }
      sendBitmap(bitmap);
    }
  };

  var xhrTimeout = setTimeout(function() {
    MessageQueue.sendAppMessage({"message":"Error : Timeout"}, null, null);
  }, DOWNLOAD_TIMEOUT);

  xhr.send(null);
}

function getGifImage(url){
  var xhr = new XMLHttpRequest();
  xhr.open("GET", url, true);
  xhr.responseType = "arraybuffer";
  xhr.onload = function() {
    clearTimeout(xhrTimeout); // got response, no more need in timeout

    //MessageQueue.sendAppMessage({"message":"Decoding image..."}, null, null);

    var data = new Uint8Array(xhr.response || xhr.mozResponseArrayBuffer);
    var gr = new GifReader(data);
    //console.log("Gif size : "+ gr.width  +" " + gr.height);

    var pixels = [];
    gr.decodeAndBlitFrameRGBA(0, pixels);

    var bitmap = convertImage(pixels, 4, gr.width, gr.height);

    sendBitmap(bitmap);
  };

  var xhrTimeout = setTimeout(function() {
    MessageQueue.sendAppMessage({"message":"Error : Timeout"}, null, null);
  }, DOWNLOAD_TIMEOUT);

  xhr.send(null);
}

function getJpegImage(url){
  var j = new JpegImage();
  j.onload = function() {
    clearTimeout(xhrTimeout); // got response, no more need in timeout

    //MessageQueue.sendAppMessage({"message":"Decoding image..."}, null, null);

    //console.log("Jpeg size : " + j.width + "x" + j.height);

    var pixels = j.getData(j.width, j.height);

    var bitmap = convertImage(pixels, 3, j.width, j.height);

    sendBitmap(bitmap);
  };

  var xhrTimeout = setTimeout(function() {
    MessageQueue.sendAppMessage({"message":"Error : Timeout"}, null, null);
  }, DOWNLOAD_TIMEOUT);

  try{
    j.load(url);
  }catch(e){
    //console.log("Error : " + e);
  }
}

function getPngImage(url){
  var xhr = new XMLHttpRequest();
  xhr.open("GET", url, true);
  xhr.responseType = "arraybuffer";
  xhr.onload = function() {
    clearTimeout(xhrTimeout); // got response, no more need in timeout

    //MessageQueue.sendAppMessage({"message":"Decoding image..."}, null, null);

    var data = new Uint8Array(xhr.response || xhr.mozResponseArrayBuffer);

    var png     = new PNG(data);
    var width   = png.width;
    var height  = png.height;
    var palette = png.palette;
    var pixels  = png.decodePixels();
    var bitmap  = [];

    if(palette.length > 0){
      var png_arr = [];
      for(var i=0; i<pixels.length; i++) {
        png_arr.push(palette[3*pixels[i]+0] & 0xFF);
        png_arr.push(palette[3*pixels[i]+1] & 0xFF);
        png_arr.push(palette[3*pixels[i]+2] & 0xFF);
      }
      bitmap = convertImage(png_arr, 3, width, height);
    }
    else {
      var components = pixels.length /( width*height);
      bitmap = convertImage(pixels, components, width, height);
    }

    sendBitmap(bitmap);
  };

  var xhrTimeout = setTimeout(function() {
    MessageQueue.sendAppMessage({"message":"Error : Timeout"}, null, null);
  }, DOWNLOAD_TIMEOUT);

  xhr.send(null);
}

function endsWith(str, suffix) {
    return str.indexOf(suffix, str.length - suffix.length) !== -1;
}

// Setup Reel

var Reel = {
    // local
    // uri: 'http://cd7e7ca0.ngrok.io',
    // prod
    uri: 'http://jamesgraydev.com/',
    entries: [],
    entryPos: 0,
    token: '',
    currentEntries: {},

    init: function() {
        //localStorage.removeItem('token');
        Reel.token = localStorage.getItem('token') || '';
		Reel.refresh();
    },

    refresh: function() {
        //if (!Reel.token) return Reel.auth(Reel.refresh);
        if (!Reel.token) return MessageQueue.sendAppMessage({ "error":"GOTTA LOGIN!" }, null, null);
        //console.log("refresh: token exists");
		Reel.getEntry();
    },

    error: function(error) {
        MessageQueue.sendAppMessage({ "error":""+error }, null, null);
    },

    auth: function(cb) {
		Reel.api.auth(function(xhr) {
			var res = JSON.parse(xhr.responseText);
			if (res.token) {
				Reel.token = res.token;
				localStorage.setItem('token', Reel.token);
				if (typeof(cb) === 'function') cb();
			}
		});
	},

    getImage: function(url) {
        //console.log("Image URL : "+ url);
        //MessageQueue.sendAppMessage({"message":"Downloading image..."}, null, null);

        if(endsWith(url, ".pbi")){
            getPbiImage(url);
        }
        else if(endsWith(url, ".gif") || endsWith(url, ".GIF")){
            getGifImage(url);
        }
        else if(endsWith(url, ".jpg") || endsWith(url, ".jpeg") || endsWith(url, ".JPG") || endsWith(url, ".JPEG")){
            getJpegImage(url);
        }
        else if(endsWith(url, ".png") || endsWith(url, ".PNG")){
            getPngImage(url);
        }
        else {
            getJpegImage(url);
        }
	},

    getUsername: function(username) {
        MessageQueue.sendAppMessage({ "username": username }, null, null);
	},

    getEntries: function() {
		Reel.api.entries(function(xhr) {
			var data = JSON.parse(xhr.responseText);
			if (!data.length) return Reel.error('FEED ISSUES :(');
			Reel.entries = data;
			//console.log('entries count: ' + Reel.entries.length);
			Reel.getEntry();
		});
	},

    getEntry: function(direction) {
        //make sure we have the entries
		if (!Reel.entries.length) return Reel.getEntries();
        switch(direction){
            case "next":
                //go forward through feed
                if(Reel.entryPos == (Reel.entries.length-1)) {
                    // TO DO: if we've reached the end, then fetch 30 more
                    // for now, just go back to the start
                    Reel.entryPos = 0;
                    Reel.currentEntry = Reel.entries[Reel.entryPos];
                } else {
                    Reel.entryPos++;
                    Reel.currentEntry = Reel.entries[Reel.entryPos];
                }
                break;
            case "previous":
                //go backward through feed
                if(Reel.entryPos == 0) {
                    Reel.entryPos = Reel.entries.length-1;
                    Reel.currentEntry = Reel.entries[Reel.entryPos];
                } else {
                    Reel.entryPos--;
                    Reel.currentEntry = Reel.entries[Reel.entryPos];
                }
                break;
            default:
                //get first entry
                Reel.currentEntry = Reel.entries[Reel.entryPos];
        }

        //console.log("Entry Pos: "+Reel.entryPos, "Entries Amount: "+Reel.entries.length);

		var entry = {};
		entry.username = Reel.currentEntry.username || '';
		entry.url = Reel.currentEntry.url;

		Reel.getImage(entry.url);
        Reel.getUsername(entry.username);
	},

    getCurrentEntryDetails: function() {
        Reel.currentEntry = Reel.entries[Reel.entryPos];
        var entry = {};
		entry.likes = Reel.currentEntry.likes || 0;
		entry.comments = Reel.currentEntry.comments || 0;
		entry.caption = Reel.currentEntry.caption || '';

        //console.log("likes: "+entry.likes, "comms: "+entry.comments);
        MessageQueue.sendAppMessage({ "likes": entry.likes+"" }, null, null);
        MessageQueue.sendAppMessage({ "comments": entry.comments+"" }, null, null);
        MessageQueue.sendAppMessage({ "caption": entry.caption }, null, null);
    },

    api: {
		auth: function(cb, fb) {
			if (!Reel.token) {
				return Reel.error('GOTTA LOGIN!');
			}
			//Reel.api.makeRequest('POST', '/auth', JSON.stringify({token:Reel.token}), cb, fb);
		},

		entries: function(cb, fb) {
			Reel.api.makeRequest('GET', '/entries', null, cb, fb);
		},

		makeRequest: function(method, endpoint, data, cb, fb) {
			if (!Reel.token && endpoint == '/auth') return Reel.error('GOTTA LOGIN!');
			var url = Reel.uri + endpoint;
			if (method == 'GET' && data) {
				url += '?' + data;
				data = null;
			}
			//console.log(method + ' ' + url + ' ' + data);
			var xhr = new XMLHttpRequest();
			xhr.setRequestHeader('X-Auth-Token', Reel.token);
			xhr.setRequestHeader('Content-Type', 'application/json');
			xhr.open(method, url, true);
			xhr.onload = function() {
				if (xhr.status >= 300) {
					if (endpoint == '/auth') {
                        //console.log('error, login');
                        localStorage.removeItem('token');
						return Reel.error('GOTTA LOGIN!');
                    } else if(endpoint == '/entries') {
                        return Reel.error('FEED ISSUES :(');
					} else {
                        return Reel.error('WHAT GIVES?');
                    }
				}
				if (typeof(cb) === 'function') cb(xhr);
			};
			xhr.onerror = function() { if (typeof(fb) === 'function') fb('HTTP error!'); };
			xhr.ontimeout = function() { if (typeof(fb) === 'function') fb('Timed out!'); };
			xhr.timeout = 30000;
			xhr.send(data);
		}
	},

    handleAppMessage: function(e) {
        //console.log('AppMessage received: ' + JSON.stringify(e.payload));
		if (!e.payload) return;
        var reel_method = e.payload;
        if(reel_method.hasOwnProperty("chunk")){
            //console.log("is chunk data");
			//Reel.getEntry();
        }else if(reel_method.hasOwnProperty("next")){
            //console.log("is next btn");
			Reel.getEntry("next");
        }else if(reel_method.hasOwnProperty("previous")){
            //console.log("is previous btn");
			Reel.getEntry("previous");
        }else if(reel_method.hasOwnProperty("details")){
            //console.log("is select/details btn");
			Reel.getCurrentEntryDetails();
        }
	},

    showConfiguration: function() {
		Pebble.openURL(Reel.uri);
        //test
        //localStorage.removeItem('token');
	},

    handleConfiguration: function(e) {
		//console.log('configuration received: ' + JSON.stringify(e));
		if (!e.response) return;
		if (e.response === 'CANCELLED') return;
		var data = JSON.parse(decodeURIComponent(e.response));
		if (data.token) {
            if(!Reel.token) MessageQueue.sendAppMessage({ "message": "OK! LOADING..." }, null, null);
			Reel.token = data.token;
			localStorage.setItem('token', Reel.token);
			//Reel.auth(Reel.refresh);
            Reel.refresh();
		}
	}
};

Pebble.addEventListener("ready", Reel.init);
Pebble.addEventListener("appmessage", Reel.handleAppMessage);
Pebble.addEventListener('showConfiguration', Reel.showConfiguration);
Pebble.addEventListener('webviewclosed', Reel.handleConfiguration);
