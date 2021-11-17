const { LLVM } = require('smake');
const { LibLog } = require('./lib');

const test = new LLVM('test', 'x86_64-linux-gnu');
test.files = ['test.cc'];
LibLog.config(test);
test.stdcxx = 'c++17';

module.exports = [test];