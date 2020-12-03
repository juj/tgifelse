mergeInto(LibraryManager.library, {
	logResults: function(size, count, time) {
		document.body.innerHTML += 'Num Asteroid configs of size ' + size + ': ' + count + ' (mod 1e12). (calculated in ' + time.toFixed(3) + ' msecs)<br>';
	},
	logResultsMt: function(size, count, time, threads) {
		document.body.innerHTML += 'Num Asteroid configs of size ' + size + ': ' + count + ' (mod 1e12). (calculated in ' + time.toFixed(3) + ' msecs)<br>';
	},
	readInputSize: function() {
		return parseFloat(location.search.substr(1) || '510510');
	}
});
