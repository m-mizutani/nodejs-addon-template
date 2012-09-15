var addon = require('./build/Release/addon');

console.log( 'This should be eight:', addon.add(3,5) );

addon.runCallback(function(msg){
  console.log(msg);
});

obj = addon.createObj ("red");
console.log (obj);

ao = new addon.blue ();
console.log (ao.Five ());
console.log (ao.Count ());
console.log (ao.Count ());
console.log (ao.Count ());

toko = new addon.orange ();
toko.set_cb (function (msg) {
	console.log (msg);
    });
toko.run_cb ();