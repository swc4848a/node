// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

var common = require('../common');
var assert = require('assert');
var net = require('net');
var spawn = require('child_process').spawn;

console.log(process.argv);

if(process.argv.length > 2 && process.argv[2] == 'child')
{
	console.log('child');
	
var gotError = false;

console.log('hello');
process.on('exit', function() {
  console.log(gotError);
  assert(gotError instanceof Error);
});

// this should fail with an async EINVAL error, not throw an exception
net.createServer(assert.fail).listen({fd:0}).on('error', function(e) {
  console.log('got somethingggg', e);
  switch(e.code) {
    case 'EINVAL':
    case 'ENOTSOCK':
      gotError = e;
      break
  }
});


setTimeout(function() {}, 5000);

}
else
{
    var child =  spawn(process.execPath, [process.argv[1], 'child'], {stdio: ['pipe', 'inherit', 'inherit']});
	child.on('exit', function(code) {
		process.exit(code);
		});
}
