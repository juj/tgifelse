mergeInto(LibraryManager.library, {
	logResults: function(size, count, time) {
		document.body.innerHTML += 'Num T shapes in a lattice grid of size ' + size + ': ' + count + '. (calculated in ' + time.toFixed(3) + ' msecs)<br>';
	},
	logResultsMt: function(size, count, time, threads) {
		document.body.innerHTML += 'Num T shapes in a lattice grid of size ' + (size-1)
								+ ': ' + count
		                        +  '. (calculated in ' + time.toFixed(3) + ' msecs)<br>';
//		                        +  'Computed using ' + threads + ' parallel threads.';
	},
	readInputSize: function() {
		return parseInt(location.search.substr(1) || '12345');
	}
});
