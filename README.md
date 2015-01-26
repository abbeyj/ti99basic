# ti99basic - TI-99/4A BASIC as a scripting language

This is a version of 
[Texas Instruments' BASIC](http://en.wikipedia.org/wiki/TI_BASIC_%28TI_99/4A%29)
as found in ROM on the
[TI-99/4A](http://en.wikipedia.org/wiki/Texas_Instruments_TI-99/4A).
Note that this is **not**
[TI Extended BASIC](http://en.wikipedia.org/wiki/TI_Extended_BASIC).
It is also not the same as the
[TI-BASIC](http://en.wikipedia.org/wiki/TI-BASIC_%28calculators%29)
used in TI's line of calculators.
You can use it in interactive mode or pass a BASIC file as a command
line parameter.

This source does not emulate
[TMS9900](http://en.wikipedia.org/wiki/Texas_Instruments_TMS9900)
code.  The function of the original
TMS9900 code is handled by native code.  The Graphics Programming
Language (GPL) code is still run in a virtual machine just as it was on
the original hardware.

## Download
* 2009-08-12 [Version 1.0](https://github.com/abbeyj/ti99basic/releases/tag/v1.0) (Win32 binary, OS X binary, source)

Windows users should also install
[Microsoft Visual C++ 2008 SP1 Redistributable Package](http://www.microsoft.com/en-us/download/details.aspx?id=5582).

## Compatibility
It has been tested with

* Mac OS X 10.4/10.5/10.6 i386/x86_64/ppc (GCC 3.3/4.2)
* Ubuntu Linux 9.04 (GCC 4.3)
* Windows XP (Visual Studio 2008)

Other CPUs, operating systems and compilers should work, too.

## Compiling
To build from source you will need a copy of
[Perl](http://www.perl.org/)
and the original TI-99/4A ROMs.  These files should have the following
MD5 sums:

```
6cc4bc2b6b3b0c33698e6a03759a4cab *rom.bin
ed8ff714542ba850bdec686840a79217 *grom.bin
```

Other versions of the files are unlikely to work.

## Usage
The specifics of the dialect of BASIC used by the TI-99/4A are
available in the original
[User's Reference Guide](ftp://ftp.whtech.com/datasheets%20and%20manuals/99-4A%20Computer/99-4A%20User%20Reference%20Guide.pdf).
The Cassette, Disk, and Sound devices are not
currently emulated.  You can use tibasic in interactive mode by just running the binary
without parameters, or you can specify an ASCII-encoded BASIC program
on the command line.  You can also use tibasic as a UNIX scripting
language by adding a hashbang line to your BASIC program and making it
executable.

```
$ ls -l hello.bas
-rwxr-xr-x 1 user user 43 2009-08-06 23:55 hello.bas
$ cat hello.bas
#!/usr/bin/tibasic
10 PRINT "HELLO WORLD!"
$ ./hello.bas
HELLO WORLD!
```

Also included are 3 sample programs in the 'samples' directory.  The
first two are sample games provided in the original manual; these
expect upper-case input at the prompts so it may be convenient to
turn on Caps Lock when running them.

<table border="1" cellpadding="4">
<tr><th>Filename</th><th>Description</th></tr>
<tr><td><a href="https://github.com/abbeyj/ti99basic/blob/master/samples/codebreaker.bas">codebreaker.bas</a></td>
    <td>A clone of <a href="http://en.wikipedia.org/wiki/Mastermind_%28board_game%29">Mastermind</a></td></tr>
<tr><td><a href="https://github.com/abbeyj/ti99basic/blob/master/samples/secretnumber.bas">secretnumber.bas</a></td>
    <td>Guess the secret number but with an unusual twist</td></tr>
<tr><td><a href="https://github.com/abbeyj/ti99basic/blob/master/samples/sieve.bas">sieve.bas</a></td>
    <td>Prints out prime numbers from 1 to 120,000</td></tr>
</table>

## Source
The latest source code can be found in the
[repository](https://github.com/abbeyj/ti99basic).

## License
Feel free to use this project for any purpose, give credit if you like,
and send back improvements to the authors, if you like, so that others can
benefit from it. See source for license details.

## Contact
[James Abbatiello](mailto:abbeyj@gmail.com)<br>
[Michael Steil](mailto:mist@c64.org)

## See Also
If you liked this you may enjoy
[cbmbasic](https://github.com/mist64/cbmbasic) - Commodore BASIC V2 as a scripting language
