import { resolve } from 'path';
import { LLVM } from 'smake';
import { LibQueue } from '@smake/queue';

export abstract class LibLog {
  static config(llvm: LLVM) {
    LibQueue.config(llvm);
    llvm.includedirs = [
      ...llvm.includedirs,
      resolve(__dirname, '..', 'include').replace(/\\/g, '/'),
    ];
  }
}
