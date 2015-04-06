# ASCII Fluid Mechanics

This is a deobfuscation and analysis effort for the 2012 entry in the [IOCCC Obfuscated C Code Contest][ioccc] by [Yuseke Endoh][endoh] - take a look at this [amazing video][vid] of his work.

The [original source code][source-obfus] is available at the [entry page][source] along with a [hint page][source-hint] that provides details on the problem and a (somewhat) [deobfuscated version][source-deobfus] of the code.

All code here is an adaption of Yuseke's work and is licensed under the [Creative Commons Attribution-ShareAlike 3.0][cc] [license][cc-lic].

Note that all the text files (extension `.txt`) included in this repo are taken directly from the challenge source for testing convenience.

To compile and run this code, run the following:-

```bash
gcc -Wall -Werror --std=c99 -O2 -lm -D_DEFAULT_SOURCE -DG=1 -DP=4 -DV=8 main.c -o fluids
./fluids < inputs/logo.txt # or any source file you prefer.
```

[ioccc]:http://www.ioccc.org/
[endoh]:http://d.hatena.ne.jp/ku-ma-me/
[vid]:https://www.youtube.com/watch?v=QMYfkOtYYlg
[source]:http://www.ioccc.org/2012/endoh1/
[source-obfus]:http://www.ioccc.org/2012/endoh1/endoh1.c
[source-hint]:http://www.ioccc.org/2012/endoh1/hint.html
[source-deobfus]:http://www.ioccc.org/2012/endoh1/endoh1_deobfuscate.c

[cc]:https://creativecommons.org/licenses/by-sa/3.0/
[cc-lic]:https://creativecommons.org/licenses/by-sa/3.0/legalcode
