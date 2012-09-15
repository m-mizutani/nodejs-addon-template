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

mikan = new addon.orange ();
mikan.set_cb (function (msg) {
	console.log (msg);
    });
mikan.run_cb ();

addon.async_task (function (res) {
	console.log ('[1] done: ' + res);
    });
addon.async_task (function (res) {
	console.log ('[2] done: ' + res);
    });
console.log ('start processing');