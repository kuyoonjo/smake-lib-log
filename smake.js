const { LLVM } = require('smake');
const { LibString } = require('@smake/string');
// const { Conan } = require('@smake/cmake');
const { LibLog } = require('./lib');

// const dw = new Conan('libdwarf/20191104', 'arm64-apple-darwin');

// const test = new LLVM('test', 'x86_64-linux-gnu');
const test = new LLVM('test', 'x86_64-pc-windows-msvc');
test.files = ['test.cc'];
LibString.config(test);
LibLog.config(test);
test.stdcxx = 'c++17';
test.useClangHeaders = true;
// test.cxxflags = [...test.cxxflags, '-g -O0'];
// test.includedirs = [
//     ...test.includedirs,
//     ...dw.includeDirs,
// ]
// test.linkdirs = [
//     ...test.linkdirs,
//     ...dw.linkDirs,
// ]
// test.libs = [...test.libs, 'dwarf'];
// test.libs = [
//     ...test.libs,
//     'pthread'
// ];

module.exports = [test];
