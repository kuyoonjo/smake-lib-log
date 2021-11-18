const { LLVM } = require('smake');
// const { Conan } = require('@smake/cmake');
const { LibLog } = require('./lib');

// const dw = new Conan('libdwarf/20191104', 'arm64-apple-darwin');

const test = new LLVM('test', 'x86_64-linux-gnu');
test.files = ['test.cc'];
LibLog.config(test);
test.stdcxx = 'c++20';
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

module.exports = [test];