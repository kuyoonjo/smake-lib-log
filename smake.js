const { LLVM } = require('smake');
const { Conan } = require('@smake/cmake');
const { LibLog } = require('./lib');

// const dw = new Conan('libdwarf/20191104', 'arm64-apple-darwin');

const test = new LLVM('test', 'arm64-apple-darwin');
test.files = ['test.cc'];
LibLog.config(test);
test.stdcxx = 'c++17';
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