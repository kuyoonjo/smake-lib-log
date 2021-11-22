const { LLVM } = require('smake');
const { LibLog } = require('./lib');

// const dw = new Conan('libdwarf/20191104', 'arm64-apple-darwin');

// const test = new LLVM('test', 'x86_64-linux-gnu');
const test = new LLVM('test', 'aarch64-linux-gnu');
// const test = new LLVM('test', 'arm64-apple-darwin');
// const test = new LLVM('test', 'x86_64-pc-windows-msvc');
test.files = ['test.cc'];
LibLog.config(test);
test.stdcxx = 'c++20';
test.useClangHeaders = true;
if (test.platform === 'linux') {
  test.libs = [
    ...test.libs,
    'pthread'
  ];
  test.ldflags = [
    ...test.ldflags,
    '-static-libstdc++',
  ];
}

module.exports = [test];
