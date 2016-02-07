Pebble.addEventListener('ready', function(e) {
	console.log("JavaScript app ready and running!\n");
});

Pebble.addEventListener('showConfiguration', function(e) {
    console.log("Showing configuration window.\n");
	Pebble.openURL('http://asarotools.com/jogger.html');
});

Pebble.addEventListener('webviewclosed', function(e) {
    console.log("Configuration window returned.");
    console.log(e.response + '\n');
    localStorage.setItem('scale', e.response);
	Pebble.sendAppMessage(JSON.parse(e.response), 
	    function(e) { console.log('Send successful.'); },
		function(e) { console.log('Send failed!');	});
});