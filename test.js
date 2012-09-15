var addon = require('./build/Release/addon');

console.log( 'This should be eight:', addon.add(3,5) );

addon.runCallback(function(msg){
  console.log(msg); // 'hello world'
});
