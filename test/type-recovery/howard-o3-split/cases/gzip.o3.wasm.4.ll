; ModuleID = '../result/gzip.o3.wasm.ll'
source_filename = "bin/gzip.o3.wasm"
target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"

@__stack_pointer = internal global i32 897632
@__notdec_mem0 = global [917504 x i8] zeroinitializer
@__notdec_mem0_0x400 = internal constant [14108 x i8] c"\1F\A0\00\1F\9E\00\1F\9D\00\1F\8B\00.tgz\00.gz\00-gz\00.taz\00_z\00.z\00-z\00-presume-input-tty\00  -t, --test        test compressed file integrity\00suffix\00lzw\00standard input\00to-stdout\00list\00test\00best\00fast\00name too short\00silent\00quiet\00file timestamp out of range for gzip format\00synchronous\00  -l, --list        list compressed file contents\00bits\00uncompress\00  -d, --decompress  decompress\00Success\00  -f, --force       force overwrite of output file and compress links\00  -q, --quiet       suppress all warnings\00  -k, --keep        keep (don't delete) input files\00  -S, --suffix=SUF  use suffix SUF on compressed files\00  -r, --recursive   operate recursively on directories\00 -- %s %s\00compr\00invalid compressed data--length error\00  -9, --best        compress better\00  -1, --fast        compress faster\00  -V, --version     display version number\00.tar\00  -n, --no-name     do not save or restore the original name and timestamp\00  -N, --name        save or restore the original name and timestamp\00  -h, --help        give this help\00keep\00written to\00version\00stdin\00read from\00bad pack level\00ascii\00replaced with\00invalid compressed data--length mismatch\00This is free software.  You may redistribute copies of it under the terms of\00%#.0f\00.exe\00recursive\00      --rsyncable   make rsync-friendly archive\00verbose\00  -L, --license     display software license\00store\00internal error in shorten_name\00no-name\00invalid compressed data -- unexpected end of file\00rsyncable\00corrupted input -- file name too large\00invalid compressed data -- Huffman code bit length out of range\00too many leaves in Huffman tree\00too few leaves in Huffman tree\00  -v, --verbose     verbose mode\00force\00internal error, invalid method\00invalid compressed data--format violated\00created\00uncompressed\00  -c, --stdout      write on standard output, keep original files unchanged\00%+d\00defla\00.Z\00POSIXLY_CORRECT\00GZIP\00 OK\00%s: %s already exists;\00%s: option '%s%s' is ambiguous; possibilities:\001.14\00Copyright (C) 1993 Jean-loup Gailly.\00There is NO WARRANTY, to the extent permitted by law.\00corrupt input.\00With no FILE, or when FILE is -, read standard input.\00Mandatory arguments to long options are mandatory for short options too.\00Copyright (C) 2025 Free Software Foundation, Inc.\00corrupt input. Use zcat to recover some data.\00the GNU General Public License <https://www.gnu.org/licenses/gpl.html>.\00Report bugs to <bug-gzip@gnu.org>.\00..\00Compress or uncompress FILEs (by default, compress FILES in-place).\00--\00      --synchronous synchronous output (safer if system crashes, but slower)\00(NULL)\00 '%s%s'\00%5.1f%%\00%5s %08lx \00pack \00%*jd %*jd \00%s%3d %02d:%02d \00-W \00??? ?? ??:?? \00 do you wish to overwrite (y or n)? \00lzh  \00method  crc     date  time  \00                            \00\1F\1E\00can't recover suffix\0A\00%s: %s: header checksum 0x%04x != computed checksum 0x%04x\0A\00\0A%s: %s: warning, unknown flags 0x%x\0A\00len %lu, siz %lu\0A\00%s: option '%s%s' doesn't allow an argument\0A\00%s: option '%s%s' requires an argument\0A\00\0A%s: %s: not in gzip format\0A\00%s: option '%s%s' is ambiguous\0A\00\0A%s: %s: compressed with %d bits, can only handle %d bits\0A\00%s %s\0A\00%s: %s compressed to %s\0A\00\0A%s: %s: %s\0A\00%s: %s: warning: %s\0A\00\0A%s: %s: invalid compressed data--length error\0A\00\0A%s: %s: invalid compressed data--crc error\0A\00%s: -b operand is not an integer\0A\00\0A%s: %s: encrypted file -- use unzip\0A\00\0A%s: %s: first entry not deflated or stored -- use unzip\0A\00%s: -Z not supported in this version\0A\00\09not overwritten\0A\00%s: option --ascii ignored on this system\0A\00%s: compressed data not %s a terminal. Use -f to force %scompression.\0AFor help, type: %s -h\0A\00%s: %s/%s: pathname too long\0A\00%s: %s: file name too long\0A\00%s: %s: file size changed while zipping\0A\00%*.*s %*.*s  ratio uncompressed_name\0A\00\0A%s: %s: not a valid zip file\0A\00Bad table\0A\00\0A%s: memory_exhausted\0A\00%s: %s has flags 0x%x -- not supported\0A\00%s: %s is encrypted -- not supported\0A\00%s: %s: unknown method %d -- not supported\0A\00%s: %s: warning, name truncated\0A\00%s: timestamp restored\0A\00%s: %s has more than one entry--rest ignored\0A\00%s: %s: extra field of %u bytes ignored\0A\00\0A%s: %s: decompression OK, trailing zero bytes ignored\0A\00%s: %s has %lu other link%s -- file ignored\0A\00%s: %s has the sticky bit set - file ignored\0A\00\0A%s: %s: decompression OK, trailing garbage ignored\0A\00%s: %s is a directory -- ignored\0A\00%s: %s: unknown suffix -- ignored\0A\00%s: %s is set-user-ID on execution - ignored\0A\00%s: %s is set-group-ID on execution - ignored\0A\00%s: %s is not a directory or a regular file - ignored\0A\00%s: %s has more than one entry -- unchanged\0A\00%s: %s already has %s suffix -- unchanged\0A\00 OK\0A\00Written by Jean-loup Gailly.\0A\00Try `%s --help' for more information.\0A\00Usage: %s [OPTION]... [FILE]...\0A\00 (totals)\0A\00%s: unrecognized option '%s%s'\0A\00%s: invalid suffix '%s'\0A\00%s: option requires an argument -- '%c'\0A\00%s: invalid option -- '%c'\0A\00%s: suffix contains '/'\0A\00%s:\09\00 \09\00PK\03\04\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\04\00\04\00\08\00\04\00\04\00\05\00\10\00\08\00\04\00\06\00 \00 \00\04\00\04\00\10\00\10\00\08\00\10\00 \00 \00\08\00\10\00\80\00\80\00\08\00 \00\80\00\00\01 \00\80\00\02\01\00\04 \00\02\01\02\01\00\10ab:cdfhH?klLmMnNqrS:tvVZ123456789\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\0B\08\00\00\00\00\00\00\00\00\00\00a\00\00\00\87\04\00\00\00\00\00\00\00\00\00\00c\00\00\00\8A\04\00\00\00\00\00\00\00\00\00\00c\00\00\00O\05\00\00\00\00\00\00\00\00\00\00d\00\00\000\05\00\00\00\00\00\00\00\00\00\00d\00\00\00?\0A\00\00\00\00\00\00\00\00\00\00f\00\00\00\CF\07\00\00\00\00\00\00\00\00\00\00h\00\00\00\D4\07\00\00\00\00\00\00\00\00\00\00k\00\00\00\91\04\00\00\00\00\00\00\00\00\00\00l\00\00\00\07\09\00\00\00\00\00\00\00\00\00\00L\00\00\004\09\00\00\00\00\00\00\00\00\00\00n\00\00\007\09\00\00\00\00\00\00\00\00\00\00N\00\00\00'\04\00\00\00\00\00\00\00\00\00\00\80\00\00\00\BB\04\00\00\00\00\00\00\00\00\00\00q\00\00\00\B4\04\00\00\00\00\00\00\00\00\00\00q\00\00\00\ED\04\00\00\00\00\00\00\00\00\00\00\82\00\00\00\A0\08\00\00\00\00\00\00\00\00\00\00r\00\00\00m\04\00\00\01\00\00\00\00\00\00\00S\00\00\00\96\04\00\00\00\00\00\00\00\00\00\00t\00\00\00\DA\08\00\00\00\00\00\00\00\00\00\00v\00\00\00\E4\07\00\00\00\00\00\00\00\00\00\00V\00\00\00\A0\04\00\00\00\00\00\00\00\00\00\001\00\00\00\9B\04\00\00\00\00\00\00\00\00\00\009\00\00\00t\04\00\00\00\00\00\00\00\00\00\00Z\00\00\00+\05\00\00\01\00\00\00\00\00\00\00b\00\00\00n\09\00\00\00\00\00\00\00\00\00\00\81\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00.\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\0F\09\00\00~\06\00\00\CC\0D\00\00%\0E\00\00\86\16\00\00\86\16\00\00\86\16\00\00\86\16\00\00\F2\0A\00\00\00\00\00\00\00\00\00\00\00\00\00\00Jan\00Feb\00Mar\00Apr\00May\00Jun\00Jul\00Aug\00Sep\00Oct\00Nov\00Dec\00\10\00\00\00\11\00\00\00\12\00\00\00\00\00\00\00\08\00\00\00\07\00\00\00\09\00\00\00\06\00\00\00\0A\00\00\00\05\00\00\00\0B\00\00\00\04\00\00\00\0C\00\00\00\03\00\00\00\0D\00\00\00\02\00\00\00\0E\00\00\00\01\00\00\00\0F\00\00\00\00\00\00\00\00\00\01\00\03\00\07\00\0F\00\1F\00?\00\7F\00\FF\00\FF\01\FF\03\FF\07\FF\0F\FF\1F\FF?\FF\7F\FF\FF\00\00\00\00\00\00\00\00\00\00\00\00\00\00\10\11\12\00\08\07\09\06\0A\05\0B\04\0C\03\0D\02\0E\01\0F\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\960\07w,a\0E\EE\BAQ\09\99\19\C4m\07\8F\F4jp5\A5c\E9\A3\95d\9E2\88\DB\0E\A4\B8\DCy\1E\E9\D5\E0\88\D9\D2\97+L\B6\09\BD|\B1~\07-\B8\E7\91\1D\BF\90d\10\B7\1D\F2 \B0jHq\B9\F3\DEA\BE\84}\D4\DA\1A\EB\E4\DDmQ\B5\D4\F4\C7\85\D3\83V\98l\13\C0\A8kdz\F9b\FD\EC\C9e\8AO\\\01\14\D9l\06cc=\0F\FA\F5\0D\08\8D\C8 n;^\10iL\E4A`\D5rqg\A2\D1\E4\03<G\D4\04K\FD\85\0D\D2k\B5\0A\A5\FA\A8\B55l\98\B2B\D6\C9\BB\DB@\F9\BC\AC\E3l\D82u\\\DFE\CF\0D\D6\DCY=\D1\AB\AC0\D9&:\00\DEQ\80Q\D7\C8\16a\D0\BF\B5\F4\B4!#\C4\B3V\99\95\BA\CF\0F\A5\BD\B8\9E\B8\02(\08\88\05_\B2\D9\0C\C6$\E9\0B\B1\87|o/\11LhX\AB\1Da\C1=-f\B6\90A\DCv\06q\DB\01\BC \D2\98*\10\D5\EF\89\85\B1q\1F\B5\B6\06\A5\E4\BF\9F3\D4\B8\E8\A2\C9\07x4\F9\00\0F\8E\A8\09\96\18\98\0E\E1\BB\0Dj\7F-=m\08\97ld\91\01\\c\E6\F4Qkkbal\1C\D80e\85N\00b\F2\ED\95\06l{\A5\01\1B\C1\F4\08\82W\C4\0F\F5\C6\D9\B0eP\E9\B7\12\EA\B8\BE\8B|\88\B9\FC\DF\1D\DDbI-\DA\15\F3|\D3\8CeL\D4\FBXa\B2M\CEQ\B5:t\00\BC\A3\E20\BB\D4A\A5\DFJ\D7\95\D8=m\C4\D1\A4\FB\F4\D6\D3j\E9iC\FC\D9n4F\88g\AD\D0\B8`\DAs-\04D\E5\1D\033_L\0A\AA\C9|\0D\DD<q\05P\AAA\02'\10\10\0B\BE\86 \0C\C9%\B5hW\B3\85o \09\D4f\B9\9F\E4a\CE\0E\F9\DE^\98\C9\D9)\22\98\D0\B0\B4\A8\D7\C7\17=\B3Y\81\0D\B4.;\\\BD\B7\ADl\BA\C0 \83\B8\ED\B6\B3\BF\9A\0C\E2\B6\03\9A\D2\B1t9G\D5\EA\AFw\D2\9D\15&\DB\04\83\16\DCs\12\0Bc\E3\84;d\94>jm\0D\A8Zjz\0B\CF\0E\E4\9D\FF\09\93'\AE\00\0A\B1\9E\07}D\93\0F\F0\D2\A3\08\87h\F2\01\1E\FE\C2\06i]Wb\F7\CBge\80q6l\19\E7\06knv\1B\D4\FE\E0+\D3\89Zz\DA\10\CCJ\DDgo\DF\B9\F9\F9\EF\BE\8EC\BE\B7\17\D5\8E\B0`\E8\A3\D6\D6~\93\D1\A1\C4\C2\D88R\F2\DFO\F1g\BB\D1gW\BC\A6\DD\06\B5?K6\B2H\DA+\0D\D8L\1B\0A\AF\F6J\036`z\04A\C3\EF`\DFU\DFg\A8\EF\8En1y\BEiF\8C\B3a\CB\1A\83f\BC\A0\D2o%6\E2hR\95w\0C\CC\03G\0B\BB\B9\16\02\22/&\05U\BE;\BA\C5(\0B\BD\B2\92Z\B4+\04j\B3\\\A7\FF\D7\C21\CF\D0\B5\8B\9E\D9,\1D\AE\DE[\B0\C2d\9B&\F2c\EC\9C\A3ju\0A\93m\02\A9\06\09\9C?6\0E\EB\85g\07r\13W\00\05\82J\BF\95\14z\B8\E2\AE+\B1{8\1B\B6\0C\9B\8E\D2\92\0D\BE\D5\E5\B7\EF\DC|!\DF\DB\0B\D4\D2\D3\86B\E2\D4\F1\F8\B3\DDhn\83\DA\1F\CD\16\BE\81[&\B9\F6\E1w\B0owG\B7\18\E6Z\08\88pj\0F\FF\CA;\06f\\\0B\01\11\FF\9Ee\8Fi\AEb\F8\D3\FFkaE\CFl\16x\E2\0A\A0\EE\D2\0D\D7T\83\04N\C2\B3\039a&g\A7\F7\16`\D0MGiI\DBwn>Jj\D1\AE\DCZ\D6\D9f\0B\DF@\F0;\D87S\AE\BC\A9\C5\9E\BB\DE\7F\CF\B2G\E9\FF\B50\1C\F2\BD\BD\8A\C2\BA\CA0\93\B3S\A6\A3\B4$\056\D0\BA\93\06\D7\CD)W\DET\BFg\D9#.zf\B3\B8Ja\C4\02\1Bh]\94+o*7\BE\0B\B4\A1\8E\0C\C3\1B\DF\05Z\8D\EF\02-\00\00\00\00A1\1B\19\82b62\C3S-+\04\C5ldE\F4w}\86\A7ZV\C7\96AO\08\8A\D9\C8I\BB\C2\D1\8A\E8\EF\FA\CB\D9\F4\E3\0CO\B5\ACM~\AE\B5\8E-\83\9E\CF\1C\98\87Q\12\C2J\10#\D9S\D3p\F4x\92A\EFaU\D7\AE.\14\E6\B57\D7\B5\98\1C\96\84\83\05Y\98\1B\82\18\A9\00\9B\DB\FA-\B0\9A\CB6\A9]]w\E6\1Cll\FF\DF?A\D4\9E\0EZ\CD\A2$\84\95\E3\15\9F\8C F\B2\A7aw\A9\BE\A6\E1\E8\F1\E7\D0\F3\E8$\83\DE\C3e\B2\C5\DA\AA\AE]]\EB\9FFD(\CCkoi\FDpv\AEk19\EFZ* ,\09\07\0Bm8\1C\12\F36F\DF\B2\07]\C6qTp\ED0ek\F4\F7\F3*\BB\B6\C21\A2u\91\1C\894\A0\07\90\FB\BC\9F\17\BA\8D\84\0Ey\DE\A9%8\EF\B2<\FFy\F3s\BEH\E8j}\1B\C5A<*\DEX\05Oy\F0D~b\E9\87-O\C2\C6\1CT\DB\01\8A\15\94@\BB\0E\8D\83\E8#\A6\C2\D98\BF\0D\C5\A08L\F4\BB!\8F\A7\96\0A\CE\96\8D\13\09\00\CC\\H1\D7E\8Bb\FAn\CAS\E1wT]\BB\BA\15l\A0\A3\D6?\8D\88\97\0E\96\91P\98\D7\DE\11\A9\CC\C7\D2\FA\E1\EC\93\CB\FA\F5\\\D7br\1D\E6yk\DE\B5T@\9F\84OYX\12\0E\16\19#\15\0F\DAp8$\9BA#=\A7k\FDe\E6Z\E6|%\09\CBWd8\D0N\A3\AE\91\01\E2\9F\8A\18!\CC\A73`\FD\BC*\AF\E1$\AD\EE\D0?\B4-\83\12\9Fl\B2\09\86\AB$H\C9\EA\15S\D0)F~\FBhwe\E2\F6y?/\B7H$6t\1B\09\1D5*\12\04\F2\BCSK\B3\8DHRp\DEey1\EF~`\FE\F3\E6\E7\BF\C2\FD\FE|\91\D0\D5=\A0\CB\CC\FA6\8A\83\BB\07\91\9AxT\BC\B19e\A7\A8K\98\83;\0A\A9\98\22\C9\FA\B5\09\88\CB\AE\10O]\EF_\0El\F4F\CD?\D9m\8C\0E\C2tC\12Z\F3\02#A\EA\C1pl\C1\80Aw\D8G\D76\97\06\E6-\8E\C5\B5\00\A5\84\84\1B\BC\1A\8AAq[\BBZh\98\E8wC\D9\D9lZ\1EO-\15_~6\0C\9C-\1B'\DD\1C\00>\12\00\98\B9S1\83\A0\90b\AE\8B\D1S\B5\92\16\C5\F4\DDW\F4\EF\C4\94\A7\C2\EF\D5\96\D9\F6\E9\BC\07\AE\A8\8D\1C\B7k\DE1\9C*\EF*\85\EDyk\CA\ACHp\D3o\1B]\F8.*F\E1\E16\DEf\A0\07\C5\7FcT\E8T\22e\F3M\E5\F3\B2\02\A4\C2\A9\1Bg\91\840&\A0\9F)\B8\AE\C5\E4\F9\9F\DE\FD:\CC\F3\D6{\FD\E8\CF\BCk\A9\80\FDZ\B2\99>\09\9F\B2\7F8\84\AB\B0$\1C,\F1\15\0752F*\1Esw1\07\B4\E1pH\F5\D0kQ6\83Fzw\B2]cN\D7\FA\CB\0F\E6\E1\D2\CC\B5\CC\F9\8D\84\D7\E0J\12\96\AF\0B#\8D\B6\C8p\A0\9D\89A\BB\84F]#\03\07l8\1A\C4?\151\85\0E\0E(B\98Og\03\A9T~\C0\FAyU\81\CBbL\1F\C58\81^\F4#\98\9D\A7\0E\B3\DC\96\15\AA\1B\00T\E5Z1O\FC\99bb\D7\D8Sy\CE\17O\E1IV~\FAP\95-\D7{\D4\1C\CCb\13\8A\8D-R\BB\964\91\E8\BB\1F\D0\D9\A0\06\EC\F3~^\AD\C2eGn\91Hl/\A0Su\E86\12:\A9\07\09#jT$\08+e?\11\E4y\A7\96\A5H\BC\8Ff\1B\91\A4'*\8A\BD\E0\BC\CB\F2\A1\8D\D0\EBb\DE\FD\C0#\EF\E6\D9\BD\E1\BC\14\FC\D0\A7\0D?\83\8A&~\B2\91?\B9$\D0p\F8\15\CBi;F\E6Bzw\FD[\B5ke\DC\F4Z~\C57\09S\EEv8H\F7\B1\AE\09\B8\F0\9F\12\A13\CC?\8Ar\FD$\93\00\00\00\007j\C2\01n\D4\84\03Y\BEF\02\DC\A8\09\07\EB\C2\CB\06\B2|\8D\04\85\16O\05\B8Q\13\0E\8F;\D1\0F\D6\85\97\0D\E1\EFU\0Cd\F9\1A\09S\93\D8\08\0A-\9E\0A=G\\\0Bp\A3&\1CG\C9\E4\1D\1Ew\A2\1F)\1D`\1E\AC\0B/\1B\9Ba\ED\1A\C2\DF\AB\18\F5\B5i\19\C8\F25\12\FF\98\F7\13\A6&\B1\11\91Ls\10\14Z<\15#0\FE\14z\8E\B8\16M\E4z\17\E0FM8\D7,\8F9\8E\92\C9;\B9\F8\0B:<\EED?\0B\84\86>R:\C0<eP\02=X\17^6o}\9C76\C3\DA5\01\A9\184\84\BFW1\B3\D5\950\EAk\D32\DD\01\113\90\E5k$\A7\8F\A9%\FE1\EF'\C9[-&LMb#{'\A0\22\22\99\E6 \15\F3$!(\B4x*\1F\DE\BA+F`\FC)q\0A>(\F4\1Cq-\C3v\B3,\9A\C8\F5.\AD\A27/\C0\8D\9Ap\F7\E7Xq\AEY\1Es\993\DCr\1C%\93w+OQvr\F1\17tE\9B\D5ux\DC\89~O\B6K\7F\16\08\0D}!b\CF|\A4t\80y\93\1EBx\CA\A0\04z\FD\CA\C6{\B0.\BCl\87D~m\DE\FA8o\E9\90\FAnl\86\B5k[\ECwj\02R1h58\F3i\08\7F\AFb?\15mcf\AB+aQ\C1\E9`\D4\D7\A6e\E3\BDdd\BA\03\22f\8Di\E0g \CB\D7H\17\A1\15IN\1FSKyu\91J\FCc\DEO\CB\09\1CN\92\B7ZL\A5\DD\98M\98\9A\C4F\AF\F0\06G\F6N@E\C1$\82DD2\CDAsX\0F@*\E6IB\1D\8C\8BCPh\F1Tg\023U>\BCuW\09\D6\B7V\8C\C0\F8S\BB\AA:R\E2\14|P\D5~\BEQ\E89\E2Z\DFS [\86\EDfY\B1\87\A4X4\91\EB]\03\FB)\\ZEo^m/\AD_\80\1B5\E1\B7q\F7\E0\EE\CF\B1\E2\D9\A5s\E3\\\B3<\E6k\D9\FE\E72g\B8\E5\05\0Dz\E48J&\EF\0F \E4\EEV\9E\A2\ECa\F4`\ED\E4\E2/\E8\D3\88\ED\E9\8A6\AB\EB\BD\\i\EA\F0\B8\13\FD\C7\D2\D1\FC\9El\97\FE\A9\06U\FF,\10\1A\FA\1Bz\D8\FBB\C4\9E\F9u\AE\\\F8H\E9\00\F3\7F\83\C2\F2&=\84\F0\11WF\F1\94A\09\F4\A3+\CB\F5\FA\95\8D\F7\CD\FFO\F6`]x\D9W7\BA\D8\0E\89\FC\DA9\E3>\DB\BC\F5q\DE\8B\9F\B3\DF\D2!\F5\DD\E5K7\DC\D8\0Ck\D7\EFf\A9\D6\B6\D8\EF\D4\81\B2-\D5\04\A4b\D03\CE\A0\D1jp\E6\D3]\1A$\D2\10\FE^\C5'\94\9C\C4~*\DA\C6I@\18\C7\CCVW\C2\FB<\95\C3\A2\82\D3\C1\95\E8\11\C0\A8\AFM\CB\9F\C5\8F\CA\C6{\C9\C8\F1\11\0B\C9t\07D\CCCm\86\CD\1A\D3\C0\CF-\B9\02\CE@\96\AF\91w\FCm\90.B+\92\19(\E9\93\9C>\A6\96\ABTd\97\F2\EA\22\95\C5\80\E0\94\F8\C7\BC\9F\CF\AD~\9E\96\138\9C\A1y\FA\9D$o\B5\98\13\05w\99J\BB1\9B}\D1\F3\9A05\89\8D\07_K\8C^\E1\0D\8Ei\8B\CF\8F\EC\9D\80\8A\DB\F7B\8B\82I\04\89\B5#\C6\88\88d\9A\83\BF\0EX\82\E6\B0\1E\80\D1\DA\DC\81T\CC\93\84c\A6Q\85:\18\17\87\0Dr\D5\86\A0\D0\E2\A9\97\BA \A8\CE\04f\AA\F9n\A4\AB|x\EB\AEK\12)\AF\12\ACo\AD%\C6\AD\AC\18\81\F1\A7/\EB3\A6vUu\A4A?\B7\A5\C4)\F8\A0\F3C:\A1\AA\FD|\A3\9D\97\BE\A2\D0s\C4\B5\E7\19\06\B4\BE\A7@\B6\89\CD\82\B7\0C\DB\CD\B2;\B1\0F\B3b\0FI\B1Ue\8B\B0h\22\D7\BB_H\15\BA\06\F6S\B81\9C\91\B9\B4\8A\DE\BC\83\E0\1C\BD\DA^Z\BF\ED4\98\BE\00\00\00\00eg\BC\B8\8B\C8\09\AA\EE\AF\B5\12W\97b\8F2\F0\DE7\DC_k%\B98\D7\9D\EF(\B4\C5\8AO\08}d\E0\BDo\01\87\01\D7\B8\BF\D6J\DD\D8j\F23w\DF\E0V\10cX\9FW\19P\FA0\A5\E8\14\9F\10\FAq\F8\ACB\C8\C0{\DF\AD\A7\C7gC\08ru&o\CE\CDp\7F\AD\95\15\18\11-\FB\B7\A4?\9E\D0\18\87'\E8\CF\1AB\8Fs\A2\AC \C6\B0\C9Gz\08>\AF2\A0[\C8\8E\18\B5g;\0A\D0\00\87\B2i8P/\0C_\EC\97\E2\F0Y\85\87\97\E5=\D1\87\86e\B4\E0:\DDZO\8F\CF?(3w\86\10\E4\EA\E3wXR\0D\D8\ED@h\BFQ\F8\A1\F8+\F0\C4\9F\97H*0\22ZOW\9E\E2\F6oI\7F\93\08\F5\C7}\A7@\D5\18\C0\FCmN\D0\9F5+\B7#\8D\C5\18\96\9F\A0\7F*'\19G\FD\BA| A\02\92\8F\F4\10\F7\E8H\A8=X\14\9BX?\A8#\B6\90\1D1\D3\F7\A1\89j\CFv\14\0F\A8\CA\AC\E1\07\7F\BE\84`\C3\06\D2p\A0^\B7\17\1C\E6Y\B8\A9\F4<\DF\15L\85\E7\C2\D1\E0\80~i\0E/\CB{kHw\C3\A2\0F\0D\CB\C7h\B1s)\C7\04aL\A0\B8\D9\F5\98oD\90\FF\D3\FC~Pf\EE\1B7\DAVM'\B9\0E(@\05\B6\C6\EF\B0\A4\A3\88\0C\1C\1A\B0\DB\81\7F\D7g9\91x\D2+\F4\1Fn\93\03\F7&;f\90\9A\83\88?/\91\EDX\93)T`D\B41\07\F8\0C\DF\A8M\1E\BA\CF\F1\A6\EC\DF\92\FE\89\B8.Fg\17\9BT\02p'\EC\BBH\F0q\DE/L\C90\80\F9\DBU\E7Ec\9C\A0?k\F9\C7\83\D3\17h6\C1r\0F\8Ay\CB7]\E4\AEP\E1\\@\FFTN%\98\E8\F6s\88\8B\AE\16\EF7\16\F8@\82\04\9D'>\BC$\1F\E9!AxU\99\AF\D7\E0\8B\CA\B0\\3;\B6Y\ED^\D1\E5U\B0~PG\D5\19\EC\FFl!;b\09F\87\DA\E7\E92\C8\82\8E\8Ep\D4\9E\ED(\B1\F9Q\90_V\E4\82:1X:\83\09\8F\A7\E6n3\1F\08\C1\86\0Dm\A6:\B5\A4\E1@\BD\C1\86\FC\05/)I\17JN\F5\AF\F3v\222\96\11\9E\8Ax\BE+\98\1D\D9\97 K\C9\F4x.\AEH\C0\C0\01\FD\D2\A5fAj\1C^\96\F7y9*O\97\96\9F]\F2\F1#\E5\05\19kM`~\D7\F5\8E\D1b\E7\EB\B6\DE_R\8E\09\C27\E9\B5z\D9F\00h\BC!\BC\D0\EA1\DF\88\8FVc0a\F9\D6\22\04\9Ej\9A\BD\A6\BD\07\D8\C1\01\BF6n\B4\ADS\09\08\15\9ANr\1D\FF)\CE\A5\11\86{\B7t\E1\C7\0F\CD\D9\10\92\A8\BE\AC*F\11\198#v\A5\80uf\C6\D8\10\01z`\FE\AE\CFr\9B\C9s\CA\22\F1\A4WG\96\18\EF\A99\AD\FD\CC^\11E\06\EEMvc\89\F1\CE\8D&D\DC\E8A\F8dQy/\F94\1E\93A\DA\B1&S\BF\D6\9A\EB\E9\C6\F9\B3\8C\A1E\0Bb\0E\F0\19\07iL\A1\BEQ\9B<\DB6'\845\99\92\96P\FE..\99\B9T&\FC\DE\E8\9E\12q]\8Cw\16\E14\CE.6\A9\ABI\8A\11E\E6?\03 \81\83\BBv\91\E0\E3\13\F6\\[\FDY\E9I\98>U\F1!\06\82lDa>\D4\AA\CE\8B\C6\CF\A97~8A\7F\D6]&\C3n\B3\89v|\D6\EE\CA\C4o\D6\1DY\0A\B1\A1\E1\E4\1E\14\F3\81y\A8K\D7i\CB\13\B2\0Ew\AB\\\A1\C2\B99\C6~\01\80\FE\A9\9C\E5\99\15$\0B6\A06nQ\1C\8E\A7\16f\86\C2q\DA>,\DEo,I\B9\D3\94\F0\81\04\09\95\E6\B8\B1{I\0D\A3\1E.\B1\1BH>\D2C-Yn\FB\C3\F6\DB\E9\A6\91gQ\1F\A9\B0\CCz\CE\0Ct\94a\B9f\F1\06\05\DE\00\00\00\00\B0)`=`S\C0z\D0z\A0G\C0\A6\80\F5p\8F\E0\C8\A0\F5@\8F\10\DC \B2\C1Kp0qb\10\0D\A1\18\B0J\111\D0w\01\ED\F0\C5\B1\C4\90\F8a\BE0\BF\D1\97P\82\82\97\E0`2\BE\80]\E2\C4 \1AR\ED@'B1`\95\F2\18\00\A8\22b\A0\EF\92K\C0\D2C\DC\90P\F3\F5\F0m#\8FP*\93\A60\17\83z\10\A53Sp\98\E3)\D0\DFS\00\B0\E2\04/\C1\C1\B4\06\A1\FCd|\01\BB\D4Ua\86\C4\89A4t\A0!\09\A4\DA\81N\14\F3\E1s\C5d\B1\F1uM\D1\CC\A57q\8B\15\1E\11\B6\05\C21\04\B5\EBQ9e\91\F1~\D5\B8\91C\86\B8!\A16\91A\9C\E6\EB\E1\DBV\C2\81\E6F\1E\A1T\F67\C1i&Ma.\96d\01\13G\F3Q\91\F7\DA1\AC'\A0\91\EB\97\89\F1\D6\87U\D1d7|\B1Y\E7\06\11\1EW/q#IX\F3X\F9q\93e)\0B3\22\99\22S\1F\89\FEs\AD9\D7\13\90\E9\AD\B3\D7Y\84\D3\EA\88\13\83h8:\E3U\E8@C\12Xi#/H\B5\03\9D\F8\9Cc\A0(\E6\C3\E7\98\CF\A3\DA\CB\CF\138{\E6s\05\AB\9C\D3B\1B\B5\B3\7F\0Bi\93\CD\BB@\F3\F0k:S\B7\DB\133\8A\0A\84c\08\BA\AD\035j\D7\A3r\DA\FE\C3O\CA\22\E3\FDz\0B\83\C0\AAq#\87\1AXC\BAMw2\99\FD^R\A4-$\F2\E3\9D\0D\92\DE\8D\D1\B2l=\F8\D2Q\ED\82r\16]\AB\12+\8C<B\A9<\15\22\94\ECo\82\D3\\F\E2\EEL\9A\C2\\\FC\B3\A2a,\C9\02&\9C\E0b\1B\CF\E0\D2\F9\7F\C9\B2\C4\AF\B3\12\83\1F\9Ar\BE\0FFR\0C\BFo21o\15\92v\DF<\F2K\0E\AB\A2\C9\BE\82\C2\F4n\F8b\B3\DE\D1\02\8E\CE\0D\22<~$B\01\AE^\E2F\1Ew\82{\92\B0\E6\B1\22\99\86\8C\F2\E3&\CBB\CAF\F6R\16fD\E2?\06y2E\A6>\82l\C6\03S\FB\96\81\E3\D2\F6\BC3\A8V\FB\83\816\C6\93]\16t#tvI\F3\0E\D6\0EC'\B63\10'\06\D1\A0\0Ef\ECpt\C6\AB\C0]\A6\96\D0\81\86$`\A8\E6\19\B0\D2F^\00\FB&c\D1lv\E1aE\16\DC\B1?\B6\9B\01\16\D6\A6\11\CA\F6\14\A1\E3\96)q\996n\C1\B0VS\96\9F'p&\B6GM\F6\CC\E7\0AF\E5\877V9\A7\85\E6\10\C7\B86jg\FF\86C\07\C2W\D4W@\E7\FD7}7\87\97:\87\AE\F7\07\97r\D7\B5'[\B7\88\F7!\17\CFG\08w\F2\14\08\C7\10\A4!\A7-t[\07j\C4rgW\D4\AEG\E5d\87'\D8\B4\FD\87\9F\04\D4\E7\A2\D5C\B7 ej\D7\1D\B5\10wZ\059\17g\15\E57\D5\A5\CCW\E8u\B6\F7\AF\C5\9F\97\92\DB\E8\15\E9k\C1u\D4\BB\BB\D5\93\0B\92\B5\AE\1BN\95\1C\ABg\F5!{\1DUf\CB45[\1A\A3e\D9\AA\8A\05\E4z\F0\A5\A3\CA\D9\C5\9E\DA\05\E5,j,\85\11\BAV%V\0A\7FEkY\7F\F5\89\E9V\95\B49,5\F3\89\05U\CE\99\D9u|)\F0\15A\F9\8A\B5\06I\A3\D5;\984\85\B9(\1D\E5\84\F8gE\C3HN%\FEX\92\05L\E8\BBeq8\C1\C56\88\E8\A5\0B\DF\C7\D4(o\EE\B4\15\BF\94\14R\0F\BDto\1FaT\DD\AFH4\E0\7F2\94\A7\CF\1B\F4\9A\1E\8C\A4\18\AE\A5\C4%~\DFdb\CE\F6\04_\DE*$\EDn\03D\D0\BEy\E4\97\0EP\84\AA]P4H\EDyTu=\03\F42\8D*\94\0F\9D\F6\B4\BD-\DF\D4\80\FD\A5t\C7M\8C\14\FA\9C\1BDx,2$E\FCH\84\02La\E4?\\\BD\C4\8D\EC\94\A4\B0<\EE\04\F7\8C\C7d\CA\00\00\00\00\A5\D3\\\CB\0B\A1\C8M\AEr\94\86\16B\91\9B\B3\91\CDP\1D\E3Y\D6\B80\05\1Dm\82S\EC\C8Q\0F'f#\9B\A1\C3\F0\C7j{\C0\C2w\DE\13\9E\BCpa\0A:\D5\B2V\F1\9B\02\D6\03>\D1\8A\C8\90\A3\1EN5pB\85\8D@G\98(\93\1BS\86\E1\8F\D5#2\D3\1E\F6\80\85\EFSS\D9$\FD!M\A2X\F2\11i\E0\C2\14tE\11H\BF\EBc\DC9N\B0\80\F26\05\AC\07\93\D6\F0\CC=\A4dJ\98w8\81 G=\9C\85\94aW+\E6\F5\D1\8E5\A9\1A[\87\FF\EB\FET\A3 P&7\A6\F5\F5kmM\C5np\E8\162\BBFd\A6=\E3\B7\FA\F6\AD\07z\04\08\D4&\CF\A6\A6\B2I\03u\EE\82\BBE\EB\9F\1E\96\B7T\B0\E4#\D2\157\7F\19\C0\85)\E8eVu#\CB$\E1\A5n\F7\BDn\D6\C7\B8ss\14\E4\B8\DDfp>x\B5,\F5l\0AX\0F\C9\D9\04\C4g\AB\90B\C2x\CC\89zH\C9\94\DF\9B\95_q\E9\01\D9\D4:]\12\01\88\0B\E3\A4[W(\0A)\C3\AE\AF\FA\9Fe\17\CA\9Ax\B2\19\C6\B3\1CkR5\B9\B8\0E\FE\F7\08\8E\0CR\DB\D2\C7\FC\A9FAYz\1A\8A\E1J\1F\97D\99C\\\EA\EB\D7\DAO8\8B\11\9A\8A\DD\E0?Y\81+\91+\15\AD4\F8If\8C\C8L{)\1B\10\B0\87i\846\22\BA\D8\FDZ\0F\F4\08\FF\DC\A8\C3Q\AE<E\F4}`\8ELMe\93\E9\9E9XG\EC\AD\DE\E2?\F1\157\8D\A7\E4\92^\FB/<,o\A9\99\FF3b!\CF6\7F\84\1Cj\B4*n\FE2\8F\BD\A2\F9\C1\0D\22\0Bd\DE~\C0\CA\AC\EAFo\7F\B6\8D\D7O\B3\90r\9C\EF[\DC\EE{\DDy='\16\AC\8Fq\E7\09\\-,\A7.\B9\AA\02\FD\E5a\BA\CD\E0|\1F\1E\BC\B7\B1l(1\14\BFt\FA\D8\14\B0\1E}\C7\EC\D5\D3\B5xSvf$\98\CEV!\85k\85}N\C5\F7\E9\C8`$\B5\03\B5\96\E3\F2\10E\BF9\BE7+\BF\1B\E4wt\A3\D4ri\06\07.\A2\A8u\BA$\0D\A6\E6\EFC\16f\1D\E6\C5:\D6H\B7\AEP\EDd\F2\9BUT\F7\86\F0\87\ABM^\F5?\CB\FB&c\00.\945\F1\8BGi:%5\FD\BC\80\E6\A1w8\D6\A4j\9D\05\F8\A13wl'\96\A40\EC\EE\11\1C\19K\C2@\D2\E5\B0\D4T@c\88\9F\F8S\8D\82]\80\D1I\F3\F2E\CFV!\19\04\83\93O\F5&@\13>\882\87\B8-\E1\DBs\95\D1\DEn0\02\82\A5\9Ep\16#;\A3J\E8u\13\CA\1A\D0\C0\96\D1~\B2\02W\DBa^\9CcQ[\81\C6\82\07Jh\F0\93\CC\CD#\CF\07\18\91\99\F6\BDB\C5=\130Q\BB\B6\E3\0Dp\0E\D3\08m\AB\00T\A6\05r\C0 \A0\A1\9C\EB\B4\1E\E8\11\11\CD\B4\DA\BF\BF \\\1Al|\97\A2\\y\8A\07\8F%A\A9\FD\B1\C7\0C.\ED\0C\D9\9C\BB\FD|O\E76\D2=s\B0w\EE/{\CF\DE*fj\0Dv\AD\C4\7F\E2+a\AC\BE\E0/\1C>\12\8A\CFb\D9$\BD\F6_\81n\AA\949^\AF\89\9C\8D\F3B2\FFg\C4\97,;\0FB\9Em\FE\E7M15I?\A5\B3\EC\EC\F9xT\DC\FCe\F1\0F\A0\AE_}4(\FA\AEh\E3\82\1BD\16'\C8\18\DD\89\BA\8C[,i\D0\90\94Y\D5\8D1\8A\89F\9F\F8\1D\C0:+A\0B\EF\99\17\FAJJK1\E48\DF\B7A\EB\83|\F9\DB\86a\\\08\DA\AA\F2zN,W\A9\12\E7\19\19\92\15\BC\CA\CE\DE\12\B8ZX\B7k\06\93\0F[\03\8E\AA\88_E\04\FA\CB\C3\A1)\97\08t\9B\C1\F9\D1H\9D2\7F:\09\B4\DA\E9U\7Fb\D9Pb\C7\0A\0C\A9ix\98/\CC\AB\C4\E4\00\00\00\00\B4\0Bw\A6)\11\9F\97\9D\1A\E81\13$O\F4\A7/8R:5\D0c\8E>\A7\C5gN\EF3\D3E\98\95N_p\A4\FAT\07\02tj\A0\C7\C0a\D7a]{?P\E9pH\F6\CE\9C\DEgz\97\A9\C1\E7\8DA\F0S\866V\DD\B8\91\93i\B3\E65\F4\A9\0E\04@\A2y\A2\A9\D21T\1D\D9F\F2\80\C3\AE\C34\C8\D9e\BA\F6~\A0\0E\FD\09\06\93\E7\E17'\EC\96\91\9C9\BD\CF(2\CAi\B5(\22X\01#U\FE\8F\1D\F2;;\16\85\9D\A6\0Cm\AC\12\07\1A\0A\FBwR\FCO|%Z\D2f\CDkfm\BA\CD\E8S\1D\08\\Xj\AE\C1B\82\9FuI\F59R\A5c\A8\E6\AE\14\0E{\B4\FC?\CF\BF\8B\99A\81,\\\F5\8A[\FAh\90\B3\CB\DC\9B\C4m5\EB\8C\9B\81\E0\FB=\1C\FA\13\0C\A8\F1d\AA&\CF\C3o\92\C4\B4\C9\0F\DE\\\F8\BB\D5+^yu\0BD\CD~|\E2Pd\94\D3\E4o\E3ujQD\B0\DEZ3\16C@\DB'\F7K\AC\81\1E;\E4w\AA0\93\D17*{\E0\83!\0CF\0D\1F\AB\83\B9\14\DC%$\0E4\14\90\05C\B2\B7\E9\D5#\03\E2\A2\85\9E\F8J\B4*\F3=\12\A4\CD\9A\D7\10\C6\EDq\8D\DC\05@9\D7r\E6\D0\A7:\10d\ACM\B6\F9\B6\A5\87M\BD\D2!\C3\83u\E4w\88\02B\EA\92\EAs^\99\9D\D5\E5L\B6\8BQG\C1-\CC])\1CxV^\BA\F6h\F9\7FBc\8E\D9\DFyf\E8kr\11N\82\02Y\B86\09.\1E\AB\13\C6/\1F\18\B1\89\91&\16L%-a\EA\B87\89\DB\0C<\FE}+\D0h\EC\9F\DB\1FJ\02\C1\F7{\B6\CA\80\DD8\F4'\18\8C\FFP\BE\11\E5\B8\8F\A5\EE\CF)L\9E\87\DF\F8\95\F0ye\8F\18H\D1\84o\EE_\BA\C8+\EB\B1\BF\8Dv\ABW\BC\C2\A0 \1A\F2\EA\16\88F\E1a.\DB\FB\89\1Fo\F0\FE\B9\E1\CEY|U\C5.\DA\C8\DF\C6\EB|\D4\B1M\95\A4\F9\BB!\AF\8E\1D\BC\B5f,\08\BE\11\8A\86\80\B6O2\8B\C1\E9\AF\91)\D8\1B\9A^~<v\C8\EF\88}\BFI\15gWx\A1l \DE/R\87\1B\9BY\F0\BD\06C\18\8C\B2Ho*[8'\DC\EF3Pzr)\B8K\C6\22\CF\EDH\1Ch(\FC\17\1F\8Ea\0D\F7\BF\D5\06\80\19n\D3\ABG\DA\D8\DC\E1G\C24\D0\F3\C9Cv}\F7\E4\B3\C9\FC\93\15T\E6{$\E0\ED\0C\82\09\9DDt\BD\963\D2 \8C\DB\E3\94\87\ACE\1A\B9\0B\80\AE\B2|&3\A8\94\17\87\A3\E3\B1\A0Ou \14D\02\86\89^\EA\B7=U\9D\11\B3k:\D4\07`Mr\9Az\A5C.q\D2\E5\C7\01\9A\13s\0A\ED\B5\EE\10\05\84Z\1Br\22\D4%\D5\E7`.\A2A\FD4JpI?=\D6\8B\9F\1D\CC?\94jj\A2\8E\82[\16\85\F5\FD\98\BBR8,\B0%\9E\B1\AA\CD\AF\05\A1\BA\09\EC\D1\F2\FFX\DA\85Y\C5\C0mhq\CB\1A\CE\FF\F5\BD\0BK\FE\CA\AD\D6\E4\22\9Cb\EFU:E\03\C3\AB\F1\08\B4\0Dl\12\\<\D8\19+\9AV'\8C_\E2,\FB\F9\7F6\13\C8\CB=dn\22M,\98\96F[>\0B\\\B3\0F\BFW\C4\A91icl\85b\14\CA\18x\FC\FB\ACs\8B]\17\A6\A0\03\A3\AD\D7\A5>\B7?\94\8A\BCH2\04\82\EF\F7\B0\89\98Q-\93p`\99\98\07\C6p\E8O0\C4\E38\96Y\F9\D0\A7\ED\F2\A7\01c\CC\00\C4\D7\C7wbJ\DD\9FS\FE\D6\E8\F5\D9:~dm1\09\C2\F0+\E1\F3D \96U\CA\1E1\90~\15F6\E3\0F\AE\07W\04\D9\A1\BEt\91W\0A\7F\E6\F1\97e\0E\C0#nyf\ADP\DE\A3\19[\A9\05\84AA40J6\92\00\00\00\00\9E\00\AA\CC}\07%B\E3\07\8F\8E\FA\0EJ\84d\0E\E0H\87\09o\C6\19\09\C5\0A\B5\1B\E5\D3+\1BO\1F\C8\1C\C0\91V\1Cj]O\15\AFW\D1\15\05\9B2\12\8A\15\AC\12 \D9+1\BB|\B51\11\B0V6\9E>\C864\F2\D1?\F1\F8O?[4\AC8\D4\BA28~v\9E*^\AF\00*\F4c\E3-{\ED}-\D1!d$\14+\FA$\BE\E7\19#1i\87#\9B\A5Vbv\F9\C8b\DC5+eS\BB\B5e\F9w\ACl<}2l\96\B1\D1k\19?Ok\B3\F3\E3y\93*}y9\E6\9E~\B6h\00~\1C\A4\19w\D9\AE\87wsbdp\FC\EC\FApV }S\CD\85\E3SgI\00T\E8\C7\9ETB\0B\87]\87\01\19]-\CD\FAZ\A2CdZ\08\8F\C8H(VVH\82\9A\B5O\0D\14+O\A7\D82Fb\D2\ACF\C8\1EOAG\90\D1A\ED\\\ED\C2\9D)s\C27\E5\90\C5\B8k\0E\C5\12\A7\17\CC\D7\AD\89\CC}aj\CB\F2\EF\F4\CBX#X\D9x\FA\C6\D9\D26%\DE]\B8\BB\DE\F7t\A2\D72~<\D7\98\B2\DF\D0\17<A\D0\BD\F0\C6\F3&UX\F3\8C\99\BB\F4\03\17%\F4\A9\DB<\FDl\D1\A2\FD\C6\1DA\FAI\93\DF\FA\E3_s\E8\C3\86\ED\E8iJ\0E\EF\E6\C4\90\EFL\08\89\E6\89\02\17\E6#\CE\F4\E1\AC@j\E1\06\8C\BB\A0\EB\D0%\A0A\1C\C6\A7\CE\92X\A7d^A\AE\A1T\DF\AE\0B\98<\A9\84\16\A2\A9.\DA\0E\BB\0E\03\90\BB\A4\CFs\BC+A\ED\BC\81\8D\F4\B5D\87j\B5\EEK\89\B2a\C5\17\B2\CB\09\90\91P\AC\0E\91\FA`\ED\96u\EEs\96\DF\22j\9F\1A(\F4\9F\B0\E4\17\98?j\89\98\95\A6%\8A\B5\7F\BB\8A\1F\B3X\8D\90=\C6\8D:\F1\DF\84\FF\FBA\84U7\A2\83\DA\B9<\83pu\DA\85;SD\85\91\9F\A7\82\1E\119\82\B4\DD \8Bq\D7\BE\8B\DB\1B]\8CT\95\C3\8C\FEYo\9E\DE\80\F1\9EtL\12\99\FB\C2\8C\99Q\0E\95\90\94\04\0B\90>\C8\E8\97\B1Fv\97\1B\8A\F1\B4\80/o\B4*\E3\8C\B3\A5m\12\B3\0F\A1\0B\BA\CA\AB\95\BA`gv\BD\EF\E9\E8\BDE%D\AFe\FC\DA\AF\CF09\A8@\BE\A7\A8\EAr\BE\A1/x \A1\85\B4\C3\A6\0A:]\A6\A0\F6\8C\E7M\AA\12\E7\E7f\F1\E0h\E8o\E0\C2$v\E9\07.\E8\E9\AD\E2\0B\EE\22l\95\EE\88\A09\FC\A8y\A7\FC\02\B5D\FB\8D;\DA\FB'\F7\C3\F2\E2\FD]\F2H1\BE\F5\C7\BF \F5ms\A7\D6\F6\D69\D6\\\1A\DA\D1\D3\94D\D1yX]\D8\BCR\C3\D8\16\9E \DF\99\10\BE\DF3\DC\12\CD\13\05\8C\CD\B9\C9o\CA6G\F1\CA\9C\8B\E8\C3Y\81v\C3\F3M\95\C4|\C3\0B\C4\D6\0F7G\A6z\A9G\0C\B6J@\838\D4@)\F4\CDI\EC\FESIF2\B0N\C9\BC.Ncp\82\\C\A9\1C\\\E9e\FF[f\EBa[\CC'xR\09-\E6R\A3\E1\05U,o\9BU\86\A3\1Cv\1D\06\82v\B7\CAaq8D\FFq\92\88\E6xW\82xx\FDN\9B\7Fr\C0\05\7F\D8\0C\A9m\F8\D57mR\19\D4j\DD\97Jjw[Sc\B2Q\CDc\18\9D.d\97\13\B0d=\DFa%\D0\83\FF%zO\1C\22\F5\C1\82\22_\0D\9B+\9A\07\05+0\CB\E6,\BFEx,\15\89\D4>5PJ>\9F\9C\A99\10\1279\BA\DE.0\7F\D4\B00\D5\18S7Z\96\CD7\F0ZJ\14k\FF\D4\14\C137\13N\BD\A9\13\E4q\B0\1A!{.\1A\8B\B7\CD\1D\049S\1D\AE\F5\FF\0F\8E,a\0F$\E0\82\08\ABn\1C\08\01\A2\05\01\C4\A8\9B\01ndx\06\E1\EA\E6\06K&\00\00\00\00\09\00\00\00\00\00\00\00\00\00\00\00Unknown error %d\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00(\00\00\00N\00\00\00U\00\00\00L\00\00\00L\00\00\00)\00\00\00\00\00\00\00%+.2d\00%+.2d\00\00\00\00\00\00\00\00\00\01\00\00\00\05\00\00\00\19\00\00\00}\00\00\00q\02\00\005\0C\00\00\09=\00\00-1\01\00\E1\F5\05\00e\CD\1D\00\F9\02\95\00\DD\0E\E9\02QJ\8D\0E\95s\C2H\14\00\00\00\14\00\00\00\03\00\00\00\03\00\00\00\05\00\00\00\05\00\00\00\0A\00\00\00\0A\00\00\00\14\00\00\00\14\00\00\00\03\00\00\00\03\00\00\00\0A\00\00\00\0A\00\00\00\0A\00\00\00\0A\00\00\00\14\00\00\00\14\00\00\00A\00\00\00\09\00\00\00\11\00\00\00!\00\00\00A\00\00\00\09\00\00\00!\00\00\00!\00\00\00A\00\00\00\16\00\00\00\03\00\00\00\06\00\00\00\0B\00\00\00\16\00\00\00\03\00\00\00\0B\00\00\00\0B\00\00\00\16\00\00\00\11\00\00\00\03\00\00\00\05\00\00\00\09\00\00\00\11\00\00\00\03\00\00\00\09\00\00\00\09\00\00\00\11\00\00\00", section ".addr_0x400", align 1
@__notdec_mem0_0x3b20 = internal global [800 x i8] c"\10\00\00\00\08\00\00\00\06\00\00\00\FF\FF\FF\FF\FF\FF\FF\FF\FF\FF\FF\FF\08\00\00\00\FF\FF\FF\FF\00\00\00\00\11\04\00\00!\04\00\00$\04\00\00\F8\0A\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\11\04\00\00!\04\00\00\19\04\00\00\0C\04\00\00\15\04\00\00$\04\00\00\1E\04\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\03\00\04\00\05\00\06\00\07\00\08\00\09\00\0A\00\0B\00\0D\00\0F\00\11\00\13\00\17\00\1B\00\1F\00#\00+\003\00;\00C\00S\00c\00s\00\83\00\A3\00\C3\00\E3\00\02\01\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\01\00\01\00\01\00\01\00\02\00\02\00\02\00\02\00\03\00\03\00\03\00\03\00\04\00\04\00\04\00\04\00\05\00\05\00\05\00\05\00\00\00c\00c\00\00\00\01\00\02\00\03\00\04\00\05\00\07\00\09\00\0D\00\11\00\19\00!\001\00A\00a\00\81\00\C1\00\01\01\81\01\01\02\01\03\01\04\01\06\01\08\01\0C\01\10\01\18\01 \010\01@\01`\00\00\00\00\00\00\00\00\00\00\00\00\01\00\01\00\02\00\02\00\03\00\03\00\04\00\04\00\05\00\05\00\06\00\06\00\07\00\07\00\08\00\08\00\09\00\09\00\0A\00\0A\00\0B\00\0B\00\0C\00\0C\00\0D\00\0D\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\01\00\00\00\01\00\00\00\01\00\00\00\01\00\00\00\02\00\00\00\02\00\00\00\02\00\00\00\02\00\00\00\03\00\00\00\03\00\00\00\03\00\00\00\03\00\00\00\04\00\00\00\04\00\00\00\04\00\00\00\04\00\00\00\05\00\00\00\05\00\00\00\05\00\00\00\05\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\01\00\00\00\01\00\00\00\02\00\00\00\02\00\00\00\03\00\00\00\03\00\00\00\04\00\00\00\04\00\00\00\05\00\00\00\05\00\00\00\06\00\00\00\06\00\00\00\07\00\00\00\07\00\00\00\08\00\00\00\08\00\00\00\09\00\00\00\09\00\00\00\0A\00\00\00\0A\00\00\00\0B\00\00\00\0B\00\00\00\0C\00\00\00\0C\00\00\00\0D\00\00\00\0D\00\00\00\80\96\0C\00\E0\81\0C\00\90<\00\00\01\01\00\00\1E\01\00\00\0F\00\00\00\00\00\00\00\80\9F\0C\00@}\0C\00\10=\00\00\00\00\00\00\1E\00\00\00\0F\00\00\00\00\00\00\00\80\A0\0C\00\00\00\00\00\E0=\00\00\00\00\00\00\13\00\00\00\07\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\02\00\00\00\03\00\00\00\07\00\00\00\80\00\00\00Z\0B\00\00\01\00\00\00\01\00\00\00?\00\00\00", section ".addr_0x3b20", align 1
@table_0 = global [10 x void ()*] [void ()* null, void ()* bitcast (i32 (i32, i32)* @file_read to void ()*), void ()* bitcast (void (i32)* @abort_gzip_signal to void ()*), void ()* bitcast (i32 (i32, i32)* @unzip to void ()*), void ()* bitcast (i32 (i32, i32)* @unpack to void ()*), void ()* bitcast (i32 (i32, i32)* @unlzw to void ()*), void ()* bitcast (i32 (i32, i32)* @unlzh to void ()*), void ()* bitcast (i32 (i32, i32)* @copy to void ()*), void ()* bitcast (i32 (i32, i32)* @zip to void ()*), void ()* bitcast (i32 (i32, i32, i32)* @direntry_cmp_name to void ()*)]

declare i32 @memset(i32, i32, i32)

declare i32 @memcpy(i32, i32, i32)

declare i32 @strlen(i32)

declare i32 @strcmp(i32, i32)

declare i32 @atoi(i32)

declare i32 @close(i32)

declare i32 @fdatasync(i32)

declare i32 @__errno_location()

declare i32 @strcpy(i32, i32)

declare void @exit(i32)

declare i32 @strcat(i32, i32)

declare i32 @closedir(i32)

declare i32 @memcmp(i32, i32, i32)

declare i32 @sigemptyset(i32)

declare i32 @sigaction(i32, i32, i32)

declare i32 @sigaddset(i32, i32)

declare i32 @sigismember(i32, i32)

declare i32 @sigprocmask(i32, i32, i32)

declare i32 @isatty(i32)

declare i32 @strcspn(i32, i32)

declare i32 @strrchr(i32, i32)

declare i32 @fsync(i32)

declare i32 @fstat(i32, i32)

declare i32 @localtime(i32)

declare i32 @memmove(i32, i32, i32)

declare i32 @fchown(i32, i32, i32)

declare i32 @fchmod(i32, i32)

declare i32 @signal(i32, i32)

declare i32 @raise(i32)

declare i32 @write(i32, i32, i32)

declare i32 @read(i32, i32, i32)

declare i32 @tolower(i32)

declare i32 @getenv(i32)

declare i32 @strspn(i32, i32)

declare i32 @fileno(i32)

declare i32 @fclose(i32)

declare i32 @__freading(i32)

declare i64 @lseek(i32, i64, i32)

declare i32 @fcntl(i32, i32, i32)

declare i32 @open(i32, i32, i32)

declare i32 @fdopendir(i32)

declare i32 @fflush(i32)

declare i64 @ftello(i32)

declare i32 @__fpurge(i32)

declare void @free(i32)

declare i32 @fseeko(i32, i64, i32)

declare i32 @strchr(i32, i32)

declare i32 @strncmp(i32, i32, i32)

declare void @flockfile(i32)

declare void @funlockfile(i32)

declare i32 @lstat(i32, i32)

declare i32 @openat(i32, i32, i32, i32)

declare i32 @readdir(i32)

declare void @qsort_r(i32, i32, i32, i32, i32)

declare i32 @stpcpy(i32, i32)

declare i32 @stat(i32, i32)

declare i32 @strerror(i32)

declare i32 @sprintf(i32, i32, i32)

declare void @abort()

declare i32 @unlink(i32)

declare i32 @fstatat(i32, i32, i32, i32)

declare i32 @unlinkat(i32, i32, i32)

declare i32 @utimensat(i32, i32, i32, i32)

declare i32 @futimens(i32, i32)

declare i32 @clock_gettime(i32, i32)

declare i32 @futimesat(i32, i32, i32)

declare i32 @utime(i32, i32)

declare i32 @realloc(i32, i32)

declare i32 @getchar()

declare i32 @calloc(i32, i32)

declare void @__fseterr(i32)

declare i32 @fwrite(i32, i32, i32, i32)

declare i32 @malloc(i32)

declare i32 @wcrtomb(i32, i32, i32)

declare i32 @wcslen(i32)

declare i32 @__unordtf2(i64, i64, i64, i64)

declare void @copysignl(i32, i64, i64, i64, i64)

declare i32 @__lttf2(i64, i64, i64, i64)

declare void @__addtf3(i32, i64, i64, i64, i64)

declare i32 @__gttf2(i64, i64, i64, i64)

declare i32 @__netf2(i64, i64, i64, i64)

declare void @frexpl(i32, i64, i64, i32)

declare void @ldexpl(i32, i64, i64, i32)

declare i32 @__fixtfsi(i64, i64)

declare void @__floatsitf(i32, i32)

declare void @__subtf3(i32, i64, i64, i64, i64)

declare void @__multf3(i32, i64, i64, i64, i64)

declare double @copysign(double, double)

declare double @frexp(double, i32)

declare double @ldexp(double, i32)

declare i32 @__eqtf2(i64, i64, i64, i64)

declare i32 @snprintf(i32, i32, i32, i32)

declare i32 @__ctype_get_mb_cur_max()

declare i32 @strnlen(i32, i32)

declare i32 @__getf2(i64, i64, i64, i64)

declare double @__trunctfdf2(i64, i64)

declare void @__floatunsitf(i32, i32)

; Function Attrs: null_pointer_is_valid
define internal void @bi_init(i32 %_arg_0) #0 {
allocator:
  store i32 0, i32* inttoptr (i32 16388 to i32*), align 4
  store i16 0, i16* inttoptr (i32 16384 to i16*), align 16384
  %0 = icmp eq i32 %_arg_0, -1
  br i1 %0, label %blk_exit, label %brif_next

blk_exit:                                         ; preds = %brif_next, %allocator
  ret void

brif_next:                                        ; preds = %allocator
  store i32 1, i32* inttoptr (i32 16392 to i32*), align 8
  br label %blk_exit
}

; Function Attrs: null_pointer_is_valid
declare dso_local void @send_bits(i32, i32) #0

; Function Attrs: null_pointer_is_valid
define internal i32 @bi_reverse(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %0 = add i32 %_arg_1, 1
  br label %loop_entry

loop_entry:                                       ; preds = %loop_entry, %allocator
  %_local_2.0 = phi i32 [ 0, %allocator ], [ %3, %loop_entry ]
  %_param_1.0 = phi i32 [ %0, %allocator ], [ %5, %loop_entry ]
  %_param_0.0 = phi i32 [ %_arg_0, %allocator ], [ %4, %loop_entry ]
  %1 = and i32 %_param_0.0, 1
  %2 = or i32 %_local_2.0, %1
  %3 = shl i32 %2, 1
  %4 = lshr i32 %_param_0.0, 1
  %5 = add i32 %_param_1.0, -1
  %6 = icmp sgt i32 %5, 1
  br i1 %6, label %loop_entry, label %brif_next

brif_next:                                        ; preds = %loop_entry
  %7 = and i32 %2, 2147483647
  ret i32 %7
}

; Function Attrs: null_pointer_is_valid
declare dso_local void @bi_windup() #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @copy_block(i32, i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i64 @gzip_deflate(i32) #0

; Function Attrs: null_pointer_is_valid
define internal i32 @longest_match(i32 %_arg_0) #0 {
allocator:
  %0 = load i32, i32* inttoptr (i32 16432 to i32*), align 16
  %1 = lshr i32 %0, 2
  %2 = load i32, i32* inttoptr (i32 16408 to i32*), align 8
  %3 = load i32, i32* inttoptr (i32 16436 to i32*), align 4
  %4 = icmp ult i32 %2, %3
  %5 = select i1 %4, i32 %0, i32 %1
  %6 = load i32, i32* inttoptr (i32 16404 to i32*), align 4
  %7 = call i32 @llvm.usub.sat.i32(i32 %6, i32 32506)
  %8 = add i32 %6, 618496
  %9 = add i32 %6, 618754
  %10 = add i32 %8, %2
  %11 = inttoptr i32 %10 to i8*
  %12 = load i8, i8* %11, align 1
  %13 = zext i8 %12 to i32
  %14 = add i32 %10, -1
  %15 = inttoptr i32 %14 to i8*
  %16 = load i8, i8* %15, align 1
  %17 = zext i8 %16 to i32
  %18 = load i32, i32* inttoptr (i32 16444 to i32*), align 4
  br label %loop_entry

return:                                           ; preds = %brif_next53, %blk_exit
  %return_0 = phi i32 [ %44, %brif_next53 ], [ %_local_2.1, %blk_exit ]
  ret i32 %return_0

loop_entry:                                       ; preds = %blk_exit, %allocator
  %_local_9.0 = phi i32 [ %17, %allocator ], [ %_local_9.1, %blk_exit ]
  %_local_8.0 = phi i32 [ %13, %allocator ], [ %_local_8.1, %blk_exit ]
  %_local_3.0 = phi i32 [ %5, %allocator ], [ %30, %blk_exit ]
  %_local_2.0 = phi i32 [ %2, %allocator ], [ %_local_2.1, %blk_exit ]
  %_param_0.0 = phi i32 [ %_arg_0, %allocator ], [ %29, %blk_exit ]
  %19 = add i32 %_param_0.0, 618496
  %20 = add i32 %19, %_local_2.0
  %21 = inttoptr i32 %20 to i8*
  %22 = load i8, i8* %21, align 1
  %23 = trunc i32 %_local_8.0 to i8
  %.not = icmp eq i8 %22, %23
  br i1 %.not, label %brif_next, label %blk_exit

blk_exit:                                         ; preds = %blk_exit54, %blk_exit15, %brif_next10, %brif_next6, %brif_next, %loop_entry
  %_local_9.1 = phi i32 [ %109, %blk_exit54 ], [ %_local_9.0, %blk_exit15 ], [ %_local_9.0, %brif_next10 ], [ %_local_9.0, %brif_next6 ], [ %_local_9.0, %brif_next ], [ %_local_9.0, %loop_entry ]
  %_local_8.1 = phi i32 [ %104, %blk_exit54 ], [ %_local_8.0, %blk_exit15 ], [ %_local_8.0, %brif_next10 ], [ %_local_8.0, %brif_next6 ], [ %_local_8.0, %brif_next ], [ %_local_8.0, %loop_entry ]
  %_local_2.1 = phi i32 [ %44, %blk_exit54 ], [ %_local_2.0, %blk_exit15 ], [ %_local_2.0, %brif_next10 ], [ %_local_2.0, %brif_next6 ], [ %_local_2.0, %brif_next ], [ %_local_2.0, %loop_entry ]
  %24 = shl i32 %_param_0.0, 1
  %25 = and i32 %24, 65534
  %26 = add nuw nsw i32 %25, 684032
  %27 = inttoptr i32 %26 to i16*
  %28 = load i16, i16* %27, align 2
  %29 = zext i16 %28 to i32
  %.not77 = icmp uge i32 %7, %29
  %30 = add i32 %_local_3.0, -1
  %brif_val63.not = icmp eq i32 %30, 0
  %or.cond79 = select i1 %.not77, i1 true, i1 %brif_val63.not
  br i1 %or.cond79, label %return, label %loop_entry

brif_next:                                        ; preds = %loop_entry
  %31 = add i32 %20, -1
  %32 = inttoptr i32 %31 to i8*
  %33 = load i8, i8* %32, align 1
  %34 = trunc i32 %_local_9.0 to i8
  %.not65 = icmp eq i8 %33, %34
  br i1 %.not65, label %brif_next6, label %blk_exit

brif_next6:                                       ; preds = %brif_next
  %35 = inttoptr i32 %19 to i8*
  %36 = load i8, i8* %35, align 1
  %37 = inttoptr i32 %8 to i8*
  %38 = load i8, i8* %37, align 1
  %.not66 = icmp eq i8 %36, %38
  br i1 %.not66, label %brif_next10, label %blk_exit

brif_next10:                                      ; preds = %brif_next6
  %calcOffset11 = add i32 %_param_0.0, 618497
  %39 = inttoptr i32 %calcOffset11 to i8*
  %40 = load i8, i8* %39, align 1
  %calcOffset12 = add i32 %6, 618497
  %41 = inttoptr i32 %calcOffset12 to i8*
  %42 = load i8, i8* %41, align 1
  %.not67 = icmp eq i8 %40, %42
  br i1 %.not67, label %loop_entry16, label %blk_exit

blk_exit15:                                       ; preds = %blk_exit45, %brif_next40, %brif_next36, %brif_next32, %brif_next28, %brif_next24, %brif_next20, %loop_entry16
  %_local_12.0 = phi i32 [ %99, %blk_exit45 ], [ %83, %brif_next40 ], [ %77, %brif_next36 ], [ %71, %brif_next32 ], [ %65, %brif_next28 ], [ %59, %brif_next24 ], [ %53, %brif_next20 ], [ %46, %loop_entry16 ]
  %43 = sub i32 %_local_12.0, %9
  %44 = add i32 %43, 258
  %.not76 = icmp sgt i32 %44, %_local_2.0
  br i1 %.not76, label %brif_next53, label %blk_exit

loop_entry16:                                     ; preds = %brif_next44, %brif_next10
  %_local_11.0 = phi i32 [ 0, %brif_next10 ], [ %89, %brif_next44 ]
  %45 = add i32 %8, %_local_11.0
  %46 = add i32 %45, 3
  %47 = inttoptr i32 %46 to i8*
  %48 = load i8, i8* %47, align 1
  %49 = add i32 %_param_0.0, %_local_11.0
  %50 = add i32 %49, 618499
  %51 = inttoptr i32 %50 to i8*
  %52 = load i8, i8* %51, align 1
  %.not68 = icmp eq i8 %48, %52
  br i1 %.not68, label %brif_next20, label %blk_exit15

brif_next20:                                      ; preds = %loop_entry16
  %53 = add i32 %45, 4
  %54 = inttoptr i32 %53 to i8*
  %55 = load i8, i8* %54, align 1
  %56 = add i32 %49, 618500
  %57 = inttoptr i32 %56 to i8*
  %58 = load i8, i8* %57, align 1
  %.not69 = icmp eq i8 %55, %58
  br i1 %.not69, label %brif_next24, label %blk_exit15

brif_next24:                                      ; preds = %brif_next20
  %59 = add i32 %45, 5
  %60 = inttoptr i32 %59 to i8*
  %61 = load i8, i8* %60, align 1
  %62 = add i32 %49, 618501
  %63 = inttoptr i32 %62 to i8*
  %64 = load i8, i8* %63, align 1
  %.not70 = icmp eq i8 %61, %64
  br i1 %.not70, label %brif_next28, label %blk_exit15

brif_next28:                                      ; preds = %brif_next24
  %65 = add i32 %45, 6
  %66 = inttoptr i32 %65 to i8*
  %67 = load i8, i8* %66, align 1
  %68 = add i32 %49, 618502
  %69 = inttoptr i32 %68 to i8*
  %70 = load i8, i8* %69, align 1
  %.not71 = icmp eq i8 %67, %70
  br i1 %.not71, label %brif_next32, label %blk_exit15

brif_next32:                                      ; preds = %brif_next28
  %71 = add i32 %45, 7
  %72 = inttoptr i32 %71 to i8*
  %73 = load i8, i8* %72, align 1
  %74 = add i32 %49, 618503
  %75 = inttoptr i32 %74 to i8*
  %76 = load i8, i8* %75, align 1
  %.not72 = icmp eq i8 %73, %76
  br i1 %.not72, label %brif_next36, label %blk_exit15

brif_next36:                                      ; preds = %brif_next32
  %77 = add i32 %45, 8
  %78 = inttoptr i32 %77 to i8*
  %79 = load i8, i8* %78, align 1
  %80 = add i32 %49, 618504
  %81 = inttoptr i32 %80 to i8*
  %82 = load i8, i8* %81, align 1
  %.not73 = icmp eq i8 %79, %82
  br i1 %.not73, label %brif_next40, label %blk_exit15

brif_next40:                                      ; preds = %brif_next36
  %83 = add i32 %45, 9
  %84 = inttoptr i32 %83 to i8*
  %85 = load i8, i8* %84, align 1
  %86 = add i32 %49, 618505
  %87 = inttoptr i32 %86 to i8*
  %88 = load i8, i8* %87, align 1
  %.not74 = icmp eq i8 %85, %88
  br i1 %.not74, label %brif_next44, label %blk_exit15

brif_next44:                                      ; preds = %brif_next40
  %89 = add i32 %_local_11.0, 8
  %90 = add i32 %45, 10
  %91 = inttoptr i32 %90 to i8*
  %92 = load i8, i8* %91, align 1
  %93 = add i32 %49, 618506
  %94 = inttoptr i32 %93 to i8*
  %95 = load i8, i8* %94, align 1
  %.not75 = icmp eq i8 %92, %95
  %96 = or i32 %_local_11.0, 2
  %97 = icmp ult i32 %96, 250
  %or.cond = select i1 %.not75, i1 %97, i1 false
  br i1 %or.cond, label %loop_entry16, label %blk_exit45

blk_exit45:                                       ; preds = %brif_next44
  %98 = add i32 %8, %89
  %99 = add i32 %98, 2
  br label %blk_exit15

brif_next53:                                      ; preds = %blk_exit15
  store i32 %_param_0.0, i32* inttoptr (i32 16412 to i32*), align 4
  %100 = icmp slt i32 %44, %18
  br i1 %100, label %blk_exit54, label %return

blk_exit54:                                       ; preds = %brif_next53
  %101 = add i32 %8, %44
  %102 = inttoptr i32 %101 to i8*
  %103 = load i8, i8* %102, align 1
  %104 = zext i8 %103 to i32
  %105 = add i32 %43, %8
  %106 = add i32 %105, 257
  %107 = inttoptr i32 %106 to i8*
  %108 = load i8, i8* %107, align 1
  %109 = zext i8 %108 to i32
  br label %blk_exit
}

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @main(i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @treat_file(i32) #0

; Function Attrs: null_pointer_is_valid
define internal void @do_exit(i32 %_arg_0) #0 {
allocator:
  %0 = load i8, i8* inttoptr (i32 818440 to i8*), align 8
  %1 = icmp eq i8 %0, 0
  br i1 %1, label %blk_exit, label %brif_next

blk_exit:                                         ; preds = %allocator
  store i8 1, i8* inttoptr (i32 818440 to i8*), align 8
  %2 = load i32, i32* inttoptr (i32 16472 to i32*), align 8
  call void @rpl_free(i32 %2)
  store i32 0, i32* inttoptr (i32 16472 to i32*), align 8
  call void @exit(i32 %_arg_0)
  unreachable

brif_next:                                        ; preds = %allocator
  call void @exit(i32 %_arg_0)
  unreachable
}

; Function Attrs: null_pointer_is_valid
declare dso_local void @try_help() #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @treat_stdin() #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @do_list(i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @open_and_stat() #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @get_suffix(i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @progerror(i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @get_method(i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @abort_gzip_signal(i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @copy_stat() #0

; Function Attrs: null_pointer_is_valid
define internal i32 @atdir_eq(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %select_cond.not = icmp eq i32 %_arg_1, 0
  %0 = select i1 %select_cond.not, i32 6336, i32 %_arg_0
  %1 = select i1 %select_cond.not, i32 1, i32 %_arg_1
  %2 = call i32 @memcmp(i32 816352, i32 %0, i32 %1)
  %brif_val.not = icmp eq i32 %2, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit

blk_exit:                                         ; preds = %brif_next, %allocator
  %_local_2.0 = phi i32 [ %7, %brif_next ], [ 0, %allocator ]
  ret i32 %_local_2.0

brif_next:                                        ; preds = %allocator
  %3 = add i32 %1, 816352
  %4 = inttoptr i32 %3 to i8*
  %5 = load i8, i8* %4, align 1
  %6 = icmp eq i8 %5, 0
  %7 = zext i1 %6 to i32
  br label %blk_exit
}

; Function Attrs: null_pointer_is_valid
declare dso_local void @remove_output_file(i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @finish_up_gzip(i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @abort_gzip() #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @discard_input_bytes(i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @gzip_inflate() #0

; Function Attrs: null_pointer_is_valid
define internal i32 @huft_build(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2, i32 %_arg_3, i32 %_arg_4, i32 %_arg_5, i32 %_arg_6) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -1376
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = add i32 %0, -80
  %3 = call i32 @memset(i32 %2, i32 0, i32 68)
  %4 = add i32 %_arg_1, -1
  %5 = and i32 %_arg_1, 3
  %6 = icmp eq i32 %5, 0
  br i1 %6, label %blk_exit, label %loop_entry

blk_exit:                                         ; preds = %loop_entry, %allocator
  %_local_10.0 = phi i32 [ %_arg_0, %allocator ], [ %17, %loop_entry ]
  %_local_9.0 = phi i32 [ %_arg_1, %allocator ], [ %16, %loop_entry ]
  %7 = icmp ult i32 %4, 3
  br i1 %7, label %blk_exit.blk_exit5_crit_edge, label %loop_entry8

blk_exit.blk_exit5_crit_edge:                     ; preds = %blk_exit
  %.pre = add i32 %0, -80
  br label %blk_exit5

loop_entry:                                       ; preds = %loop_entry, %allocator
  %_local_11.0 = phi i32 [ %18, %loop_entry ], [ %5, %allocator ]
  %_local_10.1 = phi i32 [ %17, %loop_entry ], [ %_arg_0, %allocator ]
  %_local_9.1 = phi i32 [ %16, %loop_entry ], [ %_arg_1, %allocator ]
  %8 = add i32 %0, -80
  %9 = inttoptr i32 %_local_10.1 to i32*
  %10 = load i32, i32* %9, align 4
  %11 = shl i32 %10, 2
  %12 = add i32 %8, %11
  %13 = inttoptr i32 %12 to i32*
  %14 = load i32, i32* %13, align 4
  %15 = add i32 %14, 1
  store i32 %15, i32* %13, align 4
  %16 = add i32 %_local_9.1, -1
  %17 = add i32 %_local_10.1, 4
  %18 = add i32 %_local_11.0, -1
  %brif_val3.not = icmp eq i32 %18, 0
  br i1 %brif_val3.not, label %blk_exit, label %loop_entry

blk_exit5:                                        ; preds = %loop_entry8, %blk_exit.blk_exit5_crit_edge
  %calcOffset25.pre-phi = phi i32 [ %.pre, %blk_exit.blk_exit5_crit_edge ], [ %21, %loop_entry8 ]
  %19 = inttoptr i32 %calcOffset25.pre-phi to i32*
  %20 = load i32, i32* %19, align 4
  %.not = icmp eq i32 %20, %_arg_1
  br i1 %.not, label %brif_next27, label %blk_exit24

loop_entry8:                                      ; preds = %loop_entry8, %blk_exit
  %_local_10.2 = phi i32 [ %_local_10.0, %blk_exit ], [ %53, %loop_entry8 ]
  %_local_9.2 = phi i32 [ %_local_9.0, %blk_exit ], [ %54, %loop_entry8 ]
  %21 = add i32 %0, -80
  %22 = inttoptr i32 %_local_10.2 to i32*
  %23 = load i32, i32* %22, align 4
  %24 = shl i32 %23, 2
  %25 = add i32 %21, %24
  %26 = inttoptr i32 %25 to i32*
  %27 = load i32, i32* %26, align 4
  %28 = add i32 %27, 1
  store i32 %28, i32* %26, align 4
  %29 = add i32 %_local_10.2, 4
  %30 = inttoptr i32 %29 to i32*
  %31 = load i32, i32* %30, align 4
  %32 = shl i32 %31, 2
  %33 = add i32 %21, %32
  %34 = inttoptr i32 %33 to i32*
  %35 = load i32, i32* %34, align 4
  %36 = add i32 %35, 1
  store i32 %36, i32* %34, align 4
  %37 = add i32 %_local_10.2, 8
  %38 = inttoptr i32 %37 to i32*
  %39 = load i32, i32* %38, align 4
  %40 = shl i32 %39, 2
  %41 = add i32 %21, %40
  %42 = inttoptr i32 %41 to i32*
  %43 = load i32, i32* %42, align 4
  %44 = add i32 %43, 1
  store i32 %44, i32* %42, align 4
  %45 = add i32 %_local_10.2, 12
  %46 = inttoptr i32 %45 to i32*
  %47 = load i32, i32* %46, align 4
  %48 = shl i32 %47, 2
  %49 = add i32 %21, %48
  %50 = inttoptr i32 %49 to i32*
  %51 = load i32, i32* %50, align 4
  %52 = add i32 %51, 1
  store i32 %52, i32* %50, align 4
  %53 = add i32 %_local_10.2, 16
  %54 = add i32 %_local_9.2, -4
  %brif_val21.not = icmp eq i32 %54, 0
  br i1 %brif_val21.not, label %blk_exit5, label %loop_entry8

blk_exit23:                                       ; preds = %loop_entry242, %brif_next238, %brif_next236, %blk_exit204, %loop_entry136, %blk_exit133, %blk_exit28, %brif_next27
  %_local_12.0 = phi i32 [ 0, %blk_exit28 ], [ %246, %blk_exit204 ], [ 3, %brif_next236 ], [ 3, %brif_next238 ], [ 3, %loop_entry242 ], [ 2, %blk_exit133 ], [ 2, %loop_entry136 ], [ 3, %brif_next27 ]
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %_local_12.0

blk_exit24:                                       ; preds = %blk_exit5
  %55 = inttoptr i32 %_arg_6 to i32*
  %56 = load i32, i32* %55, align 4
  %calcOffset38 = add i32 %0, -76
  %57 = inttoptr i32 %calcOffset38 to i32*
  %58 = load i32, i32* %57, align 4
  %brif_val39.not = icmp eq i32 %58, 0
  br i1 %brif_val39.not, label %brif_next40, label %blk_exit37

brif_next27:                                      ; preds = %blk_exit5
  %59 = call i32 @rpl_malloc(i32 24)
  %brif_val29.not = icmp eq i32 %59, 0
  br i1 %brif_val29.not, label %blk_exit23, label %blk_exit28

blk_exit28:                                       ; preds = %brif_next27
  %60 = load i32, i32* inttoptr (i32 818456 to i32*), align 8
  %61 = add i32 %60, 3
  store i32 %61, i32* inttoptr (i32 818456 to i32*), align 8
  %calcOffset31 = add i32 %59, 16
  %62 = inttoptr i32 %calcOffset31 to i16*
  store i16 355, i16* %62, align 2
  %calcOffset32 = add i32 %59, 8
  %63 = inttoptr i32 %calcOffset32 to i16*
  store i16 355, i16* %63, align 2
  %calcOffset33 = add i32 %59, 4
  %64 = inttoptr i32 %calcOffset33 to i32*
  store i32 0, i32* %64, align 4
  %65 = inttoptr i32 %_arg_5 to i32*
  store i32 %calcOffset32, i32* %65, align 4
  %66 = inttoptr i32 %_arg_6 to i32*
  store i32 1, i32* %66, align 4
  br label %blk_exit23

blk_exit37:                                       ; preds = %brif_next82, %brif_next79, %brif_next76, %brif_next73, %brif_next70, %brif_next67, %brif_next64, %brif_next61, %brif_next58, %brif_next55, %brif_next52, %brif_next49, %brif_next46, %brif_next43, %brif_next40, %blk_exit24
  %_local_14.0 = phi i32 [ %101, %brif_next82 ], [ 15, %brif_next79 ], [ 14, %brif_next76 ], [ 13, %brif_next73 ], [ 12, %brif_next70 ], [ 11, %brif_next67 ], [ 10, %brif_next64 ], [ 9, %brif_next61 ], [ 8, %brif_next58 ], [ 7, %brif_next55 ], [ 6, %brif_next52 ], [ 5, %brif_next49 ], [ 4, %brif_next46 ], [ 3, %brif_next43 ], [ 2, %brif_next40 ], [ 1, %blk_exit24 ]
  %67 = icmp ult i32 %56, %_local_14.0
  %68 = select i1 %67, i32 %_local_14.0, i32 %56
  %calcOffset86 = add i32 %0, -16
  %69 = inttoptr i32 %calcOffset86 to i32*
  %70 = load i32, i32* %69, align 4
  %brif_val87.not = icmp eq i32 %70, 0
  br i1 %brif_val87.not, label %brif_next88, label %blk_exit85

brif_next40:                                      ; preds = %blk_exit24
  %calcOffset41 = add i32 %0, -72
  %71 = inttoptr i32 %calcOffset41 to i32*
  %72 = load i32, i32* %71, align 4
  %brif_val42.not = icmp eq i32 %72, 0
  br i1 %brif_val42.not, label %brif_next43, label %blk_exit37

brif_next43:                                      ; preds = %brif_next40
  %calcOffset44 = add i32 %0, -68
  %73 = inttoptr i32 %calcOffset44 to i32*
  %74 = load i32, i32* %73, align 4
  %brif_val45.not = icmp eq i32 %74, 0
  br i1 %brif_val45.not, label %brif_next46, label %blk_exit37

brif_next46:                                      ; preds = %brif_next43
  %calcOffset47 = add i32 %0, -64
  %75 = inttoptr i32 %calcOffset47 to i32*
  %76 = load i32, i32* %75, align 4
  %brif_val48.not = icmp eq i32 %76, 0
  br i1 %brif_val48.not, label %brif_next49, label %blk_exit37

brif_next49:                                      ; preds = %brif_next46
  %calcOffset50 = add i32 %0, -60
  %77 = inttoptr i32 %calcOffset50 to i32*
  %78 = load i32, i32* %77, align 4
  %brif_val51.not = icmp eq i32 %78, 0
  br i1 %brif_val51.not, label %brif_next52, label %blk_exit37

brif_next52:                                      ; preds = %brif_next49
  %calcOffset53 = add i32 %0, -56
  %79 = inttoptr i32 %calcOffset53 to i32*
  %80 = load i32, i32* %79, align 4
  %brif_val54.not = icmp eq i32 %80, 0
  br i1 %brif_val54.not, label %brif_next55, label %blk_exit37

brif_next55:                                      ; preds = %brif_next52
  %calcOffset56 = add i32 %0, -52
  %81 = inttoptr i32 %calcOffset56 to i32*
  %82 = load i32, i32* %81, align 4
  %brif_val57.not = icmp eq i32 %82, 0
  br i1 %brif_val57.not, label %brif_next58, label %blk_exit37

brif_next58:                                      ; preds = %brif_next55
  %calcOffset59 = add i32 %0, -48
  %83 = inttoptr i32 %calcOffset59 to i32*
  %84 = load i32, i32* %83, align 4
  %brif_val60.not = icmp eq i32 %84, 0
  br i1 %brif_val60.not, label %brif_next61, label %blk_exit37

brif_next61:                                      ; preds = %brif_next58
  %calcOffset62 = add i32 %0, -44
  %85 = inttoptr i32 %calcOffset62 to i32*
  %86 = load i32, i32* %85, align 4
  %brif_val63.not = icmp eq i32 %86, 0
  br i1 %brif_val63.not, label %brif_next64, label %blk_exit37

brif_next64:                                      ; preds = %brif_next61
  %calcOffset65 = add i32 %0, -40
  %87 = inttoptr i32 %calcOffset65 to i32*
  %88 = load i32, i32* %87, align 4
  %brif_val66.not = icmp eq i32 %88, 0
  br i1 %brif_val66.not, label %brif_next67, label %blk_exit37

brif_next67:                                      ; preds = %brif_next64
  %calcOffset68 = add i32 %0, -36
  %89 = inttoptr i32 %calcOffset68 to i32*
  %90 = load i32, i32* %89, align 4
  %brif_val69.not = icmp eq i32 %90, 0
  br i1 %brif_val69.not, label %brif_next70, label %blk_exit37

brif_next70:                                      ; preds = %brif_next67
  %calcOffset71 = add i32 %0, -32
  %91 = inttoptr i32 %calcOffset71 to i32*
  %92 = load i32, i32* %91, align 4
  %brif_val72.not = icmp eq i32 %92, 0
  br i1 %brif_val72.not, label %brif_next73, label %blk_exit37

brif_next73:                                      ; preds = %brif_next70
  %calcOffset74 = add i32 %0, -28
  %93 = inttoptr i32 %calcOffset74 to i32*
  %94 = load i32, i32* %93, align 4
  %brif_val75.not = icmp eq i32 %94, 0
  br i1 %brif_val75.not, label %brif_next76, label %blk_exit37

brif_next76:                                      ; preds = %brif_next73
  %calcOffset77 = add i32 %0, -24
  %95 = inttoptr i32 %calcOffset77 to i32*
  %96 = load i32, i32* %95, align 4
  %brif_val78.not = icmp eq i32 %96, 0
  br i1 %brif_val78.not, label %brif_next79, label %blk_exit37

brif_next79:                                      ; preds = %brif_next76
  %calcOffset80 = add i32 %0, -20
  %97 = inttoptr i32 %calcOffset80 to i32*
  %98 = load i32, i32* %97, align 4
  %brif_val81.not = icmp eq i32 %98, 0
  br i1 %brif_val81.not, label %brif_next82, label %blk_exit37

brif_next82:                                      ; preds = %brif_next79
  %calcOffset83 = add i32 %0, -16
  %99 = inttoptr i32 %calcOffset83 to i32*
  %100 = load i32, i32* %99, align 4
  %select_cond.not = icmp eq i32 %100, 0
  %101 = select i1 %select_cond.not, i32 17, i32 16
  br label %blk_exit37

blk_exit85:                                       ; preds = %brif_next130, %brif_next127, %brif_next124, %brif_next121, %brif_next118, %brif_next115, %brif_next112, %brif_next109, %brif_next106, %brif_next103, %brif_next100, %brif_next97, %brif_next94, %brif_next91, %brif_next88, %blk_exit37
  %_local_16.0 = phi i32 [ %135, %brif_next130 ], [ 2, %brif_next127 ], [ 3, %brif_next124 ], [ 4, %brif_next121 ], [ 5, %brif_next118 ], [ 6, %brif_next115 ], [ 7, %brif_next112 ], [ 8, %brif_next109 ], [ 9, %brif_next106 ], [ 10, %brif_next103 ], [ 11, %brif_next100 ], [ 12, %brif_next97 ], [ 13, %brif_next94 ], [ 14, %brif_next91 ], [ 15, %brif_next88 ], [ 16, %blk_exit37 ]
  %_local_13.0 = phi i32 [ %133, %brif_next130 ], [ 1, %brif_next127 ], [ 1, %brif_next124 ], [ 1, %brif_next121 ], [ 1, %brif_next118 ], [ 1, %brif_next115 ], [ 1, %brif_next112 ], [ 1, %brif_next109 ], [ 1, %brif_next106 ], [ 1, %brif_next103 ], [ 1, %brif_next100 ], [ 1, %brif_next97 ], [ 1, %brif_next94 ], [ 1, %brif_next91 ], [ 1, %brif_next88 ], [ 1, %blk_exit37 ]
  %102 = icmp ugt i32 %68, %_local_16.0
  %103 = select i1 %102, i32 %_local_16.0, i32 %68
  store i32 %103, i32* %55, align 4
  %104 = shl i32 1, %_local_14.0
  %.not297 = icmp ugt i32 %_local_16.0, %_local_14.0
  br i1 %.not297, label %brif_next135, label %blk_exit133

brif_next88:                                      ; preds = %blk_exit37
  %calcOffset89 = add i32 %0, -20
  %105 = inttoptr i32 %calcOffset89 to i32*
  %106 = load i32, i32* %105, align 4
  %brif_val90.not = icmp eq i32 %106, 0
  br i1 %brif_val90.not, label %brif_next91, label %blk_exit85

brif_next91:                                      ; preds = %brif_next88
  %calcOffset92 = add i32 %0, -24
  %107 = inttoptr i32 %calcOffset92 to i32*
  %108 = load i32, i32* %107, align 4
  %brif_val93.not = icmp eq i32 %108, 0
  br i1 %brif_val93.not, label %brif_next94, label %blk_exit85

brif_next94:                                      ; preds = %brif_next91
  %calcOffset95 = add i32 %0, -28
  %109 = inttoptr i32 %calcOffset95 to i32*
  %110 = load i32, i32* %109, align 4
  %brif_val96.not = icmp eq i32 %110, 0
  br i1 %brif_val96.not, label %brif_next97, label %blk_exit85

brif_next97:                                      ; preds = %brif_next94
  %calcOffset98 = add i32 %0, -32
  %111 = inttoptr i32 %calcOffset98 to i32*
  %112 = load i32, i32* %111, align 4
  %brif_val99.not = icmp eq i32 %112, 0
  br i1 %brif_val99.not, label %brif_next100, label %blk_exit85

brif_next100:                                     ; preds = %brif_next97
  %calcOffset101 = add i32 %0, -36
  %113 = inttoptr i32 %calcOffset101 to i32*
  %114 = load i32, i32* %113, align 4
  %brif_val102.not = icmp eq i32 %114, 0
  br i1 %brif_val102.not, label %brif_next103, label %blk_exit85

brif_next103:                                     ; preds = %brif_next100
  %calcOffset104 = add i32 %0, -40
  %115 = inttoptr i32 %calcOffset104 to i32*
  %116 = load i32, i32* %115, align 4
  %brif_val105.not = icmp eq i32 %116, 0
  br i1 %brif_val105.not, label %brif_next106, label %blk_exit85

brif_next106:                                     ; preds = %brif_next103
  %calcOffset107 = add i32 %0, -44
  %117 = inttoptr i32 %calcOffset107 to i32*
  %118 = load i32, i32* %117, align 4
  %brif_val108.not = icmp eq i32 %118, 0
  br i1 %brif_val108.not, label %brif_next109, label %blk_exit85

brif_next109:                                     ; preds = %brif_next106
  %calcOffset110 = add i32 %0, -48
  %119 = inttoptr i32 %calcOffset110 to i32*
  %120 = load i32, i32* %119, align 4
  %brif_val111.not = icmp eq i32 %120, 0
  br i1 %brif_val111.not, label %brif_next112, label %blk_exit85

brif_next112:                                     ; preds = %brif_next109
  %calcOffset113 = add i32 %0, -52
  %121 = inttoptr i32 %calcOffset113 to i32*
  %122 = load i32, i32* %121, align 4
  %brif_val114.not = icmp eq i32 %122, 0
  br i1 %brif_val114.not, label %brif_next115, label %blk_exit85

brif_next115:                                     ; preds = %brif_next112
  %calcOffset116 = add i32 %0, -56
  %123 = inttoptr i32 %calcOffset116 to i32*
  %124 = load i32, i32* %123, align 4
  %brif_val117.not = icmp eq i32 %124, 0
  br i1 %brif_val117.not, label %brif_next118, label %blk_exit85

brif_next118:                                     ; preds = %brif_next115
  %calcOffset119 = add i32 %0, -60
  %125 = inttoptr i32 %calcOffset119 to i32*
  %126 = load i32, i32* %125, align 4
  %brif_val120.not = icmp eq i32 %126, 0
  br i1 %brif_val120.not, label %brif_next121, label %blk_exit85

brif_next121:                                     ; preds = %brif_next118
  %calcOffset122 = add i32 %0, -64
  %127 = inttoptr i32 %calcOffset122 to i32*
  %128 = load i32, i32* %127, align 4
  %brif_val123.not = icmp eq i32 %128, 0
  br i1 %brif_val123.not, label %brif_next124, label %blk_exit85

brif_next124:                                     ; preds = %brif_next121
  %calcOffset125 = add i32 %0, -68
  %129 = inttoptr i32 %calcOffset125 to i32*
  %130 = load i32, i32* %129, align 4
  %brif_val126.not = icmp eq i32 %130, 0
  br i1 %brif_val126.not, label %brif_next127, label %blk_exit85

brif_next127:                                     ; preds = %brif_next124
  %calcOffset128 = add i32 %0, -72
  %131 = inttoptr i32 %calcOffset128 to i32*
  %132 = load i32, i32* %131, align 4
  %brif_val129.not = icmp eq i32 %132, 0
  br i1 %brif_val129.not, label %brif_next130, label %blk_exit85

brif_next130:                                     ; preds = %brif_next127
  %133 = zext i1 %brif_val39.not to i32
  %134 = icmp ne i32 %58, 0
  %135 = zext i1 %134 to i32
  br label %blk_exit85

blk_exit133:                                      ; preds = %brif_next139, %blk_exit85
  %_local_9.3 = phi i32 [ %148, %brif_next139 ], [ %104, %blk_exit85 ]
  %136 = shl nuw nsw i32 %_local_16.0, 2
  %137 = add i32 %calcOffset25.pre-phi, %136
  %138 = inttoptr i32 %137 to i32*
  %139 = load i32, i32* %138, align 4
  %140 = sub i32 %_local_9.3, %139
  %brif_val143.not = icmp sgt i32 %140, -1
  br i1 %brif_val143.not, label %brif_next144, label %blk_exit23

brif_next135:                                     ; preds = %blk_exit85
  %141 = sub nsw i32 %_local_16.0, %_local_14.0
  %142 = shl nuw nsw i32 %_local_14.0, 2
  %143 = add i32 %calcOffset25.pre-phi, %142
  br label %loop_entry136

loop_entry136:                                    ; preds = %brif_next139, %brif_next135
  %_local_11.1 = phi i32 [ %141, %brif_next135 ], [ %149, %brif_next139 ]
  %_local_10.3 = phi i32 [ %143, %brif_next135 ], [ %147, %brif_next139 ]
  %_local_9.4 = phi i32 [ %104, %brif_next135 ], [ %148, %brif_next139 ]
  %144 = inttoptr i32 %_local_10.3 to i32*
  %145 = load i32, i32* %144, align 4
  %146 = sub i32 %_local_9.4, %145
  %brif_val138.not = icmp sgt i32 %146, -1
  br i1 %brif_val138.not, label %brif_next139, label %blk_exit23

brif_next139:                                     ; preds = %loop_entry136
  %147 = add i32 %_local_10.3, 4
  %148 = shl i32 %146, 1
  %149 = add i32 %_local_11.1, -1
  %brif_val140.not = icmp eq i32 %149, 0
  br i1 %brif_val140.not, label %blk_exit133, label %loop_entry136

brif_next144:                                     ; preds = %blk_exit133
  store i32 %_local_9.3, i32* %138, align 4
  %calcOffset146 = add i32 %0, -1372
  %150 = inttoptr i32 %calcOffset146 to i32*
  store i32 0, i32* %150, align 4
  %151 = add nsw i32 %_local_16.0, -1
  %152 = icmp eq i32 %151, 0
  br i1 %152, label %blk_exit147, label %brif_next149

blk_exit147:                                      ; preds = %loop_entry166, %blk_exit150, %brif_next144
  %153 = icmp ugt i32 %_arg_1, 1
  %154 = select i1 %153, i32 %_arg_1, i32 1
  %155 = and i32 %154, 1
  %156 = icmp ult i32 %_arg_1, 2
  br i1 %156, label %blk_exit172, label %brif_next174

brif_next149:                                     ; preds = %brif_next144
  %157 = or i32 %calcOffset25.pre-phi, 4
  %158 = and i32 %151, 3
  %159 = or i32 %1, 8
  %160 = add nsw i32 %_local_16.0, -2
  %161 = icmp ult i32 %160, 3
  br i1 %161, label %blk_exit150, label %brif_next152

blk_exit150:                                      ; preds = %loop_entry153, %brif_next149
  %_local_11.2 = phi i32 [ 0, %brif_next149 ], [ %184, %loop_entry153 ]
  %_local_10.4 = phi i32 [ %157, %brif_next149 ], [ %187, %loop_entry153 ]
  %_local_9.5 = phi i32 [ %159, %brif_next149 ], [ %186, %loop_entry153 ]
  %162 = icmp eq i32 %158, 0
  br i1 %162, label %blk_exit147, label %loop_entry166

brif_next152:                                     ; preds = %brif_next149
  %163 = and i32 %151, -4
  br label %loop_entry153

loop_entry153:                                    ; preds = %loop_entry153, %brif_next152
  %_local_11.3 = phi i32 [ 0, %brif_next152 ], [ %184, %loop_entry153 ]
  %_local_10.5 = phi i32 [ %157, %brif_next152 ], [ %187, %loop_entry153 ]
  %_local_9.6 = phi i32 [ %159, %brif_next152 ], [ %186, %loop_entry153 ]
  %_param_6.0 = phi i32 [ %163, %brif_next152 ], [ %188, %loop_entry153 ]
  %164 = inttoptr i32 %_local_10.5 to i32*
  %165 = load i32, i32* %164, align 4
  %166 = add i32 %165, %_local_11.3
  %167 = inttoptr i32 %_local_9.6 to i32*
  store i32 %166, i32* %167, align 4
  %168 = add i32 %_local_9.6, 4
  %169 = add i32 %_local_10.5, 4
  %170 = inttoptr i32 %169 to i32*
  %171 = load i32, i32* %170, align 4
  %172 = add i32 %171, %166
  %173 = inttoptr i32 %168 to i32*
  store i32 %172, i32* %173, align 4
  %174 = add i32 %_local_9.6, 8
  %175 = add i32 %_local_10.5, 8
  %176 = inttoptr i32 %175 to i32*
  %177 = load i32, i32* %176, align 4
  %178 = add i32 %177, %172
  %179 = inttoptr i32 %174 to i32*
  store i32 %178, i32* %179, align 4
  %180 = add i32 %_local_9.6, 12
  %181 = add i32 %_local_10.5, 12
  %182 = inttoptr i32 %181 to i32*
  %183 = load i32, i32* %182, align 4
  %184 = add i32 %183, %178
  %185 = inttoptr i32 %180 to i32*
  store i32 %184, i32* %185, align 4
  %186 = add i32 %_local_9.6, 16
  %187 = add i32 %_local_10.5, 16
  %188 = add i32 %_param_6.0, -4
  %brif_val162.not = icmp eq i32 %188, 0
  br i1 %brif_val162.not, label %blk_exit150, label %loop_entry153

loop_entry166:                                    ; preds = %loop_entry166, %blk_exit150
  %_local_12.1 = phi i32 [ %158, %blk_exit150 ], [ %195, %loop_entry166 ]
  %_local_11.4 = phi i32 [ %_local_11.2, %blk_exit150 ], [ %191, %loop_entry166 ]
  %_local_10.6 = phi i32 [ %_local_10.4, %blk_exit150 ], [ %194, %loop_entry166 ]
  %_local_9.7 = phi i32 [ %_local_9.5, %blk_exit150 ], [ %193, %loop_entry166 ]
  %189 = inttoptr i32 %_local_10.6 to i32*
  %190 = load i32, i32* %189, align 4
  %191 = add i32 %190, %_local_11.4
  %192 = inttoptr i32 %_local_9.7 to i32*
  store i32 %191, i32* %192, align 4
  %193 = add i32 %_local_9.7, 4
  %194 = add i32 %_local_10.6, 4
  %195 = add i32 %_local_12.1, -1
  %brif_val169.not = icmp eq i32 %195, 0
  br i1 %brif_val169.not, label %blk_exit147, label %loop_entry166

blk_exit172:                                      ; preds = %blk_exit183, %blk_exit147
  %_local_10.7 = phi i32 [ 0, %blk_exit147 ], [ %197, %blk_exit183 ]
  %_param_0.0 = phi i32 [ %_arg_0, %blk_exit147 ], [ %214, %blk_exit183 ]
  %196 = icmp eq i32 %155, 0
  br i1 %196, label %blk_exit192, label %brif_next194

brif_next174:                                     ; preds = %blk_exit147
  %197 = and i32 %154, -2
  br label %loop_entry175

loop_entry175:                                    ; preds = %blk_exit183, %brif_next174
  %_local_10.8 = phi i32 [ 0, %brif_next174 ], [ %215, %blk_exit183 ]
  %_param_0.1 = phi i32 [ %_arg_0, %brif_next174 ], [ %214, %blk_exit183 ]
  %198 = inttoptr i32 %_param_0.1 to i32*
  %199 = load i32, i32* %198, align 4
  %200 = icmp eq i32 %199, 0
  br i1 %200, label %blk_exit176, label %brif_next179

blk_exit176:                                      ; preds = %brif_next179, %loop_entry175
  %201 = add i32 %_param_0.1, 4
  %202 = inttoptr i32 %201 to i32*
  %203 = load i32, i32* %202, align 4
  %204 = icmp eq i32 %203, 0
  br i1 %204, label %blk_exit183, label %brif_next186

brif_next179:                                     ; preds = %loop_entry175
  %205 = shl i32 %199, 2
  %206 = add i32 %1, %205
  %207 = inttoptr i32 %206 to i32*
  %208 = load i32, i32* %207, align 4
  %209 = add i32 %208, 1
  store i32 %209, i32* %207, align 4
  %210 = add i32 %0, -1296
  %211 = shl i32 %208, 2
  %212 = add i32 %210, %211
  %213 = inttoptr i32 %212 to i32*
  store i32 %_local_10.8, i32* %213, align 4
  br label %blk_exit176

blk_exit183:                                      ; preds = %brif_next186, %blk_exit176
  %214 = add i32 %_param_0.1, 8
  %215 = add i32 %_local_10.8, 2
  %.not298 = icmp eq i32 %197, %215
  br i1 %.not298, label %blk_exit172, label %loop_entry175

brif_next186:                                     ; preds = %blk_exit176
  %216 = shl i32 %203, 2
  %217 = add i32 %1, %216
  %218 = inttoptr i32 %217 to i32*
  %219 = load i32, i32* %218, align 4
  %220 = add i32 %219, 1
  store i32 %220, i32* %218, align 4
  %221 = add i32 %0, -1296
  %222 = shl i32 %219, 2
  %223 = add i32 %221, %222
  %224 = or i32 %_local_10.8, 1
  %225 = inttoptr i32 %223 to i32*
  store i32 %224, i32* %225, align 4
  br label %blk_exit183

blk_exit192:                                      ; preds = %brif_next197, %brif_next194, %blk_exit172
  %calcOffset201 = add i32 %0, -144
  %226 = inttoptr i32 %calcOffset201 to i32*
  store i32 0, i32* %226, align 4
  %227 = add i32 %1, %136
  %228 = inttoptr i32 %227 to i32*
  %229 = load i32, i32* %228, align 4
  %230 = inttoptr i32 %1 to i32*
  store i32 0, i32* %230, align 4
  %231 = icmp ult i32 %_local_16.0, %_local_14.0
  br i1 %231, label %blk_exit204, label %brif_next206

brif_next194:                                     ; preds = %blk_exit172
  %232 = inttoptr i32 %_param_0.0 to i32*
  %233 = load i32, i32* %232, align 4
  %234 = icmp eq i32 %233, 0
  br i1 %234, label %blk_exit192, label %brif_next197

brif_next197:                                     ; preds = %brif_next194
  %235 = shl i32 %233, 2
  %236 = add i32 %1, %235
  %237 = inttoptr i32 %236 to i32*
  %238 = load i32, i32* %237, align 4
  %239 = add i32 %238, 1
  store i32 %239, i32* %237, align 4
  %240 = add i32 %0, -1296
  %241 = shl i32 %238, 2
  %242 = add i32 %240, %241
  %243 = inttoptr i32 %242 to i32*
  store i32 %_local_10.7, i32* %243, align 4
  br label %blk_exit192

blk_exit204:                                      ; preds = %blk_exit209, %blk_exit192
  %244 = icmp ne i32 %140, 0
  %245 = zext i1 %244 to i32
  %246 = and i32 %_local_13.0, %245
  br label %blk_exit23

brif_next206:                                     ; preds = %blk_exit192
  %247 = shl nuw nsw i32 %_local_14.0, 2
  %248 = add i32 %247, %calcOffset25.pre-phi
  %249 = add i32 %0, -1296
  %250 = shl i32 %229, 2
  %251 = add i32 %249, %250
  %252 = add i32 %0, -1380
  %253 = sub nsw i32 0, %103
  br label %loop_entry208

loop_entry208:                                    ; preds = %blk_exit209, %brif_next206
  %_local_24.0 = phi i32 [ %253, %brif_next206 ], [ %_local_24.1, %blk_exit209 ]
  %_local_22.0 = phi i32 [ %249, %brif_next206 ], [ %_local_22.1, %blk_exit209 ]
  %_local_19.0.in = phi i32 [ %248, %brif_next206 ], [ %_local_19.0, %blk_exit209 ]
  %_local_15.0 = phi i32 [ 0, %brif_next206 ], [ %_local_15.1, %blk_exit209 ]
  %_local_14.1 = phi i32 [ %_local_14.0, %brif_next206 ], [ %259, %blk_exit209 ]
  %_local_12.2 = phi i32 [ 0, %brif_next206 ], [ %_local_12.3, %blk_exit209 ]
  %_param_5.0 = phi i32 [ %_arg_5, %brif_next206 ], [ %_param_5.1, %blk_exit209 ]
  %_local_30.0 = phi i32 [ 0, %brif_next206 ], [ %_local_30.1, %blk_exit209 ]
  %_param_1.0 = phi i32 [ -1, %brif_next206 ], [ %_param_1.1, %blk_exit209 ]
  %_param_0.2 = phi i32 [ 0, %brif_next206 ], [ %_param_0.3, %blk_exit209 ]
  %_local_19.0 = add i32 %_local_19.0.in, 4
  %254 = shl i32 %_local_14.1, 2
  %255 = add i32 %calcOffset25.pre-phi, %254
  %256 = inttoptr i32 %255 to i32*
  %257 = load i32, i32* %256, align 4
  %258 = icmp eq i32 %257, 0
  br i1 %258, label %blk_exit209, label %brif_next212

blk_exit209:                                      ; preds = %blk_exit285, %loop_entry208
  %_local_24.1 = phi i32 [ %_local_24.0, %loop_entry208 ], [ %_local_24.5, %blk_exit285 ]
  %_local_22.1 = phi i32 [ %_local_22.0, %loop_entry208 ], [ %_local_22.3, %blk_exit285 ]
  %_local_15.1 = phi i32 [ %_local_15.0, %loop_entry208 ], [ %_local_15.3, %blk_exit285 ]
  %_local_12.3 = phi i32 [ %_local_12.2, %loop_entry208 ], [ %_local_12.5, %blk_exit285 ]
  %_param_5.1 = phi i32 [ %_param_5.0, %loop_entry208 ], [ %_param_5.3, %blk_exit285 ]
  %_local_30.1 = phi i32 [ %_local_30.0, %loop_entry208 ], [ %_local_30.6, %blk_exit285 ]
  %_param_1.1 = phi i32 [ %_param_1.0, %loop_entry208 ], [ %_param_1.5, %blk_exit285 ]
  %_param_0.3 = phi i32 [ %_param_0.2, %loop_entry208 ], [ %357, %blk_exit285 ]
  %.not307 = icmp eq i32 %_local_14.1, %_local_16.0
  %259 = add i32 %_local_14.1, 1
  br i1 %.not307, label %blk_exit204, label %loop_entry208

brif_next212:                                     ; preds = %loop_entry208
  %260 = add i32 %_local_14.1, -1
  %261 = shl i32 1, %260
  br label %loop_entry213

loop_entry213:                                    ; preds = %blk_exit285, %brif_next212
  %_local_25.0 = phi i32 [ %257, %brif_next212 ], [ %356, %blk_exit285 ]
  %_local_24.2 = phi i32 [ %_local_24.0, %brif_next212 ], [ %_local_24.5, %blk_exit285 ]
  %_local_22.2 = phi i32 [ %_local_22.0, %brif_next212 ], [ %_local_22.3, %blk_exit285 ]
  %_local_15.2 = phi i32 [ %_local_15.0, %brif_next212 ], [ %_local_15.3, %blk_exit285 ]
  %_local_12.4 = phi i32 [ %_local_12.2, %brif_next212 ], [ %_local_12.5, %blk_exit285 ]
  %_param_5.2 = phi i32 [ %_param_5.0, %brif_next212 ], [ %_param_5.3, %blk_exit285 ]
  %_local_30.2 = phi i32 [ %_local_30.0, %brif_next212 ], [ %_local_30.6, %blk_exit285 ]
  %_param_1.2 = phi i32 [ %_param_1.0, %brif_next212 ], [ %_param_1.5, %blk_exit285 ]
  %_param_0.4 = phi i32 [ %_param_0.2, %brif_next212 ], [ %357, %blk_exit285 ]
  %262 = add i32 %_local_24.2, %103
  %263 = icmp sgt i32 %_local_14.1, %262
  br i1 %263, label %blk_exit215, label %brif_next217

blk_exit214:                                      ; preds = %blk_exit249, %brif_next217
  %_local_24.3 = phi i32 [ %_local_24.2, %brif_next217 ], [ %_local_9.8, %blk_exit249 ]
  %_local_15.3 = phi i32 [ %_local_15.2, %brif_next217 ], [ %269, %blk_exit249 ]
  %_local_12.5 = phi i32 [ %_local_12.4, %brif_next217 ], [ %287, %blk_exit249 ]
  %_param_6.1 = phi i32 [ %265, %brif_next217 ], [ %266, %blk_exit249 ]
  %_param_5.3 = phi i32 [ %_param_5.2, %brif_next217 ], [ %calcOffset247, %blk_exit249 ]
  %_local_30.3 = phi i32 [ %_local_30.2, %brif_next217 ], [ %_local_30.5, %blk_exit249 ]
  %_param_1.3 = phi i32 [ %_param_1.2, %brif_next217 ], [ %268, %blk_exit249 ]
  %.not299 = icmp ult i32 %_local_22.2, %251
  br i1 %.not299, label %brif_next261, label %blk_exit259

blk_exit215:                                      ; preds = %loop_entry213
  %264 = sub i32 %_local_16.0, %262
  br label %loop_entry218

brif_next217:                                     ; preds = %loop_entry213
  %265 = sub i32 %_local_14.1, %_local_24.2
  br label %blk_exit214

loop_entry218:                                    ; preds = %blk_exit249, %blk_exit215
  %_local_27.0 = phi i32 [ %264, %blk_exit215 ], [ %303, %blk_exit249 ]
  %_local_24.4 = phi i32 [ %_local_24.2, %blk_exit215 ], [ %_local_9.8, %blk_exit249 ]
  %_local_9.8 = phi i32 [ %262, %blk_exit215 ], [ %304, %blk_exit249 ]
  %_param_5.4 = phi i32 [ %_param_5.2, %blk_exit215 ], [ %calcOffset247, %blk_exit249 ]
  %_local_30.4 = phi i32 [ %_local_30.2, %blk_exit215 ], [ %_local_30.5, %blk_exit249 ]
  %_param_1.4 = phi i32 [ %_param_1.2, %blk_exit215 ], [ %268, %blk_exit249 ]
  %266 = sub i32 %_local_14.1, %_local_9.8
  %267 = shl i32 1, %266
  %.not303 = icmp ugt i32 %267, %_local_25.0
  br i1 %.not303, label %brif_next221, label %blk_exit219

blk_exit219:                                      ; preds = %brif_next231, %loop_entry228, %brif_next224, %brif_next221, %loop_entry218
  %_local_9.9 = phi i32 [ %_local_9.10, %loop_entry228 ], [ %276, %brif_next224 ], [ %266, %brif_next221 ], [ %266, %loop_entry218 ], [ %278, %brif_next231 ]
  %268 = add i32 %_param_1.4, 1
  %269 = shl i32 1, %_local_9.9
  %270 = add i32 %269, 1
  %271 = shl i32 %270, 3
  %272 = call i32 @rpl_malloc(i32 %271)
  %brif_val235.not = icmp eq i32 %272, 0
  br i1 %brif_val235.not, label %brif_next236, label %blk_exit234

brif_next221:                                     ; preds = %loop_entry218
  %273 = sub i32 %_local_16.0, %_local_9.8
  %274 = icmp ugt i32 %273, %103
  %275 = select i1 %274, i32 %103, i32 %273
  %.not304 = icmp ult i32 %266, %275
  br i1 %.not304, label %brif_next224, label %blk_exit219

brif_next224:                                     ; preds = %brif_next221
  %276 = add i32 %266, 1
  %.not305 = icmp ult i32 %276, %275
  br i1 %.not305, label %brif_next226, label %blk_exit219

brif_next226:                                     ; preds = %brif_next224
  %277 = icmp ugt i32 %_local_27.0, %103
  %278 = select i1 %277, i32 %103, i32 %_local_27.0
  %279 = sub i32 %267, %_local_25.0
  br label %loop_entry228

loop_entry228:                                    ; preds = %brif_next231, %brif_next226
  %_local_11.5 = phi i32 [ %279, %brif_next226 ], [ %284, %brif_next231 ]
  %_local_10.9 = phi i32 [ %_local_19.0, %brif_next226 ], [ %283, %brif_next231 ]
  %_local_9.10 = phi i32 [ %276, %brif_next226 ], [ %285, %brif_next231 ]
  %280 = shl i32 %_local_11.5, 1
  %281 = inttoptr i32 %_local_10.9 to i32*
  %282 = load i32, i32* %281, align 4
  %.not306 = icmp ugt i32 %280, %282
  br i1 %.not306, label %brif_next231, label %blk_exit219

brif_next231:                                     ; preds = %loop_entry228
  %283 = add i32 %_local_10.9, 4
  %284 = sub i32 %280, %282
  %285 = add i32 %_local_9.10, 1
  %286 = icmp ult i32 %285, %275
  br i1 %286, label %loop_entry228, label %blk_exit219

blk_exit234:                                      ; preds = %blk_exit219
  %287 = add i32 %272, 8
  %288 = inttoptr i32 %_param_5.4 to i32*
  store i32 %287, i32* %288, align 4
  %289 = load i32, i32* inttoptr (i32 818456 to i32*), align 8
  %290 = add i32 %289, %270
  store i32 %290, i32* inttoptr (i32 818456 to i32*), align 8
  %calcOffset247 = add i32 %272, 4
  %291 = inttoptr i32 %calcOffset247 to i32*
  store i32 0, i32* %291, align 4
  %292 = shl i32 %268, 2
  %293 = add i32 %calcOffset201, %292
  %294 = inttoptr i32 %293 to i32*
  store i32 %287, i32* %294, align 4
  %295 = icmp eq i32 %268, 0
  br i1 %295, label %blk_exit249, label %brif_next251

brif_next236:                                     ; preds = %blk_exit219
  %296 = icmp eq i32 %268, 0
  br i1 %296, label %blk_exit23, label %brif_next238

brif_next238:                                     ; preds = %brif_next236
  %297 = load i32, i32* %226, align 4
  %298 = icmp eq i32 %297, 0
  br i1 %298, label %blk_exit23, label %loop_entry242

loop_entry242:                                    ; preds = %loop_entry242, %brif_next238
  %_local_10.10 = phi i32 [ %297, %brif_next238 ], [ %301, %loop_entry242 ]
  %299 = add i32 %_local_10.10, -4
  %300 = inttoptr i32 %299 to i32*
  %301 = load i32, i32* %300, align 4
  %302 = add i32 %_local_10.10, -8
  call void @rpl_free(i32 %302)
  %brif_val244.not = icmp eq i32 %301, 0
  br i1 %brif_val244.not, label %blk_exit23, label %loop_entry242

blk_exit249:                                      ; preds = %brif_next251, %blk_exit234
  %_local_30.5 = phi i32 [ %_local_30.4, %blk_exit234 ], [ %287, %brif_next251 ]
  %303 = sub i32 %_local_27.0, %103
  %304 = add i32 %_local_9.8, %103
  %305 = icmp sgt i32 %_local_14.1, %304
  br i1 %305, label %loop_entry218, label %blk_exit214

brif_next251:                                     ; preds = %blk_exit234
  %306 = add i32 %1, %292
  %307 = inttoptr i32 %306 to i32*
  store i32 %_param_0.4, i32* %307, align 4
  %308 = shl i32 %_param_1.4, 2
  %309 = add i32 %calcOffset201, %308
  %310 = inttoptr i32 %309 to i32*
  %311 = load i32, i32* %310, align 4
  %312 = lshr i32 %_param_0.4, %_local_24.4
  %313 = shl i32 %312, 3
  %314 = add i32 %311, %313
  %calcOffset254 = add i32 %314, 4
  %315 = inttoptr i32 %calcOffset254 to i32*
  store i32 %287, i32* %315, align 4
  %calcOffset255 = add i32 %314, 1
  %316 = inttoptr i32 %calcOffset255 to i8*
  %317 = trunc i32 %103 to i8
  store i8 %317, i8* %316, align 1
  %318 = inttoptr i32 %314 to i8*
  %319 = trunc i32 %_local_9.9 to i8
  %320 = add i8 %319, 16
  store i8 %320, i8* %318, align 1
  br label %blk_exit249

blk_exit259:                                      ; preds = %brif_next265, %blk_exit262, %blk_exit214
  %_local_22.3 = phi i32 [ %341, %brif_next265 ], [ %332, %blk_exit262 ], [ %_local_22.2, %blk_exit214 ]
  %_local_11.6 = phi i8 [ %340, %brif_next265 ], [ %335, %blk_exit262 ], [ 99, %blk_exit214 ]
  %_local_30.6 = phi i32 [ %338, %brif_next265 ], [ %331, %blk_exit262 ], [ %_local_30.3, %blk_exit214 ]
  %321 = lshr i32 %_param_0.4, %_local_24.3
  %.not301 = icmp ult i32 %321, %_local_15.3
  br i1 %.not301, label %brif_next271, label %blk_exit269

brif_next261:                                     ; preds = %blk_exit214
  %322 = inttoptr i32 %_local_22.2 to i32*
  %323 = load i32, i32* %322, align 4
  %.not300 = icmp ult i32 %323, %_arg_2
  br i1 %.not300, label %brif_next265, label %blk_exit262

blk_exit262:                                      ; preds = %brif_next261
  %324 = and i32 %_local_30.3, -65536
  %325 = sub i32 %323, %_arg_2
  %326 = shl i32 %325, 1
  %327 = add i32 %326, %_arg_3
  %328 = inttoptr i32 %327 to i16*
  %329 = load i16, i16* %328, align 2
  %330 = zext i16 %329 to i32
  %331 = or i32 %324, %330
  %332 = add i32 %_local_22.2, 4
  %333 = add i32 %326, %_arg_4
  %334 = inttoptr i32 %333 to i8*
  %335 = load i8, i8* %334, align 1
  br label %blk_exit259

brif_next265:                                     ; preds = %brif_next261
  %336 = and i32 %323, 65535
  %337 = and i32 %_local_30.3, -65536
  %338 = or i32 %336, %337
  %339 = icmp ult i32 %323, 256
  %340 = select i1 %339, i8 16, i8 15
  %341 = add i32 %_local_22.2, 4
  br label %blk_exit259

blk_exit269:                                      ; preds = %loop_entry272, %blk_exit259
  %342 = and i32 %261, %_param_0.4
  %brif_val280.not = icmp eq i32 %342, 0
  br i1 %brif_val280.not, label %blk_exit278, label %loop_entry282

brif_next271:                                     ; preds = %blk_exit259
  %343 = shl i32 1, %_param_6.1
  %344 = shl i32 %343, 3
  %345 = shl i32 %321, 3
  %346 = add i32 %_local_12.5, %345
  br label %loop_entry272

loop_entry272:                                    ; preds = %loop_entry272, %brif_next271
  %_local_10.11 = phi i32 [ %346, %brif_next271 ], [ %353, %loop_entry272 ]
  %_local_9.11 = phi i32 [ %321, %brif_next271 ], [ %354, %loop_entry272 ]
  %347 = inttoptr i32 %_local_10.11 to i8*
  store i8 %_local_11.6, i8* %347, align 1
  %348 = add i32 %_local_10.11, 4
  %349 = inttoptr i32 %348 to i32*
  store i32 %_local_30.6, i32* %349, align 4
  %350 = add i32 %_local_10.11, 1
  %351 = inttoptr i32 %350 to i8*
  %352 = trunc i32 %_param_6.1 to i8
  store i8 %352, i8* %351, align 1
  %353 = add i32 %_local_10.11, %344
  %354 = add i32 %_local_9.11, %343
  %355 = icmp ult i32 %354, %_local_15.3
  br i1 %355, label %loop_entry272, label %blk_exit269

blk_exit278:                                      ; preds = %loop_entry282, %blk_exit269
  %_local_10.12 = phi i32 [ %367, %loop_entry282 ], [ %261, %blk_exit269 ]
  %_param_0.5 = phi i32 [ %366, %loop_entry282 ], [ %_param_0.4, %blk_exit269 ]
  %356 = add i32 %_local_25.0, -1
  %357 = xor i32 %_local_10.12, %_param_0.5
  %358 = shl i32 -1, %_local_24.3
  %359 = xor i32 %358, -1
  %360 = and i32 %357, %359
  %361 = shl i32 %_param_1.3, 2
  %362 = add i32 %1, %361
  %363 = inttoptr i32 %362 to i32*
  %364 = load i32, i32* %363, align 4
  %365 = icmp eq i32 %360, %364
  br i1 %365, label %blk_exit285, label %brif_next288

loop_entry282:                                    ; preds = %loop_entry282, %blk_exit269
  %_local_10.13 = phi i32 [ %367, %loop_entry282 ], [ %261, %blk_exit269 ]
  %_param_0.6 = phi i32 [ %366, %loop_entry282 ], [ %_param_0.4, %blk_exit269 ]
  %366 = xor i32 %_local_10.13, %_param_0.6
  %367 = lshr i32 %_local_10.13, 1
  %368 = and i32 %366, %367
  %brif_val283.not = icmp eq i32 %368, 0
  br i1 %brif_val283.not, label %blk_exit278, label %loop_entry282

blk_exit285:                                      ; preds = %loop_entry289, %blk_exit278
  %_local_24.5 = phi i32 [ %_local_24.3, %blk_exit278 ], [ %375, %loop_entry289 ]
  %_param_1.5 = phi i32 [ %_param_1.3, %blk_exit278 ], [ %370, %loop_entry289 ]
  %brif_val293.not = icmp eq i32 %356, 0
  br i1 %brif_val293.not, label %blk_exit209, label %loop_entry213

brif_next288:                                     ; preds = %blk_exit278
  %369 = add i32 %252, %361
  br label %loop_entry289

loop_entry289:                                    ; preds = %loop_entry289, %brif_next288
  %_local_10.14 = phi i32 [ %369, %brif_next288 ], [ %374, %loop_entry289 ]
  %_local_9.12 = phi i32 [ %_local_24.3, %brif_next288 ], [ %375, %loop_entry289 ]
  %_param_1.6 = phi i32 [ %_param_1.3, %brif_next288 ], [ %370, %loop_entry289 ]
  %370 = add i32 %_param_1.6, -1
  %371 = sub i32 %_local_9.12, %103
  %372 = inttoptr i32 %_local_10.14 to i32*
  %373 = load i32, i32* %372, align 4
  %374 = add i32 %_local_10.14, -4
  %375 = sub i32 %_local_9.12, %103
  %376 = shl i32 -1, %371
  %377 = xor i32 %376, -1
  %378 = and i32 %357, %377
  %.not302 = icmp eq i32 %373, %378
  br i1 %.not302, label %blk_exit285, label %loop_entry289
}

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @inflate_codes(i32, i32, i32, i32) #0

; Function Attrs: null_pointer_is_valid
define internal void @ct_init(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  store i32 %_arg_1, i32* inttoptr (i32 818464 to i32*), align 32
  store i32 %_arg_0, i32* inttoptr (i32 818460 to i32*), align 4
  store i64 0, i64* inttoptr (i32 818472 to i64*), align 8
  store i64 0, i64* inttoptr (i32 818480 to i64*), align 16
  %0 = load i16, i16* inttoptr (i32 818498 to i16*), align 2
  %brif_val.not = icmp eq i16 %0, 0
  br i1 %brif_val.not, label %loop_entry, label %blk_exit

blk_exit:                                         ; preds = %brif_next573, %allocator
  ret void

loop_entry:                                       ; preds = %blk_exit1, %allocator
  %_local_3.0 = phi i32 [ %_local_3.1, %blk_exit1 ], [ 0, %allocator ]
  %_local_2.0 = phi i32 [ %8, %blk_exit1 ], [ 0, %allocator ]
  %1 = shl i32 %_local_2.0, 2
  %2 = add i32 %1, 818624
  %3 = inttoptr i32 %2 to i32*
  store i32 %_local_3.0, i32* %3, align 4
  %4 = add i32 %1, 15504
  %5 = inttoptr i32 %4 to i32*
  %6 = load i32, i32* %5, align 4
  %7 = icmp eq i32 %6, 31
  br i1 %7, label %blk_exit1, label %brif_next4

blk_exit1:                                        ; preds = %loop_entry14, %blk_exit5, %loop_entry
  %_local_3.1 = phi i32 [ %_local_3.0, %loop_entry ], [ %15, %blk_exit5 ], [ %15, %loop_entry14 ]
  %8 = add i32 %_local_2.0, 1
  %.not = icmp eq i32 %8, 28
  br i1 %.not, label %brif_next26, label %loop_entry

brif_next4:                                       ; preds = %loop_entry
  %9 = shl i32 1, %6
  %10 = icmp sgt i32 %9, 1
  %11 = select i1 %10, i32 %9, i32 1
  %12 = add nsw i32 %11, -1
  %13 = and i32 %11, 7
  %14 = icmp eq i32 %13, 0
  br i1 %14, label %blk_exit5, label %loop_entry8

blk_exit5:                                        ; preds = %loop_entry8, %brif_next4
  %_param_1.0 = phi i32 [ %_local_3.0, %brif_next4 ], [ %20, %loop_entry8 ]
  %15 = add i32 %_local_3.0, %11
  %16 = icmp ult i32 %12, 7
  br i1 %16, label %blk_exit1, label %brif_next13

loop_entry8:                                      ; preds = %loop_entry8, %brif_next4
  %_param_1.1 = phi i32 [ %20, %loop_entry8 ], [ %_local_3.0, %brif_next4 ]
  %_param_0.0 = phi i32 [ %21, %loop_entry8 ], [ %13, %brif_next4 ]
  %17 = add i32 %_param_1.1, 818752
  %18 = inttoptr i32 %17 to i8*
  %19 = trunc i32 %_local_2.0 to i8
  store i8 %19, i8* %18, align 1
  %20 = add i32 %_param_1.1, 1
  %21 = add i32 %_param_0.0, -1
  %brif_val10.not = icmp eq i32 %21, 0
  br i1 %brif_val10.not, label %blk_exit5, label %loop_entry8

brif_next13:                                      ; preds = %blk_exit5
  %22 = sub i32 %_param_1.0, %15
  br label %loop_entry14

loop_entry14:                                     ; preds = %loop_entry14, %brif_next13
  %_local_4.0 = phi i32 [ 0, %brif_next13 ], [ %41, %loop_entry14 ]
  %23 = add i32 %_param_1.0, %_local_4.0
  %24 = add i32 %23, 818759
  %25 = inttoptr i32 %24 to i8*
  %26 = trunc i32 %_local_2.0 to i8
  store i8 %26, i8* %25, align 1
  %27 = add i32 %23, 818758
  %28 = inttoptr i32 %27 to i8*
  store i8 %26, i8* %28, align 1
  %29 = add i32 %23, 818757
  %30 = inttoptr i32 %29 to i8*
  store i8 %26, i8* %30, align 1
  %31 = add i32 %23, 818756
  %32 = inttoptr i32 %31 to i8*
  store i8 %26, i8* %32, align 1
  %33 = add i32 %23, 818755
  %34 = inttoptr i32 %33 to i8*
  store i8 %26, i8* %34, align 1
  %35 = add i32 %23, 818754
  %36 = inttoptr i32 %35 to i8*
  store i8 %26, i8* %36, align 1
  %37 = add i32 %23, 818753
  %38 = inttoptr i32 %37 to i8*
  store i8 %26, i8* %38, align 1
  %39 = add i32 %23, 818752
  %40 = inttoptr i32 %39 to i8*
  store i8 %26, i8* %40, align 1
  %41 = add i32 %_local_4.0, 8
  %42 = sub nuw nsw i32 -8, %_local_4.0
  %brif_val23.not = icmp eq i32 %22, %42
  br i1 %brif_val23.not, label %blk_exit1, label %loop_entry14

brif_next26:                                      ; preds = %blk_exit1
  %43 = add i32 %_local_3.1, 818751
  %44 = inttoptr i32 %43 to i8*
  store i8 28, i8* %44, align 1
  store i32 0, i32* inttoptr (i32 819008 to i32*), align 64
  %45 = load i32, i32* inttoptr (i32 15632 to i32*), align 16
  %46 = icmp eq i32 %45, 31
  br i1 %46, label %blk_exit28, label %brif_next30

blk_exit28:                                       ; preds = %loop_entry41, %blk_exit32, %brif_next26
  %_local_2.1 = phi i32 [ 0, %brif_next26 ], [ %51, %blk_exit32 ], [ %51, %loop_entry41 ]
  store i32 %_local_2.1, i32* inttoptr (i32 819012 to i32*), align 4
  %47 = load i32, i32* inttoptr (i32 15636 to i32*), align 4
  %48 = icmp eq i32 %47, 31
  br i1 %48, label %blk_exit45, label %brif_next47

brif_next30:                                      ; preds = %brif_next26
  %49 = shl i32 1, %45
  %50 = icmp sgt i32 %49, 1
  %51 = select i1 %50, i32 %49, i32 1
  %52 = and i32 %51, 7
  %53 = add nsw i32 %51, -1
  %54 = icmp ult i32 %53, 7
  br i1 %54, label %blk_exit32, label %brif_next34

blk_exit32:                                       ; preds = %loop_entry35, %brif_next30
  %_param_0.1 = phi i32 [ 0, %brif_next30 ], [ %56, %loop_entry35 ]
  %55 = icmp eq i32 %52, 0
  br i1 %55, label %blk_exit28, label %brif_next40

brif_next34:                                      ; preds = %brif_next30
  %56 = and i32 %51, 2147483640
  br label %loop_entry35

loop_entry35:                                     ; preds = %loop_entry35, %brif_next34
  %_param_0.2 = phi i32 [ 0, %brif_next34 ], [ %59, %loop_entry35 ]
  %57 = add i32 %_param_0.2, 819136
  %58 = inttoptr i32 %57 to i64*
  store i64 0, i64* %58, align 8
  %59 = add i32 %_param_0.2, 8
  %.not574 = icmp eq i32 %56, %59
  br i1 %.not574, label %blk_exit32, label %loop_entry35

brif_next40:                                      ; preds = %blk_exit32
  %60 = add nuw i32 %_param_0.1, 819136
  br label %loop_entry41

loop_entry41:                                     ; preds = %loop_entry41, %brif_next40
  %_param_1.2 = phi i32 [ %52, %brif_next40 ], [ %63, %loop_entry41 ]
  %_param_0.3 = phi i32 [ %60, %brif_next40 ], [ %62, %loop_entry41 ]
  %61 = inttoptr i32 %_param_0.3 to i8*
  store i8 0, i8* %61, align 1
  %62 = add i32 %_param_0.3, 1
  %63 = add i32 %_param_1.2, -1
  %brif_val43.not = icmp eq i32 %63, 0
  br i1 %brif_val43.not, label %blk_exit28, label %loop_entry41

blk_exit45:                                       ; preds = %loop_entry58, %blk_exit49, %blk_exit28
  %_local_2.2 = phi i32 [ %_local_2.1, %blk_exit28 ], [ %72, %blk_exit49 ], [ %72, %loop_entry58 ]
  store i32 %_local_2.2, i32* inttoptr (i32 819016 to i32*), align 8
  %64 = load i32, i32* inttoptr (i32 15640 to i32*), align 8
  %65 = icmp eq i32 %64, 31
  br i1 %65, label %blk_exit62, label %brif_next64

brif_next47:                                      ; preds = %blk_exit28
  %66 = shl i32 1, %47
  %67 = icmp sgt i32 %66, 1
  %68 = select i1 %67, i32 %66, i32 1
  %69 = add nsw i32 %68, -1
  %70 = and i32 %68, 7
  %71 = icmp eq i32 %70, 0
  br i1 %71, label %blk_exit49, label %loop_entry52

blk_exit49:                                       ; preds = %loop_entry52, %brif_next47
  %_param_0.4 = phi i32 [ %_local_2.1, %brif_next47 ], [ %76, %loop_entry52 ]
  %72 = add nuw i32 %_local_2.1, %68
  %73 = icmp ult i32 %69, 7
  br i1 %73, label %blk_exit45, label %brif_next57

loop_entry52:                                     ; preds = %loop_entry52, %brif_next47
  %_param_1.3 = phi i32 [ %77, %loop_entry52 ], [ %70, %brif_next47 ]
  %_param_0.5 = phi i32 [ %76, %loop_entry52 ], [ %_local_2.1, %brif_next47 ]
  %74 = add i32 %_param_0.5, 819136
  %75 = inttoptr i32 %74 to i8*
  store i8 1, i8* %75, align 1
  %76 = add i32 %_param_0.5, 1
  %77 = add i32 %_param_1.3, -1
  %brif_val54.not = icmp eq i32 %77, 0
  br i1 %brif_val54.not, label %blk_exit49, label %loop_entry52

brif_next57:                                      ; preds = %blk_exit49
  %78 = sub i32 %_param_0.4, %72
  br label %loop_entry58

loop_entry58:                                     ; preds = %loop_entry58, %brif_next57
  %_param_1.4 = phi i32 [ 0, %brif_next57 ], [ %82, %loop_entry58 ]
  %79 = add i32 %_param_0.4, %_param_1.4
  %80 = add i32 %79, 819136
  %81 = inttoptr i32 %80 to i64*
  store i64 72340172838076673, i64* %81, align 8
  %82 = add i32 %_param_1.4, 8
  %83 = sub nuw nsw i32 -8, %_param_1.4
  %brif_val60.not = icmp eq i32 %78, %83
  br i1 %brif_val60.not, label %blk_exit45, label %loop_entry58

blk_exit62:                                       ; preds = %loop_entry75, %blk_exit66, %blk_exit45
  %_local_2.3 = phi i32 [ %_local_2.2, %blk_exit45 ], [ %92, %blk_exit66 ], [ %92, %loop_entry75 ]
  store i32 %_local_2.3, i32* inttoptr (i32 819020 to i32*), align 4
  %84 = load i32, i32* inttoptr (i32 15644 to i32*), align 4
  %85 = icmp eq i32 %84, 31
  br i1 %85, label %blk_exit79, label %brif_next81

brif_next64:                                      ; preds = %blk_exit45
  %86 = shl i32 1, %64
  %87 = icmp sgt i32 %86, 1
  %88 = select i1 %87, i32 %86, i32 1
  %89 = add nsw i32 %88, -1
  %90 = and i32 %88, 7
  %91 = icmp eq i32 %90, 0
  br i1 %91, label %blk_exit66, label %loop_entry69

blk_exit66:                                       ; preds = %loop_entry69, %brif_next64
  %_param_0.6 = phi i32 [ %_local_2.2, %brif_next64 ], [ %96, %loop_entry69 ]
  %92 = add i32 %_local_2.2, %88
  %93 = icmp ult i32 %89, 7
  br i1 %93, label %blk_exit62, label %brif_next74

loop_entry69:                                     ; preds = %loop_entry69, %brif_next64
  %_param_1.5 = phi i32 [ %97, %loop_entry69 ], [ %90, %brif_next64 ]
  %_param_0.7 = phi i32 [ %96, %loop_entry69 ], [ %_local_2.2, %brif_next64 ]
  %94 = add i32 %_param_0.7, 819136
  %95 = inttoptr i32 %94 to i8*
  store i8 2, i8* %95, align 1
  %96 = add i32 %_param_0.7, 1
  %97 = add i32 %_param_1.5, -1
  %brif_val71.not = icmp eq i32 %97, 0
  br i1 %brif_val71.not, label %blk_exit66, label %loop_entry69

brif_next74:                                      ; preds = %blk_exit66
  %98 = sub i32 %_param_0.6, %92
  br label %loop_entry75

loop_entry75:                                     ; preds = %loop_entry75, %brif_next74
  %_param_1.6 = phi i32 [ 0, %brif_next74 ], [ %102, %loop_entry75 ]
  %99 = add i32 %_param_0.6, %_param_1.6
  %100 = add i32 %99, 819136
  %101 = inttoptr i32 %100 to i64*
  store i64 144680345676153346, i64* %101, align 8
  %102 = add i32 %_param_1.6, 8
  %103 = sub nuw nsw i32 -8, %_param_1.6
  %brif_val77.not = icmp eq i32 %98, %103
  br i1 %brif_val77.not, label %blk_exit62, label %loop_entry75

blk_exit79:                                       ; preds = %loop_entry92, %blk_exit83, %blk_exit62
  %_local_2.4 = phi i32 [ %_local_2.3, %blk_exit62 ], [ %112, %blk_exit83 ], [ %112, %loop_entry92 ]
  store i32 %_local_2.4, i32* inttoptr (i32 819024 to i32*), align 16
  %104 = load i32, i32* inttoptr (i32 15648 to i32*), align 32
  %105 = icmp eq i32 %104, 31
  br i1 %105, label %blk_exit96, label %brif_next98

brif_next81:                                      ; preds = %blk_exit62
  %106 = shl i32 1, %84
  %107 = icmp sgt i32 %106, 1
  %108 = select i1 %107, i32 %106, i32 1
  %109 = add nsw i32 %108, -1
  %110 = and i32 %108, 7
  %111 = icmp eq i32 %110, 0
  br i1 %111, label %blk_exit83, label %loop_entry86

blk_exit83:                                       ; preds = %loop_entry86, %brif_next81
  %_param_0.8 = phi i32 [ %_local_2.3, %brif_next81 ], [ %116, %loop_entry86 ]
  %112 = add i32 %_local_2.3, %108
  %113 = icmp ult i32 %109, 7
  br i1 %113, label %blk_exit79, label %brif_next91

loop_entry86:                                     ; preds = %loop_entry86, %brif_next81
  %_param_1.7 = phi i32 [ %117, %loop_entry86 ], [ %110, %brif_next81 ]
  %_param_0.9 = phi i32 [ %116, %loop_entry86 ], [ %_local_2.3, %brif_next81 ]
  %114 = add i32 %_param_0.9, 819136
  %115 = inttoptr i32 %114 to i8*
  store i8 3, i8* %115, align 1
  %116 = add i32 %_param_0.9, 1
  %117 = add i32 %_param_1.7, -1
  %brif_val88.not = icmp eq i32 %117, 0
  br i1 %brif_val88.not, label %blk_exit83, label %loop_entry86

brif_next91:                                      ; preds = %blk_exit83
  %118 = sub i32 %_param_0.8, %112
  br label %loop_entry92

loop_entry92:                                     ; preds = %loop_entry92, %brif_next91
  %_param_1.8 = phi i32 [ 0, %brif_next91 ], [ %122, %loop_entry92 ]
  %119 = add i32 %_param_0.8, %_param_1.8
  %120 = add i32 %119, 819136
  %121 = inttoptr i32 %120 to i64*
  store i64 217020518514230019, i64* %121, align 8
  %122 = add i32 %_param_1.8, 8
  %123 = sub nuw nsw i32 -8, %_param_1.8
  %brif_val94.not = icmp eq i32 %118, %123
  br i1 %brif_val94.not, label %blk_exit79, label %loop_entry92

blk_exit96:                                       ; preds = %loop_entry109, %blk_exit100, %blk_exit79
  %_local_2.5 = phi i32 [ %_local_2.4, %blk_exit79 ], [ %132, %blk_exit100 ], [ %132, %loop_entry109 ]
  store i32 %_local_2.5, i32* inttoptr (i32 819028 to i32*), align 4
  %124 = load i32, i32* inttoptr (i32 15652 to i32*), align 4
  %125 = icmp eq i32 %124, 31
  br i1 %125, label %blk_exit113, label %brif_next115

brif_next98:                                      ; preds = %blk_exit79
  %126 = shl i32 1, %104
  %127 = icmp sgt i32 %126, 1
  %128 = select i1 %127, i32 %126, i32 1
  %129 = add nsw i32 %128, -1
  %130 = and i32 %128, 7
  %131 = icmp eq i32 %130, 0
  br i1 %131, label %blk_exit100, label %loop_entry103

blk_exit100:                                      ; preds = %loop_entry103, %brif_next98
  %_param_0.10 = phi i32 [ %_local_2.4, %brif_next98 ], [ %136, %loop_entry103 ]
  %132 = add i32 %_local_2.4, %128
  %133 = icmp ult i32 %129, 7
  br i1 %133, label %blk_exit96, label %brif_next108

loop_entry103:                                    ; preds = %loop_entry103, %brif_next98
  %_param_1.9 = phi i32 [ %137, %loop_entry103 ], [ %130, %brif_next98 ]
  %_param_0.11 = phi i32 [ %136, %loop_entry103 ], [ %_local_2.4, %brif_next98 ]
  %134 = add i32 %_param_0.11, 819136
  %135 = inttoptr i32 %134 to i8*
  store i8 4, i8* %135, align 1
  %136 = add i32 %_param_0.11, 1
  %137 = add i32 %_param_1.9, -1
  %brif_val105.not = icmp eq i32 %137, 0
  br i1 %brif_val105.not, label %blk_exit100, label %loop_entry103

brif_next108:                                     ; preds = %blk_exit100
  %138 = sub i32 %_param_0.10, %132
  br label %loop_entry109

loop_entry109:                                    ; preds = %loop_entry109, %brif_next108
  %_param_1.10 = phi i32 [ 0, %brif_next108 ], [ %142, %loop_entry109 ]
  %139 = add i32 %_param_0.10, %_param_1.10
  %140 = add i32 %139, 819136
  %141 = inttoptr i32 %140 to i64*
  store i64 289360691352306692, i64* %141, align 8
  %142 = add i32 %_param_1.10, 8
  %143 = sub nuw nsw i32 -8, %_param_1.10
  %brif_val111.not = icmp eq i32 %138, %143
  br i1 %brif_val111.not, label %blk_exit96, label %loop_entry109

blk_exit113:                                      ; preds = %loop_entry126, %blk_exit117, %blk_exit96
  %_local_2.6 = phi i32 [ %_local_2.5, %blk_exit96 ], [ %152, %blk_exit117 ], [ %152, %loop_entry126 ]
  store i32 %_local_2.6, i32* inttoptr (i32 819032 to i32*), align 8
  %144 = load i32, i32* inttoptr (i32 15656 to i32*), align 8
  %145 = icmp eq i32 %144, 31
  br i1 %145, label %blk_exit130, label %brif_next132

brif_next115:                                     ; preds = %blk_exit96
  %146 = shl i32 1, %124
  %147 = icmp sgt i32 %146, 1
  %148 = select i1 %147, i32 %146, i32 1
  %149 = add nsw i32 %148, -1
  %150 = and i32 %148, 7
  %151 = icmp eq i32 %150, 0
  br i1 %151, label %blk_exit117, label %loop_entry120

blk_exit117:                                      ; preds = %loop_entry120, %brif_next115
  %_param_0.12 = phi i32 [ %_local_2.5, %brif_next115 ], [ %156, %loop_entry120 ]
  %152 = add i32 %_local_2.5, %148
  %153 = icmp ult i32 %149, 7
  br i1 %153, label %blk_exit113, label %brif_next125

loop_entry120:                                    ; preds = %loop_entry120, %brif_next115
  %_param_1.11 = phi i32 [ %157, %loop_entry120 ], [ %150, %brif_next115 ]
  %_param_0.13 = phi i32 [ %156, %loop_entry120 ], [ %_local_2.5, %brif_next115 ]
  %154 = add i32 %_param_0.13, 819136
  %155 = inttoptr i32 %154 to i8*
  store i8 5, i8* %155, align 1
  %156 = add i32 %_param_0.13, 1
  %157 = add i32 %_param_1.11, -1
  %brif_val122.not = icmp eq i32 %157, 0
  br i1 %brif_val122.not, label %blk_exit117, label %loop_entry120

brif_next125:                                     ; preds = %blk_exit117
  %158 = sub i32 %_param_0.12, %152
  br label %loop_entry126

loop_entry126:                                    ; preds = %loop_entry126, %brif_next125
  %_param_1.12 = phi i32 [ 0, %brif_next125 ], [ %162, %loop_entry126 ]
  %159 = add i32 %_param_0.12, %_param_1.12
  %160 = add i32 %159, 819136
  %161 = inttoptr i32 %160 to i64*
  store i64 361700864190383365, i64* %161, align 8
  %162 = add i32 %_param_1.12, 8
  %163 = sub nuw nsw i32 -8, %_param_1.12
  %brif_val128.not = icmp eq i32 %158, %163
  br i1 %brif_val128.not, label %blk_exit113, label %loop_entry126

blk_exit130:                                      ; preds = %loop_entry143, %blk_exit134, %blk_exit113
  %_local_2.7 = phi i32 [ %_local_2.6, %blk_exit113 ], [ %172, %blk_exit134 ], [ %172, %loop_entry143 ]
  store i32 %_local_2.7, i32* inttoptr (i32 819036 to i32*), align 4
  %164 = load i32, i32* inttoptr (i32 15660 to i32*), align 4
  %165 = icmp eq i32 %164, 31
  br i1 %165, label %blk_exit147, label %brif_next149

brif_next132:                                     ; preds = %blk_exit113
  %166 = shl i32 1, %144
  %167 = icmp sgt i32 %166, 1
  %168 = select i1 %167, i32 %166, i32 1
  %169 = add nsw i32 %168, -1
  %170 = and i32 %168, 7
  %171 = icmp eq i32 %170, 0
  br i1 %171, label %blk_exit134, label %loop_entry137

blk_exit134:                                      ; preds = %loop_entry137, %brif_next132
  %_param_0.14 = phi i32 [ %_local_2.6, %brif_next132 ], [ %176, %loop_entry137 ]
  %172 = add i32 %_local_2.6, %168
  %173 = icmp ult i32 %169, 7
  br i1 %173, label %blk_exit130, label %brif_next142

loop_entry137:                                    ; preds = %loop_entry137, %brif_next132
  %_param_1.13 = phi i32 [ %177, %loop_entry137 ], [ %170, %brif_next132 ]
  %_param_0.15 = phi i32 [ %176, %loop_entry137 ], [ %_local_2.6, %brif_next132 ]
  %174 = add i32 %_param_0.15, 819136
  %175 = inttoptr i32 %174 to i8*
  store i8 6, i8* %175, align 1
  %176 = add i32 %_param_0.15, 1
  %177 = add i32 %_param_1.13, -1
  %brif_val139.not = icmp eq i32 %177, 0
  br i1 %brif_val139.not, label %blk_exit134, label %loop_entry137

brif_next142:                                     ; preds = %blk_exit134
  %178 = sub i32 %_param_0.14, %172
  br label %loop_entry143

loop_entry143:                                    ; preds = %loop_entry143, %brif_next142
  %_param_1.14 = phi i32 [ 0, %brif_next142 ], [ %182, %loop_entry143 ]
  %179 = add i32 %_param_0.14, %_param_1.14
  %180 = add i32 %179, 819136
  %181 = inttoptr i32 %180 to i64*
  store i64 434041037028460038, i64* %181, align 8
  %182 = add i32 %_param_1.14, 8
  %183 = sub nuw nsw i32 -8, %_param_1.14
  %brif_val145.not = icmp eq i32 %178, %183
  br i1 %brif_val145.not, label %blk_exit130, label %loop_entry143

blk_exit147:                                      ; preds = %loop_entry160, %blk_exit151, %blk_exit130
  %_local_2.8 = phi i32 [ %_local_2.7, %blk_exit130 ], [ %192, %blk_exit151 ], [ %192, %loop_entry160 ]
  store i32 %_local_2.8, i32* inttoptr (i32 819040 to i32*), align 32
  %184 = load i32, i32* inttoptr (i32 15664 to i32*), align 16
  %185 = icmp eq i32 %184, 31
  br i1 %185, label %blk_exit164, label %brif_next166

brif_next149:                                     ; preds = %blk_exit130
  %186 = shl i32 1, %164
  %187 = icmp sgt i32 %186, 1
  %188 = select i1 %187, i32 %186, i32 1
  %189 = add nsw i32 %188, -1
  %190 = and i32 %188, 7
  %191 = icmp eq i32 %190, 0
  br i1 %191, label %blk_exit151, label %loop_entry154

blk_exit151:                                      ; preds = %loop_entry154, %brif_next149
  %_param_0.16 = phi i32 [ %_local_2.7, %brif_next149 ], [ %196, %loop_entry154 ]
  %192 = add i32 %_local_2.7, %188
  %193 = icmp ult i32 %189, 7
  br i1 %193, label %blk_exit147, label %brif_next159

loop_entry154:                                    ; preds = %loop_entry154, %brif_next149
  %_param_1.15 = phi i32 [ %197, %loop_entry154 ], [ %190, %brif_next149 ]
  %_param_0.17 = phi i32 [ %196, %loop_entry154 ], [ %_local_2.7, %brif_next149 ]
  %194 = add i32 %_param_0.17, 819136
  %195 = inttoptr i32 %194 to i8*
  store i8 7, i8* %195, align 1
  %196 = add i32 %_param_0.17, 1
  %197 = add i32 %_param_1.15, -1
  %brif_val156.not = icmp eq i32 %197, 0
  br i1 %brif_val156.not, label %blk_exit151, label %loop_entry154

brif_next159:                                     ; preds = %blk_exit151
  %198 = sub i32 %_param_0.16, %192
  br label %loop_entry160

loop_entry160:                                    ; preds = %loop_entry160, %brif_next159
  %_param_1.16 = phi i32 [ 0, %brif_next159 ], [ %202, %loop_entry160 ]
  %199 = add i32 %_param_0.16, %_param_1.16
  %200 = add i32 %199, 819136
  %201 = inttoptr i32 %200 to i64*
  store i64 506381209866536711, i64* %201, align 8
  %202 = add i32 %_param_1.16, 8
  %203 = sub nuw nsw i32 -8, %_param_1.16
  %brif_val162.not = icmp eq i32 %198, %203
  br i1 %brif_val162.not, label %blk_exit147, label %loop_entry160

blk_exit164:                                      ; preds = %loop_entry177, %blk_exit168, %blk_exit147
  %_local_2.9 = phi i32 [ %_local_2.8, %blk_exit147 ], [ %212, %blk_exit168 ], [ %212, %loop_entry177 ]
  store i32 %_local_2.9, i32* inttoptr (i32 819044 to i32*), align 4
  %204 = load i32, i32* inttoptr (i32 15668 to i32*), align 4
  %205 = icmp eq i32 %204, 31
  br i1 %205, label %blk_exit181, label %brif_next183

brif_next166:                                     ; preds = %blk_exit147
  %206 = shl i32 1, %184
  %207 = icmp sgt i32 %206, 1
  %208 = select i1 %207, i32 %206, i32 1
  %209 = add nsw i32 %208, -1
  %210 = and i32 %208, 7
  %211 = icmp eq i32 %210, 0
  br i1 %211, label %blk_exit168, label %loop_entry171

blk_exit168:                                      ; preds = %loop_entry171, %brif_next166
  %_param_0.18 = phi i32 [ %_local_2.8, %brif_next166 ], [ %216, %loop_entry171 ]
  %212 = add i32 %_local_2.8, %208
  %213 = icmp ult i32 %209, 7
  br i1 %213, label %blk_exit164, label %brif_next176

loop_entry171:                                    ; preds = %loop_entry171, %brif_next166
  %_param_1.17 = phi i32 [ %217, %loop_entry171 ], [ %210, %brif_next166 ]
  %_param_0.19 = phi i32 [ %216, %loop_entry171 ], [ %_local_2.8, %brif_next166 ]
  %214 = add i32 %_param_0.19, 819136
  %215 = inttoptr i32 %214 to i8*
  store i8 8, i8* %215, align 1
  %216 = add i32 %_param_0.19, 1
  %217 = add i32 %_param_1.17, -1
  %brif_val173.not = icmp eq i32 %217, 0
  br i1 %brif_val173.not, label %blk_exit168, label %loop_entry171

brif_next176:                                     ; preds = %blk_exit168
  %218 = sub i32 %_param_0.18, %212
  br label %loop_entry177

loop_entry177:                                    ; preds = %loop_entry177, %brif_next176
  %_param_1.18 = phi i32 [ 0, %brif_next176 ], [ %222, %loop_entry177 ]
  %219 = add i32 %_param_0.18, %_param_1.18
  %220 = add i32 %219, 819136
  %221 = inttoptr i32 %220 to i64*
  store i64 578721382704613384, i64* %221, align 8
  %222 = add i32 %_param_1.18, 8
  %223 = sub nuw nsw i32 -8, %_param_1.18
  %brif_val179.not = icmp eq i32 %218, %223
  br i1 %brif_val179.not, label %blk_exit164, label %loop_entry177

blk_exit181:                                      ; preds = %loop_entry194, %blk_exit185, %blk_exit164
  %_local_2.10 = phi i32 [ %_local_2.9, %blk_exit164 ], [ %232, %blk_exit185 ], [ %232, %loop_entry194 ]
  store i32 %_local_2.10, i32* inttoptr (i32 819048 to i32*), align 8
  %224 = load i32, i32* inttoptr (i32 15672 to i32*), align 8
  %225 = icmp eq i32 %224, 31
  br i1 %225, label %blk_exit198, label %brif_next200

brif_next183:                                     ; preds = %blk_exit164
  %226 = shl i32 1, %204
  %227 = icmp sgt i32 %226, 1
  %228 = select i1 %227, i32 %226, i32 1
  %229 = add nsw i32 %228, -1
  %230 = and i32 %228, 7
  %231 = icmp eq i32 %230, 0
  br i1 %231, label %blk_exit185, label %loop_entry188

blk_exit185:                                      ; preds = %loop_entry188, %brif_next183
  %_param_0.20 = phi i32 [ %_local_2.9, %brif_next183 ], [ %236, %loop_entry188 ]
  %232 = add i32 %_local_2.9, %228
  %233 = icmp ult i32 %229, 7
  br i1 %233, label %blk_exit181, label %brif_next193

loop_entry188:                                    ; preds = %loop_entry188, %brif_next183
  %_param_1.19 = phi i32 [ %237, %loop_entry188 ], [ %230, %brif_next183 ]
  %_param_0.21 = phi i32 [ %236, %loop_entry188 ], [ %_local_2.9, %brif_next183 ]
  %234 = add i32 %_param_0.21, 819136
  %235 = inttoptr i32 %234 to i8*
  store i8 9, i8* %235, align 1
  %236 = add i32 %_param_0.21, 1
  %237 = add i32 %_param_1.19, -1
  %brif_val190.not = icmp eq i32 %237, 0
  br i1 %brif_val190.not, label %blk_exit185, label %loop_entry188

brif_next193:                                     ; preds = %blk_exit185
  %238 = sub i32 %_param_0.20, %232
  br label %loop_entry194

loop_entry194:                                    ; preds = %loop_entry194, %brif_next193
  %_param_1.20 = phi i32 [ 0, %brif_next193 ], [ %242, %loop_entry194 ]
  %239 = add i32 %_param_0.20, %_param_1.20
  %240 = add i32 %239, 819136
  %241 = inttoptr i32 %240 to i64*
  store i64 651061555542690057, i64* %241, align 8
  %242 = add i32 %_param_1.20, 8
  %243 = sub nuw nsw i32 -8, %_param_1.20
  %brif_val196.not = icmp eq i32 %238, %243
  br i1 %brif_val196.not, label %blk_exit181, label %loop_entry194

blk_exit198:                                      ; preds = %loop_entry211, %blk_exit202, %blk_exit181
  %_local_2.11 = phi i32 [ %_local_2.10, %blk_exit181 ], [ %252, %blk_exit202 ], [ %252, %loop_entry211 ]
  store i32 %_local_2.11, i32* inttoptr (i32 819052 to i32*), align 4
  %244 = load i32, i32* inttoptr (i32 15676 to i32*), align 4
  %245 = icmp eq i32 %244, 31
  br i1 %245, label %blk_exit215, label %brif_next217

brif_next200:                                     ; preds = %blk_exit181
  %246 = shl i32 1, %224
  %247 = icmp sgt i32 %246, 1
  %248 = select i1 %247, i32 %246, i32 1
  %249 = add nsw i32 %248, -1
  %250 = and i32 %248, 7
  %251 = icmp eq i32 %250, 0
  br i1 %251, label %blk_exit202, label %loop_entry205

blk_exit202:                                      ; preds = %loop_entry205, %brif_next200
  %_param_0.22 = phi i32 [ %_local_2.10, %brif_next200 ], [ %256, %loop_entry205 ]
  %252 = add i32 %_local_2.10, %248
  %253 = icmp ult i32 %249, 7
  br i1 %253, label %blk_exit198, label %brif_next210

loop_entry205:                                    ; preds = %loop_entry205, %brif_next200
  %_param_1.21 = phi i32 [ %257, %loop_entry205 ], [ %250, %brif_next200 ]
  %_param_0.23 = phi i32 [ %256, %loop_entry205 ], [ %_local_2.10, %brif_next200 ]
  %254 = add i32 %_param_0.23, 819136
  %255 = inttoptr i32 %254 to i8*
  store i8 10, i8* %255, align 1
  %256 = add i32 %_param_0.23, 1
  %257 = add i32 %_param_1.21, -1
  %brif_val207.not = icmp eq i32 %257, 0
  br i1 %brif_val207.not, label %blk_exit202, label %loop_entry205

brif_next210:                                     ; preds = %blk_exit202
  %258 = sub i32 %_param_0.22, %252
  br label %loop_entry211

loop_entry211:                                    ; preds = %loop_entry211, %brif_next210
  %_param_1.22 = phi i32 [ 0, %brif_next210 ], [ %262, %loop_entry211 ]
  %259 = add i32 %_param_0.22, %_param_1.22
  %260 = add i32 %259, 819136
  %261 = inttoptr i32 %260 to i64*
  store i64 723401728380766730, i64* %261, align 8
  %262 = add i32 %_param_1.22, 8
  %263 = sub nuw nsw i32 -8, %_param_1.22
  %brif_val213.not = icmp eq i32 %258, %263
  br i1 %brif_val213.not, label %blk_exit198, label %loop_entry211

blk_exit215:                                      ; preds = %loop_entry228, %blk_exit219, %blk_exit198
  %_local_2.12 = phi i32 [ %_local_2.11, %blk_exit198 ], [ %272, %blk_exit219 ], [ %272, %loop_entry228 ]
  store i32 %_local_2.12, i32* inttoptr (i32 819056 to i32*), align 16
  %264 = load i32, i32* inttoptr (i32 15680 to i32*), align 64
  %265 = icmp eq i32 %264, 31
  br i1 %265, label %blk_exit232, label %brif_next234

brif_next217:                                     ; preds = %blk_exit198
  %266 = shl i32 1, %244
  %267 = icmp sgt i32 %266, 1
  %268 = select i1 %267, i32 %266, i32 1
  %269 = add nsw i32 %268, -1
  %270 = and i32 %268, 7
  %271 = icmp eq i32 %270, 0
  br i1 %271, label %blk_exit219, label %loop_entry222

blk_exit219:                                      ; preds = %loop_entry222, %brif_next217
  %_param_0.24 = phi i32 [ %_local_2.11, %brif_next217 ], [ %276, %loop_entry222 ]
  %272 = add i32 %_local_2.11, %268
  %273 = icmp ult i32 %269, 7
  br i1 %273, label %blk_exit215, label %brif_next227

loop_entry222:                                    ; preds = %loop_entry222, %brif_next217
  %_param_1.23 = phi i32 [ %277, %loop_entry222 ], [ %270, %brif_next217 ]
  %_param_0.25 = phi i32 [ %276, %loop_entry222 ], [ %_local_2.11, %brif_next217 ]
  %274 = add i32 %_param_0.25, 819136
  %275 = inttoptr i32 %274 to i8*
  store i8 11, i8* %275, align 1
  %276 = add i32 %_param_0.25, 1
  %277 = add i32 %_param_1.23, -1
  %brif_val224.not = icmp eq i32 %277, 0
  br i1 %brif_val224.not, label %blk_exit219, label %loop_entry222

brif_next227:                                     ; preds = %blk_exit219
  %278 = sub i32 %_param_0.24, %272
  br label %loop_entry228

loop_entry228:                                    ; preds = %loop_entry228, %brif_next227
  %_param_1.24 = phi i32 [ 0, %brif_next227 ], [ %282, %loop_entry228 ]
  %279 = add i32 %_param_0.24, %_param_1.24
  %280 = add i32 %279, 819136
  %281 = inttoptr i32 %280 to i64*
  store i64 795741901218843403, i64* %281, align 8
  %282 = add i32 %_param_1.24, 8
  %283 = sub nuw nsw i32 -8, %_param_1.24
  %brif_val230.not = icmp eq i32 %278, %283
  br i1 %brif_val230.not, label %blk_exit215, label %loop_entry228

blk_exit232:                                      ; preds = %loop_entry245, %blk_exit236, %blk_exit215
  %_local_2.13 = phi i32 [ %_local_2.12, %blk_exit215 ], [ %292, %blk_exit236 ], [ %292, %loop_entry245 ]
  store i32 %_local_2.13, i32* inttoptr (i32 819060 to i32*), align 4
  %284 = load i32, i32* inttoptr (i32 15684 to i32*), align 4
  %285 = icmp eq i32 %284, 31
  br i1 %285, label %blk_exit249, label %brif_next251

brif_next234:                                     ; preds = %blk_exit215
  %286 = shl i32 1, %264
  %287 = icmp sgt i32 %286, 1
  %288 = select i1 %287, i32 %286, i32 1
  %289 = add nsw i32 %288, -1
  %290 = and i32 %288, 7
  %291 = icmp eq i32 %290, 0
  br i1 %291, label %blk_exit236, label %loop_entry239

blk_exit236:                                      ; preds = %loop_entry239, %brif_next234
  %_param_0.26 = phi i32 [ %_local_2.12, %brif_next234 ], [ %296, %loop_entry239 ]
  %292 = add i32 %_local_2.12, %288
  %293 = icmp ult i32 %289, 7
  br i1 %293, label %blk_exit232, label %brif_next244

loop_entry239:                                    ; preds = %loop_entry239, %brif_next234
  %_param_1.25 = phi i32 [ %297, %loop_entry239 ], [ %290, %brif_next234 ]
  %_param_0.27 = phi i32 [ %296, %loop_entry239 ], [ %_local_2.12, %brif_next234 ]
  %294 = add i32 %_param_0.27, 819136
  %295 = inttoptr i32 %294 to i8*
  store i8 12, i8* %295, align 1
  %296 = add i32 %_param_0.27, 1
  %297 = add i32 %_param_1.25, -1
  %brif_val241.not = icmp eq i32 %297, 0
  br i1 %brif_val241.not, label %blk_exit236, label %loop_entry239

brif_next244:                                     ; preds = %blk_exit236
  %298 = sub i32 %_param_0.26, %292
  br label %loop_entry245

loop_entry245:                                    ; preds = %loop_entry245, %brif_next244
  %_param_1.26 = phi i32 [ 0, %brif_next244 ], [ %302, %loop_entry245 ]
  %299 = add i32 %_param_0.26, %_param_1.26
  %300 = add i32 %299, 819136
  %301 = inttoptr i32 %300 to i64*
  store i64 868082074056920076, i64* %301, align 8
  %302 = add i32 %_param_1.26, 8
  %303 = sub nuw nsw i32 -8, %_param_1.26
  %brif_val247.not = icmp eq i32 %298, %303
  br i1 %brif_val247.not, label %blk_exit232, label %loop_entry245

blk_exit249:                                      ; preds = %loop_entry262, %blk_exit253, %blk_exit232
  %_local_2.14 = phi i32 [ %_local_2.13, %blk_exit232 ], [ %312, %blk_exit253 ], [ %312, %loop_entry262 ]
  store i32 %_local_2.14, i32* inttoptr (i32 819064 to i32*), align 8
  %304 = load i32, i32* inttoptr (i32 15688 to i32*), align 8
  %305 = icmp eq i32 %304, 31
  br i1 %305, label %blk_exit266, label %brif_next268

brif_next251:                                     ; preds = %blk_exit232
  %306 = shl i32 1, %284
  %307 = icmp sgt i32 %306, 1
  %308 = select i1 %307, i32 %306, i32 1
  %309 = add nsw i32 %308, -1
  %310 = and i32 %308, 7
  %311 = icmp eq i32 %310, 0
  br i1 %311, label %blk_exit253, label %loop_entry256

blk_exit253:                                      ; preds = %loop_entry256, %brif_next251
  %_param_0.28 = phi i32 [ %_local_2.13, %brif_next251 ], [ %316, %loop_entry256 ]
  %312 = add i32 %_local_2.13, %308
  %313 = icmp ult i32 %309, 7
  br i1 %313, label %blk_exit249, label %brif_next261

loop_entry256:                                    ; preds = %loop_entry256, %brif_next251
  %_param_1.27 = phi i32 [ %317, %loop_entry256 ], [ %310, %brif_next251 ]
  %_param_0.29 = phi i32 [ %316, %loop_entry256 ], [ %_local_2.13, %brif_next251 ]
  %314 = add i32 %_param_0.29, 819136
  %315 = inttoptr i32 %314 to i8*
  store i8 13, i8* %315, align 1
  %316 = add i32 %_param_0.29, 1
  %317 = add i32 %_param_1.27, -1
  %brif_val258.not = icmp eq i32 %317, 0
  br i1 %brif_val258.not, label %blk_exit253, label %loop_entry256

brif_next261:                                     ; preds = %blk_exit253
  %318 = sub i32 %_param_0.28, %312
  br label %loop_entry262

loop_entry262:                                    ; preds = %loop_entry262, %brif_next261
  %_param_1.28 = phi i32 [ 0, %brif_next261 ], [ %322, %loop_entry262 ]
  %319 = add i32 %_param_0.28, %_param_1.28
  %320 = add i32 %319, 819136
  %321 = inttoptr i32 %320 to i64*
  store i64 940422246894996749, i64* %321, align 8
  %322 = add i32 %_param_1.28, 8
  %323 = sub nuw nsw i32 -8, %_param_1.28
  %brif_val264.not = icmp eq i32 %318, %323
  br i1 %brif_val264.not, label %blk_exit249, label %loop_entry262

blk_exit266:                                      ; preds = %loop_entry279, %blk_exit270, %blk_exit249
  %_local_2.15 = phi i32 [ %_local_2.14, %blk_exit249 ], [ %332, %blk_exit270 ], [ %332, %loop_entry279 ]
  store i32 %_local_2.15, i32* inttoptr (i32 819068 to i32*), align 4
  %324 = load i32, i32* inttoptr (i32 15692 to i32*), align 4
  %325 = icmp eq i32 %324, 31
  br i1 %325, label %blk_exit283, label %brif_next285

brif_next268:                                     ; preds = %blk_exit249
  %326 = shl i32 1, %304
  %327 = icmp sgt i32 %326, 1
  %328 = select i1 %327, i32 %326, i32 1
  %329 = add nsw i32 %328, -1
  %330 = and i32 %328, 7
  %331 = icmp eq i32 %330, 0
  br i1 %331, label %blk_exit270, label %loop_entry273

blk_exit270:                                      ; preds = %loop_entry273, %brif_next268
  %_param_0.30 = phi i32 [ %_local_2.14, %brif_next268 ], [ %336, %loop_entry273 ]
  %332 = add i32 %_local_2.14, %328
  %333 = icmp ult i32 %329, 7
  br i1 %333, label %blk_exit266, label %brif_next278

loop_entry273:                                    ; preds = %loop_entry273, %brif_next268
  %_param_1.29 = phi i32 [ %337, %loop_entry273 ], [ %330, %brif_next268 ]
  %_param_0.31 = phi i32 [ %336, %loop_entry273 ], [ %_local_2.14, %brif_next268 ]
  %334 = add i32 %_param_0.31, 819136
  %335 = inttoptr i32 %334 to i8*
  store i8 14, i8* %335, align 1
  %336 = add i32 %_param_0.31, 1
  %337 = add i32 %_param_1.29, -1
  %brif_val275.not = icmp eq i32 %337, 0
  br i1 %brif_val275.not, label %blk_exit270, label %loop_entry273

brif_next278:                                     ; preds = %blk_exit270
  %338 = sub i32 %_param_0.30, %332
  br label %loop_entry279

loop_entry279:                                    ; preds = %loop_entry279, %brif_next278
  %_param_1.30 = phi i32 [ 0, %brif_next278 ], [ %342, %loop_entry279 ]
  %339 = add i32 %_param_0.30, %_param_1.30
  %340 = add i32 %339, 819136
  %341 = inttoptr i32 %340 to i64*
  store i64 1012762419733073422, i64* %341, align 8
  %342 = add i32 %_param_1.30, 8
  %343 = sub nuw nsw i32 -8, %_param_1.30
  %brif_val281.not = icmp eq i32 %338, %343
  br i1 %brif_val281.not, label %blk_exit266, label %loop_entry279

blk_exit283:                                      ; preds = %loop_entry296, %blk_exit287, %blk_exit266
  %_local_2.16 = phi i32 [ %_local_2.15, %blk_exit266 ], [ %355, %blk_exit287 ], [ %355, %loop_entry296 ]
  %344 = and i32 %_local_2.16, -128
  store i32 %344, i32* inttoptr (i32 819072 to i32*), align 128
  %345 = ashr i32 %_local_2.16, 7
  %346 = load i32, i32* inttoptr (i32 15696 to i32*), align 16
  %347 = add i32 %346, -7
  %348 = icmp eq i32 %347, 31
  br i1 %348, label %blk_exit300, label %brif_next302

brif_next285:                                     ; preds = %blk_exit266
  %349 = shl i32 1, %324
  %350 = icmp sgt i32 %349, 1
  %351 = select i1 %350, i32 %349, i32 1
  %352 = add nsw i32 %351, -1
  %353 = and i32 %351, 7
  %354 = icmp eq i32 %353, 0
  br i1 %354, label %blk_exit287, label %loop_entry290

blk_exit287:                                      ; preds = %loop_entry290, %brif_next285
  %_param_0.32 = phi i32 [ %_local_2.15, %brif_next285 ], [ %359, %loop_entry290 ]
  %355 = add i32 %_local_2.15, %351
  %356 = icmp ult i32 %352, 7
  br i1 %356, label %blk_exit283, label %brif_next295

loop_entry290:                                    ; preds = %loop_entry290, %brif_next285
  %_param_1.31 = phi i32 [ %360, %loop_entry290 ], [ %353, %brif_next285 ]
  %_param_0.33 = phi i32 [ %359, %loop_entry290 ], [ %_local_2.15, %brif_next285 ]
  %357 = add i32 %_param_0.33, 819136
  %358 = inttoptr i32 %357 to i8*
  store i8 15, i8* %358, align 1
  %359 = add i32 %_param_0.33, 1
  %360 = add i32 %_param_1.31, -1
  %brif_val292.not = icmp eq i32 %360, 0
  br i1 %brif_val292.not, label %blk_exit287, label %loop_entry290

brif_next295:                                     ; preds = %blk_exit287
  %361 = sub i32 %_param_0.32, %355
  br label %loop_entry296

loop_entry296:                                    ; preds = %loop_entry296, %brif_next295
  %_param_1.32 = phi i32 [ 0, %brif_next295 ], [ %365, %loop_entry296 ]
  %362 = add i32 %_param_0.32, %_param_1.32
  %363 = add i32 %362, 819136
  %364 = inttoptr i32 %363 to i64*
  store i64 1085102592571150095, i64* %364, align 8
  %365 = add i32 %_param_1.32, 8
  %366 = sub nuw nsw i32 -8, %_param_1.32
  %brif_val298.not = icmp eq i32 %361, %366
  br i1 %brif_val298.not, label %blk_exit283, label %loop_entry296

blk_exit300:                                      ; preds = %blk_exit311, %blk_exit283
  %_local_2.17 = phi i32 [ %345, %blk_exit283 ], [ %_local_2.19, %blk_exit311 ]
  %_param_0.34 = phi i32 [ %344, %blk_exit283 ], [ %383, %blk_exit311 ]
  store i32 %_param_0.34, i32* inttoptr (i32 819076 to i32*), align 4
  %367 = load i32, i32* inttoptr (i32 15700 to i32*), align 4
  %368 = add i32 %367, -7
  %369 = icmp eq i32 %368, 31
  br i1 %369, label %blk_exit318, label %brif_next320

brif_next302:                                     ; preds = %blk_exit283
  %370 = shl i32 1, %347
  %371 = icmp sgt i32 %370, 1
  %372 = select i1 %371, i32 %370, i32 1
  %373 = and i32 %372, 3
  %374 = add nsw i32 %372, -1
  %375 = icmp ult i32 %374, 3
  br i1 %375, label %blk_exit304, label %brif_next306

blk_exit304:                                      ; preds = %brif_next310, %brif_next302
  %_local_2.18 = phi i32 [ %345, %brif_next302 ], [ %382, %brif_next310 ]
  %376 = icmp eq i32 %373, 0
  br i1 %376, label %blk_exit311, label %loop_entry314

brif_next306:                                     ; preds = %brif_next302
  %377 = and i32 %372, 2147483644
  br label %loop_entry307

loop_entry307:                                    ; preds = %loop_entry307, %brif_next306
  %_param_0.35 = phi i32 [ 0, %brif_next306 ], [ %381, %loop_entry307 ]
  %378 = add i32 %345, %_param_0.35
  %379 = add i32 %378, 819392
  %380 = inttoptr i32 %379 to i32*
  store i32 269488144, i32* %380, align 4
  %381 = add i32 %_param_0.35, 4
  %.not575 = icmp eq i32 %377, %381
  br i1 %.not575, label %brif_next310, label %loop_entry307

brif_next310:                                     ; preds = %loop_entry307
  %382 = add i32 %345, %377
  br label %blk_exit304

blk_exit311:                                      ; preds = %loop_entry314, %blk_exit304
  %_local_2.19 = phi i32 [ %_local_2.18, %blk_exit304 ], [ %386, %loop_entry314 ]
  %383 = shl i32 %_local_2.19, 7
  br label %blk_exit300

loop_entry314:                                    ; preds = %loop_entry314, %blk_exit304
  %_local_2.20 = phi i32 [ %_local_2.18, %blk_exit304 ], [ %386, %loop_entry314 ]
  %_param_1.33 = phi i32 [ %373, %blk_exit304 ], [ %387, %loop_entry314 ]
  %384 = add i32 %_local_2.20, 819392
  %385 = inttoptr i32 %384 to i8*
  store i8 16, i8* %385, align 1
  %386 = add i32 %_local_2.20, 1
  %387 = add i32 %_param_1.33, -1
  %brif_val316.not = icmp eq i32 %387, 0
  br i1 %brif_val316.not, label %blk_exit311, label %loop_entry314

blk_exit318:                                      ; preds = %blk_exit329, %blk_exit300
  %_local_2.21 = phi i32 [ %_local_2.17, %blk_exit300 ], [ %_local_2.23, %blk_exit329 ]
  %_param_0.36 = phi i32 [ %_param_0.34, %blk_exit300 ], [ %404, %blk_exit329 ]
  store i32 %_param_0.36, i32* inttoptr (i32 819080 to i32*), align 8
  %388 = load i32, i32* inttoptr (i32 15704 to i32*), align 8
  %389 = add i32 %388, -7
  %390 = icmp eq i32 %389, 31
  br i1 %390, label %blk_exit336, label %brif_next338

brif_next320:                                     ; preds = %blk_exit300
  %391 = shl i32 1, %368
  %392 = icmp sgt i32 %391, 1
  %393 = select i1 %392, i32 %391, i32 1
  %394 = and i32 %393, 3
  %395 = add nsw i32 %393, -1
  %396 = icmp ult i32 %395, 3
  br i1 %396, label %blk_exit322, label %brif_next324

blk_exit322:                                      ; preds = %brif_next328, %brif_next320
  %_local_2.22 = phi i32 [ %_local_2.17, %brif_next320 ], [ %403, %brif_next328 ]
  %397 = icmp eq i32 %394, 0
  br i1 %397, label %blk_exit329, label %loop_entry332

brif_next324:                                     ; preds = %brif_next320
  %398 = and i32 %393, 2147483644
  br label %loop_entry325

loop_entry325:                                    ; preds = %loop_entry325, %brif_next324
  %_param_0.37 = phi i32 [ 0, %brif_next324 ], [ %402, %loop_entry325 ]
  %399 = add i32 %_local_2.17, %_param_0.37
  %400 = add i32 %399, 819392
  %401 = inttoptr i32 %400 to i32*
  store i32 286331153, i32* %401, align 4
  %402 = add i32 %_param_0.37, 4
  %.not576 = icmp eq i32 %398, %402
  br i1 %.not576, label %brif_next328, label %loop_entry325

brif_next328:                                     ; preds = %loop_entry325
  %403 = add i32 %_local_2.17, %398
  br label %blk_exit322

blk_exit329:                                      ; preds = %loop_entry332, %blk_exit322
  %_local_2.23 = phi i32 [ %_local_2.22, %blk_exit322 ], [ %407, %loop_entry332 ]
  %404 = shl i32 %_local_2.23, 7
  br label %blk_exit318

loop_entry332:                                    ; preds = %loop_entry332, %blk_exit322
  %_local_2.24 = phi i32 [ %_local_2.22, %blk_exit322 ], [ %407, %loop_entry332 ]
  %_param_1.34 = phi i32 [ %394, %blk_exit322 ], [ %408, %loop_entry332 ]
  %405 = add i32 %_local_2.24, 819392
  %406 = inttoptr i32 %405 to i8*
  store i8 17, i8* %406, align 1
  %407 = add i32 %_local_2.24, 1
  %408 = add i32 %_param_1.34, -1
  %brif_val334.not = icmp eq i32 %408, 0
  br i1 %brif_val334.not, label %blk_exit329, label %loop_entry332

blk_exit336:                                      ; preds = %blk_exit347, %blk_exit318
  %_local_2.25 = phi i32 [ %_local_2.21, %blk_exit318 ], [ %_local_2.27, %blk_exit347 ]
  %_param_0.38 = phi i32 [ %_param_0.36, %blk_exit318 ], [ %425, %blk_exit347 ]
  store i32 %_param_0.38, i32* inttoptr (i32 819084 to i32*), align 4
  %409 = load i32, i32* inttoptr (i32 15708 to i32*), align 4
  %410 = add i32 %409, -7
  %411 = icmp eq i32 %410, 31
  br i1 %411, label %blk_exit354, label %brif_next356

brif_next338:                                     ; preds = %blk_exit318
  %412 = shl i32 1, %389
  %413 = icmp sgt i32 %412, 1
  %414 = select i1 %413, i32 %412, i32 1
  %415 = and i32 %414, 3
  %416 = add nsw i32 %414, -1
  %417 = icmp ult i32 %416, 3
  br i1 %417, label %blk_exit340, label %brif_next342

blk_exit340:                                      ; preds = %brif_next346, %brif_next338
  %_local_2.26 = phi i32 [ %_local_2.21, %brif_next338 ], [ %424, %brif_next346 ]
  %418 = icmp eq i32 %415, 0
  br i1 %418, label %blk_exit347, label %loop_entry350

brif_next342:                                     ; preds = %brif_next338
  %419 = and i32 %414, 2147483644
  br label %loop_entry343

loop_entry343:                                    ; preds = %loop_entry343, %brif_next342
  %_param_0.39 = phi i32 [ 0, %brif_next342 ], [ %423, %loop_entry343 ]
  %420 = add i32 %_local_2.21, %_param_0.39
  %421 = add i32 %420, 819392
  %422 = inttoptr i32 %421 to i32*
  store i32 303174162, i32* %422, align 4
  %423 = add i32 %_param_0.39, 4
  %.not577 = icmp eq i32 %419, %423
  br i1 %.not577, label %brif_next346, label %loop_entry343

brif_next346:                                     ; preds = %loop_entry343
  %424 = add i32 %_local_2.21, %419
  br label %blk_exit340

blk_exit347:                                      ; preds = %loop_entry350, %blk_exit340
  %_local_2.27 = phi i32 [ %_local_2.26, %blk_exit340 ], [ %428, %loop_entry350 ]
  %425 = shl i32 %_local_2.27, 7
  br label %blk_exit336

loop_entry350:                                    ; preds = %loop_entry350, %blk_exit340
  %_local_2.28 = phi i32 [ %_local_2.26, %blk_exit340 ], [ %428, %loop_entry350 ]
  %_param_1.35 = phi i32 [ %415, %blk_exit340 ], [ %429, %loop_entry350 ]
  %426 = add i32 %_local_2.28, 819392
  %427 = inttoptr i32 %426 to i8*
  store i8 18, i8* %427, align 1
  %428 = add i32 %_local_2.28, 1
  %429 = add i32 %_param_1.35, -1
  %brif_val352.not = icmp eq i32 %429, 0
  br i1 %brif_val352.not, label %blk_exit347, label %loop_entry350

blk_exit354:                                      ; preds = %blk_exit365, %blk_exit336
  %_local_2.29 = phi i32 [ %_local_2.25, %blk_exit336 ], [ %_local_2.31, %blk_exit365 ]
  %_param_0.40 = phi i32 [ %_param_0.38, %blk_exit336 ], [ %446, %blk_exit365 ]
  store i32 %_param_0.40, i32* inttoptr (i32 819088 to i32*), align 16
  %430 = load i32, i32* inttoptr (i32 15712 to i32*), align 32
  %431 = add i32 %430, -7
  %432 = icmp eq i32 %431, 31
  br i1 %432, label %blk_exit372, label %brif_next374

brif_next356:                                     ; preds = %blk_exit336
  %433 = shl i32 1, %410
  %434 = icmp sgt i32 %433, 1
  %435 = select i1 %434, i32 %433, i32 1
  %436 = and i32 %435, 3
  %437 = add nsw i32 %435, -1
  %438 = icmp ult i32 %437, 3
  br i1 %438, label %blk_exit358, label %brif_next360

blk_exit358:                                      ; preds = %brif_next364, %brif_next356
  %_local_2.30 = phi i32 [ %_local_2.25, %brif_next356 ], [ %445, %brif_next364 ]
  %439 = icmp eq i32 %436, 0
  br i1 %439, label %blk_exit365, label %loop_entry368

brif_next360:                                     ; preds = %brif_next356
  %440 = and i32 %435, 2147483644
  br label %loop_entry361

loop_entry361:                                    ; preds = %loop_entry361, %brif_next360
  %_param_0.41 = phi i32 [ 0, %brif_next360 ], [ %444, %loop_entry361 ]
  %441 = add i32 %_local_2.25, %_param_0.41
  %442 = add i32 %441, 819392
  %443 = inttoptr i32 %442 to i32*
  store i32 320017171, i32* %443, align 4
  %444 = add i32 %_param_0.41, 4
  %.not578 = icmp eq i32 %440, %444
  br i1 %.not578, label %brif_next364, label %loop_entry361

brif_next364:                                     ; preds = %loop_entry361
  %445 = add i32 %_local_2.25, %440
  br label %blk_exit358

blk_exit365:                                      ; preds = %loop_entry368, %blk_exit358
  %_local_2.31 = phi i32 [ %_local_2.30, %blk_exit358 ], [ %449, %loop_entry368 ]
  %446 = shl i32 %_local_2.31, 7
  br label %blk_exit354

loop_entry368:                                    ; preds = %loop_entry368, %blk_exit358
  %_local_2.32 = phi i32 [ %_local_2.30, %blk_exit358 ], [ %449, %loop_entry368 ]
  %_param_1.36 = phi i32 [ %436, %blk_exit358 ], [ %450, %loop_entry368 ]
  %447 = add i32 %_local_2.32, 819392
  %448 = inttoptr i32 %447 to i8*
  store i8 19, i8* %448, align 1
  %449 = add i32 %_local_2.32, 1
  %450 = add i32 %_param_1.36, -1
  %brif_val370.not = icmp eq i32 %450, 0
  br i1 %brif_val370.not, label %blk_exit365, label %loop_entry368

blk_exit372:                                      ; preds = %blk_exit383, %blk_exit354
  %_local_2.33 = phi i32 [ %_local_2.29, %blk_exit354 ], [ %_local_2.35, %blk_exit383 ]
  %_param_0.42 = phi i32 [ %_param_0.40, %blk_exit354 ], [ %467, %blk_exit383 ]
  store i32 %_param_0.42, i32* inttoptr (i32 819092 to i32*), align 4
  %451 = load i32, i32* inttoptr (i32 15716 to i32*), align 4
  %452 = add i32 %451, -7
  %453 = icmp eq i32 %452, 31
  br i1 %453, label %blk_exit390, label %brif_next392

brif_next374:                                     ; preds = %blk_exit354
  %454 = shl i32 1, %431
  %455 = icmp sgt i32 %454, 1
  %456 = select i1 %455, i32 %454, i32 1
  %457 = and i32 %456, 3
  %458 = add nsw i32 %456, -1
  %459 = icmp ult i32 %458, 3
  br i1 %459, label %blk_exit376, label %brif_next378

blk_exit376:                                      ; preds = %brif_next382, %brif_next374
  %_local_2.34 = phi i32 [ %_local_2.29, %brif_next374 ], [ %466, %brif_next382 ]
  %460 = icmp eq i32 %457, 0
  br i1 %460, label %blk_exit383, label %loop_entry386

brif_next378:                                     ; preds = %brif_next374
  %461 = and i32 %456, 2147483644
  br label %loop_entry379

loop_entry379:                                    ; preds = %loop_entry379, %brif_next378
  %_param_0.43 = phi i32 [ 0, %brif_next378 ], [ %465, %loop_entry379 ]
  %462 = add i32 %_local_2.29, %_param_0.43
  %463 = add i32 %462, 819392
  %464 = inttoptr i32 %463 to i32*
  store i32 336860180, i32* %464, align 4
  %465 = add i32 %_param_0.43, 4
  %.not579 = icmp eq i32 %461, %465
  br i1 %.not579, label %brif_next382, label %loop_entry379

brif_next382:                                     ; preds = %loop_entry379
  %466 = add i32 %_local_2.29, %461
  br label %blk_exit376

blk_exit383:                                      ; preds = %loop_entry386, %blk_exit376
  %_local_2.35 = phi i32 [ %_local_2.34, %blk_exit376 ], [ %470, %loop_entry386 ]
  %467 = shl i32 %_local_2.35, 7
  br label %blk_exit372

loop_entry386:                                    ; preds = %loop_entry386, %blk_exit376
  %_local_2.36 = phi i32 [ %_local_2.34, %blk_exit376 ], [ %470, %loop_entry386 ]
  %_param_1.37 = phi i32 [ %457, %blk_exit376 ], [ %471, %loop_entry386 ]
  %468 = add i32 %_local_2.36, 819392
  %469 = inttoptr i32 %468 to i8*
  store i8 20, i8* %469, align 1
  %470 = add i32 %_local_2.36, 1
  %471 = add i32 %_param_1.37, -1
  %brif_val388.not = icmp eq i32 %471, 0
  br i1 %brif_val388.not, label %blk_exit383, label %loop_entry386

blk_exit390:                                      ; preds = %blk_exit401, %blk_exit372
  %_local_2.37 = phi i32 [ %_local_2.33, %blk_exit372 ], [ %_local_2.39, %blk_exit401 ]
  %_param_0.44 = phi i32 [ %_param_0.42, %blk_exit372 ], [ %488, %blk_exit401 ]
  store i32 %_param_0.44, i32* inttoptr (i32 819096 to i32*), align 8
  %472 = load i32, i32* inttoptr (i32 15720 to i32*), align 8
  %473 = add i32 %472, -7
  %474 = icmp eq i32 %473, 31
  br i1 %474, label %blk_exit408, label %brif_next410

brif_next392:                                     ; preds = %blk_exit372
  %475 = shl i32 1, %452
  %476 = icmp sgt i32 %475, 1
  %477 = select i1 %476, i32 %475, i32 1
  %478 = and i32 %477, 3
  %479 = add nsw i32 %477, -1
  %480 = icmp ult i32 %479, 3
  br i1 %480, label %blk_exit394, label %brif_next396

blk_exit394:                                      ; preds = %brif_next400, %brif_next392
  %_local_2.38 = phi i32 [ %_local_2.33, %brif_next392 ], [ %487, %brif_next400 ]
  %481 = icmp eq i32 %478, 0
  br i1 %481, label %blk_exit401, label %loop_entry404

brif_next396:                                     ; preds = %brif_next392
  %482 = and i32 %477, 2147483644
  br label %loop_entry397

loop_entry397:                                    ; preds = %loop_entry397, %brif_next396
  %_param_0.45 = phi i32 [ 0, %brif_next396 ], [ %486, %loop_entry397 ]
  %483 = add i32 %_local_2.33, %_param_0.45
  %484 = add i32 %483, 819392
  %485 = inttoptr i32 %484 to i32*
  store i32 353703189, i32* %485, align 4
  %486 = add i32 %_param_0.45, 4
  %.not580 = icmp eq i32 %482, %486
  br i1 %.not580, label %brif_next400, label %loop_entry397

brif_next400:                                     ; preds = %loop_entry397
  %487 = add i32 %_local_2.33, %482
  br label %blk_exit394

blk_exit401:                                      ; preds = %loop_entry404, %blk_exit394
  %_local_2.39 = phi i32 [ %_local_2.38, %blk_exit394 ], [ %491, %loop_entry404 ]
  %488 = shl i32 %_local_2.39, 7
  br label %blk_exit390

loop_entry404:                                    ; preds = %loop_entry404, %blk_exit394
  %_local_2.40 = phi i32 [ %_local_2.38, %blk_exit394 ], [ %491, %loop_entry404 ]
  %_param_1.38 = phi i32 [ %478, %blk_exit394 ], [ %492, %loop_entry404 ]
  %489 = add i32 %_local_2.40, 819392
  %490 = inttoptr i32 %489 to i8*
  store i8 21, i8* %490, align 1
  %491 = add i32 %_local_2.40, 1
  %492 = add i32 %_param_1.38, -1
  %brif_val406.not = icmp eq i32 %492, 0
  br i1 %brif_val406.not, label %blk_exit401, label %loop_entry404

blk_exit408:                                      ; preds = %blk_exit419, %blk_exit390
  %_local_2.41 = phi i32 [ %_local_2.37, %blk_exit390 ], [ %_local_2.43, %blk_exit419 ]
  %_param_0.46 = phi i32 [ %_param_0.44, %blk_exit390 ], [ %509, %blk_exit419 ]
  store i32 %_param_0.46, i32* inttoptr (i32 819100 to i32*), align 4
  %493 = load i32, i32* inttoptr (i32 15724 to i32*), align 4
  %494 = add i32 %493, -7
  %495 = icmp eq i32 %494, 31
  br i1 %495, label %blk_exit426, label %brif_next428

brif_next410:                                     ; preds = %blk_exit390
  %496 = shl i32 1, %473
  %497 = icmp sgt i32 %496, 1
  %498 = select i1 %497, i32 %496, i32 1
  %499 = and i32 %498, 3
  %500 = add nsw i32 %498, -1
  %501 = icmp ult i32 %500, 3
  br i1 %501, label %blk_exit412, label %brif_next414

blk_exit412:                                      ; preds = %brif_next418, %brif_next410
  %_local_2.42 = phi i32 [ %_local_2.37, %brif_next410 ], [ %508, %brif_next418 ]
  %502 = icmp eq i32 %499, 0
  br i1 %502, label %blk_exit419, label %loop_entry422

brif_next414:                                     ; preds = %brif_next410
  %503 = and i32 %498, 2147483644
  br label %loop_entry415

loop_entry415:                                    ; preds = %loop_entry415, %brif_next414
  %_param_0.47 = phi i32 [ 0, %brif_next414 ], [ %507, %loop_entry415 ]
  %504 = add i32 %_local_2.37, %_param_0.47
  %505 = add i32 %504, 819392
  %506 = inttoptr i32 %505 to i32*
  store i32 370546198, i32* %506, align 4
  %507 = add i32 %_param_0.47, 4
  %.not581 = icmp eq i32 %503, %507
  br i1 %.not581, label %brif_next418, label %loop_entry415

brif_next418:                                     ; preds = %loop_entry415
  %508 = add i32 %_local_2.37, %503
  br label %blk_exit412

blk_exit419:                                      ; preds = %loop_entry422, %blk_exit412
  %_local_2.43 = phi i32 [ %_local_2.42, %blk_exit412 ], [ %512, %loop_entry422 ]
  %509 = shl i32 %_local_2.43, 7
  br label %blk_exit408

loop_entry422:                                    ; preds = %loop_entry422, %blk_exit412
  %_local_2.44 = phi i32 [ %_local_2.42, %blk_exit412 ], [ %512, %loop_entry422 ]
  %_param_1.39 = phi i32 [ %499, %blk_exit412 ], [ %513, %loop_entry422 ]
  %510 = add i32 %_local_2.44, 819392
  %511 = inttoptr i32 %510 to i8*
  store i8 22, i8* %511, align 1
  %512 = add i32 %_local_2.44, 1
  %513 = add i32 %_param_1.39, -1
  %brif_val424.not = icmp eq i32 %513, 0
  br i1 %brif_val424.not, label %blk_exit419, label %loop_entry422

blk_exit426:                                      ; preds = %blk_exit437, %blk_exit408
  %_local_2.45 = phi i32 [ %_local_2.41, %blk_exit408 ], [ %_local_2.47, %blk_exit437 ]
  %_param_0.48 = phi i32 [ %_param_0.46, %blk_exit408 ], [ %530, %blk_exit437 ]
  store i32 %_param_0.48, i32* inttoptr (i32 819104 to i32*), align 32
  %514 = load i32, i32* inttoptr (i32 15728 to i32*), align 16
  %515 = add i32 %514, -7
  %516 = icmp eq i32 %515, 31
  br i1 %516, label %blk_exit444, label %brif_next446

brif_next428:                                     ; preds = %blk_exit408
  %517 = shl i32 1, %494
  %518 = icmp sgt i32 %517, 1
  %519 = select i1 %518, i32 %517, i32 1
  %520 = and i32 %519, 3
  %521 = add nsw i32 %519, -1
  %522 = icmp ult i32 %521, 3
  br i1 %522, label %blk_exit430, label %brif_next432

blk_exit430:                                      ; preds = %brif_next436, %brif_next428
  %_local_2.46 = phi i32 [ %_local_2.41, %brif_next428 ], [ %529, %brif_next436 ]
  %523 = icmp eq i32 %520, 0
  br i1 %523, label %blk_exit437, label %loop_entry440

brif_next432:                                     ; preds = %brif_next428
  %524 = and i32 %519, 2147483644
  br label %loop_entry433

loop_entry433:                                    ; preds = %loop_entry433, %brif_next432
  %_param_0.49 = phi i32 [ 0, %brif_next432 ], [ %528, %loop_entry433 ]
  %525 = add i32 %_local_2.41, %_param_0.49
  %526 = add i32 %525, 819392
  %527 = inttoptr i32 %526 to i32*
  store i32 387389207, i32* %527, align 4
  %528 = add i32 %_param_0.49, 4
  %.not582 = icmp eq i32 %524, %528
  br i1 %.not582, label %brif_next436, label %loop_entry433

brif_next436:                                     ; preds = %loop_entry433
  %529 = add i32 %_local_2.41, %524
  br label %blk_exit430

blk_exit437:                                      ; preds = %loop_entry440, %blk_exit430
  %_local_2.47 = phi i32 [ %_local_2.46, %blk_exit430 ], [ %533, %loop_entry440 ]
  %530 = shl i32 %_local_2.47, 7
  br label %blk_exit426

loop_entry440:                                    ; preds = %loop_entry440, %blk_exit430
  %_local_2.48 = phi i32 [ %_local_2.46, %blk_exit430 ], [ %533, %loop_entry440 ]
  %_param_1.40 = phi i32 [ %520, %blk_exit430 ], [ %534, %loop_entry440 ]
  %531 = add i32 %_local_2.48, 819392
  %532 = inttoptr i32 %531 to i8*
  store i8 23, i8* %532, align 1
  %533 = add i32 %_local_2.48, 1
  %534 = add i32 %_param_1.40, -1
  %brif_val442.not = icmp eq i32 %534, 0
  br i1 %brif_val442.not, label %blk_exit437, label %loop_entry440

blk_exit444:                                      ; preds = %blk_exit455, %blk_exit426
  %_local_2.49 = phi i32 [ %_local_2.45, %blk_exit426 ], [ %_local_2.51, %blk_exit455 ]
  %_param_0.50 = phi i32 [ %_param_0.48, %blk_exit426 ], [ %551, %blk_exit455 ]
  store i32 %_param_0.50, i32* inttoptr (i32 819108 to i32*), align 4
  %535 = load i32, i32* inttoptr (i32 15732 to i32*), align 4
  %536 = add i32 %535, -7
  %537 = icmp eq i32 %536, 31
  br i1 %537, label %blk_exit462, label %brif_next464

brif_next446:                                     ; preds = %blk_exit426
  %538 = shl i32 1, %515
  %539 = icmp sgt i32 %538, 1
  %540 = select i1 %539, i32 %538, i32 1
  %541 = and i32 %540, 3
  %542 = add nsw i32 %540, -1
  %543 = icmp ult i32 %542, 3
  br i1 %543, label %blk_exit448, label %brif_next450

blk_exit448:                                      ; preds = %brif_next454, %brif_next446
  %_local_2.50 = phi i32 [ %_local_2.45, %brif_next446 ], [ %550, %brif_next454 ]
  %544 = icmp eq i32 %541, 0
  br i1 %544, label %blk_exit455, label %loop_entry458

brif_next450:                                     ; preds = %brif_next446
  %545 = and i32 %540, 2147483644
  br label %loop_entry451

loop_entry451:                                    ; preds = %loop_entry451, %brif_next450
  %_param_0.51 = phi i32 [ 0, %brif_next450 ], [ %549, %loop_entry451 ]
  %546 = add i32 %_local_2.45, %_param_0.51
  %547 = add i32 %546, 819392
  %548 = inttoptr i32 %547 to i32*
  store i32 404232216, i32* %548, align 4
  %549 = add i32 %_param_0.51, 4
  %.not583 = icmp eq i32 %545, %549
  br i1 %.not583, label %brif_next454, label %loop_entry451

brif_next454:                                     ; preds = %loop_entry451
  %550 = add i32 %_local_2.45, %545
  br label %blk_exit448

blk_exit455:                                      ; preds = %loop_entry458, %blk_exit448
  %_local_2.51 = phi i32 [ %_local_2.50, %blk_exit448 ], [ %554, %loop_entry458 ]
  %551 = shl i32 %_local_2.51, 7
  br label %blk_exit444

loop_entry458:                                    ; preds = %loop_entry458, %blk_exit448
  %_local_2.52 = phi i32 [ %_local_2.50, %blk_exit448 ], [ %554, %loop_entry458 ]
  %_param_1.41 = phi i32 [ %541, %blk_exit448 ], [ %555, %loop_entry458 ]
  %552 = add i32 %_local_2.52, 819392
  %553 = inttoptr i32 %552 to i8*
  store i8 24, i8* %553, align 1
  %554 = add i32 %_local_2.52, 1
  %555 = add i32 %_param_1.41, -1
  %brif_val460.not = icmp eq i32 %555, 0
  br i1 %brif_val460.not, label %blk_exit455, label %loop_entry458

blk_exit462:                                      ; preds = %blk_exit473, %blk_exit444
  %_local_2.53 = phi i32 [ %_local_2.49, %blk_exit444 ], [ %_local_2.55, %blk_exit473 ]
  %_param_0.52 = phi i32 [ %_param_0.50, %blk_exit444 ], [ %572, %blk_exit473 ]
  store i32 %_param_0.52, i32* inttoptr (i32 819112 to i32*), align 8
  %556 = load i32, i32* inttoptr (i32 15736 to i32*), align 8
  %557 = add i32 %556, -7
  %558 = icmp eq i32 %557, 31
  br i1 %558, label %blk_exit480, label %brif_next482

brif_next464:                                     ; preds = %blk_exit444
  %559 = shl i32 1, %536
  %560 = icmp sgt i32 %559, 1
  %561 = select i1 %560, i32 %559, i32 1
  %562 = and i32 %561, 3
  %563 = add nsw i32 %561, -1
  %564 = icmp ult i32 %563, 3
  br i1 %564, label %blk_exit466, label %brif_next468

blk_exit466:                                      ; preds = %brif_next472, %brif_next464
  %_local_2.54 = phi i32 [ %_local_2.49, %brif_next464 ], [ %571, %brif_next472 ]
  %565 = icmp eq i32 %562, 0
  br i1 %565, label %blk_exit473, label %loop_entry476

brif_next468:                                     ; preds = %brif_next464
  %566 = and i32 %561, 2147483644
  br label %loop_entry469

loop_entry469:                                    ; preds = %loop_entry469, %brif_next468
  %_param_0.53 = phi i32 [ 0, %brif_next468 ], [ %570, %loop_entry469 ]
  %567 = add i32 %_local_2.49, %_param_0.53
  %568 = add i32 %567, 819392
  %569 = inttoptr i32 %568 to i32*
  store i32 421075225, i32* %569, align 4
  %570 = add i32 %_param_0.53, 4
  %.not584 = icmp eq i32 %566, %570
  br i1 %.not584, label %brif_next472, label %loop_entry469

brif_next472:                                     ; preds = %loop_entry469
  %571 = add i32 %_local_2.49, %566
  br label %blk_exit466

blk_exit473:                                      ; preds = %loop_entry476, %blk_exit466
  %_local_2.55 = phi i32 [ %_local_2.54, %blk_exit466 ], [ %575, %loop_entry476 ]
  %572 = shl i32 %_local_2.55, 7
  br label %blk_exit462

loop_entry476:                                    ; preds = %loop_entry476, %blk_exit466
  %_local_2.56 = phi i32 [ %_local_2.54, %blk_exit466 ], [ %575, %loop_entry476 ]
  %_param_1.42 = phi i32 [ %562, %blk_exit466 ], [ %576, %loop_entry476 ]
  %573 = add i32 %_local_2.56, 819392
  %574 = inttoptr i32 %573 to i8*
  store i8 25, i8* %574, align 1
  %575 = add i32 %_local_2.56, 1
  %576 = add i32 %_param_1.42, -1
  %brif_val478.not = icmp eq i32 %576, 0
  br i1 %brif_val478.not, label %blk_exit473, label %loop_entry476

blk_exit480:                                      ; preds = %blk_exit491, %blk_exit462
  %_local_2.57 = phi i32 [ %_local_2.53, %blk_exit462 ], [ %_local_2.59, %blk_exit491 ]
  %_param_0.54 = phi i32 [ %_param_0.52, %blk_exit462 ], [ %593, %blk_exit491 ]
  store i32 %_param_0.54, i32* inttoptr (i32 819116 to i32*), align 4
  %577 = load i32, i32* inttoptr (i32 15740 to i32*), align 4
  %578 = add i32 %577, -7
  %579 = icmp eq i32 %578, 31
  br i1 %579, label %blk_exit498, label %brif_next500

brif_next482:                                     ; preds = %blk_exit462
  %580 = shl i32 1, %557
  %581 = icmp sgt i32 %580, 1
  %582 = select i1 %581, i32 %580, i32 1
  %583 = and i32 %582, 3
  %584 = add nsw i32 %582, -1
  %585 = icmp ult i32 %584, 3
  br i1 %585, label %blk_exit484, label %brif_next486

blk_exit484:                                      ; preds = %brif_next490, %brif_next482
  %_local_2.58 = phi i32 [ %_local_2.53, %brif_next482 ], [ %592, %brif_next490 ]
  %586 = icmp eq i32 %583, 0
  br i1 %586, label %blk_exit491, label %loop_entry494

brif_next486:                                     ; preds = %brif_next482
  %587 = and i32 %582, 2147483644
  br label %loop_entry487

loop_entry487:                                    ; preds = %loop_entry487, %brif_next486
  %_param_0.55 = phi i32 [ 0, %brif_next486 ], [ %591, %loop_entry487 ]
  %588 = add i32 %_local_2.53, %_param_0.55
  %589 = add i32 %588, 819392
  %590 = inttoptr i32 %589 to i32*
  store i32 437918234, i32* %590, align 4
  %591 = add i32 %_param_0.55, 4
  %.not585 = icmp eq i32 %587, %591
  br i1 %.not585, label %brif_next490, label %loop_entry487

brif_next490:                                     ; preds = %loop_entry487
  %592 = add i32 %_local_2.53, %587
  br label %blk_exit484

blk_exit491:                                      ; preds = %loop_entry494, %blk_exit484
  %_local_2.59 = phi i32 [ %_local_2.58, %blk_exit484 ], [ %596, %loop_entry494 ]
  %593 = shl i32 %_local_2.59, 7
  br label %blk_exit480

loop_entry494:                                    ; preds = %loop_entry494, %blk_exit484
  %_local_2.60 = phi i32 [ %_local_2.58, %blk_exit484 ], [ %596, %loop_entry494 ]
  %_param_1.43 = phi i32 [ %583, %blk_exit484 ], [ %597, %loop_entry494 ]
  %594 = add i32 %_local_2.60, 819392
  %595 = inttoptr i32 %594 to i8*
  store i8 26, i8* %595, align 1
  %596 = add i32 %_local_2.60, 1
  %597 = add i32 %_param_1.43, -1
  %brif_val496.not = icmp eq i32 %597, 0
  br i1 %brif_val496.not, label %blk_exit491, label %loop_entry494

blk_exit498:                                      ; preds = %blk_exit509, %blk_exit480
  %_local_2.61 = phi i32 [ %_local_2.57, %blk_exit480 ], [ %_local_2.63, %blk_exit509 ]
  %_param_0.56 = phi i32 [ %_param_0.54, %blk_exit480 ], [ %614, %blk_exit509 ]
  store i32 %_param_0.56, i32* inttoptr (i32 819120 to i32*), align 16
  %598 = load i32, i32* inttoptr (i32 15744 to i32*), align 128
  %599 = add i32 %598, -7
  %600 = icmp eq i32 %599, 31
  br i1 %600, label %blk_exit516, label %brif_next518

brif_next500:                                     ; preds = %blk_exit480
  %601 = shl i32 1, %578
  %602 = icmp sgt i32 %601, 1
  %603 = select i1 %602, i32 %601, i32 1
  %604 = and i32 %603, 3
  %605 = add nsw i32 %603, -1
  %606 = icmp ult i32 %605, 3
  br i1 %606, label %blk_exit502, label %brif_next504

blk_exit502:                                      ; preds = %brif_next508, %brif_next500
  %_local_2.62 = phi i32 [ %_local_2.57, %brif_next500 ], [ %613, %brif_next508 ]
  %607 = icmp eq i32 %604, 0
  br i1 %607, label %blk_exit509, label %loop_entry512

brif_next504:                                     ; preds = %brif_next500
  %608 = and i32 %603, 2147483644
  br label %loop_entry505

loop_entry505:                                    ; preds = %loop_entry505, %brif_next504
  %_param_0.57 = phi i32 [ 0, %brif_next504 ], [ %612, %loop_entry505 ]
  %609 = add i32 %_local_2.57, %_param_0.57
  %610 = add i32 %609, 819392
  %611 = inttoptr i32 %610 to i32*
  store i32 454761243, i32* %611, align 4
  %612 = add i32 %_param_0.57, 4
  %.not586 = icmp eq i32 %608, %612
  br i1 %.not586, label %brif_next508, label %loop_entry505

brif_next508:                                     ; preds = %loop_entry505
  %613 = add i32 %_local_2.57, %608
  br label %blk_exit502

blk_exit509:                                      ; preds = %loop_entry512, %blk_exit502
  %_local_2.63 = phi i32 [ %_local_2.62, %blk_exit502 ], [ %617, %loop_entry512 ]
  %614 = shl i32 %_local_2.63, 7
  br label %blk_exit498

loop_entry512:                                    ; preds = %loop_entry512, %blk_exit502
  %_local_2.64 = phi i32 [ %_local_2.62, %blk_exit502 ], [ %617, %loop_entry512 ]
  %_param_1.44 = phi i32 [ %604, %blk_exit502 ], [ %618, %loop_entry512 ]
  %615 = add i32 %_local_2.64, 819392
  %616 = inttoptr i32 %615 to i8*
  store i8 27, i8* %616, align 1
  %617 = add i32 %_local_2.64, 1
  %618 = add i32 %_param_1.44, -1
  %brif_val514.not = icmp eq i32 %618, 0
  br i1 %brif_val514.not, label %blk_exit509, label %loop_entry512

blk_exit516:                                      ; preds = %blk_exit527, %blk_exit498
  %_local_2.65 = phi i32 [ %_local_2.61, %blk_exit498 ], [ %_local_2.67, %blk_exit527 ]
  %_param_0.58 = phi i32 [ %_param_0.56, %blk_exit498 ], [ %635, %blk_exit527 ]
  store i32 %_param_0.58, i32* inttoptr (i32 819124 to i32*), align 4
  %619 = load i32, i32* inttoptr (i32 15748 to i32*), align 4
  %620 = add i32 %619, -7
  %621 = icmp eq i32 %620, 31
  br i1 %621, label %blk_exit534, label %brif_next536

brif_next518:                                     ; preds = %blk_exit498
  %622 = shl i32 1, %599
  %623 = icmp sgt i32 %622, 1
  %624 = select i1 %623, i32 %622, i32 1
  %625 = and i32 %624, 3
  %626 = add nsw i32 %624, -1
  %627 = icmp ult i32 %626, 3
  br i1 %627, label %blk_exit520, label %brif_next522

blk_exit520:                                      ; preds = %brif_next526, %brif_next518
  %_local_2.66 = phi i32 [ %_local_2.61, %brif_next518 ], [ %634, %brif_next526 ]
  %628 = icmp eq i32 %625, 0
  br i1 %628, label %blk_exit527, label %loop_entry530

brif_next522:                                     ; preds = %brif_next518
  %629 = and i32 %624, 2147483644
  br label %loop_entry523

loop_entry523:                                    ; preds = %loop_entry523, %brif_next522
  %_param_0.59 = phi i32 [ 0, %brif_next522 ], [ %633, %loop_entry523 ]
  %630 = add i32 %_local_2.61, %_param_0.59
  %631 = add i32 %630, 819392
  %632 = inttoptr i32 %631 to i32*
  store i32 471604252, i32* %632, align 4
  %633 = add i32 %_param_0.59, 4
  %.not587 = icmp eq i32 %629, %633
  br i1 %.not587, label %brif_next526, label %loop_entry523

brif_next526:                                     ; preds = %loop_entry523
  %634 = add i32 %_local_2.61, %629
  br label %blk_exit520

blk_exit527:                                      ; preds = %loop_entry530, %blk_exit520
  %_local_2.67 = phi i32 [ %_local_2.66, %blk_exit520 ], [ %638, %loop_entry530 ]
  %635 = shl i32 %_local_2.67, 7
  br label %blk_exit516

loop_entry530:                                    ; preds = %loop_entry530, %blk_exit520
  %_local_2.68 = phi i32 [ %_local_2.66, %blk_exit520 ], [ %638, %loop_entry530 ]
  %_param_1.45 = phi i32 [ %625, %blk_exit520 ], [ %639, %loop_entry530 ]
  %636 = add i32 %_local_2.68, 819392
  %637 = inttoptr i32 %636 to i8*
  store i8 28, i8* %637, align 1
  %638 = add i32 %_local_2.68, 1
  %639 = add i32 %_param_1.45, -1
  %brif_val532.not = icmp eq i32 %639, 0
  br i1 %brif_val532.not, label %blk_exit527, label %loop_entry530

blk_exit534:                                      ; preds = %loop_entry547, %blk_exit538, %blk_exit516
  store i64 0, i64* inttoptr (i32 819668 to i64*), align 8
  store i64 0, i64* inttoptr (i32 819656 to i64*), align 8
  store i64 0, i64* inttoptr (i32 819648 to i64*), align 64
  store i32 0, i32* inttoptr (i32 819676 to i32*), align 4
  store i16 0, i16* inttoptr (i32 819664 to i16*), align 16
  br label %loop_entry551

brif_next536:                                     ; preds = %blk_exit516
  %640 = shl i32 1, %620
  %641 = icmp sgt i32 %640, 1
  %642 = select i1 %641, i32 %640, i32 1
  %643 = and i32 %642, 3
  %644 = add nsw i32 %642, -1
  %645 = icmp ult i32 %644, 3
  br i1 %645, label %blk_exit538, label %brif_next540

blk_exit538:                                      ; preds = %brif_next544, %brif_next536
  %_local_2.69 = phi i32 [ %_local_2.65, %brif_next536 ], [ %652, %brif_next544 ]
  %646 = icmp eq i32 %643, 0
  br i1 %646, label %blk_exit534, label %brif_next546

brif_next540:                                     ; preds = %brif_next536
  %647 = and i32 %642, 2147483644
  br label %loop_entry541

loop_entry541:                                    ; preds = %loop_entry541, %brif_next540
  %_param_0.60 = phi i32 [ 0, %brif_next540 ], [ %651, %loop_entry541 ]
  %648 = add i32 %_local_2.65, %_param_0.60
  %649 = add i32 %648, 819392
  %650 = inttoptr i32 %649 to i32*
  store i32 488447261, i32* %650, align 4
  %651 = add i32 %_param_0.60, 4
  %.not588 = icmp eq i32 %647, %651
  br i1 %.not588, label %brif_next544, label %loop_entry541

brif_next544:                                     ; preds = %loop_entry541
  %652 = add i32 %_local_2.65, %647
  br label %blk_exit538

brif_next546:                                     ; preds = %blk_exit538
  %653 = add i32 %_local_2.69, 819392
  br label %loop_entry547

loop_entry547:                                    ; preds = %loop_entry547, %brif_next546
  %_local_2.70 = phi i32 [ %653, %brif_next546 ], [ %655, %loop_entry547 ]
  %_param_1.46 = phi i32 [ %643, %brif_next546 ], [ %656, %loop_entry547 ]
  %654 = inttoptr i32 %_local_2.70 to i8*
  store i8 29, i8* %654, align 1
  %655 = add i32 %_local_2.70, 1
  %656 = add i32 %_param_1.46, -1
  %brif_val549.not = icmp eq i32 %656, 0
  br i1 %brif_val549.not, label %blk_exit534, label %loop_entry547

loop_entry551:                                    ; preds = %loop_entry551, %blk_exit534
  %_local_2.71 = phi i32 [ 0, %blk_exit534 ], [ %675, %loop_entry551 ]
  %657 = add i32 %_local_2.71, 819714
  %658 = inttoptr i32 %657 to i16*
  store i16 8, i16* %658, align 2
  %659 = add i32 %_local_2.71, 819710
  %660 = inttoptr i32 %659 to i16*
  store i16 8, i16* %660, align 2
  %661 = add i32 %_local_2.71, 819706
  %662 = inttoptr i32 %661 to i16*
  store i16 8, i16* %662, align 2
  %663 = add i32 %_local_2.71, 819702
  %664 = inttoptr i32 %663 to i16*
  store i16 8, i16* %664, align 2
  %665 = add i32 %_local_2.71, 819698
  %666 = inttoptr i32 %665 to i16*
  store i16 8, i16* %666, align 2
  %667 = add i32 %_local_2.71, 819694
  %668 = inttoptr i32 %667 to i16*
  store i16 8, i16* %668, align 2
  %669 = add i32 %_local_2.71, 819690
  %670 = inttoptr i32 %669 to i16*
  store i16 8, i16* %670, align 2
  %671 = add i32 %_local_2.71, 819686
  %672 = inttoptr i32 %671 to i16*
  store i16 8, i16* %672, align 2
  %673 = add i32 %_local_2.71, 819682
  %674 = inttoptr i32 %673 to i16*
  store i16 8, i16* %674, align 2
  %675 = add i32 %_local_2.71, 36
  %.not589 = icmp eq i32 %675, 576
  br i1 %.not589, label %loop_entry563, label %loop_entry551

loop_entry563:                                    ; preds = %loop_entry563, %loop_entry551
  %_local_2.72 = phi i32 [ %692, %loop_entry563 ], [ 0, %loop_entry551 ]
  %676 = add i32 %_local_2.72, 820286
  %677 = inttoptr i32 %676 to i16*
  store i16 9, i16* %677, align 2
  %678 = add i32 %_local_2.72, 820282
  %679 = inttoptr i32 %678 to i16*
  store i16 9, i16* %679, align 2
  %680 = add i32 %_local_2.72, 820278
  %681 = inttoptr i32 %680 to i16*
  store i16 9, i16* %681, align 2
  %682 = add i32 %_local_2.72, 820274
  %683 = inttoptr i32 %682 to i16*
  store i16 9, i16* %683, align 2
  %684 = add i32 %_local_2.72, 820270
  %685 = inttoptr i32 %684 to i16*
  store i16 9, i16* %685, align 2
  %686 = add i32 %_local_2.72, 820266
  %687 = inttoptr i32 %686 to i16*
  store i16 9, i16* %687, align 2
  %688 = add i32 %_local_2.72, 820262
  %689 = inttoptr i32 %688 to i16*
  store i16 9, i16* %689, align 2
  %690 = add i32 %_local_2.72, 820258
  %691 = inttoptr i32 %690 to i16*
  store i16 9, i16* %691, align 2
  %692 = add i32 %_local_2.72, 32
  %.not590 = icmp eq i32 %692, 448
  br i1 %.not590, label %brif_next573, label %loop_entry563

brif_next573:                                     ; preds = %loop_entry563
  store i16 7, i16* inttoptr (i32 820798 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820794 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820790 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820786 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820782 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820778 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820774 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820770 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820766 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820762 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820758 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820754 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820750 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820746 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820742 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820738 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820734 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820730 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820726 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820722 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820718 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820714 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820710 to i16*), align 2
  store i16 7, i16* inttoptr (i32 820706 to i16*), align 2
  store i16 112, i16* inttoptr (i32 819666 to i16*), align 2
  store i16 8, i16* inttoptr (i32 820830 to i16*), align 2
  store i32 9961496, i32* inttoptr (i32 819662 to i32*), align 4
  store i16 8, i16* inttoptr (i32 820826 to i16*), align 2
  store i16 8, i16* inttoptr (i32 820822 to i16*), align 2
  store i16 8, i16* inttoptr (i32 820818 to i16*), align 2
  store i16 8, i16* inttoptr (i32 820814 to i16*), align 2
  store i16 8, i16* inttoptr (i32 820810 to i16*), align 2
  store i16 8, i16* inttoptr (i32 820806 to i16*), align 2
  store i16 8, i16* inttoptr (i32 820802 to i16*), align 2
  call void @gen_codes(i32 819680, i32 287)
  store i16 5, i16* inttoptr (i32 818498 to i16*), align 2
  store i16 5, i16* inttoptr (i32 818502 to i16*), align 2
  %693 = call i32 @bi_reverse(i32 0, i32 5)
  %694 = trunc i32 %693 to i16
  store i16 %694, i16* inttoptr (i32 818496 to i16*), align 64
  store i16 5, i16* inttoptr (i32 818506 to i16*), align 2
  %695 = call i32 @bi_reverse(i32 1, i32 5)
  %696 = trunc i32 %695 to i16
  store i16 %696, i16* inttoptr (i32 818500 to i16*), align 4
  store i16 5, i16* inttoptr (i32 818510 to i16*), align 2
  %697 = call i32 @bi_reverse(i32 2, i32 5)
  %698 = trunc i32 %697 to i16
  store i16 %698, i16* inttoptr (i32 818504 to i16*), align 8
  store i16 5, i16* inttoptr (i32 818514 to i16*), align 2
  %699 = call i32 @bi_reverse(i32 3, i32 5)
  %700 = trunc i32 %699 to i16
  store i16 %700, i16* inttoptr (i32 818508 to i16*), align 4
  store i16 5, i16* inttoptr (i32 818518 to i16*), align 2
  %701 = call i32 @bi_reverse(i32 4, i32 5)
  %702 = trunc i32 %701 to i16
  store i16 %702, i16* inttoptr (i32 818512 to i16*), align 16
  store i16 5, i16* inttoptr (i32 818522 to i16*), align 2
  %703 = call i32 @bi_reverse(i32 5, i32 5)
  %704 = trunc i32 %703 to i16
  store i16 %704, i16* inttoptr (i32 818516 to i16*), align 4
  store i16 5, i16* inttoptr (i32 818526 to i16*), align 2
  %705 = call i32 @bi_reverse(i32 6, i32 5)
  %706 = trunc i32 %705 to i16
  store i16 %706, i16* inttoptr (i32 818520 to i16*), align 8
  store i16 5, i16* inttoptr (i32 818530 to i16*), align 2
  %707 = call i32 @bi_reverse(i32 7, i32 5)
  %708 = trunc i32 %707 to i16
  store i16 %708, i16* inttoptr (i32 818524 to i16*), align 4
  store i16 5, i16* inttoptr (i32 818534 to i16*), align 2
  %709 = call i32 @bi_reverse(i32 8, i32 5)
  %710 = trunc i32 %709 to i16
  store i16 %710, i16* inttoptr (i32 818528 to i16*), align 32
  store i16 5, i16* inttoptr (i32 818538 to i16*), align 2
  %711 = call i32 @bi_reverse(i32 9, i32 5)
  %712 = trunc i32 %711 to i16
  store i16 %712, i16* inttoptr (i32 818532 to i16*), align 4
  store i16 5, i16* inttoptr (i32 818542 to i16*), align 2
  %713 = call i32 @bi_reverse(i32 10, i32 5)
  %714 = trunc i32 %713 to i16
  store i16 %714, i16* inttoptr (i32 818536 to i16*), align 8
  store i16 5, i16* inttoptr (i32 818546 to i16*), align 2
  %715 = call i32 @bi_reverse(i32 11, i32 5)
  %716 = trunc i32 %715 to i16
  store i16 %716, i16* inttoptr (i32 818540 to i16*), align 4
  store i16 5, i16* inttoptr (i32 818550 to i16*), align 2
  %717 = call i32 @bi_reverse(i32 12, i32 5)
  %718 = trunc i32 %717 to i16
  store i16 %718, i16* inttoptr (i32 818544 to i16*), align 16
  store i16 5, i16* inttoptr (i32 818554 to i16*), align 2
  %719 = call i32 @bi_reverse(i32 13, i32 5)
  %720 = trunc i32 %719 to i16
  store i16 %720, i16* inttoptr (i32 818548 to i16*), align 4
  store i16 5, i16* inttoptr (i32 818558 to i16*), align 2
  %721 = call i32 @bi_reverse(i32 14, i32 5)
  %722 = trunc i32 %721 to i16
  store i16 %722, i16* inttoptr (i32 818552 to i16*), align 8
  store i16 5, i16* inttoptr (i32 818562 to i16*), align 2
  %723 = call i32 @bi_reverse(i32 15, i32 5)
  %724 = trunc i32 %723 to i16
  store i16 %724, i16* inttoptr (i32 818556 to i16*), align 4
  store i16 5, i16* inttoptr (i32 818566 to i16*), align 2
  %725 = call i32 @bi_reverse(i32 16, i32 5)
  %726 = trunc i32 %725 to i16
  store i16 %726, i16* inttoptr (i32 818560 to i16*), align 128
  store i16 5, i16* inttoptr (i32 818570 to i16*), align 2
  %727 = call i32 @bi_reverse(i32 17, i32 5)
  %728 = trunc i32 %727 to i16
  store i16 %728, i16* inttoptr (i32 818564 to i16*), align 4
  store i16 5, i16* inttoptr (i32 818574 to i16*), align 2
  %729 = call i32 @bi_reverse(i32 18, i32 5)
  %730 = trunc i32 %729 to i16
  store i16 %730, i16* inttoptr (i32 818568 to i16*), align 8
  store i16 5, i16* inttoptr (i32 818578 to i16*), align 2
  %731 = call i32 @bi_reverse(i32 19, i32 5)
  %732 = trunc i32 %731 to i16
  store i16 %732, i16* inttoptr (i32 818572 to i16*), align 4
  store i16 5, i16* inttoptr (i32 818582 to i16*), align 2
  %733 = call i32 @bi_reverse(i32 20, i32 5)
  %734 = trunc i32 %733 to i16
  store i16 %734, i16* inttoptr (i32 818576 to i16*), align 16
  store i16 5, i16* inttoptr (i32 818586 to i16*), align 2
  %735 = call i32 @bi_reverse(i32 21, i32 5)
  %736 = trunc i32 %735 to i16
  store i16 %736, i16* inttoptr (i32 818580 to i16*), align 4
  store i16 5, i16* inttoptr (i32 818590 to i16*), align 2
  %737 = call i32 @bi_reverse(i32 22, i32 5)
  %738 = trunc i32 %737 to i16
  store i16 %738, i16* inttoptr (i32 818584 to i16*), align 8
  store i16 5, i16* inttoptr (i32 818594 to i16*), align 2
  %739 = call i32 @bi_reverse(i32 23, i32 5)
  %740 = trunc i32 %739 to i16
  store i16 %740, i16* inttoptr (i32 818588 to i16*), align 4
  store i16 5, i16* inttoptr (i32 818598 to i16*), align 2
  %741 = call i32 @bi_reverse(i32 24, i32 5)
  %742 = trunc i32 %741 to i16
  store i16 %742, i16* inttoptr (i32 818592 to i16*), align 32
  store i16 5, i16* inttoptr (i32 818602 to i16*), align 2
  %743 = call i32 @bi_reverse(i32 25, i32 5)
  %744 = trunc i32 %743 to i16
  store i16 %744, i16* inttoptr (i32 818596 to i16*), align 4
  store i16 5, i16* inttoptr (i32 818606 to i16*), align 2
  %745 = call i32 @bi_reverse(i32 26, i32 5)
  %746 = trunc i32 %745 to i16
  store i16 %746, i16* inttoptr (i32 818600 to i16*), align 8
  store i16 5, i16* inttoptr (i32 818610 to i16*), align 2
  %747 = call i32 @bi_reverse(i32 27, i32 5)
  %748 = trunc i32 %747 to i16
  store i16 %748, i16* inttoptr (i32 818604 to i16*), align 4
  store i16 5, i16* inttoptr (i32 818614 to i16*), align 2
  %749 = call i32 @bi_reverse(i32 28, i32 5)
  %750 = trunc i32 %749 to i16
  store i16 %750, i16* inttoptr (i32 818608 to i16*), align 16
  %751 = call i32 @bi_reverse(i32 29, i32 5)
  %752 = trunc i32 %751 to i16
  store i16 %752, i16* inttoptr (i32 818612 to i16*), align 4
  call void @init_block()
  br label %blk_exit
}

; Function Attrs: null_pointer_is_valid
define internal void @gen_codes(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -32
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = load i16, i16* inttoptr (i32 819648 to i16*), align 64
  %3 = zext i16 %2 to i32
  %4 = shl nuw nsw i32 %3, 1
  %calcOffset = add i32 %0, -30
  %5 = trunc i32 %4 to i16
  %6 = inttoptr i32 %calcOffset to i16*
  store i16 %5, i16* %6, align 2
  %7 = and i32 %4, 65534
  %8 = load i16, i16* inttoptr (i32 819650 to i16*), align 2
  %9 = zext i16 %8 to i32
  %10 = add nuw nsw i32 %7, %9
  %11 = shl nuw nsw i32 %10, 1
  %calcOffset1 = add i32 %0, -28
  %12 = trunc i32 %11 to i16
  %13 = inttoptr i32 %calcOffset1 to i16*
  store i16 %12, i16* %13, align 2
  %14 = and i32 %11, 65534
  %15 = load i16, i16* inttoptr (i32 819652 to i16*), align 4
  %16 = zext i16 %15 to i32
  %17 = add nuw nsw i32 %14, %16
  %18 = shl nuw nsw i32 %17, 1
  %calcOffset2 = add i32 %0, -26
  %19 = trunc i32 %18 to i16
  %20 = inttoptr i32 %calcOffset2 to i16*
  store i16 %19, i16* %20, align 2
  %21 = and i32 %18, 65534
  %22 = load i16, i16* inttoptr (i32 819654 to i16*), align 2
  %23 = zext i16 %22 to i32
  %24 = add nuw nsw i32 %21, %23
  %25 = shl nuw nsw i32 %24, 1
  %calcOffset3 = add i32 %0, -24
  %26 = trunc i32 %25 to i16
  %27 = inttoptr i32 %calcOffset3 to i16*
  store i16 %26, i16* %27, align 2
  %28 = and i32 %25, 65534
  %29 = load i16, i16* inttoptr (i32 819656 to i16*), align 8
  %30 = zext i16 %29 to i32
  %31 = add nuw nsw i32 %28, %30
  %32 = shl nuw nsw i32 %31, 1
  %calcOffset4 = add i32 %0, -22
  %33 = trunc i32 %32 to i16
  %34 = inttoptr i32 %calcOffset4 to i16*
  store i16 %33, i16* %34, align 2
  %35 = and i32 %32, 65534
  %36 = load i16, i16* inttoptr (i32 819658 to i16*), align 2
  %37 = zext i16 %36 to i32
  %38 = add nuw nsw i32 %35, %37
  %39 = shl nuw nsw i32 %38, 1
  %calcOffset5 = add i32 %0, -20
  %40 = trunc i32 %39 to i16
  %41 = inttoptr i32 %calcOffset5 to i16*
  store i16 %40, i16* %41, align 2
  %42 = and i32 %39, 65534
  %43 = load i16, i16* inttoptr (i32 819660 to i16*), align 4
  %44 = zext i16 %43 to i32
  %45 = add nuw nsw i32 %42, %44
  %46 = shl nuw nsw i32 %45, 1
  %calcOffset6 = add i32 %0, -18
  %47 = trunc i32 %46 to i16
  %48 = inttoptr i32 %calcOffset6 to i16*
  store i16 %47, i16* %48, align 2
  %49 = and i32 %46, 65534
  %50 = load i16, i16* inttoptr (i32 819662 to i16*), align 2
  %51 = zext i16 %50 to i32
  %52 = add nuw nsw i32 %49, %51
  %53 = shl nuw nsw i32 %52, 1
  %calcOffset7 = add i32 %0, -16
  %54 = trunc i32 %53 to i16
  %55 = inttoptr i32 %calcOffset7 to i16*
  store i16 %54, i16* %55, align 2
  %56 = and i32 %53, 65534
  %57 = load i16, i16* inttoptr (i32 819664 to i16*), align 16
  %58 = zext i16 %57 to i32
  %59 = add nuw nsw i32 %56, %58
  %60 = shl nuw nsw i32 %59, 1
  %calcOffset8 = add i32 %0, -14
  %61 = trunc i32 %60 to i16
  %62 = inttoptr i32 %calcOffset8 to i16*
  store i16 %61, i16* %62, align 2
  %63 = and i32 %60, 65534
  %64 = load i16, i16* inttoptr (i32 819666 to i16*), align 2
  %65 = zext i16 %64 to i32
  %66 = add nuw nsw i32 %63, %65
  %67 = shl nuw nsw i32 %66, 1
  %calcOffset9 = add i32 %0, -12
  %68 = trunc i32 %67 to i16
  %69 = inttoptr i32 %calcOffset9 to i16*
  store i16 %68, i16* %69, align 2
  %70 = and i32 %67, 65534
  %71 = load i16, i16* inttoptr (i32 819668 to i16*), align 4
  %72 = zext i16 %71 to i32
  %73 = add nuw nsw i32 %70, %72
  %74 = shl nuw nsw i32 %73, 1
  %calcOffset10 = add i32 %0, -10
  %75 = trunc i32 %74 to i16
  %76 = inttoptr i32 %calcOffset10 to i16*
  store i16 %75, i16* %76, align 2
  %77 = and i32 %74, 65534
  %78 = load i16, i16* inttoptr (i32 819670 to i16*), align 2
  %79 = zext i16 %78 to i32
  %80 = add nuw nsw i32 %77, %79
  %81 = shl nuw nsw i32 %80, 1
  %calcOffset11 = add i32 %0, -8
  %82 = trunc i32 %81 to i16
  %83 = inttoptr i32 %calcOffset11 to i16*
  store i16 %82, i16* %83, align 2
  %84 = and i32 %81, 65534
  %85 = load i16, i16* inttoptr (i32 819672 to i16*), align 8
  %86 = zext i16 %85 to i32
  %87 = add nuw nsw i32 %84, %86
  %88 = shl nuw nsw i32 %87, 1
  %calcOffset12 = add i32 %0, -6
  %89 = trunc i32 %88 to i16
  %90 = inttoptr i32 %calcOffset12 to i16*
  store i16 %89, i16* %90, align 2
  %91 = load i16, i16* inttoptr (i32 819674 to i16*), align 2
  %calcOffset13 = add i32 %0, -4
  %92 = trunc i32 %88 to i16
  %.tr24 = add i16 %91, %92
  %93 = shl i16 %.tr24, 1
  %94 = inttoptr i32 %calcOffset13 to i16*
  store i16 %93, i16* %94, align 2
  %95 = load i16, i16* inttoptr (i32 819676 to i16*), align 4
  %calcOffset14 = add i32 %0, -2
  %.tr = add i16 %95, %93
  %96 = shl i16 %.tr, 1
  %97 = inttoptr i32 %calcOffset14 to i16*
  store i16 %96, i16* %97, align 2
  %brif_val.not = icmp sgt i32 %_arg_1, -1
  br i1 %brif_val.not, label %brif_next, label %blk_exit

blk_exit:                                         ; preds = %blk_exit15, %allocator
  store i32 %0, i32* @__stack_pointer, align 4
  ret void

brif_next:                                        ; preds = %allocator
  %98 = add i32 %_arg_1, 1
  br label %loop_entry

loop_entry:                                       ; preds = %blk_exit15, %brif_next
  %_local_3.0 = phi i32 [ %98, %brif_next ], [ %105, %blk_exit15 ]
  %_param_0.0 = phi i32 [ %_arg_0, %brif_next ], [ %104, %blk_exit15 ]
  %99 = add i32 %_param_0.0, 2
  %100 = inttoptr i32 %99 to i16*
  %101 = load i16, i16* %100, align 2
  %102 = zext i16 %101 to i32
  %103 = icmp eq i16 %101, 0
  br i1 %103, label %blk_exit15, label %brif_next18

blk_exit15:                                       ; preds = %brif_next18, %loop_entry
  %104 = add i32 %_param_0.0, 4
  %105 = add i32 %_local_3.0, -1
  %brif_val22.not = icmp eq i32 %105, 0
  br i1 %brif_val22.not, label %blk_exit, label %loop_entry

brif_next18:                                      ; preds = %loop_entry
  %106 = shl nuw nsw i32 %102, 1
  %107 = add i32 %1, %106
  %108 = inttoptr i32 %107 to i16*
  %109 = load i16, i16* %108, align 2
  %110 = zext i16 %109 to i32
  %111 = add i16 %109, 1
  store i16 %111, i16* %108, align 2
  %112 = call i32 @bi_reverse(i32 %110, i32 %102)
  %113 = trunc i32 %112 to i16
  %114 = inttoptr i32 %_param_0.0 to i16*
  store i16 %113, i16* %114, align 2
  br label %blk_exit15
}

; Function Attrs: null_pointer_is_valid
define internal void @init_block() #0 {
allocator:
  br label %loop_entry

loop_entry:                                       ; preds = %loop_entry, %allocator
  %_local_0.0 = phi i32 [ -1144, %allocator ], [ %4, %loop_entry ]
  %0 = add i32 %_local_0.0, 826108
  %1 = inttoptr i32 %0 to i16*
  store i16 0, i16* %1, align 4
  %2 = add i32 %_local_0.0, 826104
  %3 = inttoptr i32 %2 to i16*
  store i16 0, i16* %3, align 8
  %4 = add i32 %_local_0.0, 8
  %brif_val.not = icmp eq i32 %4, 0
  br i1 %brif_val.not, label %brif_next, label %loop_entry

brif_next:                                        ; preds = %loop_entry
  store i16 0, i16* inttoptr (i32 827380 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827376 to i16*), align 16
  store i16 0, i16* inttoptr (i32 827372 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827368 to i16*), align 8
  store i16 0, i16* inttoptr (i32 827364 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827360 to i16*), align 32
  store i16 0, i16* inttoptr (i32 827356 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827352 to i16*), align 8
  store i16 0, i16* inttoptr (i32 827348 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827344 to i16*), align 16
  store i16 0, i16* inttoptr (i32 827340 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827336 to i16*), align 8
  store i16 0, i16* inttoptr (i32 827332 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827328 to i16*), align 64
  store i16 0, i16* inttoptr (i32 827324 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827320 to i16*), align 8
  store i16 0, i16* inttoptr (i32 827316 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827312 to i16*), align 16
  store i16 0, i16* inttoptr (i32 827308 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827304 to i16*), align 8
  store i16 0, i16* inttoptr (i32 827300 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827296 to i16*), align 32
  store i16 0, i16* inttoptr (i32 827292 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827288 to i16*), align 8
  store i16 0, i16* inttoptr (i32 827284 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827280 to i16*), align 16
  store i16 0, i16* inttoptr (i32 827276 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827272 to i16*), align 8
  store i16 0, i16* inttoptr (i32 827268 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827264 to i16*), align 128
  store i16 0, i16* inttoptr (i32 827592 to i16*), align 8
  store i16 0, i16* inttoptr (i32 827588 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827584 to i16*), align 64
  store i16 0, i16* inttoptr (i32 827580 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827576 to i16*), align 8
  store i16 0, i16* inttoptr (i32 827572 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827568 to i16*), align 16
  store i16 0, i16* inttoptr (i32 827564 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827560 to i16*), align 8
  store i16 0, i16* inttoptr (i32 827556 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827552 to i16*), align 32
  store i16 0, i16* inttoptr (i32 827548 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827544 to i16*), align 8
  store i16 0, i16* inttoptr (i32 827540 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827536 to i16*), align 16
  store i16 0, i16* inttoptr (i32 827532 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827528 to i16*), align 8
  store i16 0, i16* inttoptr (i32 827524 to i16*), align 4
  store i16 0, i16* inttoptr (i32 827520 to i16*), align 128
  store i16 1, i16* inttoptr (i32 825984 to i16*), align 128
  store i8 1, i8* inttoptr (i32 827516 to i8*), align 4
  store i32 0, i32* inttoptr (i32 824952 to i32*), align 8
  store i32 0, i32* inttoptr (i32 824948 to i32*), align 4
  store i32 0, i32* inttoptr (i32 824944 to i32*), align 16
  store i32 0, i32* inttoptr (i32 827512 to i32*), align 8
  store i32 0, i32* inttoptr (i32 827508 to i32*), align 4
  store i8 0, i8* inttoptr (i32 820832 to i8*), align 32
  ret void
}

; Function Attrs: null_pointer_is_valid
declare dso_local i64 @flush_block(i32, i32, i32, i32) #0

; Function Attrs: null_pointer_is_valid
define internal void @build_tree(i32 %_arg_0) #0 {
allocator:
  %calcOffset = add i32 %_arg_0, 4
  %0 = inttoptr i32 %calcOffset to i32*
  %1 = load i32, i32* %0, align 4
  %2 = inttoptr i32 %_arg_0 to i32*
  %3 = load i32, i32* %2, align 4
  %calcOffset6 = add i32 %_arg_0, 16
  %4 = inttoptr i32 %calcOffset6 to i32*
  %5 = load i32, i32* %4, align 4
  %6 = icmp slt i32 %5, 1
  br i1 %6, label %blk_exit5, label %brif_next

blk_exit:                                         ; preds = %blk_exit39, %blk_exit27
  %_local_8.0 = phi i32 [ %56, %blk_exit39 ], [ %41, %blk_exit27 ]
  %_local_7.0 = phi i32 [ 2, %blk_exit39 ], [ %_local_7.6, %blk_exit27 ]
  %_local_5.0 = phi i32 [ %_local_5.7, %blk_exit39 ], [ %_local_5.6, %blk_exit27 ]
  %calcOffset94 = add i32 %_arg_0, 24
  %7 = inttoptr i32 %calcOffset94 to i32*
  store i32 %_local_5.0, i32* %7, align 4
  %8 = lshr i32 %_local_7.0, 1
  br label %loop_entry95

blk_exit2:                                        ; preds = %blk_exit27, %blk_exit5
  %_local_8.1 = phi i32 [ %11, %blk_exit5 ], [ %41, %blk_exit27 ]
  %_local_7.1 = phi i32 [ 0, %blk_exit5 ], [ %_local_7.6, %blk_exit27 ]
  %_local_5.1 = phi i32 [ -1, %blk_exit5 ], [ %_local_5.6, %blk_exit27 ]
  %brif_val41.not = icmp eq i32 %1, 0
  br i1 %brif_val41.not, label %brif_next42, label %blk_exit40

blk_exit3:                                        ; preds = %brif_next26, %brif_next
  %_local_7.2 = phi i32 [ %_local_7.5, %brif_next26 ], [ 0, %brif_next ]
  %_local_6.0 = phi i32 [ %40, %brif_next26 ], [ 0, %brif_next ]
  %_local_5.2 = phi i32 [ %_local_5.5, %brif_next26 ], [ -1, %brif_next ]
  %9 = icmp eq i32 %12, 0
  br i1 %9, label %blk_exit27, label %brif_next29

blk_exit4:                                        ; preds = %brif_next
  %10 = and i32 %5, -2
  br label %loop_entry

blk_exit5:                                        ; preds = %allocator
  %11 = load i32, i32* inttoptr (i32 824948 to i32*), align 4
  br label %blk_exit2

brif_next:                                        ; preds = %allocator
  %12 = and i32 %5, 1
  %.not = icmp eq i32 %5, 1
  br i1 %.not, label %blk_exit3, label %blk_exit4

loop_entry:                                       ; preds = %blk_exit17, %blk_exit4
  %_local_10.0 = phi i32 [ %3, %blk_exit4 ], [ %29, %blk_exit17 ]
  %_local_7.3 = phi i32 [ 0, %blk_exit4 ], [ %_local_7.5, %blk_exit17 ]
  %_local_6.1 = phi i32 [ 1, %blk_exit4 ], [ %30, %blk_exit17 ]
  %_local_5.3 = phi i32 [ -1, %blk_exit4 ], [ %_local_5.5, %blk_exit17 ]
  %13 = inttoptr i32 %_local_10.0 to i16*
  %14 = load i16, i16* %13, align 2
  %15 = icmp eq i16 %14, 0
  br i1 %15, label %blk_exit10, label %brif_next13

blk_exit9:                                        ; preds = %brif_next13, %blk_exit10
  %_local_7.4 = phi i32 [ %_local_7.3, %blk_exit10 ], [ %24, %brif_next13 ]
  %_local_5.4 = phi i32 [ %_local_5.3, %blk_exit10 ], [ %27, %brif_next13 ]
  %16 = add i32 %_local_10.0, 4
  %17 = inttoptr i32 %16 to i16*
  %18 = load i16, i16* %17, align 2
  %19 = icmp eq i16 %18, 0
  br i1 %19, label %blk_exit18, label %brif_next21

blk_exit10:                                       ; preds = %loop_entry
  %20 = add i32 %_local_10.0, 2
  %21 = inttoptr i32 %20 to i16*
  store i16 0, i16* %21, align 2
  br label %blk_exit9

brif_next13:                                      ; preds = %loop_entry
  %22 = add i32 %_local_6.1, 829983
  %23 = inttoptr i32 %22 to i8*
  store i8 0, i8* %23, align 1
  %24 = add i32 %_local_7.3, 1
  %25 = shl i32 %24, 2
  %26 = add i32 %25, 827680
  %27 = add i32 %_local_6.1, -1
  %28 = inttoptr i32 %26 to i32*
  store i32 %27, i32* %28, align 4
  br label %blk_exit9

blk_exit17:                                       ; preds = %brif_next21, %blk_exit18
  %_local_7.5 = phi i32 [ %_local_7.4, %blk_exit18 ], [ %36, %brif_next21 ]
  %_local_5.5 = phi i32 [ %_local_5.4, %blk_exit18 ], [ %_local_6.1, %brif_next21 ]
  %29 = add i32 %_local_10.0, 8
  %30 = add i32 %_local_6.1, 2
  %31 = sub i32 %30, %10
  %.not303 = icmp eq i32 %31, 1
  br i1 %.not303, label %brif_next26, label %loop_entry

blk_exit18:                                       ; preds = %blk_exit9
  %32 = add i32 %_local_10.0, 6
  %33 = inttoptr i32 %32 to i16*
  store i16 0, i16* %33, align 2
  br label %blk_exit17

brif_next21:                                      ; preds = %blk_exit9
  %34 = add i32 %_local_6.1, 829984
  %35 = inttoptr i32 %34 to i8*
  store i8 0, i8* %35, align 1
  %36 = add i32 %_local_7.4, 1
  %37 = shl i32 %36, 2
  %38 = add i32 %37, 827680
  %39 = inttoptr i32 %38 to i32*
  store i32 %_local_6.1, i32* %39, align 4
  br label %blk_exit17

brif_next26:                                      ; preds = %blk_exit17
  %40 = add i32 %_local_6.1, 1
  br label %blk_exit3

blk_exit27:                                       ; preds = %brif_next33, %blk_exit30, %blk_exit3
  %_local_7.6 = phi i32 [ %_local_7.2, %blk_exit3 ], [ %_local_7.2, %blk_exit30 ], [ %51, %brif_next33 ]
  %_local_5.6 = phi i32 [ %_local_5.2, %blk_exit3 ], [ %_local_5.2, %blk_exit30 ], [ %_local_6.0, %brif_next33 ]
  %41 = load i32, i32* inttoptr (i32 824948 to i32*), align 4
  %42 = icmp sgt i32 %_local_7.6, 1
  br i1 %42, label %blk_exit, label %blk_exit2

brif_next29:                                      ; preds = %blk_exit3
  %43 = shl i32 %_local_6.0, 2
  %44 = add i32 %3, %43
  %45 = inttoptr i32 %44 to i16*
  %46 = load i16, i16* %45, align 2
  %47 = icmp eq i16 %46, 0
  br i1 %47, label %blk_exit30, label %brif_next33

blk_exit30:                                       ; preds = %brif_next29
  %calcOffset36 = add i32 %44, 2
  %48 = inttoptr i32 %calcOffset36 to i16*
  store i16 0, i16* %48, align 2
  br label %blk_exit27

brif_next33:                                      ; preds = %brif_next29
  %49 = add i32 %_local_6.0, 829984
  %50 = inttoptr i32 %49 to i8*
  store i8 0, i8* %50, align 1
  %51 = add i32 %_local_7.2, 1
  %52 = shl i32 %51, 2
  %53 = add i32 %52, 827680
  %54 = inttoptr i32 %53 to i32*
  store i32 %_local_6.0, i32* %54, align 4
  br label %blk_exit27

blk_exit39:                                       ; preds = %blk_exit76, %loop_entry53, %blk_exit43
  %_local_5.7 = phi i32 [ %_local_5.8, %blk_exit43 ], [ %97, %loop_entry53 ], [ %_local_5.11, %blk_exit76 ]
  %55 = add i32 %_local_7.1, %_local_8.1
  %56 = add i32 %55, -2
  store i32 %56, i32* inttoptr (i32 824948 to i32*), align 4
  br label %blk_exit

blk_exit40:                                       ; preds = %blk_exit2
  %57 = load i32, i32* inttoptr (i32 824952 to i32*), align 8
  %58 = and i32 %_local_7.1, 1
  %brif_val68.not = icmp eq i32 %58, 0
  br i1 %brif_val68.not, label %blk_exit66, label %blk_exit67

brif_next42:                                      ; preds = %blk_exit2
  %59 = and i32 %_local_7.1, 1
  %brif_val45.not = icmp eq i32 %59, 0
  br i1 %brif_val45.not, label %blk_exit43, label %blk_exit44

blk_exit43:                                       ; preds = %blk_exit44, %brif_next42
  %_local_6.2 = phi i32 [ %61, %blk_exit44 ], [ %_local_7.1, %brif_next42 ]
  %_local_5.8 = phi i32 [ %73, %blk_exit44 ], [ %_local_5.1, %brif_next42 ]
  %60 = icmp eq i32 %_local_7.1, 1
  br i1 %60, label %blk_exit39, label %brif_next52

blk_exit44:                                       ; preds = %brif_next42
  %61 = add i32 %_local_7.1, 1
  %62 = shl i32 %61, 2
  %63 = add i32 %62, 827680
  %64 = add i32 %_local_5.1, 1
  %65 = icmp slt i32 %_local_5.1, 2
  %66 = select i1 %65, i32 %64, i32 0
  %67 = inttoptr i32 %63 to i32*
  store i32 %66, i32* %67, align 4
  %68 = shl i32 %66, 2
  %69 = add i32 %3, %68
  %70 = inttoptr i32 %69 to i16*
  store i16 1, i16* %70, align 2
  %71 = add i32 %66, 829984
  %72 = inttoptr i32 %71 to i8*
  store i8 0, i8* %72, align 1
  %73 = select i1 %65, i32 %64, i32 %_local_5.1
  br label %blk_exit43

brif_next52:                                      ; preds = %blk_exit43
  %74 = add i32 %_local_6.2, -2
  %75 = shl i32 %_local_6.2, 2
  %76 = add i32 %75, 827688
  br label %loop_entry53

loop_entry53:                                     ; preds = %loop_entry53, %brif_next52
  %_local_6.3 = phi i32 [ %76, %brif_next52 ], [ %98, %loop_entry53 ]
  %_local_5.9 = phi i32 [ %_local_5.8, %brif_next52 ], [ %97, %loop_entry53 ]
  %_local_4.0 = phi i32 [ %74, %brif_next52 ], [ %99, %loop_entry53 ]
  %77 = add i32 %_local_6.3, -4
  %78 = add i32 %_local_5.9, 1
  %79 = icmp slt i32 %_local_5.9, 2
  %80 = select i1 %79, i32 %78, i32 0
  %81 = inttoptr i32 %77 to i32*
  store i32 %80, i32* %81, align 4
  %82 = shl i32 %80, 2
  %83 = add i32 %3, %82
  %84 = inttoptr i32 %83 to i16*
  store i16 1, i16* %84, align 2
  %85 = select i1 %79, i32 %78, i32 %_local_5.9
  %86 = add i32 %85, 1
  %87 = icmp slt i32 %85, 2
  %88 = select i1 %87, i32 %86, i32 0
  %89 = inttoptr i32 %_local_6.3 to i32*
  store i32 %88, i32* %89, align 4
  %90 = add i32 %80, 829984
  %91 = inttoptr i32 %90 to i8*
  store i8 0, i8* %91, align 1
  %92 = shl i32 %88, 2
  %93 = add i32 %3, %92
  %94 = inttoptr i32 %93 to i16*
  store i16 1, i16* %94, align 2
  %95 = add i32 %88, 829984
  %96 = inttoptr i32 %95 to i8*
  store i8 0, i8* %96, align 1
  %97 = select i1 %87, i32 %86, i32 %85
  %98 = add i32 %_local_6.3, 8
  %99 = add i32 %_local_4.0, 2
  %brif_val64.not = icmp eq i32 %99, 0
  br i1 %brif_val64.not, label %blk_exit39, label %loop_entry53

blk_exit66:                                       ; preds = %blk_exit67, %blk_exit40
  %_local_6.4 = phi i32 [ %101, %blk_exit67 ], [ %_local_7.1, %blk_exit40 ]
  %_local_5.10 = phi i32 [ %113, %blk_exit67 ], [ %_local_5.1, %blk_exit40 ]
  %_local_4.1 = phi i32 [ %118, %blk_exit67 ], [ %57, %blk_exit40 ]
  %100 = icmp eq i32 %_local_7.1, 1
  br i1 %100, label %blk_exit76, label %brif_next78

blk_exit67:                                       ; preds = %blk_exit40
  %101 = add i32 %_local_7.1, 1
  %102 = shl i32 %101, 2
  %103 = add i32 %102, 827680
  %104 = add i32 %_local_5.1, 1
  %105 = icmp slt i32 %_local_5.1, 2
  %106 = select i1 %105, i32 %104, i32 0
  %107 = inttoptr i32 %103 to i32*
  store i32 %106, i32* %107, align 4
  %108 = shl i32 %106, 2
  %109 = add i32 %3, %108
  %110 = inttoptr i32 %109 to i16*
  store i16 1, i16* %110, align 2
  %111 = add i32 %106, 829984
  %112 = inttoptr i32 %111 to i8*
  store i8 0, i8* %112, align 1
  %113 = select i1 %105, i32 %104, i32 %_local_5.1
  %114 = add i32 %1, %108
  %calcOffset75 = add i32 %114, 2
  %115 = inttoptr i32 %calcOffset75 to i16*
  %116 = load i16, i16* %115, align 2
  %117 = zext i16 %116 to i32
  %118 = sub i32 %57, %117
  br label %blk_exit66

blk_exit76:                                       ; preds = %loop_entry79, %blk_exit66
  %_local_5.11 = phi i32 [ %_local_5.10, %blk_exit66 ], [ %146, %loop_entry79 ]
  %_local_4.2 = phi i32 [ %_local_4.1, %blk_exit66 ], [ %152, %loop_entry79 ]
  store i32 %_local_4.2, i32* inttoptr (i32 824952 to i32*), align 8
  br label %blk_exit39

brif_next78:                                      ; preds = %blk_exit66
  %119 = add i32 %_local_6.4, -2
  %120 = shl i32 %_local_6.4, 2
  %121 = add i32 %120, 827688
  br label %loop_entry79

loop_entry79:                                     ; preds = %loop_entry79, %brif_next78
  %_local_11.0 = phi i32 [ %119, %brif_next78 ], [ %154, %loop_entry79 ]
  %_local_6.5 = phi i32 [ %121, %brif_next78 ], [ %153, %loop_entry79 ]
  %_local_5.12 = phi i32 [ %_local_5.10, %brif_next78 ], [ %146, %loop_entry79 ]
  %_local_4.3 = phi i32 [ %_local_4.1, %brif_next78 ], [ %152, %loop_entry79 ]
  %122 = add i32 %_local_6.5, -4
  %123 = add i32 %_local_5.12, 1
  %124 = icmp slt i32 %_local_5.12, 2
  %125 = select i1 %124, i32 %123, i32 0
  %126 = inttoptr i32 %122 to i32*
  store i32 %125, i32* %126, align 4
  %127 = shl i32 %125, 2
  %128 = add i32 %3, %127
  %129 = inttoptr i32 %128 to i16*
  store i16 1, i16* %129, align 2
  %130 = add i32 %125, 829984
  %131 = inttoptr i32 %130 to i8*
  store i8 0, i8* %131, align 1
  %132 = add i32 %1, %127
  %calcOffset84 = add i32 %132, 2
  %133 = inttoptr i32 %calcOffset84 to i16*
  %134 = load i16, i16* %133, align 2
  %135 = zext i16 %134 to i32
  %136 = select i1 %124, i32 %123, i32 %_local_5.12
  %137 = add i32 %136, 1
  %138 = icmp slt i32 %136, 2
  %139 = select i1 %138, i32 %137, i32 0
  %140 = inttoptr i32 %_local_6.5 to i32*
  store i32 %139, i32* %140, align 4
  %141 = shl i32 %139, 2
  %142 = add i32 %3, %141
  %143 = inttoptr i32 %142 to i16*
  store i16 1, i16* %143, align 2
  %144 = add i32 %139, 829984
  %145 = inttoptr i32 %144 to i8*
  store i8 0, i8* %145, align 1
  %146 = select i1 %138, i32 %137, i32 %136
  %147 = add i32 %1, %141
  %calcOffset91 = add i32 %147, 2
  %148 = inttoptr i32 %calcOffset91 to i16*
  %149 = load i16, i16* %148, align 2
  %150 = zext i16 %149 to i32
  %151 = add nuw nsw i32 %135, %150
  %152 = sub i32 %_local_4.3, %151
  %153 = add i32 %_local_6.5, 8
  %154 = add i32 %_local_11.0, 2
  %brif_val92.not = icmp eq i32 %154, 0
  br i1 %brif_val92.not, label %blk_exit76, label %loop_entry79

loop_entry95:                                     ; preds = %blk_exit97, %blk_exit
  %_local_6.6 = phi i32 [ %8, %blk_exit ], [ %164, %blk_exit97 ]
  %155 = shl i32 %_local_6.6, 2
  %156 = add i32 %155, 827680
  %157 = inttoptr i32 %156 to i32*
  %158 = load i32, i32* %157, align 4
  %159 = shl i32 %_local_6.6, 1
  %160 = icmp sgt i32 %159, %_local_7.0
  br i1 %160, label %blk_exit97, label %brif_next99

blk_exit97:                                       ; preds = %brif_next127, %blk_exit125, %blk_exit102, %loop_entry95
  %_local_6.7 = phi i32 [ %_local_6.6, %loop_entry95 ], [ %_local_6.8, %blk_exit125 ], [ %_local_9.0, %blk_exit102 ], [ %_local_9.0, %brif_next127 ]
  %161 = shl i32 %_local_6.7, 2
  %162 = add i32 %161, 827680
  %163 = inttoptr i32 %162 to i32*
  store i32 %158, i32* %163, align 4
  %164 = add i32 %_local_6.6, -1
  %165 = icmp sgt i32 %_local_6.6, 1
  br i1 %165, label %loop_entry95, label %loop_entry138

brif_next99:                                      ; preds = %loop_entry95
  %166 = shl i32 %158, 2
  %167 = add i32 %3, %166
  %168 = inttoptr i32 %167 to i16*
  %169 = load i16, i16* %168, align 2
  br label %loop_entry101

loop_entry101:                                    ; preds = %blk_exit125, %brif_next99
  %_local_10.1 = phi i32 [ %159, %brif_next99 ], [ %205, %blk_exit125 ]
  %_local_9.0 = phi i32 [ %_local_6.6, %brif_next99 ], [ %_local_6.8, %blk_exit125 ]
  %170 = icmp slt i32 %_local_10.1, %_local_7.0
  br i1 %170, label %blk_exit103, label %brif_next105

blk_exit102:                                      ; preds = %blk_exit113, %brif_next112, %blk_exit106, %brif_next105
  %.pre-phi319 = phi i32* [ %179, %blk_exit106 ], [ %187, %blk_exit113 ], [ %.pre318, %brif_next105 ], [ %187, %brif_next112 ]
  %_local_6.8 = phi i32 [ %176, %blk_exit106 ], [ %_local_10.1, %blk_exit113 ], [ %_local_10.1, %brif_next105 ], [ %_local_10.1, %brif_next112 ]
  %171 = load i32, i32* %.pre-phi319, align 4
  %172 = shl i32 %171, 2
  %173 = add i32 %3, %172
  %174 = inttoptr i32 %173 to i16*
  %175 = load i16, i16* %174, align 2
  %.not296 = icmp ult i16 %169, %175
  br i1 %.not296, label %blk_exit97, label %blk_exit120

blk_exit103:                                      ; preds = %loop_entry101
  %176 = or i32 %_local_10.1, 1
  %177 = shl i32 %176, 2
  %178 = add i32 %177, 827680
  %179 = inttoptr i32 %178 to i32*
  %180 = load i32, i32* %179, align 4
  %181 = shl i32 %180, 2
  %182 = add i32 %3, %181
  %183 = inttoptr i32 %182 to i16*
  %184 = load i16, i16* %183, align 2
  %185 = shl i32 %_local_10.1, 2
  %186 = add i32 %185, 827680
  %187 = inttoptr i32 %186 to i32*
  %188 = load i32, i32* %187, align 8
  %189 = shl i32 %188, 2
  %190 = add i32 %3, %189
  %191 = inttoptr i32 %190 to i16*
  %192 = load i16, i16* %191, align 2
  %193 = icmp ult i16 %184, %192
  br i1 %193, label %blk_exit106, label %brif_next112

brif_next105:                                     ; preds = %loop_entry101
  %.pre314 = shl i32 %_local_10.1, 2
  %.pre316 = add i32 %.pre314, 827680
  %.pre318 = inttoptr i32 %.pre316 to i32*
  br label %blk_exit102

blk_exit106:                                      ; preds = %blk_exit113, %blk_exit103
  br label %blk_exit102

brif_next112:                                     ; preds = %blk_exit103
  %194 = icmp eq i16 %184, %192
  br i1 %194, label %blk_exit113, label %blk_exit102

blk_exit113:                                      ; preds = %brif_next112
  %195 = add i32 %180, 829984
  %196 = inttoptr i32 %195 to i8*
  %197 = load i8, i8* %196, align 1
  %198 = add i32 %188, 829984
  %199 = inttoptr i32 %198 to i8*
  %200 = load i8, i8* %199, align 1
  %201 = icmp ugt i8 %197, %200
  br i1 %201, label %blk_exit102, label %blk_exit106

blk_exit120:                                      ; preds = %blk_exit102
  %.not301 = icmp eq i16 %169, %175
  br i1 %.not301, label %brif_next127, label %blk_exit125

blk_exit125:                                      ; preds = %brif_next127, %blk_exit120
  %202 = shl i32 %_local_9.0, 2
  %203 = add i32 %202, 827680
  %204 = inttoptr i32 %203 to i32*
  store i32 %171, i32* %204, align 4
  %205 = shl i32 %_local_6.8, 1
  %.not302 = icmp sgt i32 %205, %_local_7.0
  br i1 %.not302, label %blk_exit97, label %loop_entry101

brif_next127:                                     ; preds = %blk_exit120
  %206 = add i32 %158, 829984
  %207 = inttoptr i32 %206 to i8*
  %208 = load i8, i8* %207, align 1
  %209 = add i32 %171, 829984
  %210 = inttoptr i32 %209 to i8*
  %211 = load i8, i8* %210, align 1
  %212 = icmp ugt i8 %208, %211
  br i1 %212, label %blk_exit125, label %blk_exit97

loop_entry138:                                    ; preds = %blk_exit212, %blk_exit97
  %_local_18.0 = phi i32 [ %364, %blk_exit212 ], [ 2, %blk_exit97 ]
  %_local_17.0 = phi i32 [ %363, %blk_exit212 ], [ 829964, %blk_exit97 ]
  %_local_16.0 = phi i32 [ %223, %blk_exit212 ], [ 573, %blk_exit97 ]
  %_local_7.7 = phi i32 [ %218, %blk_exit212 ], [ %_local_7.0, %blk_exit97 ]
  %_local_3.0 = phi i32 [ %325, %blk_exit212 ], [ %5, %blk_exit97 ]
  %213 = load i32, i32* inttoptr (i32 827684 to i32*), align 4
  %214 = shl i32 %_local_7.7, 2
  %215 = add i32 %214, 827680
  %216 = inttoptr i32 %215 to i32*
  %217 = load i32, i32* %216, align 4
  store i32 %217, i32* inttoptr (i32 827684 to i32*), align 4
  %218 = add i32 %_local_7.7, -1
  %219 = icmp slt i32 %_local_7.7, 3
  br i1 %219, label %blk_exit140, label %brif_next142

blk_exit140:                                      ; preds = %brif_next170, %blk_exit168, %blk_exit145, %loop_entry138
  %_local_6.9 = phi i32 [ 1, %loop_entry138 ], [ %_local_6.10, %blk_exit168 ], [ %_local_9.1, %blk_exit145 ], [ %_local_9.1, %brif_next170 ]
  %220 = shl i32 %_local_6.9, 2
  %221 = add i32 %220, 827680
  %222 = inttoptr i32 %221 to i32*
  store i32 %217, i32* %222, align 4
  %223 = add i32 %_local_16.0, -2
  %224 = shl i32 %223, 2
  %225 = add i32 %224, 827680
  %226 = load i32, i32* inttoptr (i32 827684 to i32*), align 4
  %227 = inttoptr i32 %225 to i32*
  store i32 %226, i32* %227, align 4
  %228 = shl i32 %_local_16.0, 2
  %229 = add i32 %228, 827676
  %230 = inttoptr i32 %229 to i32*
  store i32 %213, i32* %230, align 4
  %231 = shl i32 %_local_3.0, 2
  %232 = add i32 %3, %231
  %233 = shl i32 %226, 2
  %234 = add i32 %3, %233
  %235 = inttoptr i32 %234 to i16*
  %236 = load i16, i16* %235, align 2
  %237 = zext i16 %236 to i32
  %238 = shl i32 %213, 2
  %239 = add i32 %3, %238
  %240 = inttoptr i32 %239 to i16*
  %241 = load i16, i16* %240, align 2
  %242 = zext i16 %241 to i32
  %243 = add nuw nsw i32 %237, %242
  %244 = trunc i32 %243 to i16
  %245 = inttoptr i32 %232 to i16*
  store i16 %244, i16* %245, align 2
  %246 = add i32 %_local_3.0, 829984
  %247 = add i32 %226, 829984
  %248 = inttoptr i32 %247 to i8*
  %249 = load i8, i8* %248, align 1
  %250 = zext i8 %249 to i32
  %251 = add i32 %213, 829984
  %252 = inttoptr i32 %251 to i8*
  %253 = load i8, i8* %252, align 1
  %254 = zext i8 %253 to i32
  %255 = icmp ugt i32 %250, %254
  %256 = select i1 %255, i32 %250, i32 %254
  %257 = add nuw nsw i32 %256, 1
  %258 = inttoptr i32 %246 to i8*
  %259 = trunc i32 %257 to i8
  store i8 %259, i8* %258, align 1
  %calcOffset188 = add i32 %234, 2
  %260 = trunc i32 %_local_3.0 to i16
  %261 = inttoptr i32 %calcOffset188 to i16*
  store i16 %260, i16* %261, align 2
  %calcOffset189 = add i32 %239, 2
  %262 = inttoptr i32 %calcOffset189 to i16*
  store i16 %260, i16* %262, align 2
  store i32 %_local_3.0, i32* inttoptr (i32 827684 to i32*), align 4
  br i1 %219, label %blk_exit190, label %brif_next192

brif_next142:                                     ; preds = %loop_entry138
  %263 = shl i32 %217, 2
  %264 = add i32 %3, %263
  %265 = inttoptr i32 %264 to i16*
  %266 = load i16, i16* %265, align 2
  br label %loop_entry144

loop_entry144:                                    ; preds = %blk_exit168, %brif_next142
  %_local_10.2 = phi i32 [ 2, %brif_next142 ], [ %302, %blk_exit168 ]
  %_local_9.1 = phi i32 [ 1, %brif_next142 ], [ %_local_6.10, %blk_exit168 ]
  %267 = icmp slt i32 %_local_10.2, %218
  br i1 %267, label %blk_exit146, label %brif_next148

blk_exit145:                                      ; preds = %blk_exit156, %brif_next155, %blk_exit149, %brif_next148
  %.pre-phi313 = phi i32* [ %276, %blk_exit149 ], [ %284, %blk_exit156 ], [ %.pre312, %brif_next148 ], [ %284, %brif_next155 ]
  %_local_6.10 = phi i32 [ %273, %blk_exit149 ], [ %_local_10.2, %blk_exit156 ], [ %_local_10.2, %brif_next148 ], [ %_local_10.2, %brif_next155 ]
  %268 = load i32, i32* %.pre-phi313, align 4
  %269 = shl i32 %268, 2
  %270 = add i32 %3, %269
  %271 = inttoptr i32 %270 to i16*
  %272 = load i16, i16* %271, align 2
  %.not297 = icmp ult i16 %266, %272
  br i1 %.not297, label %blk_exit140, label %blk_exit163

blk_exit146:                                      ; preds = %loop_entry144
  %273 = or i32 %_local_10.2, 1
  %274 = shl i32 %273, 2
  %275 = add i32 %274, 827680
  %276 = inttoptr i32 %275 to i32*
  %277 = load i32, i32* %276, align 4
  %278 = shl i32 %277, 2
  %279 = add i32 %3, %278
  %280 = inttoptr i32 %279 to i16*
  %281 = load i16, i16* %280, align 2
  %282 = shl i32 %_local_10.2, 2
  %283 = add i32 %282, 827680
  %284 = inttoptr i32 %283 to i32*
  %285 = load i32, i32* %284, align 8
  %286 = shl i32 %285, 2
  %287 = add i32 %3, %286
  %288 = inttoptr i32 %287 to i16*
  %289 = load i16, i16* %288, align 2
  %290 = icmp ult i16 %281, %289
  br i1 %290, label %blk_exit149, label %brif_next155

brif_next148:                                     ; preds = %loop_entry144
  %.pre308 = shl i32 %_local_10.2, 2
  %.pre310 = add i32 %.pre308, 827680
  %.pre312 = inttoptr i32 %.pre310 to i32*
  br label %blk_exit145

blk_exit149:                                      ; preds = %blk_exit156, %blk_exit146
  br label %blk_exit145

brif_next155:                                     ; preds = %blk_exit146
  %291 = icmp eq i16 %281, %289
  br i1 %291, label %blk_exit156, label %blk_exit145

blk_exit156:                                      ; preds = %brif_next155
  %292 = add i32 %277, 829984
  %293 = inttoptr i32 %292 to i8*
  %294 = load i8, i8* %293, align 1
  %295 = add i32 %285, 829984
  %296 = inttoptr i32 %295 to i8*
  %297 = load i8, i8* %296, align 1
  %298 = icmp ugt i8 %294, %297
  br i1 %298, label %blk_exit145, label %blk_exit149

blk_exit163:                                      ; preds = %blk_exit145
  %.not300 = icmp eq i16 %266, %272
  br i1 %.not300, label %brif_next170, label %blk_exit168

blk_exit168:                                      ; preds = %brif_next170, %blk_exit163
  %299 = shl i32 %_local_9.1, 2
  %300 = add i32 %299, 827680
  %301 = inttoptr i32 %300 to i32*
  store i32 %268, i32* %301, align 4
  %302 = shl i32 %_local_6.10, 1
  %303 = icmp slt i32 %302, %_local_7.7
  br i1 %303, label %loop_entry144, label %blk_exit140

brif_next170:                                     ; preds = %blk_exit163
  %304 = add i32 %217, 829984
  %305 = inttoptr i32 %304 to i8*
  %306 = load i8, i8* %305, align 1
  %307 = add i32 %268, 829984
  %308 = inttoptr i32 %307 to i8*
  %309 = load i8, i8* %308, align 1
  %310 = icmp ugt i8 %306, %309
  br i1 %310, label %blk_exit168, label %blk_exit140

blk_exit190:                                      ; preds = %blk_exit140
  store i32 %_local_3.0, i32* inttoptr (i32 827684 to i32*), align 4
  %311 = add i32 %228, 827668
  %312 = inttoptr i32 %311 to i32*
  store i32 %_local_3.0, i32* %312, align 4
  store i64 0, i64* inttoptr (i32 819672 to i64*), align 8
  store i64 0, i64* inttoptr (i32 819664 to i64*), align 16
  store i64 0, i64* inttoptr (i32 819656 to i64*), align 8
  store i64 0, i64* inttoptr (i32 819648 to i64*), align 64
  %calcOffset229 = add i32 %_arg_0, 8
  %313 = inttoptr i32 %calcOffset229 to i32*
  %314 = load i32, i32* %313, align 4
  %315 = load i32, i32* %0, align 4
  %calcOffset231 = add i32 %_arg_0, 20
  %316 = inttoptr i32 %calcOffset231 to i32*
  %317 = load i32, i32* %316, align 4
  %318 = load i32, i32* %7, align 4
  %calcOffset233 = add i32 %_arg_0, 12
  %319 = inttoptr i32 %calcOffset233 to i32*
  %320 = load i32, i32* %319, align 4
  %321 = load i32, i32* %2, align 4
  %322 = add i32 %321, %231
  %calcOffset235 = add i32 %322, 2
  %323 = inttoptr i32 %calcOffset235 to i16*
  store i16 0, i16* %323, align 2
  %324 = icmp sgt i32 %_local_16.0, 574
  br i1 %324, label %blk_exit236, label %brif_next238

brif_next192:                                     ; preds = %blk_exit140
  %325 = add i32 %_local_3.0, 1
  br label %loop_entry193

loop_entry193:                                    ; preds = %blk_exit218, %brif_next192
  %_local_10.3 = phi i32 [ 2, %brif_next192 ], [ %368, %blk_exit218 ]
  %_local_9.2 = phi i32 [ 1, %brif_next192 ], [ %_local_6.11, %blk_exit218 ]
  %326 = icmp slt i32 %_local_10.3, %218
  br i1 %326, label %blk_exit195, label %brif_next197

blk_exit194:                                      ; preds = %blk_exit205, %brif_next204, %blk_exit198, %brif_next197
  %.pre-phi307 = phi i32* [ %337, %blk_exit198 ], [ %345, %blk_exit205 ], [ %.pre306, %brif_next197 ], [ %345, %brif_next204 ]
  %_local_6.11 = phi i32 [ %334, %blk_exit198 ], [ %_local_10.3, %blk_exit205 ], [ %_local_10.3, %brif_next197 ], [ %_local_10.3, %brif_next204 ]
  %327 = and i32 %243, 65535
  %328 = load i32, i32* %.pre-phi307, align 4
  %329 = shl i32 %328, 2
  %330 = add i32 %3, %329
  %331 = inttoptr i32 %330 to i16*
  %332 = load i16, i16* %331, align 2
  %333 = zext i16 %332 to i32
  %.not298 = icmp ult i32 %327, %333
  br i1 %.not298, label %blk_exit212, label %blk_exit213

blk_exit195:                                      ; preds = %loop_entry193
  %334 = or i32 %_local_10.3, 1
  %335 = shl i32 %334, 2
  %336 = add i32 %335, 827680
  %337 = inttoptr i32 %336 to i32*
  %338 = load i32, i32* %337, align 4
  %339 = shl i32 %338, 2
  %340 = add i32 %3, %339
  %341 = inttoptr i32 %340 to i16*
  %342 = load i16, i16* %341, align 2
  %343 = shl i32 %_local_10.3, 2
  %344 = add i32 %343, 827680
  %345 = inttoptr i32 %344 to i32*
  %346 = load i32, i32* %345, align 8
  %347 = shl i32 %346, 2
  %348 = add i32 %3, %347
  %349 = inttoptr i32 %348 to i16*
  %350 = load i16, i16* %349, align 2
  %351 = icmp ult i16 %342, %350
  br i1 %351, label %blk_exit198, label %brif_next204

brif_next197:                                     ; preds = %loop_entry193
  %.pre = shl i32 %_local_10.3, 2
  %.pre304 = add i32 %.pre, 827680
  %.pre306 = inttoptr i32 %.pre304 to i32*
  br label %blk_exit194

blk_exit198:                                      ; preds = %blk_exit205, %blk_exit195
  br label %blk_exit194

brif_next204:                                     ; preds = %blk_exit195
  %352 = icmp eq i16 %342, %350
  br i1 %352, label %blk_exit205, label %blk_exit194

blk_exit205:                                      ; preds = %brif_next204
  %353 = add i32 %338, 829984
  %354 = inttoptr i32 %353 to i8*
  %355 = load i8, i8* %354, align 1
  %356 = add i32 %346, 829984
  %357 = inttoptr i32 %356 to i8*
  %358 = load i8, i8* %357, align 1
  %359 = icmp ugt i8 %355, %358
  br i1 %359, label %blk_exit194, label %blk_exit198

blk_exit212:                                      ; preds = %brif_next220, %blk_exit218, %blk_exit194
  %_local_6.12 = phi i32 [ %_local_6.11, %blk_exit218 ], [ %_local_9.2, %blk_exit194 ], [ %_local_9.2, %brif_next220 ]
  %360 = shl i32 %_local_6.12, 2
  %361 = add i32 %360, 827680
  %362 = inttoptr i32 %361 to i32*
  store i32 %_local_3.0, i32* %362, align 4
  %363 = add i32 %_local_17.0, -8
  %364 = add i32 %_local_18.0, 2
  br label %loop_entry138

blk_exit213:                                      ; preds = %blk_exit194
  %.not299 = icmp eq i32 %327, %333
  br i1 %.not299, label %brif_next220, label %blk_exit218

blk_exit218:                                      ; preds = %brif_next220, %blk_exit213
  %365 = shl i32 %_local_9.2, 2
  %366 = add i32 %365, 827680
  %367 = inttoptr i32 %366 to i32*
  store i32 %328, i32* %367, align 4
  %368 = shl i32 %_local_6.11, 1
  %369 = icmp slt i32 %368, %_local_7.7
  br i1 %369, label %loop_entry193, label %blk_exit212

brif_next220:                                     ; preds = %blk_exit213
  %370 = and i32 %257, 255
  %371 = add i32 %328, 829984
  %372 = inttoptr i32 %371 to i8*
  %373 = load i8, i8* %372, align 1
  %374 = zext i8 %373 to i32
  %375 = icmp ugt i32 %370, %374
  br i1 %375, label %blk_exit218, label %blk_exit212

blk_exit236:                                      ; preds = %blk_exit277, %brif_next273, %brif_next259, %blk_exit190
  call void @gen_codes(i32 %3, i32 %_local_5.0)
  ret void

brif_next238:                                     ; preds = %blk_exit190
  %376 = load i32, i32* inttoptr (i32 824952 to i32*), align 8
  br label %loop_entry239

loop_entry239:                                    ; preds = %blk_exit245, %brif_next238
  %_local_19.0 = phi i32 [ %376, %brif_next238 ], [ %_local_19.1, %blk_exit245 ]
  %_local_18.1 = phi i32 [ %_local_18.0, %brif_next238 ], [ %398, %blk_exit245 ]
  %_local_17.1 = phi i32 [ %_local_17.0, %brif_next238 ], [ %397, %blk_exit245 ]
  %_local_8.2 = phi i32 [ %_local_8.0, %brif_next238 ], [ %_local_8.3, %blk_exit245 ]
  %_local_4.4 = phi i32 [ 0, %brif_next238 ], [ %396, %blk_exit245 ]
  %377 = inttoptr i32 %_local_17.1 to i32*
  %378 = load i32, i32* %377, align 4
  %379 = shl i32 %378, 2
  %380 = add i32 %321, %379
  %calcOffset241 = add i32 %380, 2
  %381 = inttoptr i32 %calcOffset241 to i16*
  %382 = load i16, i16* %381, align 2
  %383 = zext i16 %382 to i32
  %384 = shl nuw nsw i32 %383, 2
  %385 = add i32 %321, %384
  %calcOffset242 = add i32 %385, 2
  %386 = inttoptr i32 %calcOffset242 to i16*
  %387 = load i16, i16* %386, align 2
  %388 = zext i16 %387 to i32
  %389 = add nuw nsw i32 %388, 1
  %390 = icmp sgt i32 %317, %388
  %391 = select i1 %390, i32 %389, i32 %317
  %392 = trunc i32 %391 to i16
  store i16 %392, i16* %381, align 2
  %393 = icmp sle i32 %317, %388
  %394 = zext i1 %393 to i32
  %395 = icmp sgt i32 %378, %318
  br i1 %395, label %blk_exit245, label %brif_next247

blk_exit245:                                      ; preds = %brif_next256, %blk_exit250, %loop_entry239
  %_local_19.1 = phi i32 [ %_local_19.0, %loop_entry239 ], [ %_local_19.0, %blk_exit250 ], [ %423, %brif_next256 ]
  %_local_8.3 = phi i32 [ %_local_8.2, %loop_entry239 ], [ %410, %blk_exit250 ], [ %410, %brif_next256 ]
  %396 = add i32 %_local_4.4, %394
  %397 = add i32 %_local_17.1, 4
  %398 = add i32 %_local_18.1, -1
  %brif_val258.not = icmp eq i32 %398, 0
  br i1 %brif_val258.not, label %brif_next259, label %loop_entry239

brif_next247:                                     ; preds = %loop_entry239
  %399 = shl i32 %391, 1
  %400 = add i32 %399, 819648
  %401 = inttoptr i32 %400 to i16*
  %402 = load i16, i16* %401, align 2
  %403 = add i16 %402, 1
  store i16 %403, i16* %401, align 2
  %404 = icmp slt i32 %378, %320
  br i1 %404, label %blk_exit250, label %brif_next252

blk_exit250:                                      ; preds = %brif_next252, %brif_next247
  %_local_3.1 = phi i32 [ 0, %brif_next247 ], [ %416, %brif_next252 ]
  %405 = add i32 %_local_3.1, %391
  %406 = inttoptr i32 %380 to i16*
  %407 = load i16, i16* %406, align 2
  %408 = zext i16 %407 to i32
  %409 = mul i32 %405, %408
  %410 = add i32 %409, %_local_8.2
  store i32 %410, i32* inttoptr (i32 824948 to i32*), align 4
  %411 = icmp eq i32 %315, 0
  br i1 %411, label %blk_exit245, label %brif_next256

brif_next252:                                     ; preds = %brif_next247
  %412 = sub i32 %378, %320
  %413 = shl i32 %412, 2
  %414 = add i32 %314, %413
  %415 = inttoptr i32 %414 to i32*
  %416 = load i32, i32* %415, align 4
  br label %blk_exit250

brif_next256:                                     ; preds = %blk_exit250
  %417 = add i32 %315, %379
  %calcOffset257 = add i32 %417, 2
  %418 = inttoptr i32 %calcOffset257 to i16*
  %419 = load i16, i16* %418, align 2
  %420 = zext i16 %419 to i32
  %421 = add i32 %_local_3.1, %420
  %422 = mul i32 %421, %408
  %423 = add i32 %422, %_local_19.0
  store i32 %423, i32* inttoptr (i32 824952 to i32*), align 8
  br label %blk_exit245

brif_next259:                                     ; preds = %blk_exit245
  %424 = icmp eq i32 %396, 0
  br i1 %424, label %blk_exit236, label %brif_next261

brif_next261:                                     ; preds = %brif_next259
  %425 = shl i32 %317, 1
  %426 = add i32 %425, 819646
  %427 = add i32 %425, 819648
  br label %loop_entry262

loop_entry262:                                    ; preds = %brif_next266, %brif_next261
  %_local_4.5 = phi i32 [ %396, %brif_next261 ], [ %443, %brif_next266 ]
  br label %loop_entry263

loop_entry263:                                    ; preds = %loop_entry263, %loop_entry262
  %_local_6.13 = phi i32 [ %426, %loop_entry262 ], [ %430, %loop_entry263 ]
  %428 = inttoptr i32 %_local_6.13 to i16*
  %429 = load i16, i16* %428, align 2
  %430 = add i32 %_local_6.13, -2
  %431 = icmp eq i16 %429, 0
  br i1 %431, label %loop_entry263, label %brif_next266

brif_next266:                                     ; preds = %loop_entry263
  %432 = add i16 %429, -1
  store i16 %432, i16* %428, align 2
  %433 = add i32 %_local_6.13, 2
  %434 = inttoptr i32 %433 to i16*
  %435 = load i16, i16* %434, align 2
  %436 = add i16 %435, 2
  store i16 %436, i16* %434, align 2
  %437 = inttoptr i32 %427 to i16*
  %438 = load i16, i16* %437, align 2
  %439 = zext i16 %438 to i32
  %440 = add nsw i32 %439, -1
  %441 = trunc i32 %440 to i16
  store i16 %441, i16* %437, align 2
  %442 = icmp sgt i32 %_local_4.5, 2
  %443 = add i32 %_local_4.5, -2
  br i1 %442, label %loop_entry262, label %brif_next273

brif_next273:                                     ; preds = %brif_next266
  %444 = icmp eq i32 %317, 0
  br i1 %444, label %blk_exit236, label %loop_entry276

loop_entry276:                                    ; preds = %brif_next294, %brif_next273
  %_local_10.4 = phi i32 [ %_local_10.5, %brif_next294 ], [ 573, %brif_next273 ]
  %_local_8.4 = phi i32 [ %_local_8.5, %brif_next294 ], [ %_local_8.3, %brif_next273 ]
  %_local_6.14 = phi i32 [ %474, %brif_next294 ], [ %440, %brif_next273 ]
  %_local_1.0 = phi i32 [ %447, %brif_next294 ], [ %317, %brif_next273 ]
  %445 = and i32 %_local_6.14, 65535
  %446 = icmp eq i32 %445, 0
  br i1 %446, label %blk_exit277, label %loop_entry280

blk_exit277:                                      ; preds = %blk_exit285, %loop_entry276
  %_local_10.5 = phi i32 [ %_local_10.4, %loop_entry276 ], [ %451, %blk_exit285 ]
  %_local_8.5 = phi i32 [ %_local_8.4, %loop_entry276 ], [ %_local_8.7, %blk_exit285 ]
  %447 = add i32 %_local_1.0, -1
  %448 = icmp eq i32 %447, 0
  br i1 %448, label %blk_exit236, label %brif_next294

loop_entry280:                                    ; preds = %blk_exit285, %loop_entry276
  %_local_10.6 = phi i32 [ %_local_10.4, %loop_entry276 ], [ %451, %blk_exit285 ]
  %_local_8.6 = phi i32 [ %_local_8.4, %loop_entry276 ], [ %_local_8.7, %blk_exit285 ]
  %_local_4.6 = phi i32 [ %445, %loop_entry276 ], [ %462, %blk_exit285 ]
  %449 = shl i32 %_local_10.6, 2
  %450 = add i32 %449, 827676
  br label %loop_entry281

loop_entry281:                                    ; preds = %loop_entry281, %loop_entry280
  %_local_10.7 = phi i32 [ %_local_10.6, %loop_entry280 ], [ %451, %loop_entry281 ]
  %_local_6.15 = phi i32 [ %450, %loop_entry280 ], [ %454, %loop_entry281 ]
  %451 = add i32 %_local_10.7, -1
  %452 = inttoptr i32 %_local_6.15 to i32*
  %453 = load i32, i32* %452, align 4
  %454 = add i32 %_local_6.15, -4
  %455 = icmp sgt i32 %453, %318
  br i1 %455, label %loop_entry281, label %brif_next284

brif_next284:                                     ; preds = %loop_entry281
  %456 = shl i32 %453, 2
  %457 = add i32 %321, %456
  %calcOffset286 = add i32 %457, 2
  %458 = inttoptr i32 %calcOffset286 to i16*
  %459 = load i16, i16* %458, align 2
  %460 = zext i16 %459 to i32
  %461 = icmp eq i32 %_local_1.0, %460
  br i1 %461, label %blk_exit285, label %brif_next288

blk_exit285:                                      ; preds = %brif_next288, %brif_next284
  %_local_8.7 = phi i32 [ %_local_8.6, %brif_next284 ], [ %468, %brif_next288 ]
  %462 = add i32 %_local_4.6, -1
  %brif_val291.not = icmp eq i32 %462, 0
  br i1 %brif_val291.not, label %blk_exit277, label %loop_entry280

brif_next288:                                     ; preds = %brif_next284
  %463 = sub i32 %_local_1.0, %460
  %464 = inttoptr i32 %457 to i16*
  %465 = load i16, i16* %464, align 2
  %466 = zext i16 %465 to i32
  %467 = mul i32 %463, %466
  %468 = add i32 %467, %_local_8.6
  store i32 %468, i32* inttoptr (i32 824948 to i32*), align 4
  %469 = trunc i32 %_local_1.0 to i16
  store i16 %469, i16* %458, align 2
  br label %blk_exit285

brif_next294:                                     ; preds = %blk_exit277
  %470 = shl i32 %447, 1
  %471 = add i32 %470, 819648
  %472 = inttoptr i32 %471 to i16*
  %473 = load i16, i16* %472, align 2
  %474 = zext i16 %473 to i32
  br label %loop_entry276
}

; Function Attrs: null_pointer_is_valid
declare dso_local void @compress_block(i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @send_tree(i32, i32) #0

; Function Attrs: null_pointer_is_valid
define internal i32 @ct_tally(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %0 = load i32, i32* inttoptr (i32 827508 to i32*), align 4
  %1 = add i32 %0, 20480
  %2 = inttoptr i32 %1 to i8*
  %3 = trunc i32 %_arg_1 to i8
  store i8 %3, i8* %2, align 1
  %4 = add i32 %0, 1
  store i32 %4, i32* inttoptr (i32 827508 to i32*), align 4
  %brif_val.not = icmp eq i32 %_arg_0, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit1

blk_exit:                                         ; preds = %brif_next, %blk_exit1
  %_local_3.0.in = phi i8 [ %44, %brif_next ], [ %10, %blk_exit1 ]
  %5 = shl i8 %_local_3.0.in, 1
  store i8 %5, i8* inttoptr (i32 827516 to i8*), align 4
  %6 = and i32 %4, 7
  %brif_val12.not = icmp eq i32 %6, 0
  br i1 %brif_val12.not, label %brif_next13, label %blk_exit11

blk_exit1:                                        ; preds = %allocator
  %7 = load i32, i32* inttoptr (i32 827512 to i32*), align 8
  %8 = add i32 %7, 1
  store i32 %8, i32* inttoptr (i32 827512 to i32*), align 8
  %9 = load i8, i8* inttoptr (i32 820832 to i8*), align 32
  %10 = load i8, i8* inttoptr (i32 827516 to i8*), align 4
  %11 = or i8 %9, %10
  store i8 %11, i8* inttoptr (i32 820832 to i8*), align 32
  %12 = shl i32 %7, 1
  %13 = add i32 %12, 550912
  %14 = add i32 %_arg_0, -1
  %15 = trunc i32 %14 to i16
  %16 = inttoptr i32 %13 to i16*
  store i16 %15, i16* %16, align 2
  %17 = add i32 %_arg_1, 818752
  %18 = inttoptr i32 %17 to i8*
  %19 = load i8, i8* %18, align 1
  %20 = zext i8 %19 to i32
  %21 = shl nuw nsw i32 %20, 2
  %22 = add nuw nsw i32 %21, 825988
  %23 = inttoptr i32 %22 to i16*
  %24 = load i16, i16* %23, align 4
  %25 = add i16 %24, 1
  store i16 %25, i16* %23, align 4
  %26 = ashr i32 %14, 7
  %27 = add nsw i32 %26, 256
  %28 = icmp slt i32 %_arg_0, 257
  %29 = select i1 %28, i32 %14, i32 %27
  %30 = add i32 %29, 819136
  %31 = inttoptr i32 %30 to i8*
  %32 = load i8, i8* %31, align 1
  %33 = zext i8 %32 to i32
  %34 = shl nuw nsw i32 %33, 2
  %35 = add nuw nsw i32 %34, 827264
  %36 = inttoptr i32 %35 to i16*
  %37 = load i16, i16* %36, align 4
  %38 = add i16 %37, 1
  store i16 %38, i16* %36, align 4
  br label %blk_exit

brif_next:                                        ; preds = %allocator
  %39 = shl i32 %_arg_1, 2
  %40 = add i32 %39, 824960
  %41 = inttoptr i32 %40 to i16*
  %42 = load i16, i16* %41, align 4
  %43 = add i16 %42, 1
  store i16 %43, i16* %41, align 4
  %44 = load i8, i8* inttoptr (i32 827516 to i8*), align 4
  br label %blk_exit

blk_exit11:                                       ; preds = %brif_next13, %blk_exit
  %45 = load i32, i32* inttoptr (i32 827512 to i32*), align 8
  %46 = load i32, i32* inttoptr (i32 15144 to i32*), align 8
  %47 = icmp sge i32 %46, 3
  %48 = and i32 %4, 4095
  %brif_val19.not = icmp eq i32 %48, 0
  %or.cond = select i1 %47, i1 %brif_val19.not, i1 false
  %49 = lshr i32 %4, 1
  %.not = icmp ult i32 %45, %49
  %or.cond25 = select i1 %or.cond, i1 %.not, i1 false
  br i1 %or.cond25, label %brif_next22, label %blk_exit16

brif_next13:                                      ; preds = %blk_exit
  store i8 1, i8* inttoptr (i32 827516 to i8*), align 4
  %50 = load i32, i32* inttoptr (i32 824944 to i32*), align 16
  %51 = add i32 %50, 1
  store i32 %51, i32* inttoptr (i32 824944 to i32*), align 16
  %52 = add i32 %50, 820848
  %53 = load i8, i8* inttoptr (i32 820832 to i8*), align 32
  %54 = inttoptr i32 %52 to i8*
  store i8 %53, i8* %54, align 1
  store i8 0, i8* inttoptr (i32 820832 to i8*), align 32
  br label %blk_exit11

blk_exit15:                                       ; preds = %brif_next22, %blk_exit16
  %_param_1.0 = phi i32 [ %58, %blk_exit16 ], [ 1, %brif_next22 ]
  ret i32 %_param_1.0

blk_exit16:                                       ; preds = %brif_next22, %blk_exit11
  %55 = icmp eq i32 %4, 32767
  %56 = icmp eq i32 %45, 32768
  %57 = or i1 %55, %56
  %58 = zext i1 %57 to i32
  br label %blk_exit15

brif_next22:                                      ; preds = %blk_exit11
  %59 = load i32, i32* inttoptr (i32 15748 to i32*), align 4
  %60 = add i32 %59, 5
  %61 = load i16, i16* inttoptr (i32 827380 to i16*), align 4
  %62 = zext i16 %61 to i32
  %63 = mul i32 %60, %62
  %64 = load i32, i32* inttoptr (i32 15744 to i32*), align 128
  %65 = add i32 %64, 5
  %66 = load i16, i16* inttoptr (i32 827376 to i16*), align 16
  %67 = zext i16 %66 to i32
  %68 = mul i32 %65, %67
  %69 = load i32, i32* inttoptr (i32 15740 to i32*), align 4
  %70 = add i32 %69, 5
  %71 = load i16, i16* inttoptr (i32 827372 to i16*), align 4
  %72 = zext i16 %71 to i32
  %73 = mul i32 %70, %72
  %74 = load i32, i32* inttoptr (i32 15736 to i32*), align 8
  %75 = add i32 %74, 5
  %76 = load i16, i16* inttoptr (i32 827368 to i16*), align 8
  %77 = zext i16 %76 to i32
  %78 = mul i32 %75, %77
  %79 = load i32, i32* inttoptr (i32 15732 to i32*), align 4
  %80 = add i32 %79, 5
  %81 = load i16, i16* inttoptr (i32 827364 to i16*), align 4
  %82 = zext i16 %81 to i32
  %83 = mul i32 %80, %82
  %84 = load i32, i32* inttoptr (i32 15728 to i32*), align 16
  %85 = add i32 %84, 5
  %86 = load i16, i16* inttoptr (i32 827360 to i16*), align 32
  %87 = zext i16 %86 to i32
  %88 = mul i32 %85, %87
  %89 = load i32, i32* inttoptr (i32 15724 to i32*), align 4
  %90 = add i32 %89, 5
  %91 = load i16, i16* inttoptr (i32 827356 to i16*), align 4
  %92 = zext i16 %91 to i32
  %93 = mul i32 %90, %92
  %94 = load i32, i32* inttoptr (i32 15720 to i32*), align 8
  %95 = add i32 %94, 5
  %96 = load i16, i16* inttoptr (i32 827352 to i16*), align 8
  %97 = zext i16 %96 to i32
  %98 = mul i32 %95, %97
  %99 = load i32, i32* inttoptr (i32 15716 to i32*), align 4
  %100 = add i32 %99, 5
  %101 = load i16, i16* inttoptr (i32 827348 to i16*), align 4
  %102 = zext i16 %101 to i32
  %103 = mul i32 %100, %102
  %104 = load i32, i32* inttoptr (i32 15712 to i32*), align 32
  %105 = add i32 %104, 5
  %106 = load i16, i16* inttoptr (i32 827344 to i16*), align 16
  %107 = zext i16 %106 to i32
  %108 = mul i32 %105, %107
  %109 = load i32, i32* inttoptr (i32 15708 to i32*), align 4
  %110 = add i32 %109, 5
  %111 = load i16, i16* inttoptr (i32 827340 to i16*), align 4
  %112 = zext i16 %111 to i32
  %113 = mul i32 %110, %112
  %114 = load i32, i32* inttoptr (i32 15704 to i32*), align 8
  %115 = add i32 %114, 5
  %116 = load i16, i16* inttoptr (i32 827336 to i16*), align 8
  %117 = zext i16 %116 to i32
  %118 = mul i32 %115, %117
  %119 = load i32, i32* inttoptr (i32 15700 to i32*), align 4
  %120 = add i32 %119, 5
  %121 = load i16, i16* inttoptr (i32 827332 to i16*), align 4
  %122 = zext i16 %121 to i32
  %123 = mul i32 %120, %122
  %124 = load i32, i32* inttoptr (i32 15696 to i32*), align 16
  %125 = add i32 %124, 5
  %126 = load i16, i16* inttoptr (i32 827328 to i16*), align 64
  %127 = zext i16 %126 to i32
  %128 = mul i32 %125, %127
  %129 = load i32, i32* inttoptr (i32 15692 to i32*), align 4
  %130 = add i32 %129, 5
  %131 = load i16, i16* inttoptr (i32 827324 to i16*), align 4
  %132 = zext i16 %131 to i32
  %133 = mul i32 %130, %132
  %134 = load i32, i32* inttoptr (i32 15688 to i32*), align 8
  %135 = add i32 %134, 5
  %136 = load i16, i16* inttoptr (i32 827320 to i16*), align 8
  %137 = zext i16 %136 to i32
  %138 = mul i32 %135, %137
  %139 = load i32, i32* inttoptr (i32 15684 to i32*), align 4
  %140 = add i32 %139, 5
  %141 = load i16, i16* inttoptr (i32 827316 to i16*), align 4
  %142 = zext i16 %141 to i32
  %143 = mul i32 %140, %142
  %144 = load i32, i32* inttoptr (i32 15680 to i32*), align 64
  %145 = add i32 %144, 5
  %146 = load i16, i16* inttoptr (i32 827312 to i16*), align 16
  %147 = zext i16 %146 to i32
  %148 = mul i32 %145, %147
  %149 = load i32, i32* inttoptr (i32 15676 to i32*), align 4
  %150 = add i32 %149, 5
  %151 = load i16, i16* inttoptr (i32 827308 to i16*), align 4
  %152 = zext i16 %151 to i32
  %153 = mul i32 %150, %152
  %154 = load i32, i32* inttoptr (i32 15672 to i32*), align 8
  %155 = add i32 %154, 5
  %156 = load i16, i16* inttoptr (i32 827304 to i16*), align 8
  %157 = zext i16 %156 to i32
  %158 = mul i32 %155, %157
  %159 = load i32, i32* inttoptr (i32 15668 to i32*), align 4
  %160 = add i32 %159, 5
  %161 = load i16, i16* inttoptr (i32 827300 to i16*), align 4
  %162 = zext i16 %161 to i32
  %163 = mul i32 %160, %162
  %164 = load i32, i32* inttoptr (i32 15664 to i32*), align 16
  %165 = add i32 %164, 5
  %166 = load i16, i16* inttoptr (i32 827296 to i16*), align 32
  %167 = zext i16 %166 to i32
  %168 = mul i32 %165, %167
  %169 = load i32, i32* inttoptr (i32 15660 to i32*), align 4
  %170 = add i32 %169, 5
  %171 = load i16, i16* inttoptr (i32 827292 to i16*), align 4
  %172 = zext i16 %171 to i32
  %173 = mul i32 %170, %172
  %174 = load i32, i32* inttoptr (i32 15656 to i32*), align 8
  %175 = add i32 %174, 5
  %176 = load i16, i16* inttoptr (i32 827288 to i16*), align 8
  %177 = zext i16 %176 to i32
  %178 = mul i32 %175, %177
  %179 = load i32, i32* inttoptr (i32 15652 to i32*), align 4
  %180 = add i32 %179, 5
  %181 = load i16, i16* inttoptr (i32 827284 to i16*), align 4
  %182 = zext i16 %181 to i32
  %183 = mul i32 %180, %182
  %184 = load i32, i32* inttoptr (i32 15648 to i32*), align 32
  %185 = add i32 %184, 5
  %186 = load i16, i16* inttoptr (i32 827280 to i16*), align 16
  %187 = zext i16 %186 to i32
  %188 = mul i32 %185, %187
  %189 = load i32, i32* inttoptr (i32 15644 to i32*), align 4
  %190 = add i32 %189, 5
  %191 = load i16, i16* inttoptr (i32 827276 to i16*), align 4
  %192 = zext i16 %191 to i32
  %193 = mul i32 %190, %192
  %194 = load i32, i32* inttoptr (i32 15640 to i32*), align 8
  %195 = add i32 %194, 5
  %196 = load i16, i16* inttoptr (i32 827272 to i16*), align 8
  %197 = zext i16 %196 to i32
  %198 = mul i32 %195, %197
  %199 = load i32, i32* inttoptr (i32 15636 to i32*), align 4
  %200 = add i32 %199, 5
  %201 = load i16, i16* inttoptr (i32 827268 to i16*), align 4
  %202 = zext i16 %201 to i32
  %203 = mul i32 %200, %202
  %204 = load i32, i32* inttoptr (i32 15632 to i32*), align 16
  %205 = add i32 %204, 5
  %206 = load i16, i16* inttoptr (i32 827264 to i16*), align 128
  %207 = zext i16 %206 to i32
  %208 = mul i32 %205, %207
  %209 = shl i32 %4, 3
  %210 = add i32 %208, %209
  %211 = add i32 %203, %210
  %212 = add i32 %198, %211
  %213 = add i32 %193, %212
  %214 = add i32 %188, %213
  %215 = add i32 %183, %214
  %216 = add i32 %178, %215
  %217 = add i32 %173, %216
  %218 = add i32 %168, %217
  %219 = add i32 %163, %218
  %220 = add i32 %158, %219
  %221 = add i32 %153, %220
  %222 = add i32 %148, %221
  %223 = add i32 %143, %222
  %224 = add i32 %138, %223
  %225 = add i32 %133, %224
  %226 = add i32 %128, %225
  %227 = add i32 %123, %226
  %228 = add i32 %118, %227
  %229 = add i32 %113, %228
  %230 = add i32 %108, %229
  %231 = add i32 %103, %230
  %232 = add i32 %98, %231
  %233 = add i32 %93, %232
  %234 = add i32 %88, %233
  %235 = add i32 %83, %234
  %236 = add i32 %78, %235
  %237 = add i32 %73, %236
  %238 = add i32 %68, %237
  %239 = add i32 %63, %238
  %240 = lshr i32 %239, 3
  %241 = load i32, i32* inttoptr (i32 16404 to i32*), align 4
  %242 = load i32, i32* inttoptr (i32 16424 to i32*), align 8
  %243 = sub i32 %241, %242
  %244 = lshr i32 %243, 1
  %245 = icmp ult i32 %240, %244
  br i1 %245, label %blk_exit15, label %blk_exit16
}

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @unlzh(i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @fillbuf(i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @read_pt_len(i32, i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @make_table(i32, i32, i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @unlzw(i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @unpack(i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @check_zipfile(i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @unzip(i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @copy(i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @write_error() #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @read_error() #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @write_buf(i32, i32, i32) #0

; Function Attrs: null_pointer_is_valid
define internal i32 @read_buffer(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -16
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = icmp ult i32 %_arg_2, 2147483647
  %3 = select i1 %2, i32 %_arg_2, i32 2147483647
  %4 = call i32 @read(i32 %_arg_0, i32 %_arg_1, i32 %3)
  %brif_val.not = icmp slt i32 %4, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit

blk_exit:                                         ; preds = %brif_next11, %brif_next7, %blk_exit5, %brif_next2, %brif_next, %allocator
  %_param_2.0 = phi i32 [ %4, %brif_next7 ], [ %4, %blk_exit5 ], [ %14, %brif_next11 ], [ %4, %brif_next2 ], [ %4, %brif_next ], [ %4, %allocator ]
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %_param_2.0

brif_next:                                        ; preds = %allocator
  %5 = call i32 @__errno_location()
  %6 = inttoptr i32 %5 to i32*
  %7 = load i32, i32* %6, align 4
  %.not = icmp eq i32 %7, 11
  br i1 %.not, label %brif_next2, label %blk_exit

brif_next2:                                       ; preds = %brif_next
  %8 = call i32 @rpl_fcntl(i32 %_arg_0, i32 3, i32 0)
  %brif_val3.not = icmp sgt i32 %8, -1
  br i1 %brif_val3.not, label %brif_next4, label %blk_exit

brif_next4:                                       ; preds = %brif_next2
  %9 = and i32 %8, 2048
  %brif_val6.not = icmp eq i32 %9, 0
  br i1 %brif_val6.not, label %brif_next7, label %blk_exit5

blk_exit5:                                        ; preds = %brif_next4
  %10 = and i32 %8, -2049
  %11 = inttoptr i32 %1 to i32*
  store i32 %10, i32* %11, align 4
  %12 = call i32 @rpl_fcntl(i32 %_arg_0, i32 4, i32 %1)
  %13 = icmp eq i32 %12, -1
  br i1 %13, label %blk_exit, label %brif_next11

brif_next7:                                       ; preds = %brif_next4
  store i32 11, i32* %6, align 4
  br label %blk_exit

brif_next11:                                      ; preds = %blk_exit5
  %14 = call i32 @read(i32 %_arg_0, i32 %_arg_1, i32 %3)
  br label %blk_exit
}

; Function Attrs: null_pointer_is_valid
define internal i32 @updcrc(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %0 = icmp eq i32 %_arg_0, 0
  br i1 %0, label %blk_exit, label %brif_next

blk_exit:                                         ; preds = %brif_next, %allocator
  %_local_2.0 = phi i32 [ 0, %allocator ], [ %2, %brif_next ]
  store i32 %_local_2.0, i32* inttoptr (i32 831756 to i32*), align 4
  ret i32 %_local_2.0

brif_next:                                        ; preds = %allocator
  %1 = load i32, i32* inttoptr (i32 831756 to i32*), align 4
  %2 = call i32 @crc32_update(i32 %1, i32 %_arg_0, i32 %_arg_1)
  br label %blk_exit
}

; Function Attrs: null_pointer_is_valid
define internal i32 @getcrc() #0 {
allocator:
  %0 = load i32, i32* inttoptr (i32 831756 to i32*), align 4
  ret i32 %0
}

; Function Attrs: null_pointer_is_valid
define internal void @clear_bufs() #0 {
allocator:
  store i64 0, i64* inttoptr (i32 815144 to i64*), align 8
  store i64 0, i64* inttoptr (i32 815136 to i64*), align 32
  store i32 0, i32* inttoptr (i32 816188 to i32*), align 4
  store i32 0, i32* inttoptr (i32 816192 to i32*), align 64
  store i32 0, i32* inttoptr (i32 816184 to i32*), align 8
  ret void
}

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @fill_inbuf(i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @flush_window() #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @flush_outbuf() #0

; Function Attrs: null_pointer_is_valid
define internal i32 @strlwr(i32 %_arg_0) #0 {
allocator:
  %0 = inttoptr i32 %_arg_0 to i8*
  %1 = load i8, i8* %0, align 1
  %2 = icmp eq i8 %1, 0
  br i1 %2, label %blk_exit, label %loop_entry

blk_exit:                                         ; preds = %blk_exit1, %allocator
  ret i32 %_arg_0

loop_entry:                                       ; preds = %blk_exit1, %allocator
  %_local_1.0.in = phi i8 [ %9, %blk_exit1 ], [ %1, %allocator ]
  %_local_2.0 = phi i32 [ %7, %blk_exit1 ], [ %_arg_0, %allocator ]
  %_local_1.0 = zext i8 %_local_1.0.in to i32
  %3 = add nsw i32 %_local_1.0, -91
  %4 = icmp ult i32 %3, -26
  br i1 %4, label %blk_exit1, label %brif_next3

blk_exit1:                                        ; preds = %brif_next3, %loop_entry
  %_local_1.1 = phi i32 [ %_local_1.0, %loop_entry ], [ %10, %brif_next3 ]
  %5 = inttoptr i32 %_local_2.0 to i8*
  %6 = trunc i32 %_local_1.1 to i8
  store i8 %6, i8* %5, align 1
  %7 = add i32 %_local_2.0, 1
  %8 = inttoptr i32 %7 to i8*
  %9 = load i8, i8* %8, align 1
  %brif_val6.not = icmp eq i8 %9, 0
  br i1 %brif_val6.not, label %blk_exit, label %loop_entry

brif_next3:                                       ; preds = %loop_entry
  %10 = call i32 @tolower(i32 %_local_1.0)
  br label %blk_exit1
}

; Function Attrs: null_pointer_is_valid
define internal i32 @gzip_base_name(i32 %_arg_0) #0 {
allocator:
  %0 = call i32 @last_component(i32 %_arg_0)
  ret i32 %0
}

; Function Attrs: null_pointer_is_valid
define internal i32 @xunlink(i32 %_arg_0) #0 {
allocator:
  %0 = call i32 @rpl_unlink(i32 %_arg_0)
  ret i32 %0
}

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @add_envopt(i32, i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @gzip_error(i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @xalloc_die() #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @warning(i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local void @display_ratio(i64, i64, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @zip(i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @file_read(i32, i32) #0

; Function Attrs: null_pointer_is_valid
define internal i32 @last_component(i32 %_arg_0) #0 {
allocator:
  %0 = add i32 %_arg_0, -1
  br label %loop_entry

loop_entry:                                       ; preds = %loop_entry, %allocator
  %_local_1.0 = phi i32 [ %0, %allocator ], [ %1, %loop_entry ]
  %1 = add i32 %_local_1.0, 1
  %2 = inttoptr i32 %1 to i8*
  %3 = load i8, i8* %2, align 1
  %4 = icmp eq i8 %3, 47
  br i1 %4, label %loop_entry, label %loop_entry1

blk_exit:                                         ; preds = %loop_entry1
  ret i32 %_local_1.1

loop_entry1:                                      ; preds = %blk_exit2, %loop_entry
  %_local_3.0 = phi i32 [ %5, %blk_exit2 ], [ %1, %loop_entry ]
  %_local_2.0 = phi i32 [ %storemerge, %blk_exit2 ], [ 0, %loop_entry ]
  %_local_1.1 = phi i32 [ %_local_1.2, %blk_exit2 ], [ %1, %loop_entry ]
  %_param_0.0.in = phi i8 [ %7, %blk_exit2 ], [ %3, %loop_entry ]
  switch i8 %_param_0.0.in, label %brif_next7 [
    i8 47, label %blk_exit2
    i8 0, label %blk_exit
  ]

blk_exit2:                                        ; preds = %brif_next7, %loop_entry1
  %_local_1.2 = phi i32 [ %_local_1.1, %loop_entry1 ], [ %9, %brif_next7 ]
  %storemerge = phi i32 [ 1, %loop_entry1 ], [ 0, %brif_next7 ]
  %5 = add i32 %_local_3.0, 1
  %6 = inttoptr i32 %5 to i8*
  %7 = load i8, i8* %6, align 1
  br label %loop_entry1

brif_next7:                                       ; preds = %loop_entry1
  %8 = and i32 %_local_2.0, 1
  %select_cond.not = icmp eq i32 %8, 0
  %9 = select i1 %select_cond.not, i32 %_local_1.1, i32 %_local_3.0
  br label %blk_exit2
}

; Function Attrs: null_pointer_is_valid
define internal i32 @crc32_update_no_xor(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -16
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = and i32 %_arg_2, -8
  %3 = icmp eq i32 %2, 0
  br i1 %3, label %blk_exit, label %loop_entry

blk_exit:                                         ; preds = %loop_entry, %allocator
  %_local_4.0 = phi i32 [ 0, %allocator ], [ %55, %loop_entry ]
  %_param_0.0 = phi i32 [ %_arg_0, %allocator ], [ %54, %loop_entry ]
  %4 = icmp ult i32 %_local_4.0, %_arg_2
  br i1 %4, label %blk_exit12, label %blk_exit11

loop_entry:                                       ; preds = %loop_entry, %allocator
  %_local_4.1 = phi i32 [ 0, %allocator ], [ %55, %loop_entry ]
  %_param_0.1 = phi i32 [ %_arg_0, %allocator ], [ %54, %loop_entry ]
  %5 = add i32 %0, -8
  %6 = add i32 %_local_4.1, %_arg_1
  %7 = call i32 @memcpy(i32 %5, i32 %6, i32 8)
  %8 = inttoptr i32 %5 to i64*
  %9 = load i64, i64* %8, align 8
  %sh.diff = lshr i64 %9, 46
  %tr.sh.diff = trunc i64 %sh.diff to i32
  %10 = and i32 %tr.sh.diff, 1020
  %11 = add nuw nsw i32 %10, 7632
  %12 = inttoptr i32 %11 to i32*
  %13 = load i32, i32* %12, align 4
  %sh.diff30 = lshr i64 %9, 54
  %tr.sh.diff31 = trunc i64 %sh.diff30 to i32
  %14 = and i32 %tr.sh.diff31, 1020
  %15 = add nuw nsw i32 %14, 6608
  %16 = inttoptr i32 %15 to i32*
  %17 = load i32, i32* %16, align 4
  %18 = xor i32 %13, %17
  %sh.diff32 = lshr i64 %9, 38
  %tr.sh.diff33 = trunc i64 %sh.diff32 to i32
  %19 = and i32 %tr.sh.diff33, 1020
  %20 = add nuw nsw i32 %19, 8656
  %21 = inttoptr i32 %20 to i32*
  %22 = load i32, i32* %21, align 4
  %23 = xor i32 %18, %22
  %sh.diff34 = lshr i64 %9, 30
  %tr.sh.diff35 = trunc i64 %sh.diff34 to i32
  %24 = and i32 %tr.sh.diff35, 1020
  %25 = add nuw nsw i32 %24, 9680
  %26 = inttoptr i32 %25 to i32*
  %27 = load i32, i32* %26, align 4
  %28 = xor i32 %23, %27
  %29 = trunc i64 %9 to i32
  %30 = xor i32 %_param_0.1, %29
  %31 = lshr i32 %30, 22
  %32 = and i32 %31, 1020
  %33 = add nuw nsw i32 %32, 10704
  %34 = inttoptr i32 %33 to i32*
  %35 = load i32, i32* %34, align 4
  %36 = xor i32 %28, %35
  %37 = lshr i32 %30, 14
  %38 = and i32 %37, 1020
  %39 = add nuw nsw i32 %38, 11728
  %40 = inttoptr i32 %39 to i32*
  %41 = load i32, i32* %40, align 4
  %42 = xor i32 %36, %41
  %43 = lshr i32 %30, 6
  %44 = and i32 %43, 1020
  %45 = add nuw nsw i32 %44, 12752
  %46 = inttoptr i32 %45 to i32*
  %47 = load i32, i32* %46, align 4
  %48 = xor i32 %42, %47
  %49 = shl i32 %30, 2
  %50 = and i32 %49, 1020
  %51 = add nuw nsw i32 %50, 13776
  %52 = inttoptr i32 %51 to i32*
  %53 = load i32, i32* %52, align 4
  %54 = xor i32 %48, %53
  %55 = add i32 %_local_4.1, 8
  %56 = icmp ult i32 %55, %2
  br i1 %56, label %loop_entry, label %blk_exit

blk_exit11:                                       ; preds = %brif_next28, %blk_exit18, %blk_exit12, %blk_exit
  %_param_1.0 = phi i32 [ %64, %blk_exit12 ], [ %_param_1.1, %blk_exit18 ], [ %110, %brif_next28 ], [ %_param_0.0, %blk_exit ]
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %_param_1.0

blk_exit12:                                       ; preds = %blk_exit
  %57 = add i32 %0, -8
  %58 = add i32 %_local_4.0, %_arg_1
  %59 = sub i32 %_arg_2, %_local_4.0
  %60 = call i32 @memcpy(i32 %57, i32 %58, i32 %59)
  %61 = shl i32 %59, 3
  %62 = lshr i32 %_param_0.0, %61
  %63 = icmp ugt i32 %59, 3
  %64 = select i1 %63, i32 0, i32 %62
  %65 = icmp eq i32 %59, 0
  br i1 %65, label %blk_exit11, label %brif_next16

brif_next16:                                      ; preds = %blk_exit12
  %66 = inttoptr i32 %57 to i64*
  %67 = load i64, i64* %66, align 8
  %68 = zext i32 %_param_0.0 to i64
  %69 = xor i64 %67, %68
  %70 = zext i32 %59 to i64
  %71 = and i64 %70, 1
  %.not = icmp eq i32 %59, 1
  br i1 %.not, label %blk_exit18, label %blk_exit19

blk_exit18:                                       ; preds = %brif_next26, %brif_next16
  %_local_5.0 = phi i32 [ 0, %brif_next16 ], [ %phi.bo, %brif_next26 ]
  %_param_1.1 = phi i32 [ %64, %brif_next16 ], [ %94, %brif_next26 ]
  %storemerge = phi i32 [ -1, %brif_next16 ], [ %98, %brif_next26 ]
  %72 = icmp eq i64 %71, 0
  br i1 %72, label %blk_exit11, label %brif_next28

blk_exit19:                                       ; preds = %brif_next16
  %73 = and i64 %70, 4294967294
  br label %loop_entry22

loop_entry22:                                     ; preds = %loop_entry22, %blk_exit19
  %_local_8.0 = phi i64 [ %73, %blk_exit19 ], [ %97, %loop_entry22 ]
  %_local_5.1 = phi i32 [ 0, %blk_exit19 ], [ %96, %loop_entry22 ]
  %_local_4.2 = phi i32 [ %61, %blk_exit19 ], [ %75, %loop_entry22 ]
  %_param_1.2 = phi i32 [ %64, %blk_exit19 ], [ %94, %loop_entry22 ]
  %_param_0.2 = phi i32 [ 6608, %blk_exit19 ], [ %95, %loop_entry22 ]
  %74 = add i32 %_local_4.2, -8
  %75 = add i32 %_local_4.2, -16
  %76 = zext i32 %75 to i64
  %77 = lshr i64 %69, %76
  %78 = trunc i64 %77 to i32
  %79 = shl i32 %78, 2
  %80 = and i32 %79, 1020
  %81 = add i32 %_param_0.2, %80
  %82 = add i32 %81, 1024
  %83 = inttoptr i32 %82 to i32*
  %84 = load i32, i32* %83, align 4
  %85 = zext i32 %74 to i64
  %86 = lshr i64 %69, %85
  %87 = trunc i64 %86 to i32
  %88 = shl i32 %87, 2
  %89 = and i32 %88, 1020
  %90 = add i32 %_param_0.2, %89
  %91 = inttoptr i32 %90 to i32*
  %92 = load i32, i32* %91, align 4
  %93 = xor i32 %92, %_param_1.2
  %94 = xor i32 %84, %93
  %95 = add i32 %_param_0.2, 2048
  %96 = add i32 %_local_5.1, 2
  %97 = add i64 %_local_8.0, -2
  %.not36 = icmp eq i64 %97, 0
  br i1 %.not36, label %brif_next26, label %loop_entry22

brif_next26:                                      ; preds = %loop_entry22
  %98 = sub i32 -3, %_local_5.1
  %phi.bo = shl i32 %96, 10
  br label %blk_exit18

brif_next28:                                      ; preds = %blk_exit18
  %99 = add i32 %59, %storemerge
  %100 = shl i32 %99, 3
  %101 = zext i32 %100 to i64
  %102 = lshr i64 %69, %101
  %103 = trunc i64 %102 to i32
  %104 = shl i32 %103, 2
  %105 = and i32 %104, 1020
  %106 = or i32 %_local_5.0, %105
  %107 = add i32 %106, 6608
  %108 = inttoptr i32 %107 to i32*
  %109 = load i32, i32* %108, align 4
  %110 = xor i32 %109, %_param_1.1
  br label %blk_exit11
}

; Function Attrs: null_pointer_is_valid
define internal i32 @crc32_update(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2) #0 {
allocator:
  %0 = xor i32 %_arg_0, -1
  %1 = call i32 @crc32_update_no_xor(i32 %0, i32 %_arg_1, i32 %_arg_2)
  %2 = xor i32 %1, -1
  ret i32 %2
}

; Function Attrs: null_pointer_is_valid
define internal i32 @rpl_fclose(i32 %_arg_0) #0 {
allocator:
  %0 = call i32 @fileno(i32 %_arg_0)
  %brif_val.not = icmp slt i32 %0, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit

return:                                           ; preds = %brif_next12, %blk_exit2, %blk_exit1, %brif_next
  %return_0 = phi i32 [ %3, %brif_next ], [ %9, %blk_exit2 ], [ %7, %blk_exit1 ], [ -1, %brif_next12 ]
  ret i32 %return_0

blk_exit:                                         ; preds = %allocator
  %1 = call i32 @__freading(i32 %_arg_0)
  %2 = icmp eq i32 %1, 0
  br i1 %2, label %blk_exit3, label %brif_next5

brif_next:                                        ; preds = %allocator
  %3 = call i32 @fclose(i32 %_arg_0)
  br label %return

blk_exit1:                                        ; preds = %blk_exit3
  %4 = call i32 @__errno_location()
  %5 = inttoptr i32 %4 to i32*
  %6 = load i32, i32* %5, align 4
  %7 = call i32 @fclose(i32 %_arg_0)
  %8 = icmp eq i32 %6, 0
  br i1 %8, label %return, label %brif_next12

blk_exit2:                                        ; preds = %brif_next5, %blk_exit3
  %9 = call i32 @fclose(i32 %_arg_0)
  br label %return

blk_exit3:                                        ; preds = %brif_next5, %blk_exit
  %10 = call i32 @rpl_fflush(i32 %_arg_0)
  %brif_val8.not = icmp eq i32 %10, 0
  br i1 %brif_val8.not, label %blk_exit2, label %blk_exit1

brif_next5:                                       ; preds = %blk_exit
  %11 = call i32 @fileno(i32 %_arg_0)
  %12 = call i64 @lseek(i32 %11, i64 0, i32 1)
  %13 = icmp eq i64 %12, -1
  br i1 %13, label %blk_exit2, label %blk_exit3

brif_next12:                                      ; preds = %blk_exit1
  store i32 %6, i32* %5, align 4
  br label %return
}

; Function Attrs: null_pointer_is_valid
define internal i32 @rpl_fcntl(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -112
  store i32 %1, i32* @__stack_pointer, align 4
  %calcOffset = add i32 %0, -4
  %2 = inttoptr i32 %calcOffset to i32*
  store i32 %_arg_2, i32* %2, align 4
  %3 = add i32 %_arg_1, -1024
  %4 = icmp ugt i32 %3, 10
  br i1 %4, label %blk_exit5, label %brif_next

blk_exit:                                         ; preds = %brif_next37, %blk_exit35, %blk_exit21, %blk_exit20, %blk_exit15, %blk_exit4, %blk_exit3, %blk_exit2, %blk_exit1
  %_param_1.0 = phi i32 [ %16, %blk_exit2 ], [ %22, %blk_exit3 ], [ %23, %blk_exit4 ], [ %10, %blk_exit1 ], [ %_param_1.1, %blk_exit15 ], [ -1, %blk_exit35 ], [ %_param_1.1, %brif_next37 ], [ %39, %blk_exit20 ], [ %37, %blk_exit21 ]
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %_param_1.0

blk_exit1:                                        ; preds = %blk_exit5
  %calcOffset51 = add i32 %0, -4
  %5 = inttoptr i32 %calcOffset51 to i32*
  %6 = add i32 %_arg_2, 4
  store i32 %6, i32* %5, align 4
  %7 = inttoptr i32 %_arg_2 to i32*
  %8 = load i32, i32* %7, align 4
  %calcOffset54 = add i32 %0, -96
  %9 = inttoptr i32 %calcOffset54 to i32*
  store i32 %8, i32* %9, align 4
  %10 = call i32 @fcntl(i32 %_arg_0, i32 0, i32 %calcOffset54)
  br label %blk_exit

blk_exit2:                                        ; preds = %blk_exit5
  %calcOffset47 = add i32 %0, -4
  %11 = inttoptr i32 %calcOffset47 to i32*
  %12 = add i32 %_arg_2, 4
  store i32 %12, i32* %11, align 4
  %13 = inttoptr i32 %_arg_2 to i32*
  %14 = load i32, i32* %13, align 4
  %15 = inttoptr i32 %1 to i32*
  store i32 %14, i32* %15, align 4
  %16 = call i32 @fcntl(i32 %_arg_0, i32 %_arg_1, i32 %1)
  br label %blk_exit

blk_exit3:                                        ; preds = %brif_next, %blk_exit5, %blk_exit5
  %calcOffset43 = add i32 %0, -4
  %17 = inttoptr i32 %calcOffset43 to i32*
  %18 = add i32 %_arg_2, 4
  store i32 %18, i32* %17, align 4
  %19 = inttoptr i32 %_arg_2 to i32*
  %20 = load i32, i32* %19, align 4
  %calcOffset46 = add i32 %0, -16
  %21 = inttoptr i32 %calcOffset46 to i32*
  store i32 %20, i32* %21, align 4
  %22 = call i32 @fcntl(i32 %_arg_0, i32 %_arg_1, i32 %calcOffset46)
  br label %blk_exit

blk_exit4:                                        ; preds = %brif_next7, %blk_exit5, %blk_exit5
  %23 = call i32 @fcntl(i32 %_arg_0, i32 %_arg_1, i32 0)
  br label %blk_exit

blk_exit5:                                        ; preds = %brif_next9, %allocator
  switch i32 %_arg_1, label %blk_exit2 [
    i32 0, label %blk_exit1
    i32 1, label %blk_exit4
    i32 2, label %blk_exit3
    i32 3, label %blk_exit4
    i32 4, label %blk_exit3
  ]

brif_next:                                        ; preds = %allocator
  %24 = shl i32 1, %3
  %25 = and i32 %24, 645
  %brif_val6.not = icmp eq i32 %25, 0
  br i1 %brif_val6.not, label %brif_next7, label %blk_exit3

brif_next7:                                       ; preds = %brif_next
  %26 = and i32 %24, 1282
  %brif_val8.not = icmp eq i32 %26, 0
  br i1 %brif_val8.not, label %brif_next9, label %blk_exit4

brif_next9:                                       ; preds = %brif_next7
  %.not = icmp eq i32 %3, 6
  br i1 %.not, label %brif_next11, label %blk_exit5

brif_next11:                                      ; preds = %brif_next9
  %calcOffset12 = add i32 %0, -4
  %27 = inttoptr i32 %calcOffset12 to i32*
  %28 = add i32 %_arg_2, 4
  store i32 %28, i32* %27, align 4
  %29 = inttoptr i32 %_arg_2 to i32*
  %30 = load i32, i32* %29, align 4
  %31 = load i32, i32* inttoptr (i32 831768 to i32*), align 8
  %brif_val17.not = icmp sgt i32 %31, -1
  br i1 %brif_val17.not, label %brif_next18, label %blk_exit16

blk_exit15:                                       ; preds = %brif_next29, %blk_exit16
  %_param_1.1 = phi i32 [ %39, %brif_next29 ], [ %33, %blk_exit16 ]
  %storemerge = phi i1 [ false, %brif_next29 ], [ %35, %blk_exit16 ]
  %brif_val31.not = icmp slt i32 %_param_1.1, 0
  %or.cond = select i1 %brif_val31.not, i1 true, i1 %storemerge
  br i1 %or.cond, label %blk_exit, label %brif_next34

blk_exit16:                                       ; preds = %brif_next11
  %calcOffset30 = add i32 %0, -32
  %32 = inttoptr i32 %calcOffset30 to i32*
  store i32 %30, i32* %32, align 4
  %33 = call i32 @fcntl(i32 %_arg_0, i32 0, i32 %calcOffset30)
  %34 = load i32, i32* inttoptr (i32 831768 to i32*), align 8
  %35 = icmp ne i32 %34, -1
  br label %blk_exit15

brif_next18:                                      ; preds = %brif_next11
  %calcOffset19 = add i32 %0, -48
  %36 = inttoptr i32 %calcOffset19 to i32*
  store i32 %30, i32* %36, align 4
  %37 = call i32 @fcntl(i32 %_arg_0, i32 1030, i32 %calcOffset19)
  %brif_val22.not = icmp slt i32 %37, 0
  br i1 %brif_val22.not, label %brif_next23, label %blk_exit21

blk_exit20:                                       ; preds = %brif_next23
  %calcOffset27 = add i32 %0, -64
  %38 = inttoptr i32 %calcOffset27 to i32*
  store i32 %30, i32* %38, align 4
  %39 = call i32 @fcntl(i32 %_arg_0, i32 0, i32 %calcOffset27)
  %brif_val28.not = icmp sgt i32 %39, -1
  br i1 %brif_val28.not, label %brif_next29, label %blk_exit

blk_exit21:                                       ; preds = %brif_next23, %brif_next18
  store i32 1, i32* inttoptr (i32 831768 to i32*), align 8
  br label %blk_exit

brif_next23:                                      ; preds = %brif_next18
  %40 = call i32 @__errno_location()
  %41 = inttoptr i32 %40 to i32*
  %42 = load i32, i32* %41, align 4
  %43 = icmp eq i32 %42, 22
  br i1 %43, label %blk_exit20, label %blk_exit21

brif_next29:                                      ; preds = %blk_exit20
  store i32 -1, i32* inttoptr (i32 831768 to i32*), align 8
  br label %blk_exit15

brif_next34:                                      ; preds = %blk_exit15
  %44 = call i32 @fcntl(i32 %_param_1.1, i32 1, i32 0)
  %brif_val36.not = icmp sgt i32 %44, -1
  br i1 %brif_val36.not, label %brif_next37, label %blk_exit35

blk_exit35:                                       ; preds = %brif_next37, %brif_next34
  %45 = call i32 @__errno_location()
  %46 = inttoptr i32 %45 to i32*
  %47 = load i32, i32* %46, align 4
  %48 = call i32 @close(i32 %_param_1.1)
  store i32 %47, i32* %46, align 4
  br label %blk_exit

brif_next37:                                      ; preds = %brif_next34
  %49 = or i32 %44, 1
  %calcOffset38 = add i32 %0, -80
  %50 = inttoptr i32 %calcOffset38 to i32*
  store i32 %49, i32* %50, align 4
  %51 = call i32 @fcntl(i32 %_param_1.1, i32 2, i32 %calcOffset38)
  %.not55 = icmp eq i32 %51, -1
  br i1 %.not55, label %blk_exit35, label %blk_exit
}

; Function Attrs: null_pointer_is_valid
define internal i32 @open_safer(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -16
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = and i32 %_arg_1, 64
  %brif_val.not.not = icmp eq i32 %2, 0
  br i1 %brif_val.not.not, label %blk_exit, label %brif_next

blk_exit:                                         ; preds = %brif_next, %allocator
  %_local_4.0 = phi i32 [ 0, %allocator ], [ %9, %brif_next ]
  %3 = inttoptr i32 %1 to i32*
  store i32 %_local_4.0, i32* %3, align 4
  %4 = call i32 @open(i32 %_arg_0, i32 %_arg_1, i32 %1)
  %5 = call i32 @fd_safer(i32 %4)
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %5

brif_next:                                        ; preds = %allocator
  %6 = add i32 %_arg_2, 4
  %calcOffset = add i32 %0, -4
  %7 = inttoptr i32 %calcOffset to i32*
  store i32 %6, i32* %7, align 4
  %8 = inttoptr i32 %_arg_2 to i32*
  %9 = load i32, i32* %8, align 4
  br label %blk_exit
}

; Function Attrs: null_pointer_is_valid
define internal i32 @rpl_fdopendir(i32 %_arg_0) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -144
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = call i32 @fstat(i32 %_arg_0, i32 %1)
  %brif_val.not = icmp eq i32 %2, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit

blk_exit:                                         ; preds = %brif_next3, %blk_exit1, %allocator
  %_local_2.0 = phi i32 [ %7, %blk_exit1 ], [ 0, %brif_next3 ], [ 0, %allocator ]
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %_local_2.0

brif_next:                                        ; preds = %allocator
  %calcOffset = add i32 %0, -120
  %3 = inttoptr i32 %calcOffset to i32*
  %4 = load i32, i32* %3, align 4
  %5 = and i32 %4, 61440
  %6 = icmp eq i32 %5, 16384
  br i1 %6, label %blk_exit1, label %brif_next3

blk_exit1:                                        ; preds = %brif_next
  %7 = call i32 @fdopendir(i32 %_arg_0)
  br label %blk_exit

brif_next3:                                       ; preds = %brif_next
  %8 = call i32 @__errno_location()
  %9 = inttoptr i32 %8 to i32*
  store i32 20, i32* %9, align 4
  br label %blk_exit
}

; Function Attrs: null_pointer_is_valid
define internal i32 @rpl_fflush(i32 %_arg_0) #0 {
allocator:
  %0 = icmp eq i32 %_arg_0, 0
  br i1 %0, label %blk_exit1, label %brif_next

return:                                           ; preds = %brif_next9, %brif_next6, %blk_exit4, %blk_exit1
  %return_0 = phi i32 [ %2, %blk_exit1 ], [ -1, %brif_next6 ], [ %11, %brif_next9 ], [ %5, %blk_exit4 ]
  ret i32 %return_0

blk_exit:                                         ; preds = %brif_next
  %1 = call i64 @ftello(i32 %_arg_0)
  %.not = icmp eq i64 %1, -1
  br i1 %.not, label %brif_next6, label %blk_exit4

blk_exit1:                                        ; preds = %brif_next, %allocator
  %2 = call i32 @fflush(i32 %_arg_0)
  br label %return

brif_next:                                        ; preds = %allocator
  %3 = call i32 @__freading(i32 %_arg_0)
  %brif_val2.not = icmp eq i32 %3, 0
  br i1 %brif_val2.not, label %blk_exit1, label %blk_exit

blk_exit4:                                        ; preds = %blk_exit
  %4 = call i32 @rpl_fseeko(i32 %_arg_0, i64 0, i32 1)
  %5 = call i32 @rpl_fpurge(i32 %_arg_0)
  %brif_val8.not = icmp eq i32 %5, 0
  br i1 %brif_val8.not, label %brif_next9, label %return

brif_next6:                                       ; preds = %blk_exit
  %6 = call i32 @__errno_location()
  %7 = inttoptr i32 %6 to i32*
  store i32 9, i32* %7, align 4
  br label %return

brif_next9:                                       ; preds = %blk_exit4
  %8 = call i32 @fileno(i32 %_arg_0)
  %9 = call i64 @lseek(i32 %8, i64 %1, i32 0)
  %10 = icmp eq i64 %9, -1
  %11 = sext i1 %10 to i32
  br label %return
}

; Function Attrs: null_pointer_is_valid
define internal i32 @rpl_fpurge(i32 %_arg_0) #0 {
allocator:
  %0 = call i32 @__fpurge(i32 %_arg_0)
  ret i32 0
}

; Function Attrs: null_pointer_is_valid
define internal void @rpl_free(i32 %_arg_0) #0 {
allocator:
  %0 = call i32 @__errno_location()
  %1 = inttoptr i32 %0 to i32*
  %2 = load i32, i32* %1, align 4
  call void @free(i32 %_arg_0)
  store i32 %2, i32* %1, align 4
  ret void
}

; Function Attrs: null_pointer_is_valid
define internal i32 @rpl_fseeko(i32 %_arg_0, i64 %_arg_1, i32 %_arg_2) #0 {
allocator:
  %0 = call i32 @fseeko(i32 %_arg_0, i64 %_arg_1, i32 %_arg_2)
  ret i32 %0
}

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @_getopt_internal_r(i32, i32, i32, i32, i32, i32, i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @process_long_option(i32, i32, i32, i32, i32, i32, i32, i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @rpl_getopt_internal(i32, i32, i32, i32, i32, i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @rpl_getopt_long(i32, i32, i32, i32, i32) #0

; Function Attrs: null_pointer_is_valid
define internal i32 @rpl_lstat(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %0 = call i32 @lstat(i32 %_arg_0, i32 %_arg_1)
  %brif_val.not = icmp eq i32 %0, 0
  br i1 %brif_val.not, label %brif_next, label %return

return:                                           ; preds = %brif_next9, %blk_exit6, %brif_next2, %brif_next, %allocator
  %return_0 = phi i32 [ -1, %brif_next9 ], [ 0, %brif_next ], [ %13, %blk_exit6 ], [ 0, %brif_next2 ], [ %0, %allocator ]
  ret i32 %return_0

brif_next:                                        ; preds = %allocator
  %calcOffset = add i32 %_arg_1, 24
  %1 = inttoptr i32 %calcOffset to i32*
  %2 = load i32, i32* %1, align 4
  %3 = and i32 %2, 61440
  %4 = icmp eq i32 %3, 16384
  br i1 %4, label %return, label %brif_next2

brif_next2:                                       ; preds = %brif_next
  %5 = call i32 @strlen(i32 %_arg_0)
  %6 = add i32 %5, %_arg_0
  %7 = add i32 %6, -1
  %8 = inttoptr i32 %7 to i8*
  %9 = load i8, i8* %8, align 1
  %.not = icmp eq i8 %9, 47
  br i1 %.not, label %brif_next5, label %return

brif_next5:                                       ; preds = %brif_next2
  %10 = load i32, i32* %1, align 4
  %11 = and i32 %10, 61440
  %12 = icmp eq i32 %11, 40960
  br i1 %12, label %blk_exit6, label %brif_next9

blk_exit6:                                        ; preds = %brif_next5
  %13 = call i32 @rpl_stat(i32 %_arg_0, i32 %_arg_1)
  br label %return

brif_next9:                                       ; preds = %brif_next5
  %14 = call i32 @__errno_location()
  %15 = inttoptr i32 %14 to i32*
  store i32 20, i32* %15, align 4
  br label %return
}

; Function Attrs: null_pointer_is_valid
define internal i32 @set_cloexec_flag(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -32
  store i32 %1, i32* @__stack_pointer, align 4
  %calcOffset = add i32 %0, -16
  %2 = inttoptr i32 %calcOffset to i32*
  store i32 0, i32* %2, align 4
  %3 = add i32 %0, -16
  %4 = call i32 @rpl_fcntl(i32 %_arg_0, i32 1, i32 %3)
  %brif_val.not = icmp slt i32 %4, 0
  br i1 %brif_val.not, label %blk_exit, label %blk_exit1

blk_exit:                                         ; preds = %brif_next3, %blk_exit1, %allocator
  %_local_3.0 = phi i32 [ 0, %blk_exit1 ], [ %11, %brif_next3 ], [ -1, %allocator ]
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %_local_3.0

blk_exit1:                                        ; preds = %allocator
  %5 = and i32 %4, -2
  %6 = or i32 %5, %_arg_1
  %7 = icmp eq i32 %4, %6
  br i1 %7, label %blk_exit, label %brif_next3

brif_next3:                                       ; preds = %blk_exit1
  %8 = inttoptr i32 %1 to i32*
  store i32 %6, i32* %8, align 4
  %9 = call i32 @rpl_fcntl(i32 %_arg_0, i32 2, i32 %1)
  %10 = icmp eq i32 %9, -1
  %11 = sext i1 %10 to i32
  br label %blk_exit
}

; Function Attrs: null_pointer_is_valid
define internal i32 @rpl_openat(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2, i32 %_arg_3) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -32
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = and i32 %_arg_2, 64
  %brif_val.not.not = icmp eq i32 %2, 0
  br i1 %brif_val.not.not, label %blk_exit, label %brif_next

blk_exit:                                         ; preds = %brif_next, %allocator
  %_local_5.0 = phi i32 [ 0, %allocator ], [ %14, %brif_next ]
  %calcOffset2 = add i32 %0, -16
  %3 = inttoptr i32 %calcOffset2 to i32*
  store i32 %_local_5.0, i32* %3, align 4
  %4 = load i32, i32* inttoptr (i32 831812 to i32*), align 4
  %5 = lshr i32 %4, 12
  %6 = xor i32 %5, -1
  %7 = or i32 %6, -524289
  %8 = and i32 %7, %_arg_2
  %9 = call i32 @openat(i32 %_arg_0, i32 %_arg_1, i32 %8, i32 %calcOffset2)
  %10 = and i32 %_arg_2, 524288
  %brif_val4.not.not = icmp eq i32 %10, 0
  br i1 %brif_val4.not.not, label %blk_exit3, label %brif_next5

brif_next:                                        ; preds = %allocator
  %11 = add i32 %_arg_3, 4
  %calcOffset = add i32 %0, -4
  %12 = inttoptr i32 %calcOffset to i32*
  store i32 %11, i32* %12, align 4
  %13 = inttoptr i32 %_arg_3 to i32*
  %14 = load i32, i32* %13, align 4
  br label %blk_exit

blk_exit3:                                        ; preds = %brif_next19, %brif_next11, %blk_exit9, %blk_exit6, %blk_exit
  %_param_3.0 = phi i32 [ %9, %blk_exit ], [ %9, %brif_next11 ], [ %_param_3.1, %brif_next19 ], [ %_param_3.1, %blk_exit6 ], [ %9, %blk_exit9 ]
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %_param_3.0

brif_next5:                                       ; preds = %blk_exit
  %15 = load i32, i32* inttoptr (i32 831812 to i32*), align 4
  %brif_val7.not = icmp eq i32 %15, 0
  br i1 %brif_val7.not, label %brif_next8, label %blk_exit6

blk_exit6:                                        ; preds = %brif_next14, %brif_next5
  %_param_3.1 = phi i32 [ %21, %brif_next14 ], [ %9, %brif_next5 ]
  %_local_6.0 = phi i32 [ -1, %brif_next14 ], [ %15, %brif_next5 ]
  %brif_val16.not = icmp slt i32 %_local_6.0, 0
  %brif_val18.not = icmp sgt i32 %_param_3.1, -1
  %or.cond = select i1 %brif_val16.not, i1 %brif_val18.not, i1 false
  br i1 %or.cond, label %brif_next19, label %blk_exit3

brif_next8:                                       ; preds = %brif_next5
  %brif_val10.not = icmp sgt i32 %9, -1
  br i1 %brif_val10.not, label %brif_next11, label %blk_exit9

blk_exit9:                                        ; preds = %brif_next8
  %16 = call i32 @__errno_location()
  %17 = inttoptr i32 %16 to i32*
  %18 = load i32, i32* %17, align 4
  %.not = icmp eq i32 %18, 22
  br i1 %.not, label %brif_next14, label %blk_exit3

brif_next11:                                      ; preds = %brif_next8
  store i32 1, i32* inttoptr (i32 831812 to i32*), align 4
  br label %blk_exit3

brif_next14:                                      ; preds = %blk_exit9
  %19 = inttoptr i32 %1 to i32*
  store i32 %_local_5.0, i32* %19, align 4
  %20 = and i32 %_arg_2, -524289
  %21 = call i32 @openat(i32 %_arg_0, i32 %_arg_1, i32 %20, i32 %1)
  store i32 -1, i32* inttoptr (i32 831812 to i32*), align 4
  br label %blk_exit6

brif_next19:                                      ; preds = %blk_exit6
  %22 = call i32 @set_cloexec_flag(i32 %_param_3.1, i32 1)
  br label %blk_exit3
}

; Function Attrs: null_pointer_is_valid
define internal i32 @openat_safer(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2, i32 %_arg_3) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -16
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = and i32 %_arg_2, 64
  %brif_val.not.not = icmp eq i32 %2, 0
  br i1 %brif_val.not.not, label %blk_exit, label %brif_next

blk_exit:                                         ; preds = %brif_next, %allocator
  %_local_5.0 = phi i32 [ 0, %allocator ], [ %9, %brif_next ]
  %3 = inttoptr i32 %1 to i32*
  store i32 %_local_5.0, i32* %3, align 4
  %4 = call i32 @rpl_openat(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2, i32 %1)
  %5 = call i32 @fd_safer(i32 %4)
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %5

brif_next:                                        ; preds = %allocator
  %6 = add i32 %_arg_3, 4
  %calcOffset = add i32 %0, -4
  %7 = inttoptr i32 %calcOffset to i32*
  store i32 %6, i32* %7, align 4
  %8 = inttoptr i32 %_arg_3 to i32*
  %9 = load i32, i32* %8, align 4
  br label %blk_exit
}

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @streamsavedir(i32, i32) #0

; Function Attrs: null_pointer_is_valid
define internal i32 @direntry_cmp_name(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2) #0 {
allocator:
  %0 = inttoptr i32 %_arg_0 to i32*
  %1 = load i32, i32* %0, align 4
  %2 = add i32 %1, %_arg_2
  %3 = inttoptr i32 %_arg_1 to i32*
  %4 = load i32, i32* %3, align 4
  %5 = add i32 %4, %_arg_2
  %6 = call i32 @strcmp(i32 %2, i32 %5)
  ret i32 %6
}

; Function Attrs: null_pointer_is_valid
define internal i32 @rpl_stat(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %0 = call i32 @stat(i32 %_arg_0, i32 %_arg_1)
  %brif_val.not = icmp eq i32 %0, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit

blk_exit:                                         ; preds = %brif_next5, %brif_next2, %brif_next, %allocator
  %_local_2.0 = phi i32 [ 0, %brif_next ], [ -1, %brif_next5 ], [ 0, %brif_next2 ], [ %0, %allocator ]
  ret i32 %_local_2.0

brif_next:                                        ; preds = %allocator
  %calcOffset = add i32 %_arg_1, 24
  %1 = inttoptr i32 %calcOffset to i32*
  %2 = load i32, i32* %1, align 4
  %3 = and i32 %2, 61440
  %4 = icmp eq i32 %3, 16384
  br i1 %4, label %blk_exit, label %brif_next2

brif_next2:                                       ; preds = %brif_next
  %5 = call i32 @strlen(i32 %_arg_0)
  %6 = add i32 %5, %_arg_0
  %7 = add i32 %6, -1
  %8 = inttoptr i32 %7 to i8*
  %9 = load i8, i8* %8, align 1
  %.not = icmp eq i8 %9, 47
  br i1 %.not, label %brif_next5, label %blk_exit

brif_next5:                                       ; preds = %brif_next2
  %10 = call i32 @__errno_location()
  %11 = inttoptr i32 %10 to i32*
  store i32 20, i32* %11, align 4
  br label %blk_exit
}

; Function Attrs: null_pointer_is_valid
define internal i32 @rpl_strerror(i32 %_arg_0) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -16
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = call i32 @strerror_override(i32 %_arg_0)
  %brif_val.not = icmp eq i32 %2, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit1

blk_exit:                                         ; preds = %blk_exit2
  call void @abort()
  unreachable

blk_exit1:                                        ; preds = %brif_next11, %blk_exit3, %allocator
  %_local_2.0 = phi i32 [ 831824, %blk_exit3 ], [ 831824, %brif_next11 ], [ %2, %allocator ]
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %_local_2.0

brif_next:                                        ; preds = %allocator
  %3 = call i32 @strerror(i32 %_arg_0)
  %4 = icmp eq i32 %3, 0
  br i1 %4, label %blk_exit3, label %brif_next5

blk_exit2:                                        ; preds = %brif_next5
  %5 = call i32 @strlen(i32 %3)
  %6 = icmp ugt i32 %5, 255
  br i1 %6, label %blk_exit, label %brif_next11

blk_exit3:                                        ; preds = %brif_next5, %brif_next
  %7 = inttoptr i32 %1 to i32*
  store i32 %_arg_0, i32* %7, align 4
  %8 = call i32 @sprintf(i32 831824, i32 14816, i32 %1)
  %9 = call i32 @__errno_location()
  %10 = inttoptr i32 %9 to i32*
  store i32 22, i32* %10, align 4
  br label %blk_exit1

brif_next5:                                       ; preds = %brif_next
  %11 = inttoptr i32 %3 to i8*
  %12 = load i8, i8* %11, align 1
  %brif_val6.not = icmp eq i8 %12, 0
  br i1 %brif_val6.not, label %blk_exit3, label %blk_exit2

brif_next11:                                      ; preds = %blk_exit2
  %13 = add i32 %5, 1
  %14 = call i32 @memcpy(i32 831824, i32 %3, i32 %13)
  br label %blk_exit1
}

; Function Attrs: null_pointer_is_valid
define internal i32 @strerror_override(i32 %_arg_0) #0 {
allocator:
  %select_cond.not = icmp eq i32 %_arg_0, 0
  %0 = select i1 %select_cond.not, i32 1370, i32 0
  ret i32 %0
}

; Function Attrs: null_pointer_is_valid
define internal i32 @dup_safer(i32 %_arg_0) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -16
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = inttoptr i32 %1 to i32*
  store i32 3, i32* %2, align 4
  %3 = call i32 @rpl_fcntl(i32 %_arg_0, i32 0, i32 %1)
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %3
}

; Function Attrs: null_pointer_is_valid
define internal i32 @fd_safer(i32 %_arg_0) #0 {
allocator:
  %0 = icmp ult i32 %_arg_0, 3
  br i1 %0, label %blk_exit, label %return

return:                                           ; preds = %blk_exit, %allocator
  %return_0 = phi i32 [ %1, %blk_exit ], [ %_arg_0, %allocator ]
  ret i32 %return_0

blk_exit:                                         ; preds = %allocator
  %1 = call i32 @dup_safer(i32 %_arg_0)
  %2 = call i32 @__errno_location()
  %3 = inttoptr i32 %2 to i32*
  %4 = load i32, i32* %3, align 4
  %5 = call i32 @close(i32 %_arg_0)
  store i32 %4, i32* %3, align 4
  br label %return
}

; Function Attrs: null_pointer_is_valid
define internal i32 @rpl_unlink(i32 %_arg_0) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -144
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = call i32 @strlen(i32 %_arg_0)
  %3 = icmp eq i32 %2, 0
  br i1 %3, label %blk_exit1, label %brif_next

blk_exit:                                         ; preds = %blk_exit21, %brif_next6, %blk_exit4, %blk_exit1
  %_local_3.0 = phi i32 [ %4, %blk_exit1 ], [ %9, %brif_next6 ], [ -1, %blk_exit21 ], [ -1, %blk_exit4 ]
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %_local_3.0

blk_exit1:                                        ; preds = %blk_exit10, %brif_next, %allocator
  %4 = call i32 @unlink(i32 %_arg_0)
  br label %blk_exit

brif_next:                                        ; preds = %allocator
  %5 = add i32 %2, %_arg_0
  %6 = add i32 %5, -1
  %7 = inttoptr i32 %6 to i8*
  %8 = load i8, i8* %7, align 1
  %.not = icmp eq i8 %8, 47
  br i1 %.not, label %brif_next3, label %blk_exit1

brif_next3:                                       ; preds = %brif_next
  %9 = call i32 @rpl_lstat(i32 %_arg_0, i32 %1)
  %10 = icmp eq i32 %9, 0
  br i1 %10, label %blk_exit4, label %brif_next6

blk_exit4:                                        ; preds = %brif_next6, %brif_next3
  %11 = call i32 @rpl_malloc(i32 %2)
  %12 = icmp eq i32 %11, 0
  br i1 %12, label %blk_exit, label %brif_next13

brif_next6:                                       ; preds = %brif_next3
  %13 = call i32 @__errno_location()
  %14 = inttoptr i32 %13 to i32*
  %15 = load i32, i32* %14, align 4
  %.not28 = icmp eq i32 %15, 75
  br i1 %.not28, label %blk_exit4, label %blk_exit

blk_exit10:                                       ; preds = %brif_next23, %brif_next17
  call void @rpl_free(i32 %11)
  br label %blk_exit1

brif_next13:                                      ; preds = %blk_exit4
  %16 = call i32 @memcpy(i32 %11, i32 %_arg_0, i32 %2)
  br label %loop_entry

blk_exit14:                                       ; preds = %loop_entry
  %17 = call i32 @rpl_lstat(i32 %11, i32 %1)
  %brif_val22.not = icmp eq i32 %17, 0
  br i1 %brif_val22.not, label %brif_next23, label %blk_exit21

loop_entry:                                       ; preds = %brif_next17, %brif_next13
  %_local_2.0 = phi i32 [ %2, %brif_next13 ], [ %22, %brif_next17 ]
  %18 = add i32 %11, %_local_2.0
  %19 = add i32 %18, -1
  %20 = inttoptr i32 %19 to i8*
  %21 = load i8, i8* %20, align 1
  %.not29 = icmp eq i8 %21, 47
  br i1 %.not29, label %brif_next17, label %blk_exit14

brif_next17:                                      ; preds = %loop_entry
  store i8 0, i8* %20, align 1
  %22 = add i32 %_local_2.0, -1
  %brif_val19.not = icmp eq i32 %22, 0
  br i1 %brif_val19.not, label %blk_exit10, label %loop_entry

blk_exit21:                                       ; preds = %brif_next23, %blk_exit14
  call void @rpl_free(i32 %11)
  %23 = call i32 @__errno_location()
  %24 = inttoptr i32 %23 to i32*
  store i32 1, i32* %24, align 4
  br label %blk_exit

brif_next23:                                      ; preds = %blk_exit14
  %calcOffset24 = add i32 %0, -120
  %25 = inttoptr i32 %calcOffset24 to i32*
  %26 = load i32, i32* %25, align 4
  %27 = and i32 %26, 61440
  %.not30 = icmp eq i32 %27, 40960
  br i1 %.not30, label %blk_exit21, label %blk_exit10
}

; Function Attrs: null_pointer_is_valid
define internal i32 @rpl_unlinkat(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -144
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = and i32 %_arg_2, 512
  %brif_val.not = icmp eq i32 %2, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit1

blk_exit:                                         ; preds = %blk_exit13, %brif_next8, %blk_exit1
  %_local_5.0 = phi i32 [ %3, %blk_exit1 ], [ -1, %blk_exit13 ], [ %10, %brif_next8 ]
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %_local_5.0

blk_exit1:                                        ; preds = %blk_exit12, %brif_next3, %brif_next, %allocator
  %3 = call i32 @unlinkat(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2)
  br label %blk_exit

brif_next:                                        ; preds = %allocator
  %4 = call i32 @strlen(i32 %_arg_1)
  %5 = icmp eq i32 %4, 0
  br i1 %5, label %blk_exit1, label %brif_next3

brif_next3:                                       ; preds = %brif_next
  %6 = add i32 %4, %_arg_1
  %7 = add i32 %6, -1
  %8 = inttoptr i32 %7 to i8*
  %9 = load i8, i8* %8, align 1
  %.not = icmp eq i8 %9, 47
  br i1 %.not, label %brif_next5, label %blk_exit1

brif_next5:                                       ; preds = %brif_next3
  %10 = call i32 @fstatat(i32 %_arg_0, i32 %_arg_1, i32 %1, i32 256)
  %11 = icmp eq i32 %10, 0
  br i1 %11, label %blk_exit6, label %brif_next8

blk_exit6:                                        ; preds = %brif_next8, %brif_next5
  %12 = call i32 @rpl_malloc(i32 %4)
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %blk_exit13, label %brif_next15

brif_next8:                                       ; preds = %brif_next5
  %14 = call i32 @__errno_location()
  %15 = inttoptr i32 %14 to i32*
  %16 = load i32, i32* %15, align 4
  %.not30 = icmp eq i32 %16, 75
  br i1 %.not30, label %blk_exit6, label %blk_exit

blk_exit12:                                       ; preds = %brif_next25, %brif_next19
  call void @rpl_free(i32 %12)
  br label %blk_exit1

blk_exit13:                                       ; preds = %blk_exit23, %blk_exit6
  %17 = call i32 @__errno_location()
  %18 = inttoptr i32 %17 to i32*
  store i32 1, i32* %18, align 4
  br label %blk_exit

brif_next15:                                      ; preds = %blk_exit6
  %19 = call i32 @memcpy(i32 %12, i32 %_arg_1, i32 %4)
  br label %loop_entry

blk_exit16:                                       ; preds = %loop_entry
  %20 = call i32 @fstatat(i32 %_arg_0, i32 %12, i32 %1, i32 256)
  %brif_val24.not = icmp eq i32 %20, 0
  br i1 %brif_val24.not, label %brif_next25, label %blk_exit23

loop_entry:                                       ; preds = %brif_next19, %brif_next15
  %_local_4.0 = phi i32 [ %4, %brif_next15 ], [ %25, %brif_next19 ]
  %21 = add i32 %12, %_local_4.0
  %22 = add i32 %21, -1
  %23 = inttoptr i32 %22 to i8*
  %24 = load i8, i8* %23, align 1
  %.not31 = icmp eq i8 %24, 47
  br i1 %.not31, label %brif_next19, label %blk_exit16

brif_next19:                                      ; preds = %loop_entry
  store i8 0, i8* %23, align 1
  %25 = add i32 %_local_4.0, -1
  %brif_val21.not = icmp eq i32 %25, 0
  br i1 %brif_val21.not, label %blk_exit12, label %loop_entry

blk_exit23:                                       ; preds = %brif_next25, %blk_exit16
  call void @rpl_free(i32 %12)
  br label %blk_exit13

brif_next25:                                      ; preds = %blk_exit16
  %calcOffset26 = add i32 %0, -120
  %26 = inttoptr i32 %calcOffset26 to i32*
  %27 = load i32, i32* %26, align 4
  %28 = and i32 %27, 61440
  %.not32 = icmp eq i32 %28, 40960
  br i1 %.not32, label %blk_exit23, label %blk_exit12
}

; Function Attrs: null_pointer_is_valid
define internal void @gettime(i32 %_arg_0) #0 {
allocator:
  %0 = call i32 @clock_gettime(i32 0, i32 %_arg_0)
  ret void
}

; Function Attrs: null_pointer_is_valid
define internal i32 @rpl_utime(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -144
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = call i32 @strlen(i32 %_arg_0)
  %3 = icmp eq i32 %2, 0
  br i1 %3, label %blk_exit1, label %brif_next

blk_exit:                                         ; preds = %brif_next5, %blk_exit1
  %storemerge = phi i32 [ %4, %blk_exit1 ], [ -1, %brif_next5 ]
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %storemerge

blk_exit1:                                        ; preds = %brif_next5, %brif_next3, %brif_next, %allocator
  %4 = call i32 @utime(i32 %_arg_0, i32 %_arg_1)
  br label %blk_exit

brif_next:                                        ; preds = %allocator
  %5 = add i32 %2, %_arg_0
  %6 = add i32 %5, -1
  %7 = inttoptr i32 %6 to i8*
  %8 = load i8, i8* %7, align 1
  %.not = icmp eq i8 %8, 47
  br i1 %.not, label %brif_next3, label %blk_exit1

brif_next3:                                       ; preds = %brif_next
  %9 = call i32 @rpl_stat(i32 %_arg_0, i32 %1)
  %.not9 = icmp eq i32 %9, -1
  br i1 %.not9, label %brif_next5, label %blk_exit1

brif_next5:                                       ; preds = %brif_next3
  %10 = call i32 @__errno_location()
  %11 = inttoptr i32 %10 to i32*
  %12 = load i32, i32* %11, align 4
  %13 = icmp eq i32 %12, 75
  br i1 %13, label %blk_exit1, label %blk_exit
}

; Function Attrs: null_pointer_is_valid
define internal i32 @fdutimens(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -224
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = icmp eq i32 %_arg_2, 0
  br i1 %2, label %blk_exit6, label %brif_next

blk_exit:                                         ; preds = %blk_exit124, %blk_exit86, %blk_exit60, %blk_exit51, %blk_exit44, %blk_exit43, %brif_next37, %blk_exit5, %blk_exit2, %blk_exit1
  %_local_7.0 = phi i32 [ -1, %brif_next37 ], [ %70, %blk_exit51 ], [ %6, %blk_exit2 ], [ %4, %blk_exit1 ], [ %107, %blk_exit124 ], [ 0, %blk_exit86 ], [ -1, %blk_exit44 ], [ %76, %blk_exit60 ], [ -1, %blk_exit43 ], [ -1, %blk_exit5 ]
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %_local_7.0

blk_exit1:                                        ; preds = %brif_next91, %brif_next78, %blk_exit42, %blk_exit3
  %_local_7.1 = phi i32 [ %calcOffset112, %blk_exit3 ], [ 0, %brif_next91 ], [ 0, %blk_exit42 ], [ 0, %brif_next78 ]
  %_local_5.0 = phi i32 [ %_local_5.1, %blk_exit3 ], [ 0, %brif_next91 ], [ 0, %blk_exit42 ], [ 0, %brif_next78 ]
  %brif_val125.not = phi i1 [ true, %blk_exit3 ], [ false, %brif_next91 ], [ false, %blk_exit42 ], [ false, %brif_next78 ]
  %3 = call i32 @futimesat(i32 %_arg_0, i32 0, i32 %_local_7.1)
  %select_cond119.not = icmp ne i32 %3, 0
  %4 = sext i1 %select_cond119.not to i32
  %5 = icmp eq i32 %3, 0
  %or.cond137 = select i1 %5, i1 true, i1 %brif_val36.not
  br i1 %or.cond137, label %blk_exit, label %brif_next123

blk_exit2:                                        ; preds = %brif_next91, %brif_next74, %blk_exit3
  %_local_7.2 = phi i32 [ %calcOffset112, %blk_exit3 ], [ 0, %brif_next91 ], [ 0, %brif_next74 ]
  %6 = call i32 @futimesat(i32 -100, i32 %_arg_1, i32 %_local_7.2)
  br label %blk_exit

blk_exit3:                                        ; preds = %blk_exit103, %blk_exit102, %blk_exit84, %blk_exit42
  %_local_5.1 = phi i32 [ %61, %blk_exit42 ], [ %60, %blk_exit102 ], [ %60, %blk_exit103 ], [ %60, %blk_exit84 ]
  %_local_4.0 = phi i32 [ 0, %blk_exit42 ], [ %_local_4.1, %blk_exit102 ], [ %_local_4.1, %blk_exit103 ], [ %_local_4.1, %blk_exit84 ]
  %7 = add i32 %0, -184
  %8 = add i32 %_local_5.1, 24
  %9 = inttoptr i32 %8 to i32*
  %10 = load i32, i32* %9, align 4
  %11 = sdiv i32 %10, 1000
  %12 = sext i32 %11 to i64
  %13 = inttoptr i32 %7 to i64*
  store i64 %12, i64* %13, align 8
  %14 = inttoptr i32 %_local_5.1 to i64*
  %15 = load i64, i64* %14, align 8
  %calcOffset112 = add i32 %0, -208
  %16 = inttoptr i32 %calcOffset112 to i64*
  store i64 %15, i64* %16, align 8
  %calcOffset113 = add i32 %_local_5.1, 16
  %17 = inttoptr i32 %calcOffset113 to i64*
  %18 = load i64, i64* %17, align 8
  %calcOffset114 = add i32 %0, -192
  %19 = inttoptr i32 %calcOffset114 to i64*
  store i64 %18, i64* %19, align 8
  %calcOffset115 = add i32 %_local_5.1, 8
  %20 = inttoptr i32 %calcOffset115 to i32*
  %21 = load i32, i32* %20, align 4
  %22 = sdiv i32 %21, 1000
  %23 = sext i32 %22 to i64
  %calcOffset116 = add i32 %0, -200
  %24 = inttoptr i32 %calcOffset116 to i64*
  store i64 %23, i64* %24, align 8
  %25 = icmp eq i32 %_local_4.0, 0
  br i1 %25, label %blk_exit1, label %blk_exit2

blk_exit4:                                        ; preds = %brif_next78, %brif_next74
  %brif_val92.not = phi i1 [ false, %brif_next74 ], [ true, %brif_next78 ]
  %_local_4.1 = phi i32 [ 1, %brif_next74 ], [ 0, %brif_next78 ]
  %calcOffset88 = add i32 %0, -24
  %26 = inttoptr i32 %calcOffset88 to i32*
  %27 = load i32, i32* %26, align 4
  switch i32 %27, label %blk_exit4.blk_exit84_crit_edge [
    i32 1073741822, label %blk_exit86
    i32 1073741823, label %blk_exit87
  ]

blk_exit4.blk_exit84_crit_edge:                   ; preds = %blk_exit4
  %.pre = add i32 %0, -8
  %.pre142 = inttoptr i32 %.pre to i32*
  br label %blk_exit84

blk_exit5:                                        ; preds = %blk_exit14, %brif_next
  %28 = call i32 @__errno_location()
  %29 = inttoptr i32 %28 to i32*
  store i32 22, i32* %29, align 4
  br label %blk_exit

blk_exit6:                                        ; preds = %blk_exit29, %allocator
  %_local_4.2 = phi i32 [ 0, %allocator ], [ %56, %blk_exit29 ]
  %brif_val34.not = icmp slt i32 %_arg_0, 0
  %brif_val36.not = icmp eq i32 %_arg_1, 0
  %or.cond = select i1 %brif_val34.not, i1 %brif_val36.not, i1 false
  br i1 %or.cond, label %brif_next37, label %blk_exit33

brif_next:                                        ; preds = %allocator
  %30 = add i32 %0, -8
  %31 = add i32 %_arg_2, 24
  %32 = inttoptr i32 %31 to i64*
  %33 = load i64, i64* %32, align 8
  %34 = inttoptr i32 %30 to i64*
  store i64 %33, i64* %34, align 8
  %35 = inttoptr i32 %_arg_2 to i64*
  %36 = load i64, i64* %35, align 8
  %calcOffset9 = add i32 %0, -32
  %37 = inttoptr i32 %calcOffset9 to i64*
  store i64 %36, i64* %37, align 8
  %calcOffset10 = add i32 %_arg_2, 16
  %38 = inttoptr i32 %calcOffset10 to i64*
  %39 = load i64, i64* %38, align 8
  %calcOffset11 = add i32 %0, -16
  %40 = inttoptr i32 %calcOffset11 to i64*
  store i64 %39, i64* %40, align 8
  %41 = add i32 %_arg_2, 8
  %42 = inttoptr i32 %41 to i64*
  %43 = load i64, i64* %42, align 8
  %calcOffset13 = add i32 %0, -24
  %44 = inttoptr i32 %calcOffset13 to i64*
  store i64 %43, i64* %44, align 8
  %45 = trunc i64 %43 to i32
  %46 = and i32 %45, -2
  %47 = icmp ult i32 %45, 1000000000
  %.not = icmp eq i32 %46, 1073741822
  %or.cond138 = or i1 %47, %.not
  br i1 %or.cond138, label %blk_exit14, label %blk_exit5

blk_exit14:                                       ; preds = %brif_next
  %48 = inttoptr i32 %30 to i32*
  %49 = load i32, i32* %48, align 4
  %50 = and i32 %49, -2
  %51 = icmp ult i32 %49, 1000000000
  %.not131 = icmp eq i32 %50, 1073741822
  %or.cond139 = or i1 %51, %.not131
  br i1 %or.cond139, label %blk_exit20, label %blk_exit5

blk_exit20:                                       ; preds = %blk_exit14
  br i1 %.not, label %brif_next27, label %blk_exit25

blk_exit25:                                       ; preds = %brif_next27, %blk_exit20
  %_local_9.0 = phi i32 [ 1, %brif_next27 ], [ 0, %blk_exit20 ]
  %_local_10.0 = phi i32 [ %53, %brif_next27 ], [ 0, %blk_exit20 ]
  br i1 %.not131, label %brif_next31, label %blk_exit29

brif_next27:                                      ; preds = %blk_exit20
  store i64 0, i64* %37, align 8
  %52 = icmp eq i32 %45, 1073741822
  %53 = zext i1 %52 to i32
  br label %blk_exit25

blk_exit29:                                       ; preds = %brif_next31, %blk_exit25
  %_local_9.1 = phi i32 [ 1, %brif_next31 ], [ %_local_9.0, %blk_exit25 ]
  %_local_10.1 = phi i32 [ %59, %brif_next31 ], [ %_local_10.0, %blk_exit25 ]
  %54 = icmp eq i32 %_local_10.1, 1
  %55 = zext i1 %54 to i32
  %56 = add nsw i32 %_local_9.1, %55
  br label %blk_exit6

brif_next31:                                      ; preds = %blk_exit25
  store i64 0, i64* %40, align 8
  %57 = icmp eq i32 %49, 1073741822
  %58 = zext i1 %57 to i32
  %59 = add nuw nsw i32 %_local_10.0, %58
  br label %blk_exit29

blk_exit33:                                       ; preds = %blk_exit6
  %60 = add i32 %0, -32
  %61 = select i1 %2, i32 0, i32 %60
  %62 = load i32, i32* inttoptr (i32 832080 to i32*), align 16
  %brif_val46.not = icmp sgt i32 %62, -1
  br i1 %brif_val46.not, label %brif_next47, label %blk_exit45

brif_next37:                                      ; preds = %blk_exit6
  %63 = call i32 @__errno_location()
  %64 = inttoptr i32 %63 to i32*
  store i32 9, i32* %64, align 4
  br label %blk_exit

blk_exit42:                                       ; preds = %blk_exit45
  br i1 %2, label %blk_exit1, label %blk_exit3

blk_exit43:                                       ; preds = %blk_exit66
  %65 = add i32 %0, -176
  %66 = call i32 @fstat(i32 %_arg_0, i32 %65)
  %brif_val77.not = icmp eq i32 %66, 0
  br i1 %brif_val77.not, label %brif_next78, label %blk_exit

blk_exit44:                                       ; preds = %blk_exit66, %brif_next59
  %67 = add i32 %0, -176
  %68 = call i32 @rpl_stat(i32 %_arg_1, i32 %67)
  %brif_val73.not = icmp eq i32 %68, 0
  br i1 %brif_val73.not, label %brif_next74, label %blk_exit

blk_exit45:                                       ; preds = %brif_next62, %blk_exit33
  store i32 -1, i32* inttoptr (i32 832080 to i32*), align 16
  %brif_val67.not = icmp sgt i32 %_arg_0, -1
  %69 = icmp eq i32 %_local_4.2, 0
  %or.cond141 = select i1 %brif_val67.not, i1 %69, i1 false
  br i1 %or.cond141, label %blk_exit42, label %blk_exit66

brif_next47:                                      ; preds = %blk_exit33
  br i1 %brif_val34.not, label %brif_next50, label %blk_exit48

blk_exit48:                                       ; preds = %brif_next53, %brif_next47
  %brif_val58.not = icmp slt i32 %_arg_0, 0
  br i1 %brif_val58.not, label %brif_next59, label %blk_exit57

brif_next50:                                      ; preds = %brif_next47
  %70 = call i32 @utimensat(i32 -100, i32 %_arg_1, i32 %61, i32 0)
  %71 = icmp eq i32 %70, 0
  br i1 %71, label %blk_exit51, label %brif_next53

blk_exit51:                                       ; preds = %brif_next53, %brif_next50
  store i32 1, i32* inttoptr (i32 832080 to i32*), align 16
  br label %blk_exit

brif_next53:                                      ; preds = %brif_next50
  %72 = call i32 @__errno_location()
  %73 = inttoptr i32 %72 to i32*
  %74 = load i32, i32* %73, align 4
  %75 = icmp eq i32 %74, 38
  br i1 %75, label %blk_exit48, label %blk_exit51

blk_exit57:                                       ; preds = %blk_exit48
  %76 = call i32 @futimens(i32 %_arg_0, i32 %61)
  %77 = icmp eq i32 %76, 0
  br i1 %77, label %blk_exit60, label %brif_next62

brif_next59:                                      ; preds = %blk_exit48
  store i32 -1, i32* inttoptr (i32 832080 to i32*), align 16
  br label %blk_exit44

blk_exit60:                                       ; preds = %brif_next62, %blk_exit57
  store i32 1, i32* inttoptr (i32 832080 to i32*), align 16
  br label %blk_exit

brif_next62:                                      ; preds = %blk_exit57
  %78 = call i32 @__errno_location()
  %79 = inttoptr i32 %78 to i32*
  %80 = load i32, i32* %79, align 4
  %81 = icmp eq i32 %80, 38
  br i1 %81, label %blk_exit45, label %blk_exit60

blk_exit66:                                       ; preds = %blk_exit45
  br i1 %brif_val34.not, label %blk_exit44, label %blk_exit43

brif_next74:                                      ; preds = %blk_exit44
  br i1 %2, label %blk_exit2, label %blk_exit4

brif_next78:                                      ; preds = %blk_exit43
  br i1 %2, label %blk_exit1, label %blk_exit4

blk_exit84:                                       ; preds = %brif_next96, %blk_exit85, %blk_exit4.blk_exit84_crit_edge
  %.pre-phi143 = phi i32* [ %.pre142, %blk_exit4.blk_exit84_crit_edge ], [ %89, %blk_exit85 ], [ %85, %brif_next96 ]
  %82 = add i32 %0, -16
  %83 = load i32, i32* %.pre-phi143, align 4
  switch i32 %83, label %blk_exit3 [
    i32 1073741822, label %blk_exit103
    i32 1073741823, label %blk_exit102
  ]

blk_exit85:                                       ; preds = %blk_exit87
  call void @gettime(i32 %61)
  br label %blk_exit84

blk_exit86:                                       ; preds = %blk_exit4
  %84 = add i32 %0, -8
  %85 = inttoptr i32 %84 to i32*
  %86 = load i32, i32* %85, align 4
  %87 = icmp eq i32 %86, 1073741822
  br i1 %87, label %blk_exit, label %brif_next96

blk_exit87:                                       ; preds = %blk_exit4
  %88 = add i32 %0, -8
  %89 = inttoptr i32 %88 to i32*
  %90 = load i32, i32* %89, align 4
  %.not134 = icmp eq i32 %90, 1073741823
  br i1 %.not134, label %brif_next91, label %blk_exit85

brif_next91:                                      ; preds = %blk_exit87
  br i1 %brif_val92.not, label %blk_exit1, label %blk_exit2

brif_next96:                                      ; preds = %blk_exit86
  %calcOffset97 = add i32 %0, -104
  %91 = inttoptr i32 %calcOffset97 to i64*
  %92 = load i64, i64* %91, align 8
  %93 = inttoptr i32 %61 to i64*
  store i64 %92, i64* %93, align 8
  %94 = add i32 %61, 8
  %95 = add i32 %0, -96
  %96 = inttoptr i32 %95 to i64*
  %97 = load i64, i64* %96, align 8
  %98 = inttoptr i32 %94 to i64*
  store i64 %97, i64* %98, align 8
  br label %blk_exit84

blk_exit102:                                      ; preds = %blk_exit84
  call void @gettime(i32 %82)
  br label %blk_exit3

blk_exit103:                                      ; preds = %blk_exit84
  %calcOffset105 = add i32 %0, -88
  %99 = inttoptr i32 %calcOffset105 to i64*
  %100 = load i64, i64* %99, align 8
  %101 = inttoptr i32 %82 to i64*
  store i64 %100, i64* %101, align 8
  %102 = add i32 %0, -8
  %103 = add i32 %0, -80
  %104 = inttoptr i32 %103 to i64*
  %105 = load i64, i64* %104, align 8
  %106 = inttoptr i32 %102 to i64*
  store i64 %105, i64* %106, align 8
  br label %blk_exit3

brif_next123:                                     ; preds = %blk_exit1
  br i1 %brif_val125.not, label %brif_next126, label %blk_exit124

blk_exit124:                                      ; preds = %brif_next126, %brif_next123
  %_local_7.4 = phi i32 [ %1, %brif_next126 ], [ 0, %brif_next123 ]
  %107 = call i32 @rpl_utime(i32 %_arg_1, i32 %_local_7.4)
  br label %blk_exit

brif_next126:                                     ; preds = %brif_next123
  %108 = inttoptr i32 %_local_5.0 to i64*
  %109 = load i64, i64* %108, align 8
  %calcOffset128 = add i32 %_local_5.0, 16
  %110 = inttoptr i32 %calcOffset128 to i64*
  %111 = load i64, i64* %110, align 8
  %calcOffset129 = add i32 %0, -216
  %112 = inttoptr i32 %calcOffset129 to i64*
  store i64 %111, i64* %112, align 8
  %113 = inttoptr i32 %1 to i64*
  store i64 %109, i64* %113, align 8
  br label %blk_exit124
}

; Function Attrs: null_pointer_is_valid
define internal i32 @rpl_realloc(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %0 = zext i32 %_arg_1 to i64
  %1 = sext i32 %_arg_1 to i64
  %2 = icmp eq i64 %0, %1
  br i1 %2, label %blk_exit, label %brif_next

return:                                           ; preds = %brif_next, %blk_exit
  %return_0 = phi i32 [ 0, %brif_next ], [ %4, %blk_exit ]
  ret i32 %return_0

blk_exit:                                         ; preds = %allocator
  %select_cond.not = icmp eq i32 %_arg_1, 0
  %3 = select i1 %select_cond.not, i32 1, i32 %_arg_1
  %4 = call i32 @realloc(i32 %_arg_0, i32 %3)
  br label %return

brif_next:                                        ; preds = %allocator
  %5 = call i32 @__errno_location()
  %6 = inttoptr i32 %5 to i32*
  store i32 12, i32* %6, align 4
  br label %return
}

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @ximalloc(i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @xirealloc(i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @xpalloc(i32, i32, i32, i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @xcalloc(i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @xstrdup(i32) #0

; Function Attrs: null_pointer_is_valid
define internal i32 @yesno() #0 {
allocator:
  %0 = call i32 @getchar()
  br label %loop_entry

blk_exit:                                         ; preds = %loop_entry, %loop_entry
  %1 = and i32 %0, -33
  %2 = icmp eq i32 %1, 89
  %3 = zext i1 %2 to i32
  ret i32 %3

loop_entry:                                       ; preds = %blk_exit1, %allocator
  %storemerge = phi i32 [ %0, %allocator ], [ %4, %blk_exit1 ]
  switch i32 %storemerge, label %blk_exit1 [
    i32 -1, label %blk_exit
    i32 10, label %blk_exit
  ]

blk_exit1:                                        ; preds = %loop_entry
  %4 = call i32 @getchar()
  br label %loop_entry
}

; Function Attrs: null_pointer_is_valid
define internal i32 @rpl_calloc(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %0 = icmp eq i32 %_arg_0, 0
  %1 = icmp eq i32 %_arg_1, 0
  %2 = or i1 %0, %1
  %3 = select i1 %2, i32 1, i32 %_arg_0
  %4 = select i1 %2, i32 1, i32 %_arg_1
  %umul = call { i32, i1 } @llvm.umul.with.overflow.i32(i32 %3, i32 %4)
  %5 = extractvalue { i32, i1 } %umul, 1
  %.not = xor i1 %5, true
  %umul.value = extractvalue { i32, i1 } %umul, 0
  %brif_val3.not.not = icmp sgt i32 %umul.value, -1
  %or.cond = and i1 %.not, %brif_val3.not.not
  br i1 %or.cond, label %blk_exit, label %blk_exit1

return:                                           ; preds = %blk_exit1, %blk_exit
  %return_0 = phi i32 [ 0, %blk_exit1 ], [ %6, %blk_exit ]
  ret i32 %return_0

blk_exit:                                         ; preds = %allocator
  %6 = call i32 @calloc(i32 %3, i32 %4)
  br label %return

blk_exit1:                                        ; preds = %allocator
  %7 = call i32 @__errno_location()
  %8 = inttoptr i32 %7 to i32*
  store i32 12, i32* %8, align 4
  br label %return
}

; Function Attrs: null_pointer_is_valid
declare dso_local i64 @vfzprintf(i32, i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @rpl_fprintf(i32, i32, i32) #0

; Function Attrs: null_pointer_is_valid
define internal i32 @rpl_malloc(i32 %_arg_0) #0 {
allocator:
  %select_cond.not = icmp eq i32 %_arg_0, 0
  %0 = select i1 %select_cond.not, i32 1, i32 %_arg_0
  %1 = zext i32 %0 to i64
  %2 = sext i32 %0 to i64
  %3 = icmp eq i64 %1, %2
  br i1 %3, label %blk_exit, label %brif_next

return:                                           ; preds = %brif_next, %blk_exit
  %return_0 = phi i32 [ 0, %brif_next ], [ %4, %blk_exit ]
  ret i32 %return_0

blk_exit:                                         ; preds = %allocator
  %4 = call i32 @malloc(i32 %0)
  br label %return

brif_next:                                        ; preds = %allocator
  %5 = call i32 @__errno_location()
  %6 = inttoptr i32 %5 to i32*
  store i32 12, i32* %6, align 4
  br label %return
}

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @rpl_printf(i32, i32) #0

; Function Attrs: null_pointer_is_valid
define internal i32 @printf_parse(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2) #0 {
allocator:
  %0 = inttoptr i32 %_arg_1 to i32*
  store i32 0, i32* %0, align 4
  %1 = inttoptr i32 %_arg_2 to i32*
  store i32 0, i32* %1, align 4
  %2 = add i32 %_arg_1, 16
  %calcOffset2 = add i32 %_arg_1, 4
  %3 = inttoptr i32 %calcOffset2 to i32*
  store i32 %2, i32* %3, align 4
  %4 = add i32 %_arg_2, 16
  %calcOffset3 = add i32 %_arg_2, 4
  %5 = inttoptr i32 %calcOffset3 to i32*
  store i32 %4, i32* %5, align 4
  %6 = inttoptr i32 %_arg_0 to i8*
  %7 = load i8, i8* %6, align 1
  %brif_val.not = icmp eq i8 %7, 0
  br i1 %brif_val.not, label %blk_exit5, label %loop_entry

return:                                           ; preds = %blk_exit343, %blk_exit334, %blk_exit5
  %return_0 = phi i32 [ 0, %blk_exit5 ], [ -1, %blk_exit334 ], [ -1, %blk_exit343 ]
  ret i32 %return_0

blk_exit:                                         ; preds = %blk_exit309, %brif_next305, %blk_exit270, %brif_next265, %blk_exit168, %brif_next163, %blk_exit82, %brif_next77
  %8 = load i32, i32* %5, align 4
  %9 = icmp eq i32 %8, %4
  br i1 %9, label %blk_exit339, label %brif_next342

blk_exit4:                                        ; preds = %blk_exit293, %brif_next259, %blk_exit195, %blk_exit182, %brif_next157, %brif_next151, %blk_exit96, %brif_next68, %blk_exit53, %brif_next34
  %10 = load i32, i32* %5, align 4
  %11 = icmp eq i32 %10, %4
  br i1 %11, label %blk_exit330, label %brif_next333

blk_exit5:                                        ; preds = %brif_next324, %allocator
  %_local_7.0 = phi i32 [ %_local_7.2, %brif_next324 ], [ 0, %allocator ]
  %_local_6.0 = phi i32 [ %_local_6.2, %brif_next324 ], [ 0, %allocator ]
  %_local_5.0 = phi i32 [ %phi.bo, %brif_next324 ], [ 0, %allocator ]
  %_local_3.0 = phi i32 [ %332, %brif_next324 ], [ %2, %allocator ]
  %_param_0.0 = phi i32 [ %_param_0.2, %brif_next324 ], [ %_arg_0, %allocator ]
  %calcOffset327 = add i32 %_arg_1, 12
  %12 = inttoptr i32 %calcOffset327 to i32*
  store i32 %_local_6.0, i32* %12, align 4
  %calcOffset328 = add i32 %_arg_1, 8
  %13 = inttoptr i32 %calcOffset328 to i32*
  store i32 %_local_7.0, i32* %13, align 4
  %14 = add i32 %_local_3.0, %_local_5.0
  %15 = inttoptr i32 %14 to i32*
  store i32 %_param_0.0, i32* %15, align 4
  br label %return

loop_entry:                                       ; preds = %blk_exit8, %allocator
  %_local_10.0 = phi i32 [ %_local_10.1, %blk_exit8 ], [ 7, %allocator ]
  %_local_9.0 = phi i32 [ %_local_9.1, %blk_exit8 ], [ 0, %allocator ]
  %_local_8.0 = phi i32 [ %_local_8.1, %blk_exit8 ], [ 7, %allocator ]
  %_local_7.1 = phi i32 [ %_local_7.2, %blk_exit8 ], [ 0, %allocator ]
  %_local_6.1 = phi i32 [ %_local_6.2, %blk_exit8 ], [ 0, %allocator ]
  %_local_5.1.in = phi i8 [ %19, %blk_exit8 ], [ %7, %allocator ]
  %_param_0.1 = phi i32 [ %_param_0.2, %blk_exit8 ], [ %_arg_0, %allocator ]
  %16 = add i32 %_param_0.1, 1
  %17 = icmp eq i8 %_local_5.1.in, 37
  br i1 %17, label %blk_exit9, label %blk_exit8

blk_exit8:                                        ; preds = %blk_exit316, %blk_exit242, %loop_entry
  %_local_10.1 = phi i32 [ %_local_10.7, %blk_exit242 ], [ %_local_10.7, %blk_exit316 ], [ %_local_10.0, %loop_entry ]
  %_local_9.1 = phi i32 [ %_local_9.7, %blk_exit242 ], [ %_local_9.7, %blk_exit316 ], [ %_local_9.0, %loop_entry ]
  %_local_8.1 = phi i32 [ %_local_8.0, %blk_exit242 ], [ %318, %blk_exit316 ], [ %_local_8.0, %loop_entry ]
  %_local_7.2 = phi i32 [ %_local_7.3, %blk_exit242 ], [ %_local_7.3, %blk_exit316 ], [ %_local_7.1, %loop_entry ]
  %_local_6.2 = phi i32 [ %_local_6.3, %blk_exit242 ], [ %_local_6.3, %blk_exit316 ], [ %_local_6.1, %loop_entry ]
  %_param_0.2 = phi i32 [ %276, %blk_exit242 ], [ %276, %blk_exit316 ], [ %16, %loop_entry ]
  %18 = inttoptr i32 %_param_0.2 to i8*
  %19 = load i8, i8* %18, align 1
  %brif_val323.not = icmp eq i8 %19, 0
  br i1 %brif_val323.not, label %brif_next324, label %loop_entry

blk_exit9:                                        ; preds = %loop_entry
  %20 = load i32, i32* %3, align 4
  %21 = load i32, i32* %0, align 4
  %22 = mul i32 %21, 44
  %23 = add i32 %20, %22
  %calcOffset14 = add i32 %23, 40
  %24 = inttoptr i32 %calcOffset14 to i32*
  store i32 -1, i32* %24, align 4
  %calcOffset15 = add i32 %23, 32
  %25 = inttoptr i32 %calcOffset15 to i32*
  store i32 -1, i32* %25, align 4
  %calcOffset16 = add i32 %23, 24
  %26 = inttoptr i32 %calcOffset16 to i64*
  store i64 0, i64* %26, align 8
  %calcOffset17 = add i32 %23, 16
  %27 = inttoptr i32 %calcOffset17 to i64*
  store i64 -4294967296, i64* %27, align 8
  %calcOffset18 = add i32 %23, 8
  %28 = inttoptr i32 %calcOffset18 to i64*
  store i64 0, i64* %28, align 8
  %29 = inttoptr i32 %23 to i32*
  store i32 %_param_0.1, i32* %29, align 4
  %30 = inttoptr i32 %16 to i8*
  %31 = load i8, i8* %30, align 1
  %32 = zext i8 %31 to i32
  %33 = add nuw nsw i32 %32, 208
  %34 = and i32 %33, 254
  %35 = icmp ugt i32 %34, 9
  br i1 %35, label %blk_exit20, label %brif_next23

blk_exit20:                                       ; preds = %brif_next36, %brif_next27, %blk_exit9
  %_local_12.0 = phi i32 [ -1, %blk_exit9 ], [ %64, %brif_next36 ], [ -1, %brif_next27 ]
  %_local_11.0 = phi i32 [ %16, %blk_exit9 ], [ %58, %brif_next36 ], [ %16, %brif_next27 ]
  %36 = add i32 %23, 28
  %37 = add i32 %23, 20
  %38 = add i32 %23, 12
  %39 = add i32 %_local_11.0, 2
  br label %loop_entry41

brif_next23:                                      ; preds = %blk_exit9
  %40 = add i32 %_param_0.1, 2
  br label %loop_entry24

loop_entry24:                                     ; preds = %loop_entry24, %brif_next23
  %_param_0.3 = phi i32 [ %40, %brif_next23 ], [ %43, %loop_entry24 ]
  %41 = inttoptr i32 %_param_0.3 to i8*
  %42 = load i8, i8* %41, align 1
  %43 = add i32 %_param_0.3, 1
  %44 = add i8 %42, -48
  %45 = icmp ult i8 %44, 10
  br i1 %45, label %loop_entry24, label %brif_next27

brif_next27:                                      ; preds = %loop_entry24
  %.not = icmp eq i8 %42, 36
  br i1 %.not, label %loop_entry30, label %blk_exit20

loop_entry30:                                     ; preds = %loop_entry30, %brif_next27
  %_local_15.0 = phi i32 [ %58, %loop_entry30 ], [ %40, %brif_next27 ]
  %_local_14.0 = phi i32 [ %57, %loop_entry30 ], [ %32, %brif_next27 ]
  %_param_0.4 = phi i32 [ %54, %loop_entry30 ], [ 0, %brif_next27 ]
  %46 = shl nuw i32 %_local_14.0, 24
  %47 = ashr exact i32 %46, 24
  %48 = mul i32 %_param_0.4, 10
  %49 = icmp ult i32 %_param_0.4, 429496730
  %50 = select i1 %49, i32 %48, i32 -1
  %51 = add i32 %47, %50
  %52 = add i32 %51, -48
  %53 = icmp ult i32 %52, %50
  %54 = select i1 %53, i32 -1, i32 %52
  %55 = inttoptr i32 %_local_15.0 to i8*
  %56 = load i8, i8* %55, align 1
  %57 = zext i8 %56 to i32
  %58 = add i32 %_local_15.0, 1
  %59 = add nuw nsw i32 %57, 208
  %60 = and i32 %59, 254
  %61 = icmp ult i32 %60, 10
  br i1 %61, label %loop_entry30, label %brif_next34

brif_next34:                                      ; preds = %loop_entry30
  %62 = add i32 %54, 1
  %63 = icmp ult i32 %62, 2
  br i1 %63, label %blk_exit4, label %brif_next36

brif_next36:                                      ; preds = %brif_next34
  %64 = add i32 %54, -1
  br label %blk_exit20

blk_exit37:                                       ; preds = %blk_exit39, %blk_exit38
  %_local_10.2 = phi i32 [ %_local_10.0, %blk_exit39 ], [ %_local_10.3, %blk_exit38 ]
  %_local_9.2 = phi i32 [ %_local_9.0, %blk_exit39 ], [ %_local_9.3, %blk_exit38 ]
  %_local_7.3 = phi i32 [ %_local_7.1, %blk_exit39 ], [ %_local_7.4, %blk_exit38 ]
  %_local_24.0 = phi i32 [ %83, %blk_exit39 ], [ %68, %blk_exit38 ]
  %_param_0.5 = phi i32 [ %storemerge, %blk_exit39 ], [ %_param_0.6, %blk_exit38 ]
  %65 = icmp eq i32 %_local_24.0, 46
  br i1 %65, label %blk_exit120, label %blk_exit119

blk_exit38:                                       ; preds = %brif_next115, %blk_exit106, %blk_exit96
  %_local_10.3 = phi i32 [ %_local_10.0, %brif_next115 ], [ %_local_10.4, %blk_exit96 ], [ %_local_10.4, %blk_exit106 ]
  %_local_9.3 = phi i32 [ %_local_9.0, %brif_next115 ], [ %_local_9.4, %blk_exit96 ], [ %_local_9.4, %blk_exit106 ]
  %_local_7.4 = phi i32 [ %156, %brif_next115 ], [ %136, %blk_exit96 ], [ %136, %blk_exit106 ]
  %_param_0.6 = phi i32 [ %153, %brif_next115 ], [ %_param_0.7, %blk_exit96 ], [ %_param_0.7, %blk_exit106 ]
  %66 = inttoptr i32 %_param_0.6 to i8*
  %67 = load i8, i8* %66, align 1
  %68 = zext i8 %67 to i32
  br label %blk_exit37

blk_exit39:                                       ; preds = %loop_entry41
  %69 = add nuw nsw i32 %83, 208
  %70 = and i32 %69, 254
  %71 = icmp ugt i32 %70, 9
  br i1 %71, label %blk_exit37, label %brif_next110

blk_exit40:                                       ; preds = %loop_entry41
  %72 = inttoptr i32 %38 to i32*
  store i32 %storemerge, i32* %72, align 4
  %73 = add i32 %storemerge, 1
  %74 = inttoptr i32 %calcOffset17 to i32*
  store i32 %73, i32* %74, align 4
  %75 = inttoptr i32 %73 to i8*
  %76 = load i8, i8* %75, align 1
  %77 = zext i8 %76 to i32
  %78 = add nuw nsw i32 %77, 208
  %79 = and i32 %78, 254
  %80 = icmp ugt i32 %79, 9
  br i1 %80, label %blk_exit53, label %loop_entry57

loop_entry41:                                     ; preds = %blk_exit42, %blk_exit20
  %_local_15.1 = phi i32 [ 0, %blk_exit20 ], [ %84, %blk_exit42 ]
  %_local_14.1 = phi i32 [ %39, %blk_exit20 ], [ %86, %blk_exit42 ]
  %storemerge = phi i32 [ %_local_11.0, %blk_exit20 ], [ %87, %blk_exit42 ]
  %81 = inttoptr i32 %storemerge to i8*
  %82 = load i8, i8* %81, align 1
  %83 = zext i8 %82 to i32
  switch i8 %82, label %blk_exit39 [
    i8 32, label %blk_exit45
    i8 48, label %blk_exit43
    i8 45, label %blk_exit47
    i8 35, label %blk_exit44
    i8 43, label %blk_exit46
    i8 42, label %blk_exit40
    i8 39, label %blk_exit42
  ]

blk_exit42:                                       ; preds = %blk_exit47, %blk_exit46, %blk_exit45, %blk_exit44, %blk_exit43, %loop_entry41
  %_local_5.2 = phi i32 [ 1, %loop_entry41 ], [ 4, %blk_exit46 ], [ 16, %blk_exit44 ], [ 2, %blk_exit47 ], [ 32, %blk_exit43 ], [ 8, %blk_exit45 ]
  %84 = or i32 %_local_15.1, %_local_5.2
  %85 = inttoptr i32 %calcOffset18 to i32*
  store i32 %84, i32* %85, align 4
  %86 = add i32 %_local_14.1, 1
  %87 = add i32 %storemerge, 1
  br label %loop_entry41

blk_exit43:                                       ; preds = %loop_entry41
  br label %blk_exit42

blk_exit44:                                       ; preds = %loop_entry41
  br label %blk_exit42

blk_exit45:                                       ; preds = %loop_entry41
  br label %blk_exit42

blk_exit46:                                       ; preds = %loop_entry41
  br label %blk_exit42

blk_exit47:                                       ; preds = %loop_entry41
  br label %blk_exit42

blk_exit52:                                       ; preds = %brif_next74, %brif_next70
  %_local_11.2 = phi i32 [ %_local_9.0, %brif_next74 ], [ %114, %brif_next70 ]
  %_local_9.4 = phi i32 [ %116, %brif_next74 ], [ %_local_9.0, %brif_next70 ]
  %_param_0.7 = phi i32 [ %73, %brif_next74 ], [ %108, %brif_next70 ]
  %88 = icmp ult i32 %_local_11.2, %_local_10.0
  br i1 %88, label %blk_exit52.blk_exit75_crit_edge, label %brif_next77

blk_exit52.blk_exit75_crit_edge:                  ; preds = %blk_exit52
  %.pre = load i32, i32* %5, align 4
  br label %blk_exit75

blk_exit53:                                       ; preds = %brif_next60, %blk_exit40
  %89 = inttoptr i32 %37 to i32*
  store i32 %_local_9.0, i32* %89, align 4
  %90 = icmp eq i32 %_local_9.0, -1
  br i1 %90, label %blk_exit4, label %brif_next74

loop_entry57:                                     ; preds = %loop_entry57, %blk_exit40
  %_local_5.3 = phi i32 [ %93, %loop_entry57 ], [ %_local_14.1, %blk_exit40 ]
  %91 = inttoptr i32 %_local_5.3 to i8*
  %92 = load i8, i8* %91, align 1
  %93 = add i32 %_local_5.3, 1
  %94 = add i8 %92, -48
  %95 = icmp ult i8 %94, 10
  br i1 %95, label %loop_entry57, label %brif_next60

brif_next60:                                      ; preds = %loop_entry57
  %.not368 = icmp eq i8 %92, 36
  br i1 %.not368, label %loop_entry63, label %blk_exit53

loop_entry63:                                     ; preds = %loop_entry63, %brif_next60
  %_local_15.2 = phi i32 [ %107, %loop_entry63 ], [ %77, %brif_next60 ]
  %_local_14.2 = phi i32 [ %108, %loop_entry63 ], [ %_local_14.1, %brif_next60 ]
  %_local_5.4 = phi i32 [ %104, %loop_entry63 ], [ 0, %brif_next60 ]
  %96 = shl nuw i32 %_local_15.2, 24
  %97 = ashr exact i32 %96, 24
  %98 = mul i32 %_local_5.4, 10
  %99 = icmp ult i32 %_local_5.4, 429496730
  %100 = select i1 %99, i32 %98, i32 -1
  %101 = add i32 %97, %100
  %102 = add i32 %101, -48
  %103 = icmp ult i32 %102, %100
  %104 = select i1 %103, i32 -1, i32 %102
  %105 = inttoptr i32 %_local_14.2 to i8*
  %106 = load i8, i8* %105, align 1
  %107 = zext i8 %106 to i32
  %108 = add i32 %_local_14.2, 1
  %109 = add nuw nsw i32 %107, 208
  %110 = and i32 %109, 254
  %111 = icmp ult i32 %110, 10
  br i1 %111, label %loop_entry63, label %brif_next68

brif_next68:                                      ; preds = %loop_entry63
  %112 = add i32 %104, 1
  %113 = icmp ult i32 %112, 2
  br i1 %113, label %blk_exit4, label %brif_next70

brif_next70:                                      ; preds = %brif_next68
  %114 = add i32 %104, -1
  %115 = inttoptr i32 %37 to i32*
  store i32 %114, i32* %115, align 4
  br label %blk_exit52

brif_next74:                                      ; preds = %blk_exit53
  %116 = add i32 %_local_9.0, 1
  br label %blk_exit52

blk_exit75:                                       ; preds = %blk_exit89, %blk_exit52.blk_exit75_crit_edge
  %117 = phi i32 [ %.pre, %blk_exit52.blk_exit75_crit_edge ], [ %storemerge370, %blk_exit89 ]
  %_local_10.4 = phi i32 [ %_local_10.0, %blk_exit52.blk_exit75_crit_edge ], [ %124, %blk_exit89 ]
  %118 = load i32, i32* %1, align 4
  %119 = icmp ugt i32 %118, %_local_11.2
  br i1 %119, label %blk_exit96, label %brif_next99

brif_next77:                                      ; preds = %blk_exit52
  %120 = shl i32 %_local_10.0, 1
  %select_cond78.not.inv = icmp sgt i32 %_local_10.0, -1
  %121 = select i1 %select_cond78.not.inv, i32 %120, i32 -1
  %122 = add i32 %_local_11.2, 1
  %123 = icmp ugt i32 %121, %_local_11.2
  %124 = select i1 %123, i32 %121, i32 %122
  %125 = icmp ugt i32 %124, 134217727
  br i1 %125, label %blk_exit, label %brif_next81

brif_next81:                                      ; preds = %brif_next77
  %126 = shl i32 %124, 5
  %127 = load i32, i32* %5, align 4
  %128 = icmp eq i32 %127, %4
  br i1 %128, label %blk_exit83, label %brif_next86

blk_exit82:                                       ; preds = %brif_next86, %blk_exit83
  %storemerge370 = phi i32 [ %131, %brif_next86 ], [ %130, %blk_exit83 ]
  %129 = icmp eq i32 %storemerge370, 0
  br i1 %129, label %blk_exit, label %brif_next88

blk_exit83:                                       ; preds = %brif_next81
  %130 = call i32 @rpl_malloc(i32 %126)
  br label %blk_exit82

brif_next86:                                      ; preds = %brif_next81
  %131 = call i32 @rpl_realloc(i32 %127, i32 %126)
  br label %blk_exit82

brif_next88:                                      ; preds = %blk_exit82
  %132 = load i32, i32* %5, align 4
  %.not371 = icmp eq i32 %132, %4
  br i1 %.not371, label %brif_next92, label %blk_exit89

blk_exit89:                                       ; preds = %brif_next92, %brif_next88
  store i32 %storemerge370, i32* %5, align 4
  br label %blk_exit75

brif_next92:                                      ; preds = %brif_next88
  %133 = load i32, i32* %1, align 4
  %134 = shl i32 %133, 5
  %135 = call i32 @memcpy(i32 %storemerge370, i32 %4, i32 %134)
  br label %blk_exit89

blk_exit96:                                       ; preds = %brif_next103, %blk_exit75
  %select_cond105.not = icmp eq i32 %_local_7.1, 0
  %136 = select i1 %select_cond105.not, i32 1, i32 %_local_7.1
  %137 = shl i32 %_local_11.2, 5
  %138 = add i32 %117, %137
  %139 = inttoptr i32 %138 to i32*
  %140 = load i32, i32* %139, align 4
  switch i32 %140, label %blk_exit4 [
    i32 0, label %blk_exit106
    i32 5, label %blk_exit38
  ]

brif_next99:                                      ; preds = %blk_exit75
  %141 = shl i32 %118, 5
  %142 = add i32 %117, %141
  br label %loop_entry100

loop_entry100:                                    ; preds = %loop_entry100, %brif_next99
  %_local_14.3 = phi i32 [ %118, %brif_next99 ], [ %145, %loop_entry100 ]
  %_local_5.5 = phi i32 [ %142, %brif_next99 ], [ %144, %loop_entry100 ]
  %143 = inttoptr i32 %_local_5.5 to i32*
  store i32 0, i32* %143, align 4
  %144 = add i32 %_local_5.5, 32
  %145 = add i32 %_local_14.3, 1
  %.not372 = icmp ugt i32 %145, %_local_11.2
  br i1 %.not372, label %brif_next103, label %loop_entry100

brif_next103:                                     ; preds = %loop_entry100
  store i32 %145, i32* %1, align 4
  br label %blk_exit96

blk_exit106:                                      ; preds = %blk_exit96
  store i32 5, i32* %139, align 4
  br label %blk_exit38

brif_next110:                                     ; preds = %blk_exit39
  %146 = inttoptr i32 %38 to i32*
  store i32 %storemerge, i32* %146, align 4
  br label %loop_entry112

loop_entry112:                                    ; preds = %loop_entry112, %brif_next110
  %_local_5.6 = phi i32 [ 0, %brif_next110 ], [ %148, %loop_entry112 ]
  %147 = add i32 %storemerge, %_local_5.6
  %148 = add i32 %_local_5.6, 1
  %149 = inttoptr i32 %147 to i8*
  %150 = load i8, i8* %149, align 1
  %151 = add i8 %150, -48
  %152 = icmp ult i8 %151, 10
  br i1 %152, label %loop_entry112, label %brif_next115

brif_next115:                                     ; preds = %loop_entry112
  %153 = add i32 %storemerge, %_local_5.6
  %154 = inttoptr i32 %calcOffset17 to i32*
  store i32 %153, i32* %154, align 4
  %155 = icmp ult i32 %_local_7.1, %_local_5.6
  %156 = select i1 %155, i32 %_local_5.6, i32 %_local_7.1
  br label %blk_exit38

blk_exit119:                                      ; preds = %blk_exit192, %blk_exit182, %brif_next131, %blk_exit37
  %_local_14.4 = phi i32 [ %_local_14.5, %blk_exit182 ], [ %_local_14.5, %blk_exit192 ], [ %178, %brif_next131 ], [ %_param_0.5, %blk_exit37 ]
  %_local_10.5 = phi i32 [ %_local_10.6, %blk_exit182 ], [ %_local_10.6, %blk_exit192 ], [ %_local_10.2, %brif_next131 ], [ %_local_10.2, %blk_exit37 ]
  %_local_9.5 = phi i32 [ %_local_9.6, %blk_exit182 ], [ %_local_9.6, %blk_exit192 ], [ %_local_9.2, %brif_next131 ], [ %_local_9.2, %blk_exit37 ]
  %_local_6.3 = phi i32 [ %230, %blk_exit182 ], [ %230, %blk_exit192 ], [ %181, %brif_next131 ], [ %_local_6.1, %blk_exit37 ]
  %157 = inttoptr i32 %_local_14.4 to i8*
  %158 = load i8, i8* %157, align 1
  switch i8 %158, label %blk_exit195 [
    i8 76, label %blk_exit197
    i8 122, label %blk_exit199
    i8 119, label %blk_exit198
    i8 116, label %blk_exit199
    i8 108, label %blk_exit200
    i8 106, label %blk_exit197
    i8 104, label %blk_exit201
    i8 90, label %blk_exit199
  ]

blk_exit120:                                      ; preds = %blk_exit37
  %calcOffset123 = add i32 %_param_0.5, 1
  %159 = inttoptr i32 %calcOffset123 to i8*
  %160 = load i8, i8* %159, align 1
  %161 = inttoptr i32 %calcOffset16 to i32*
  store i32 %_param_0.5, i32* %161, align 4
  %162 = icmp eq i8 %160, 42
  br i1 %162, label %blk_exit125, label %loop_entry128

blk_exit125:                                      ; preds = %blk_exit120
  %163 = add i32 %_param_0.5, 2
  %164 = inttoptr i32 %36 to i32*
  store i32 %163, i32* %164, align 4
  %165 = inttoptr i32 %163 to i8*
  %166 = load i8, i8* %165, align 1
  %167 = zext i8 %166 to i32
  %168 = add nuw nsw i32 %167, 208
  %169 = and i32 %168, 254
  %170 = icmp ugt i32 %169, 9
  br i1 %170, label %blk_exit136, label %brif_next139

loop_entry128:                                    ; preds = %loop_entry128, %blk_exit120
  %_local_5.7 = phi i32 [ %172, %loop_entry128 ], [ 0, %blk_exit120 ]
  %171 = add i32 %_param_0.5, %_local_5.7
  %172 = add i32 %_local_5.7, 1
  %173 = add i32 %171, 1
  %174 = inttoptr i32 %173 to i8*
  %175 = load i8, i8* %174, align 1
  %176 = add i8 %175, -48
  %177 = icmp ult i8 %176, 10
  br i1 %177, label %loop_entry128, label %brif_next131

brif_next131:                                     ; preds = %loop_entry128
  %178 = add i32 %_param_0.5, %172
  %179 = inttoptr i32 %36 to i32*
  store i32 %178, i32* %179, align 4
  %180 = icmp ult i32 %_local_6.1, %172
  %181 = select i1 %180, i32 %172, i32 %_local_6.1
  br label %blk_exit119

blk_exit135:                                      ; preds = %brif_next160, %brif_next153, %blk_exit136
  %_local_14.5 = phi i32 [ %163, %brif_next160 ], [ %163, %blk_exit136 ], [ %202, %brif_next153 ]
  %_local_11.3 = phi i32 [ %_local_9.2, %brif_next160 ], [ %183, %blk_exit136 ], [ %208, %brif_next153 ]
  %_local_9.6 = phi i32 [ %210, %brif_next160 ], [ %_local_9.2, %blk_exit136 ], [ %_local_9.2, %brif_next153 ]
  %182 = icmp ult i32 %_local_11.3, %_local_10.2
  br i1 %182, label %blk_exit135.blk_exit161_crit_edge, label %brif_next163

blk_exit135.blk_exit161_crit_edge:                ; preds = %blk_exit135
  %.pre373 = load i32, i32* %5, align 4
  br label %blk_exit161

blk_exit136:                                      ; preds = %brif_next143, %blk_exit125
  %183 = load i32, i32* %25, align 4
  %.not367 = icmp eq i32 %183, -1
  br i1 %.not367, label %brif_next157, label %blk_exit135

brif_next139:                                     ; preds = %blk_exit125
  %184 = add i32 %_param_0.5, 3
  br label %loop_entry140

loop_entry140:                                    ; preds = %loop_entry140, %brif_next139
  %_param_0.8 = phi i32 [ %184, %brif_next139 ], [ %187, %loop_entry140 ]
  %185 = inttoptr i32 %_param_0.8 to i8*
  %186 = load i8, i8* %185, align 1
  %187 = add i32 %_param_0.8, 1
  %188 = add i8 %186, -48
  %189 = icmp ult i8 %188, 10
  br i1 %189, label %loop_entry140, label %brif_next143

brif_next143:                                     ; preds = %loop_entry140
  %.not362 = icmp eq i8 %186, 36
  br i1 %.not362, label %loop_entry146, label %blk_exit136

loop_entry146:                                    ; preds = %loop_entry146, %brif_next143
  %_local_15.3 = phi i32 [ %202, %loop_entry146 ], [ %184, %brif_next143 ]
  %_local_11.4 = phi i32 [ %201, %loop_entry146 ], [ %167, %brif_next143 ]
  %_param_0.9 = phi i32 [ %198, %loop_entry146 ], [ 0, %brif_next143 ]
  %190 = shl nuw i32 %_local_11.4, 24
  %191 = ashr exact i32 %190, 24
  %192 = mul i32 %_param_0.9, 10
  %193 = icmp ult i32 %_param_0.9, 429496730
  %194 = select i1 %193, i32 %192, i32 -1
  %195 = add i32 %191, %194
  %196 = add i32 %195, -48
  %197 = icmp ult i32 %196, %194
  %198 = select i1 %197, i32 -1, i32 %196
  %199 = inttoptr i32 %_local_15.3 to i8*
  %200 = load i8, i8* %199, align 1
  %201 = zext i8 %200 to i32
  %202 = add i32 %_local_15.3, 1
  %203 = add nuw nsw i32 %201, 208
  %204 = and i32 %203, 254
  %205 = icmp ult i32 %204, 10
  br i1 %205, label %loop_entry146, label %brif_next151

brif_next151:                                     ; preds = %loop_entry146
  %206 = add i32 %198, 1
  %207 = icmp ult i32 %206, 2
  br i1 %207, label %blk_exit4, label %brif_next153

brif_next153:                                     ; preds = %brif_next151
  %208 = add i32 %198, -1
  store i32 %208, i32* %25, align 4
  br label %blk_exit135

brif_next157:                                     ; preds = %blk_exit136
  store i32 %_local_9.2, i32* %25, align 4
  %209 = icmp eq i32 %_local_9.2, -1
  br i1 %209, label %blk_exit4, label %brif_next160

brif_next160:                                     ; preds = %brif_next157
  %210 = add i32 %_local_9.2, 1
  br label %blk_exit135

blk_exit161:                                      ; preds = %blk_exit175, %blk_exit135.blk_exit161_crit_edge
  %211 = phi i32 [ %.pre373, %blk_exit135.blk_exit161_crit_edge ], [ %storemerge364, %blk_exit175 ]
  %_local_10.6 = phi i32 [ %_local_10.2, %blk_exit135.blk_exit161_crit_edge ], [ %218, %blk_exit175 ]
  %212 = load i32, i32* %1, align 4
  %213 = icmp ugt i32 %212, %_local_11.3
  br i1 %213, label %blk_exit182, label %brif_next185

brif_next163:                                     ; preds = %blk_exit135
  %214 = shl i32 %_local_10.2, 1
  %select_cond164.not.inv = icmp sgt i32 %_local_10.2, -1
  %215 = select i1 %select_cond164.not.inv, i32 %214, i32 -1
  %216 = add i32 %_local_11.3, 1
  %217 = icmp ugt i32 %215, %_local_11.3
  %218 = select i1 %217, i32 %215, i32 %216
  %219 = icmp ugt i32 %218, 134217727
  br i1 %219, label %blk_exit, label %brif_next167

brif_next167:                                     ; preds = %brif_next163
  %220 = shl i32 %218, 5
  %221 = load i32, i32* %5, align 4
  %222 = icmp eq i32 %221, %4
  br i1 %222, label %blk_exit169, label %brif_next172

blk_exit168:                                      ; preds = %brif_next172, %blk_exit169
  %storemerge364 = phi i32 [ %225, %brif_next172 ], [ %224, %blk_exit169 ]
  %223 = icmp eq i32 %storemerge364, 0
  br i1 %223, label %blk_exit, label %brif_next174

blk_exit169:                                      ; preds = %brif_next167
  %224 = call i32 @rpl_malloc(i32 %220)
  br label %blk_exit168

brif_next172:                                     ; preds = %brif_next167
  %225 = call i32 @rpl_realloc(i32 %221, i32 %220)
  br label %blk_exit168

brif_next174:                                     ; preds = %blk_exit168
  %226 = load i32, i32* %5, align 4
  %.not365 = icmp eq i32 %226, %4
  br i1 %.not365, label %brif_next178, label %blk_exit175

blk_exit175:                                      ; preds = %brif_next178, %brif_next174
  store i32 %storemerge364, i32* %5, align 4
  br label %blk_exit161

brif_next178:                                     ; preds = %brif_next174
  %227 = load i32, i32* %1, align 4
  %228 = shl i32 %227, 5
  %229 = call i32 @memcpy(i32 %storemerge364, i32 %4, i32 %228)
  br label %blk_exit175

blk_exit182:                                      ; preds = %brif_next189, %blk_exit161
  %.inv = icmp ugt i32 %_local_6.1, 2
  %230 = select i1 %.inv, i32 %_local_6.1, i32 2
  %231 = shl i32 %_local_11.3, 5
  %232 = add i32 %211, %231
  %233 = inttoptr i32 %232 to i32*
  %234 = load i32, i32* %233, align 4
  switch i32 %234, label %blk_exit4 [
    i32 0, label %blk_exit192
    i32 5, label %blk_exit119
  ]

brif_next185:                                     ; preds = %blk_exit161
  %235 = shl i32 %212, 5
  %236 = add i32 %211, %235
  br label %loop_entry186

loop_entry186:                                    ; preds = %loop_entry186, %brif_next185
  %_local_5.8 = phi i32 [ %212, %brif_next185 ], [ %239, %loop_entry186 ]
  %_param_0.10 = phi i32 [ %236, %brif_next185 ], [ %238, %loop_entry186 ]
  %237 = inttoptr i32 %_param_0.10 to i32*
  store i32 0, i32* %237, align 4
  %238 = add i32 %_param_0.10, 32
  %239 = add i32 %_local_5.8, 1
  %.not366 = icmp ugt i32 %239, %_local_11.3
  br i1 %.not366, label %brif_next189, label %loop_entry186

brif_next189:                                     ; preds = %loop_entry186
  store i32 %239, i32* %1, align 4
  br label %blk_exit182

blk_exit192:                                      ; preds = %blk_exit182
  store i32 5, i32* %233, align 4
  br label %blk_exit119

blk_exit195:                                      ; preds = %brif_next218, %blk_exit200, %blk_exit198, %blk_exit198, %blk_exit198, %blk_exit198, %blk_exit196, %blk_exit119
  %240 = phi i8 [ %158, %blk_exit119 ], [ %.pre374, %blk_exit196 ], [ 119, %brif_next218 ], [ 119, %blk_exit198 ], [ 119, %blk_exit198 ], [ 119, %blk_exit198 ], [ 119, %blk_exit198 ], [ %247, %blk_exit200 ]
  %_local_15.4 = phi i32 [ 27, %blk_exit119 ], [ %_local_15.5, %blk_exit196 ], [ 27, %brif_next218 ], [ 27, %blk_exit198 ], [ 27, %blk_exit198 ], [ 27, %blk_exit198 ], [ 27, %blk_exit198 ], [ 27, %blk_exit200 ]
  %_local_14.6 = phi i32 [ %_local_14.4, %blk_exit119 ], [ %_local_14.7, %blk_exit196 ], [ %_local_14.4, %brif_next218 ], [ %_local_14.4, %blk_exit198 ], [ %_local_14.4, %blk_exit198 ], [ %_local_14.4, %blk_exit198 ], [ %_local_14.4, %blk_exit198 ], [ %calcOffset208, %blk_exit200 ]
  %_local_11.5 = phi i32 [ 5, %blk_exit119 ], [ %_local_11.6, %blk_exit196 ], [ 5, %brif_next218 ], [ 5, %blk_exit198 ], [ 5, %blk_exit198 ], [ 5, %blk_exit198 ], [ 5, %blk_exit198 ], [ 7, %blk_exit200 ]
  %_local_5.9 = phi i32 [ 6, %blk_exit119 ], [ %_local_5.10, %blk_exit196 ], [ 6, %brif_next218 ], [ 6, %blk_exit198 ], [ 6, %blk_exit198 ], [ 6, %blk_exit198 ], [ 6, %blk_exit198 ], [ 8, %blk_exit200 ]
  %_local_23.0 = phi i32 [ 0, %blk_exit119 ], [ %_local_23.1, %blk_exit196 ], [ 0, %brif_next218 ], [ 0, %blk_exit198 ], [ 0, %blk_exit198 ], [ 0, %blk_exit198 ], [ 0, %blk_exit198 ], [ 0, %blk_exit200 ]
  %select_cond253.not = phi i1 [ true, %blk_exit119 ], [ true, %blk_exit196 ], [ true, %brif_next218 ], [ true, %blk_exit198 ], [ true, %blk_exit198 ], [ true, %blk_exit198 ], [ true, %blk_exit198 ], [ false, %blk_exit200 ]
  %241 = sext i8 %240 to i32
  switch i32 %241, label %blk_exit4 [
    i32 37, label %blk_exit242
    i32 120, label %blk_exit250
    i32 117, label %blk_exit250
    i32 115, label %blk_exit246
    i32 112, label %blk_exit244
    i32 111, label %blk_exit250
    i32 105, label %blk_exit243
    i32 103, label %blk_exit249
    i32 102, label %blk_exit249
    i32 101, label %blk_exit249
    i32 100, label %blk_exit243
    i32 99, label %blk_exit248
    i32 98, label %blk_exit250
    i32 97, label %blk_exit249
    i32 88, label %blk_exit250
    i32 83, label %blk_exit245
    i32 71, label %blk_exit249
    i32 70, label %blk_exit249
    i32 69, label %blk_exit249
    i32 67, label %blk_exit247
    i32 65, label %blk_exit249
  ]

blk_exit196:                                      ; preds = %brif_next241, %brif_next238, %brif_next235, %brif_next232, %brif_next229, %brif_next226, %blk_exit222, %blk_exit221, %blk_exit220, %blk_exit219, %blk_exit214, %blk_exit213, %blk_exit212, %blk_exit211, %blk_exit207, %blk_exit203, %blk_exit201, %blk_exit199, %blk_exit198, %blk_exit197
  %_local_15.5 = phi i32 [ 27, %blk_exit203 ], [ 28, %blk_exit207 ], [ 27, %brif_next232 ], [ 27, %blk_exit219 ], [ 27, %brif_next229 ], [ 27, %blk_exit220 ], [ 27, %blk_exit222 ], [ 27, %brif_next226 ], [ 27, %blk_exit221 ], [ 27, %blk_exit214 ], [ 27, %brif_next241 ], [ 27, %blk_exit211 ], [ 27, %brif_next238 ], [ 27, %blk_exit212 ], [ 27, %brif_next235 ], [ 27, %blk_exit213 ], [ 27, %blk_exit199 ], [ 28, %blk_exit197 ], [ 27, %blk_exit201 ], [ 27, %blk_exit198 ]
  %_local_14.7 = phi i32 [ %252, %blk_exit203 ], [ %253, %blk_exit207 ], [ %272, %brif_next232 ], [ %_local_14.4, %blk_exit219 ], [ %271, %brif_next229 ], [ %_local_14.4, %blk_exit220 ], [ %269, %blk_exit222 ], [ %270, %brif_next226 ], [ %_local_14.4, %blk_exit221 ], [ %260, %blk_exit214 ], [ %275, %brif_next241 ], [ %_local_14.4, %blk_exit211 ], [ %274, %brif_next238 ], [ %_local_14.4, %blk_exit212 ], [ %273, %brif_next235 ], [ %_local_14.4, %blk_exit213 ], [ %245, %blk_exit199 ], [ %242, %blk_exit197 ], [ %calcOffset204, %blk_exit201 ], [ %_local_14.4, %blk_exit198 ]
  %_local_11.6 = phi i32 [ 1, %blk_exit203 ], [ 9, %blk_exit207 ], [ 25, %brif_next232 ], [ 5, %blk_exit219 ], [ 23, %brif_next229 ], [ 5, %blk_exit220 ], [ 19, %blk_exit222 ], [ 21, %brif_next226 ], [ 5, %blk_exit221 ], [ 11, %blk_exit214 ], [ 17, %brif_next241 ], [ 5, %blk_exit211 ], [ 15, %brif_next238 ], [ 5, %blk_exit212 ], [ 13, %brif_next235 ], [ 5, %blk_exit213 ], [ 5, %blk_exit199 ], [ 9, %blk_exit197 ], [ 3, %blk_exit201 ], [ 5, %blk_exit198 ]
  %_local_5.10 = phi i32 [ 2, %blk_exit203 ], [ 10, %blk_exit207 ], [ 26, %brif_next232 ], [ 6, %blk_exit219 ], [ 24, %brif_next229 ], [ 6, %blk_exit220 ], [ 20, %blk_exit222 ], [ 22, %brif_next226 ], [ 6, %blk_exit221 ], [ 12, %blk_exit214 ], [ 18, %brif_next241 ], [ 6, %blk_exit211 ], [ 16, %brif_next238 ], [ 6, %blk_exit212 ], [ 14, %brif_next235 ], [ 6, %blk_exit213 ], [ 6, %blk_exit199 ], [ 10, %blk_exit197 ], [ 4, %blk_exit201 ], [ 6, %blk_exit198 ]
  %_local_23.1 = phi i32 [ 0, %blk_exit203 ], [ 1, %blk_exit207 ], [ 0, %brif_next232 ], [ 0, %blk_exit219 ], [ 0, %brif_next229 ], [ 0, %blk_exit220 ], [ 0, %blk_exit222 ], [ 0, %brif_next226 ], [ 0, %blk_exit221 ], [ 0, %blk_exit214 ], [ 0, %brif_next241 ], [ 0, %blk_exit211 ], [ 0, %brif_next238 ], [ 0, %blk_exit212 ], [ 0, %brif_next235 ], [ 0, %blk_exit213 ], [ 0, %blk_exit199 ], [ 1, %blk_exit197 ], [ 0, %blk_exit201 ], [ 0, %blk_exit198 ]
  %.phi.trans.insert = inttoptr i32 %_local_14.7 to i8*
  %.pre374 = load i8, i8* %.phi.trans.insert, align 1
  br label %blk_exit195

blk_exit197:                                      ; preds = %blk_exit119, %blk_exit119
  %242 = add i32 %_local_14.4, 1
  br label %blk_exit196

blk_exit198:                                      ; preds = %blk_exit119
  %calcOffset216 = add i32 %_local_14.4, 1
  %243 = inttoptr i32 %calcOffset216 to i8*
  %244 = load i8, i8* %243, align 1
  switch i8 %244, label %blk_exit196 [
    i8 49, label %blk_exit213
    i8 50, label %blk_exit195
    i8 51, label %blk_exit212
    i8 52, label %blk_exit195
    i8 53, label %blk_exit195
    i8 54, label %blk_exit211
    i8 55, label %blk_exit195
    i8 56, label %blk_exit214
    i8 102, label %brif_next218
  ]

blk_exit199:                                      ; preds = %blk_exit119, %blk_exit119, %blk_exit119
  %245 = add i32 %_local_14.4, 1
  br label %blk_exit196

blk_exit200:                                      ; preds = %blk_exit119
  %calcOffset208 = add i32 %_local_14.4, 1
  %246 = inttoptr i32 %calcOffset208 to i8*
  %247 = load i8, i8* %246, align 1
  %248 = icmp eq i8 %247, 108
  br i1 %248, label %blk_exit207, label %blk_exit195

blk_exit201:                                      ; preds = %blk_exit119
  %calcOffset204 = add i32 %_local_14.4, 1
  %249 = inttoptr i32 %calcOffset204 to i8*
  %250 = load i8, i8* %249, align 1
  %251 = icmp eq i8 %250, 104
  br i1 %251, label %blk_exit203, label %blk_exit196

blk_exit203:                                      ; preds = %blk_exit201
  %252 = add i32 %_local_14.4, 2
  br label %blk_exit196

blk_exit207:                                      ; preds = %blk_exit200
  %253 = add i32 %_local_14.4, 2
  br label %blk_exit196

blk_exit211:                                      ; preds = %blk_exit198
  %calcOffset239 = add i32 %_local_14.4, 2
  %254 = inttoptr i32 %calcOffset239 to i8*
  %255 = load i8, i8* %254, align 1
  %.not355 = icmp eq i8 %255, 52
  br i1 %.not355, label %brif_next241, label %blk_exit196

blk_exit212:                                      ; preds = %blk_exit198
  %calcOffset236 = add i32 %_local_14.4, 2
  %256 = inttoptr i32 %calcOffset236 to i8*
  %257 = load i8, i8* %256, align 1
  %.not356 = icmp eq i8 %257, 50
  br i1 %.not356, label %brif_next238, label %blk_exit196

blk_exit213:                                      ; preds = %blk_exit198
  %calcOffset233 = add i32 %_local_14.4, 2
  %258 = inttoptr i32 %calcOffset233 to i8*
  %259 = load i8, i8* %258, align 1
  %.not357 = icmp eq i8 %259, 54
  br i1 %.not357, label %brif_next235, label %blk_exit196

blk_exit214:                                      ; preds = %blk_exit198
  %260 = add i32 %_local_14.4, 2
  br label %blk_exit196

brif_next218:                                     ; preds = %blk_exit198
  %calcOffset223 = add i32 %_local_14.4, 2
  %261 = inttoptr i32 %calcOffset223 to i8*
  %262 = load i8, i8* %261, align 1
  switch i8 %262, label %blk_exit195 [
    i8 49, label %blk_exit221
    i8 56, label %blk_exit222
    i8 51, label %blk_exit220
    i8 54, label %blk_exit219
  ]

blk_exit219:                                      ; preds = %brif_next218
  %calcOffset230 = add i32 %_local_14.4, 3
  %263 = inttoptr i32 %calcOffset230 to i8*
  %264 = load i8, i8* %263, align 1
  %.not359 = icmp eq i8 %264, 52
  br i1 %.not359, label %brif_next232, label %blk_exit196

blk_exit220:                                      ; preds = %brif_next218
  %calcOffset227 = add i32 %_local_14.4, 3
  %265 = inttoptr i32 %calcOffset227 to i8*
  %266 = load i8, i8* %265, align 1
  %.not360 = icmp eq i8 %266, 50
  br i1 %.not360, label %brif_next229, label %blk_exit196

blk_exit221:                                      ; preds = %brif_next218
  %calcOffset224 = add i32 %_local_14.4, 3
  %267 = inttoptr i32 %calcOffset224 to i8*
  %268 = load i8, i8* %267, align 1
  %.not361 = icmp eq i8 %268, 54
  br i1 %.not361, label %brif_next226, label %blk_exit196

blk_exit222:                                      ; preds = %brif_next218
  %269 = add i32 %_local_14.4, 3
  br label %blk_exit196

brif_next226:                                     ; preds = %blk_exit221
  %270 = add i32 %_local_14.4, 4
  br label %blk_exit196

brif_next229:                                     ; preds = %blk_exit220
  %271 = add i32 %_local_14.4, 4
  br label %blk_exit196

brif_next232:                                     ; preds = %blk_exit219
  %272 = add i32 %_local_14.4, 4
  br label %blk_exit196

brif_next235:                                     ; preds = %blk_exit213
  %273 = add i32 %_local_14.4, 3
  br label %blk_exit196

brif_next238:                                     ; preds = %blk_exit212
  %274 = add i32 %_local_14.4, 3
  br label %blk_exit196

brif_next241:                                     ; preds = %blk_exit211
  %275 = add i32 %_local_14.4, 3
  br label %blk_exit196

blk_exit242:                                      ; preds = %brif_next296, %blk_exit293, %blk_exit195
  %_local_10.7 = phi i32 [ %_local_10.8, %brif_next296 ], [ %_local_10.8, %blk_exit293 ], [ %_local_10.5, %blk_exit195 ]
  %_local_9.7 = phi i32 [ %_local_9.8, %brif_next296 ], [ %_local_9.8, %blk_exit293 ], [ %_local_9.5, %blk_exit195 ]
  %_local_24.1 = phi i32 [ %_local_24.2, %brif_next296 ], [ %_local_24.2, %blk_exit293 ], [ 37, %blk_exit195 ]
  %276 = add i32 %_local_14.6, 1
  %calcOffset300 = add i32 %23, 4
  %277 = inttoptr i32 %calcOffset300 to i32*
  store i32 %276, i32* %277, align 4
  %calcOffset301 = add i32 %23, 36
  %278 = inttoptr i32 %calcOffset301 to i8*
  %279 = trunc i32 %_local_24.1 to i8
  store i8 %279, i8* %278, align 1
  %280 = load i32, i32* %0, align 4
  %281 = add i32 %280, 1
  store i32 %281, i32* %0, align 4
  %282 = icmp ult i32 %281, %_local_8.0
  br i1 %282, label %blk_exit8, label %brif_next305

blk_exit243:                                      ; preds = %blk_exit250, %blk_exit249, %blk_exit248, %blk_exit247, %blk_exit246, %blk_exit245, %blk_exit244, %blk_exit195, %blk_exit195
  %_local_11.7 = phi i32 [ 30, %blk_exit247 ], [ 32, %blk_exit245 ], [ %286, %blk_exit248 ], [ %_local_15.4, %blk_exit249 ], [ %_local_11.5, %blk_exit195 ], [ %_local_11.5, %blk_exit195 ], [ 33, %blk_exit244 ], [ %284, %blk_exit246 ], [ %_local_5.9, %blk_exit250 ]
  %_local_24.2 = phi i32 [ 99, %blk_exit247 ], [ 115, %blk_exit245 ], [ 99, %blk_exit248 ], [ %241, %blk_exit249 ], [ %241, %blk_exit195 ], [ %241, %blk_exit195 ], [ 112, %blk_exit244 ], [ 115, %blk_exit246 ], [ %241, %blk_exit250 ]
  store i32 %_local_12.0, i32* %24, align 4
  %.not348 = icmp eq i32 %_local_12.0, -1
  br i1 %.not348, label %brif_next259, label %blk_exit257

blk_exit244:                                      ; preds = %blk_exit195
  br label %blk_exit243

blk_exit245:                                      ; preds = %blk_exit195
  br label %blk_exit243

blk_exit246:                                      ; preds = %blk_exit195
  %select_cond254.not = icmp eq i32 %_local_23.0, 0
  %283 = and i1 %select_cond253.not, %select_cond254.not
  %284 = select i1 %283, i32 31, i32 32
  br label %blk_exit243

blk_exit247:                                      ; preds = %blk_exit195
  br label %blk_exit243

blk_exit248:                                      ; preds = %blk_exit195
  %select_cond252.not = icmp eq i32 %_local_23.0, 0
  %285 = and i1 %select_cond253.not, %select_cond252.not
  %286 = select i1 %285, i32 29, i32 30
  br label %blk_exit243

blk_exit249:                                      ; preds = %blk_exit195, %blk_exit195, %blk_exit195, %blk_exit195, %blk_exit195, %blk_exit195, %blk_exit195, %blk_exit195
  br label %blk_exit243

blk_exit250:                                      ; preds = %blk_exit195, %blk_exit195, %blk_exit195, %blk_exit195, %blk_exit195
  br label %blk_exit243

blk_exit257:                                      ; preds = %brif_next262, %blk_exit243
  %_local_12.1 = phi i32 [ %_local_9.5, %brif_next262 ], [ %_local_12.0, %blk_exit243 ]
  %_local_9.8 = phi i32 [ %289, %brif_next262 ], [ %_local_9.5, %blk_exit243 ]
  %287 = icmp ult i32 %_local_12.1, %_local_10.5
  br i1 %287, label %blk_exit257.blk_exit263_crit_edge, label %brif_next265

blk_exit257.blk_exit263_crit_edge:                ; preds = %blk_exit257
  %.pre375 = load i32, i32* %5, align 4
  br label %blk_exit263

brif_next259:                                     ; preds = %blk_exit243
  store i32 %_local_9.5, i32* %24, align 4
  %288 = icmp eq i32 %_local_9.5, -1
  br i1 %288, label %blk_exit4, label %brif_next262

brif_next262:                                     ; preds = %brif_next259
  %289 = add i32 %_local_9.5, 1
  br label %blk_exit257

blk_exit263:                                      ; preds = %blk_exit277, %blk_exit257.blk_exit263_crit_edge
  %290 = phi i32 [ %.pre375, %blk_exit257.blk_exit263_crit_edge ], [ %storemerge349, %blk_exit277 ]
  %_local_10.8 = phi i32 [ %_local_10.5, %blk_exit257.blk_exit263_crit_edge ], [ %297, %blk_exit277 ]
  %291 = load i32, i32* %1, align 4
  %292 = icmp ugt i32 %291, %_local_12.1
  br i1 %292, label %blk_exit284, label %brif_next287

brif_next265:                                     ; preds = %blk_exit257
  %293 = shl i32 %_local_10.5, 1
  %select_cond266.not.inv = icmp sgt i32 %_local_10.5, -1
  %294 = select i1 %select_cond266.not.inv, i32 %293, i32 -1
  %295 = add i32 %_local_12.1, 1
  %296 = icmp ugt i32 %294, %_local_12.1
  %297 = select i1 %296, i32 %294, i32 %295
  %298 = icmp ugt i32 %297, 134217727
  br i1 %298, label %blk_exit, label %brif_next269

brif_next269:                                     ; preds = %brif_next265
  %299 = shl i32 %297, 5
  %300 = load i32, i32* %5, align 4
  %301 = icmp eq i32 %300, %4
  br i1 %301, label %blk_exit271, label %brif_next274

blk_exit270:                                      ; preds = %brif_next274, %blk_exit271
  %storemerge349 = phi i32 [ %304, %brif_next274 ], [ %303, %blk_exit271 ]
  %302 = icmp eq i32 %storemerge349, 0
  br i1 %302, label %blk_exit, label %brif_next276

blk_exit271:                                      ; preds = %brif_next269
  %303 = call i32 @rpl_malloc(i32 %299)
  br label %blk_exit270

brif_next274:                                     ; preds = %brif_next269
  %304 = call i32 @rpl_realloc(i32 %300, i32 %299)
  br label %blk_exit270

brif_next276:                                     ; preds = %blk_exit270
  %305 = load i32, i32* %5, align 4
  %.not350 = icmp eq i32 %305, %4
  br i1 %.not350, label %brif_next280, label %blk_exit277

blk_exit277:                                      ; preds = %brif_next280, %brif_next276
  store i32 %storemerge349, i32* %5, align 4
  br label %blk_exit263

brif_next280:                                     ; preds = %brif_next276
  %306 = load i32, i32* %1, align 4
  %307 = shl i32 %306, 5
  %308 = call i32 @memcpy(i32 %storemerge349, i32 %4, i32 %307)
  br label %blk_exit277

blk_exit284:                                      ; preds = %brif_next291, %blk_exit263
  %309 = shl i32 %_local_12.1, 5
  %310 = add i32 %290, %309
  %311 = inttoptr i32 %310 to i32*
  %312 = load i32, i32* %311, align 4
  %brif_val295.not = icmp eq i32 %312, 0
  br i1 %brif_val295.not, label %brif_next296, label %blk_exit293

brif_next287:                                     ; preds = %blk_exit263
  %313 = shl i32 %291, 5
  %314 = add i32 %290, %313
  br label %loop_entry288

loop_entry288:                                    ; preds = %loop_entry288, %brif_next287
  %_local_5.11 = phi i32 [ %291, %brif_next287 ], [ %317, %loop_entry288 ]
  %_param_0.12 = phi i32 [ %314, %brif_next287 ], [ %316, %loop_entry288 ]
  %315 = inttoptr i32 %_param_0.12 to i32*
  store i32 0, i32* %315, align 4
  %316 = add i32 %_param_0.12, 32
  %317 = add i32 %_local_5.11, 1
  %.not351 = icmp ugt i32 %317, %_local_12.1
  br i1 %.not351, label %brif_next291, label %loop_entry288

brif_next291:                                     ; preds = %loop_entry288
  store i32 %317, i32* %1, align 4
  br label %blk_exit284

blk_exit293:                                      ; preds = %blk_exit284
  %.not354 = icmp eq i32 %312, %_local_11.7
  br i1 %.not354, label %blk_exit242, label %blk_exit4

brif_next296:                                     ; preds = %blk_exit284
  store i32 %_local_11.7, i32* %311, align 4
  br label %blk_exit242

brif_next305:                                     ; preds = %blk_exit242
  %318 = shl i32 %_local_8.0, 1
  %select_cond306.not.inv = icmp sgt i32 %_local_8.0, -1
  %319 = select i1 %select_cond306.not.inv, i32 %318, i32 -1
  %320 = icmp ugt i32 %319, 97612893
  br i1 %320, label %blk_exit, label %brif_next308

brif_next308:                                     ; preds = %brif_next305
  %321 = mul i32 %319, 44
  %322 = load i32, i32* %3, align 4
  %323 = icmp eq i32 %322, %2
  br i1 %323, label %blk_exit310, label %brif_next313

blk_exit309:                                      ; preds = %brif_next313, %blk_exit310
  %storemerge352 = phi i32 [ %326, %brif_next313 ], [ %325, %blk_exit310 ]
  %324 = icmp eq i32 %storemerge352, 0
  br i1 %324, label %blk_exit, label %brif_next315

blk_exit310:                                      ; preds = %brif_next308
  %325 = call i32 @rpl_malloc(i32 %321)
  br label %blk_exit309

brif_next313:                                     ; preds = %brif_next308
  %326 = call i32 @rpl_realloc(i32 %322, i32 %321)
  br label %blk_exit309

brif_next315:                                     ; preds = %blk_exit309
  %327 = load i32, i32* %3, align 4
  %.not353 = icmp eq i32 %327, %2
  br i1 %.not353, label %brif_next319, label %blk_exit316

blk_exit316:                                      ; preds = %brif_next319, %brif_next315
  store i32 %storemerge352, i32* %3, align 4
  br label %blk_exit8

brif_next319:                                     ; preds = %brif_next315
  %328 = load i32, i32* %0, align 4
  %329 = mul i32 %328, 44
  %330 = call i32 @memcpy(i32 %storemerge352, i32 %2, i32 %329)
  br label %blk_exit316

brif_next324:                                     ; preds = %blk_exit8
  %331 = load i32, i32* %0, align 4
  %332 = load i32, i32* %3, align 4
  %phi.bo = mul i32 %331, 44
  br label %blk_exit5

blk_exit330:                                      ; preds = %brif_next333, %blk_exit4
  %333 = load i32, i32* %3, align 4
  %334 = icmp eq i32 %333, %2
  br i1 %334, label %blk_exit334, label %brif_next337

brif_next333:                                     ; preds = %blk_exit4
  call void @rpl_free(i32 %10)
  br label %blk_exit330

blk_exit334:                                      ; preds = %brif_next337, %blk_exit330
  %335 = call i32 @__errno_location()
  %336 = inttoptr i32 %335 to i32*
  store i32 22, i32* %336, align 4
  br label %return

brif_next337:                                     ; preds = %blk_exit330
  call void @rpl_free(i32 %333)
  br label %blk_exit334

blk_exit339:                                      ; preds = %brif_next342, %blk_exit
  %337 = load i32, i32* %3, align 4
  %338 = icmp eq i32 %337, %2
  br i1 %338, label %blk_exit343, label %brif_next346

brif_next342:                                     ; preds = %blk_exit
  call void @rpl_free(i32 %8)
  br label %blk_exit339

blk_exit343:                                      ; preds = %brif_next346, %blk_exit339
  %339 = call i32 @__errno_location()
  %340 = inttoptr i32 %339 to i32*
  store i32 12, i32* %340, align 4
  br label %return

brif_next346:                                     ; preds = %blk_exit339
  call void @rpl_free(i32 %337)
  br label %blk_exit343
}

; Function Attrs: null_pointer_is_valid
define internal i32 @printf_fetchargs(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %0 = inttoptr i32 %_arg_1 to i32*
  %1 = load i32, i32* %0, align 4
  %2 = icmp eq i32 %1, 0
  br i1 %2, label %blk_exit, label %brif_next

blk_exit:                                         ; preds = %blk_exit3, %loop_entry, %allocator
  %_local_4.0 = phi i32 [ -1, %loop_entry ], [ 0, %blk_exit3 ], [ 0, %allocator ]
  ret i32 %_local_4.0

brif_next:                                        ; preds = %allocator
  %calcOffset2 = add i32 %_arg_1, 4
  %3 = inttoptr i32 %calcOffset2 to i32*
  %4 = load i32, i32* %3, align 4
  br label %loop_entry

loop_entry:                                       ; preds = %blk_exit3, %brif_next
  %_local_3.0 = phi i32 [ 0, %brif_next ], [ %8, %blk_exit3 ]
  %_local_2.0 = phi i32 [ %4, %brif_next ], [ %7, %blk_exit3 ]
  %_param_0.0 = phi i32 [ %_arg_0, %brif_next ], [ %_param_0.1, %blk_exit3 ]
  %5 = inttoptr i32 %_local_2.0 to i32*
  %6 = load i32, i32* %5, align 4
  switch i32 %6, label %blk_exit [
    i32 1, label %blk_exit49
    i32 2, label %blk_exit48
    i32 3, label %blk_exit47
    i32 4, label %blk_exit46
    i32 5, label %blk_exit45
    i32 6, label %blk_exit44
    i32 7, label %blk_exit43
    i32 8, label %blk_exit42
    i32 9, label %blk_exit41
    i32 10, label %blk_exit40
    i32 11, label %blk_exit39
    i32 12, label %blk_exit38
    i32 13, label %blk_exit37
    i32 14, label %blk_exit36
    i32 15, label %blk_exit35
    i32 16, label %blk_exit34
    i32 17, label %blk_exit33
    i32 18, label %blk_exit32
    i32 19, label %blk_exit31
    i32 20, label %blk_exit30
    i32 21, label %blk_exit29
    i32 22, label %blk_exit28
    i32 23, label %blk_exit27
    i32 24, label %blk_exit26
    i32 25, label %blk_exit25
    i32 26, label %blk_exit24
    i32 27, label %blk_exit23
    i32 28, label %blk_exit22
    i32 29, label %blk_exit21
    i32 30, label %blk_exit20
    i32 31, label %blk_exit19
    i32 32, label %blk_exit18
    i32 33, label %blk_exit17
    i32 34, label %blk_exit16
    i32 35, label %blk_exit15
    i32 36, label %blk_exit14
    i32 37, label %blk_exit13
    i32 38, label %blk_exit12
    i32 39, label %blk_exit11
    i32 40, label %blk_exit10
    i32 41, label %blk_exit9
    i32 42, label %blk_exit8
    i32 43, label %blk_exit7
    i32 44, label %blk_exit6
    i32 45, label %blk_exit5
    i32 46, label %blk_exit4
  ]

blk_exit3:                                        ; preds = %blk_exit49, %blk_exit48, %blk_exit47, %blk_exit46, %blk_exit45, %blk_exit44, %blk_exit43, %blk_exit42, %blk_exit41, %blk_exit40, %blk_exit39, %blk_exit38, %blk_exit37, %blk_exit36, %blk_exit35, %blk_exit34, %blk_exit33, %blk_exit32, %blk_exit31, %blk_exit30, %blk_exit29, %blk_exit28, %blk_exit27, %blk_exit26, %blk_exit25, %blk_exit24, %blk_exit23, %blk_exit22, %blk_exit21, %blk_exit20, %blk_exit19, %blk_exit18, %blk_exit17, %blk_exit16, %blk_exit15, %blk_exit14, %blk_exit13, %blk_exit12, %blk_exit11, %blk_exit10, %blk_exit9, %blk_exit8, %blk_exit7, %blk_exit6, %blk_exit5, %blk_exit4
  %_param_0.1 = phi i32 [ %15, %blk_exit4 ], [ %20, %blk_exit5 ], [ %25, %blk_exit6 ], [ %30, %blk_exit7 ], [ %35, %blk_exit8 ], [ %40, %blk_exit9 ], [ %45, %blk_exit10 ], [ %50, %blk_exit11 ], [ %55, %blk_exit12 ], [ %60, %blk_exit13 ], [ %65, %blk_exit14 ], [ %70, %blk_exit15 ], [ %75, %blk_exit16 ], [ %80, %blk_exit17 ], [ %86, %blk_exit18 ], [ %92, %blk_exit19 ], [ %97, %blk_exit20 ], [ %102, %blk_exit21 ], [ %113, %blk_exit22 ], [ %120, %blk_exit23 ], [ %127, %blk_exit24 ], [ %134, %blk_exit25 ], [ %139, %blk_exit26 ], [ %144, %blk_exit27 ], [ %149, %blk_exit28 ], [ %154, %blk_exit29 ], [ %160, %blk_exit30 ], [ %166, %blk_exit31 ], [ %173, %blk_exit32 ], [ %180, %blk_exit33 ], [ %185, %blk_exit34 ], [ %190, %blk_exit35 ], [ %196, %blk_exit36 ], [ %202, %blk_exit37 ], [ %208, %blk_exit38 ], [ %214, %blk_exit39 ], [ %221, %blk_exit40 ], [ %228, %blk_exit41 ], [ %233, %blk_exit42 ], [ %238, %blk_exit43 ], [ %243, %blk_exit44 ], [ %248, %blk_exit45 ], [ %254, %blk_exit46 ], [ %260, %blk_exit47 ], [ %266, %blk_exit48 ], [ %272, %blk_exit49 ]
  %7 = add i32 %_local_2.0, 32
  %8 = add i32 %_local_3.0, 1
  %9 = load i32, i32* %0, align 4
  %10 = icmp ult i32 %8, %9
  br i1 %10, label %loop_entry, label %blk_exit

blk_exit4:                                        ; preds = %loop_entry
  %11 = add i32 %_local_2.0, 16
  %12 = inttoptr i32 %_param_0.0 to i32*
  %13 = load i32, i32* %12, align 4
  %14 = inttoptr i32 %11 to i32*
  store i32 %13, i32* %14, align 4
  %15 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit5:                                        ; preds = %loop_entry
  %16 = add i32 %_local_2.0, 16
  %17 = inttoptr i32 %_param_0.0 to i32*
  %18 = load i32, i32* %17, align 4
  %19 = inttoptr i32 %16 to i32*
  store i32 %18, i32* %19, align 4
  %20 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit6:                                        ; preds = %loop_entry
  %21 = add i32 %_local_2.0, 16
  %22 = inttoptr i32 %_param_0.0 to i32*
  %23 = load i32, i32* %22, align 4
  %24 = inttoptr i32 %21 to i32*
  store i32 %23, i32* %24, align 4
  %25 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit7:                                        ; preds = %loop_entry
  %26 = add i32 %_local_2.0, 16
  %27 = inttoptr i32 %_param_0.0 to i32*
  %28 = load i32, i32* %27, align 4
  %29 = inttoptr i32 %26 to i32*
  store i32 %28, i32* %29, align 4
  %30 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit8:                                        ; preds = %loop_entry
  %31 = add i32 %_local_2.0, 16
  %32 = inttoptr i32 %_param_0.0 to i32*
  %33 = load i32, i32* %32, align 4
  %34 = inttoptr i32 %31 to i32*
  store i32 %33, i32* %34, align 4
  %35 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit9:                                        ; preds = %loop_entry
  %36 = add i32 %_local_2.0, 16
  %37 = inttoptr i32 %_param_0.0 to i32*
  %38 = load i32, i32* %37, align 4
  %39 = inttoptr i32 %36 to i32*
  store i32 %38, i32* %39, align 4
  %40 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit10:                                       ; preds = %loop_entry
  %41 = add i32 %_local_2.0, 16
  %42 = inttoptr i32 %_param_0.0 to i32*
  %43 = load i32, i32* %42, align 4
  %44 = inttoptr i32 %41 to i32*
  store i32 %43, i32* %44, align 4
  %45 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit11:                                       ; preds = %loop_entry
  %46 = add i32 %_local_2.0, 16
  %47 = inttoptr i32 %_param_0.0 to i32*
  %48 = load i32, i32* %47, align 4
  %49 = inttoptr i32 %46 to i32*
  store i32 %48, i32* %49, align 4
  %50 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit12:                                       ; preds = %loop_entry
  %51 = add i32 %_local_2.0, 16
  %52 = inttoptr i32 %_param_0.0 to i32*
  %53 = load i32, i32* %52, align 4
  %54 = inttoptr i32 %51 to i32*
  store i32 %53, i32* %54, align 4
  %55 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit13:                                       ; preds = %loop_entry
  %56 = add i32 %_local_2.0, 16
  %57 = inttoptr i32 %_param_0.0 to i32*
  %58 = load i32, i32* %57, align 4
  %59 = inttoptr i32 %56 to i32*
  store i32 %58, i32* %59, align 4
  %60 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit14:                                       ; preds = %loop_entry
  %61 = add i32 %_local_2.0, 16
  %62 = inttoptr i32 %_param_0.0 to i32*
  %63 = load i32, i32* %62, align 4
  %64 = inttoptr i32 %61 to i32*
  store i32 %63, i32* %64, align 4
  %65 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit15:                                       ; preds = %loop_entry
  %66 = add i32 %_local_2.0, 16
  %67 = inttoptr i32 %_param_0.0 to i32*
  %68 = load i32, i32* %67, align 4
  %69 = inttoptr i32 %66 to i32*
  store i32 %68, i32* %69, align 4
  %70 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit16:                                       ; preds = %loop_entry
  %71 = add i32 %_local_2.0, 16
  %72 = inttoptr i32 %_param_0.0 to i32*
  %73 = load i32, i32* %72, align 4
  %74 = inttoptr i32 %71 to i32*
  store i32 %73, i32* %74, align 4
  %75 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit17:                                       ; preds = %loop_entry
  %76 = add i32 %_local_2.0, 16
  %77 = inttoptr i32 %_param_0.0 to i32*
  %78 = load i32, i32* %77, align 4
  %79 = inttoptr i32 %76 to i32*
  store i32 %78, i32* %79, align 4
  %80 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit18:                                       ; preds = %loop_entry
  %81 = add i32 %_local_2.0, 16
  %82 = inttoptr i32 %_param_0.0 to i32*
  %83 = load i32, i32* %82, align 4
  %select_cond116.not = icmp eq i32 %83, 0
  %84 = select i1 %select_cond116.not, i32 14848, i32 %83
  %85 = inttoptr i32 %81 to i32*
  store i32 %84, i32* %85, align 4
  %86 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit19:                                       ; preds = %loop_entry
  %87 = add i32 %_local_2.0, 16
  %88 = inttoptr i32 %_param_0.0 to i32*
  %89 = load i32, i32* %88, align 4
  %select_cond.not = icmp eq i32 %89, 0
  %90 = select i1 %select_cond.not, i32 3498, i32 %89
  %91 = inttoptr i32 %87 to i32*
  store i32 %90, i32* %91, align 4
  %92 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit20:                                       ; preds = %loop_entry
  %93 = add i32 %_local_2.0, 16
  %94 = inttoptr i32 %_param_0.0 to i32*
  %95 = load i32, i32* %94, align 4
  %96 = inttoptr i32 %93 to i32*
  store i32 %95, i32* %96, align 4
  %97 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit21:                                       ; preds = %loop_entry
  %98 = add i32 %_local_2.0, 16
  %99 = inttoptr i32 %_param_0.0 to i32*
  %100 = load i32, i32* %99, align 4
  %101 = inttoptr i32 %98 to i32*
  store i32 %100, i32* %101, align 4
  %102 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit22:                                       ; preds = %loop_entry
  %103 = add i32 %_local_2.0, 16
  %104 = add i32 %_param_0.0, 15
  %105 = and i32 %104, -16
  %106 = inttoptr i32 %105 to i64*
  %107 = load i64, i64* %106, align 16
  %108 = inttoptr i32 %103 to i64*
  store i64 %107, i64* %108, align 8
  %109 = add i32 %_local_2.0, 24
  %calcOffset107 = or i32 %105, 8
  %110 = inttoptr i32 %calcOffset107 to i64*
  %111 = load i64, i64* %110, align 8
  %112 = inttoptr i32 %109 to i64*
  store i64 %111, i64* %112, align 8
  %113 = add i32 %105, 16
  br label %blk_exit3

blk_exit23:                                       ; preds = %loop_entry
  %114 = add i32 %_local_2.0, 16
  %115 = add i32 %_param_0.0, 7
  %116 = and i32 %115, -8
  %117 = inttoptr i32 %116 to double*
  %118 = load double, double* %117, align 8
  %119 = inttoptr i32 %114 to double*
  store double %118, double* %119, align 8
  %120 = add i32 %116, 8
  br label %blk_exit3

blk_exit24:                                       ; preds = %loop_entry
  %121 = add i32 %_local_2.0, 16
  %122 = add i32 %_param_0.0, 7
  %123 = and i32 %122, -8
  %124 = inttoptr i32 %123 to i64*
  %125 = load i64, i64* %124, align 8
  %126 = inttoptr i32 %121 to i64*
  store i64 %125, i64* %126, align 8
  %127 = add i32 %123, 8
  br label %blk_exit3

blk_exit25:                                       ; preds = %loop_entry
  %128 = add i32 %_local_2.0, 16
  %129 = add i32 %_param_0.0, 7
  %130 = and i32 %129, -8
  %131 = inttoptr i32 %130 to i64*
  %132 = load i64, i64* %131, align 8
  %133 = inttoptr i32 %128 to i64*
  store i64 %132, i64* %133, align 8
  %134 = add i32 %130, 8
  br label %blk_exit3

blk_exit26:                                       ; preds = %loop_entry
  %135 = add i32 %_local_2.0, 16
  %136 = inttoptr i32 %_param_0.0 to i32*
  %137 = load i32, i32* %136, align 4
  %138 = inttoptr i32 %135 to i32*
  store i32 %137, i32* %138, align 4
  %139 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit27:                                       ; preds = %loop_entry
  %140 = add i32 %_local_2.0, 16
  %141 = inttoptr i32 %_param_0.0 to i32*
  %142 = load i32, i32* %141, align 4
  %143 = inttoptr i32 %140 to i32*
  store i32 %142, i32* %143, align 4
  %144 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit28:                                       ; preds = %loop_entry
  %145 = add i32 %_local_2.0, 16
  %146 = inttoptr i32 %_param_0.0 to i32*
  %147 = load i32, i32* %146, align 4
  %148 = inttoptr i32 %145 to i32*
  store i32 %147, i32* %148, align 4
  %149 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit29:                                       ; preds = %loop_entry
  %150 = add i32 %_local_2.0, 16
  %151 = inttoptr i32 %_param_0.0 to i32*
  %152 = load i32, i32* %151, align 4
  %153 = inttoptr i32 %150 to i32*
  store i32 %152, i32* %153, align 4
  %154 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit30:                                       ; preds = %loop_entry
  %155 = add i32 %_local_2.0, 16
  %156 = inttoptr i32 %_param_0.0 to i32*
  %157 = load i32, i32* %156, align 4
  %158 = inttoptr i32 %155 to i8*
  %159 = trunc i32 %157 to i8
  store i8 %159, i8* %158, align 1
  %160 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit31:                                       ; preds = %loop_entry
  %161 = add i32 %_local_2.0, 16
  %162 = inttoptr i32 %_param_0.0 to i32*
  %163 = load i32, i32* %162, align 4
  %164 = inttoptr i32 %161 to i8*
  %165 = trunc i32 %163 to i8
  store i8 %165, i8* %164, align 1
  %166 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit32:                                       ; preds = %loop_entry
  %167 = add i32 %_local_2.0, 16
  %168 = add i32 %_param_0.0, 7
  %169 = and i32 %168, -8
  %170 = inttoptr i32 %169 to i64*
  %171 = load i64, i64* %170, align 8
  %172 = inttoptr i32 %167 to i64*
  store i64 %171, i64* %172, align 8
  %173 = add i32 %169, 8
  br label %blk_exit3

blk_exit33:                                       ; preds = %loop_entry
  %174 = add i32 %_local_2.0, 16
  %175 = add i32 %_param_0.0, 7
  %176 = and i32 %175, -8
  %177 = inttoptr i32 %176 to i64*
  %178 = load i64, i64* %177, align 8
  %179 = inttoptr i32 %174 to i64*
  store i64 %178, i64* %179, align 8
  %180 = add i32 %176, 8
  br label %blk_exit3

blk_exit34:                                       ; preds = %loop_entry
  %181 = add i32 %_local_2.0, 16
  %182 = inttoptr i32 %_param_0.0 to i32*
  %183 = load i32, i32* %182, align 4
  %184 = inttoptr i32 %181 to i32*
  store i32 %183, i32* %184, align 4
  %185 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit35:                                       ; preds = %loop_entry
  %186 = add i32 %_local_2.0, 16
  %187 = inttoptr i32 %_param_0.0 to i32*
  %188 = load i32, i32* %187, align 4
  %189 = inttoptr i32 %186 to i32*
  store i32 %188, i32* %189, align 4
  %190 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit36:                                       ; preds = %loop_entry
  %191 = add i32 %_local_2.0, 16
  %192 = inttoptr i32 %_param_0.0 to i32*
  %193 = load i32, i32* %192, align 4
  %194 = trunc i32 %193 to i16
  %195 = inttoptr i32 %191 to i16*
  store i16 %194, i16* %195, align 2
  %196 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit37:                                       ; preds = %loop_entry
  %197 = add i32 %_local_2.0, 16
  %198 = inttoptr i32 %_param_0.0 to i32*
  %199 = load i32, i32* %198, align 4
  %200 = trunc i32 %199 to i16
  %201 = inttoptr i32 %197 to i16*
  store i16 %200, i16* %201, align 2
  %202 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit38:                                       ; preds = %loop_entry
  %203 = add i32 %_local_2.0, 16
  %204 = inttoptr i32 %_param_0.0 to i32*
  %205 = load i32, i32* %204, align 4
  %206 = inttoptr i32 %203 to i8*
  %207 = trunc i32 %205 to i8
  store i8 %207, i8* %206, align 1
  %208 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit39:                                       ; preds = %loop_entry
  %209 = add i32 %_local_2.0, 16
  %210 = inttoptr i32 %_param_0.0 to i32*
  %211 = load i32, i32* %210, align 4
  %212 = inttoptr i32 %209 to i8*
  %213 = trunc i32 %211 to i8
  store i8 %213, i8* %212, align 1
  %214 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit40:                                       ; preds = %loop_entry
  %215 = add i32 %_local_2.0, 16
  %216 = add i32 %_param_0.0, 7
  %217 = and i32 %216, -8
  %218 = inttoptr i32 %217 to i64*
  %219 = load i64, i64* %218, align 8
  %220 = inttoptr i32 %215 to i64*
  store i64 %219, i64* %220, align 8
  %221 = add i32 %217, 8
  br label %blk_exit3

blk_exit41:                                       ; preds = %loop_entry
  %222 = add i32 %_local_2.0, 16
  %223 = add i32 %_param_0.0, 7
  %224 = and i32 %223, -8
  %225 = inttoptr i32 %224 to i64*
  %226 = load i64, i64* %225, align 8
  %227 = inttoptr i32 %222 to i64*
  store i64 %226, i64* %227, align 8
  %228 = add i32 %224, 8
  br label %blk_exit3

blk_exit42:                                       ; preds = %loop_entry
  %229 = add i32 %_local_2.0, 16
  %230 = inttoptr i32 %_param_0.0 to i32*
  %231 = load i32, i32* %230, align 4
  %232 = inttoptr i32 %229 to i32*
  store i32 %231, i32* %232, align 4
  %233 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit43:                                       ; preds = %loop_entry
  %234 = add i32 %_local_2.0, 16
  %235 = inttoptr i32 %_param_0.0 to i32*
  %236 = load i32, i32* %235, align 4
  %237 = inttoptr i32 %234 to i32*
  store i32 %236, i32* %237, align 4
  %238 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit44:                                       ; preds = %loop_entry
  %239 = add i32 %_local_2.0, 16
  %240 = inttoptr i32 %_param_0.0 to i32*
  %241 = load i32, i32* %240, align 4
  %242 = inttoptr i32 %239 to i32*
  store i32 %241, i32* %242, align 4
  %243 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit45:                                       ; preds = %loop_entry
  %244 = add i32 %_local_2.0, 16
  %245 = inttoptr i32 %_param_0.0 to i32*
  %246 = load i32, i32* %245, align 4
  %247 = inttoptr i32 %244 to i32*
  store i32 %246, i32* %247, align 4
  %248 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit46:                                       ; preds = %loop_entry
  %249 = add i32 %_local_2.0, 16
  %250 = inttoptr i32 %_param_0.0 to i32*
  %251 = load i32, i32* %250, align 4
  %252 = trunc i32 %251 to i16
  %253 = inttoptr i32 %249 to i16*
  store i16 %252, i16* %253, align 2
  %254 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit47:                                       ; preds = %loop_entry
  %255 = add i32 %_local_2.0, 16
  %256 = inttoptr i32 %_param_0.0 to i32*
  %257 = load i32, i32* %256, align 4
  %258 = trunc i32 %257 to i16
  %259 = inttoptr i32 %255 to i16*
  store i16 %258, i16* %259, align 2
  %260 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit48:                                       ; preds = %loop_entry
  %261 = add i32 %_local_2.0, 16
  %262 = inttoptr i32 %_param_0.0 to i32*
  %263 = load i32, i32* %262, align 4
  %264 = inttoptr i32 %261 to i8*
  %265 = trunc i32 %263 to i8
  store i8 %265, i8* %264, align 1
  %266 = add i32 %_param_0.0, 4
  br label %blk_exit3

blk_exit49:                                       ; preds = %loop_entry
  %267 = add i32 %_local_2.0, 16
  %268 = inttoptr i32 %_param_0.0 to i32*
  %269 = load i32, i32* %268, align 4
  %270 = inttoptr i32 %267 to i8*
  %271 = trunc i32 %269 to i8
  store i8 %271, i8* %270, align 1
  %272 = add i32 %_param_0.0, 4
  br label %blk_exit3
}

; Function Attrs: null_pointer_is_valid
define internal i32 @gl_signbitl(i64 %_arg_0, i64 %_arg_1) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -16
  store i32 %1, i32* @__stack_pointer, align 4
  call void @copysignl(i32 %1, i64 0, i64 4611404543450677248, i64 %_arg_0, i64 %_arg_1)
  %2 = inttoptr i32 %1 to i64*
  %3 = load i64, i64* %2, align 8
  %calcOffset1 = add i32 %0, -8
  %4 = inttoptr i32 %calcOffset1 to i64*
  %5 = load i64, i64* %4, align 8
  %6 = call i32 @__lttf2(i64 %3, i64 %5, i64 0, i64 0)
  store i32 %0, i32* @__stack_pointer, align 4
  %.lobit = lshr i32 %6, 31
  ret i32 %.lobit
}

; Function Attrs: null_pointer_is_valid
define internal void @printf_frexpl(i32 %_arg_0, i64 %_arg_1, i64 %_arg_2, i32 %_arg_3) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -64
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = add i32 %0, -32
  %3 = add i32 %0, -4
  call void @frexpl(i32 %2, i64 %_arg_1, i64 %_arg_2, i32 %3)
  %4 = add i32 %0, -48
  %calcOffset = add i32 %0, -32
  %5 = inttoptr i32 %calcOffset to i64*
  %6 = load i64, i64* %5, align 8
  %calcOffset1 = add i32 %0, -24
  %7 = inttoptr i32 %calcOffset1 to i64*
  %8 = load i64, i64* %7, align 8
  call void @__addtf3(i32 %4, i64 %6, i64 %8, i64 %6, i64 %8)
  %calcOffset2 = add i32 %0, -4
  %9 = inttoptr i32 %calcOffset2 to i32*
  %10 = load i32, i32* %9, align 4
  %11 = add i32 %10, -1
  store i32 %11, i32* %9, align 4
  %12 = add i32 %0, -40
  %13 = inttoptr i32 %12 to i64*
  %14 = load i64, i64* %13, align 8
  %15 = inttoptr i32 %4 to i64*
  %16 = load i64, i64* %15, align 8
  %17 = icmp slt i32 %10, -16381
  br i1 %17, label %blk_exit6, label %blk_exit

blk_exit:                                         ; preds = %blk_exit6, %allocator
  %_param_2.0 = phi i64 [ %25, %blk_exit6 ], [ %16, %allocator ]
  %_param_1.0 = phi i64 [ %23, %blk_exit6 ], [ %14, %allocator ]
  %_local_7.0 = phi i32 [ -16382, %blk_exit6 ], [ %11, %allocator ]
  %18 = inttoptr i32 %_arg_3 to i32*
  store i32 %_local_7.0, i32* %18, align 4
  %calcOffset10 = add i32 %_arg_0, 8
  %19 = inttoptr i32 %calcOffset10 to i64*
  store i64 %_param_1.0, i64* %19, align 8
  %20 = inttoptr i32 %_arg_0 to i64*
  store i64 %_param_2.0, i64* %20, align 8
  store i32 %0, i32* @__stack_pointer, align 4
  ret void

blk_exit6:                                        ; preds = %allocator
  %21 = add i32 %10, 16381
  call void @ldexpl(i32 %1, i64 %16, i64 %14, i32 %21)
  %calcOffset7 = add i32 %0, -56
  %22 = inttoptr i32 %calcOffset7 to i64*
  %23 = load i64, i64* %22, align 8
  %24 = inttoptr i32 %1 to i64*
  %25 = load i64, i64* %24, align 8
  br label %blk_exit
}

; Function Attrs: null_pointer_is_valid
define internal i32 @gl_signbitd(double %_arg_0) #0 {
allocator:
  %0 = call double @copysign(double 1.000000e+00, double %_arg_0)
  %1 = fcmp olt double %0, 0.000000e+00
  %2 = zext i1 %1 to i32
  ret i32 %2
}

; Function Attrs: null_pointer_is_valid
define internal double @printf_frexp(double %_arg_0, i32 %_arg_1) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -16
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = add i32 %0, -4
  %3 = call double @frexp(double %_arg_0, i32 %2)
  %4 = inttoptr i32 %2 to i32*
  %5 = load i32, i32* %4, align 4
  %6 = add i32 %5, -1
  store i32 %6, i32* %4, align 4
  %7 = fadd double %3, %3
  %8 = icmp slt i32 %5, -1021
  br i1 %8, label %blk_exit2, label %blk_exit

blk_exit:                                         ; preds = %blk_exit2, %allocator
  %_local_5.0 = phi i32 [ -1022, %blk_exit2 ], [ %6, %allocator ]
  %_param_0.0 = phi double [ %11, %blk_exit2 ], [ %7, %allocator ]
  %9 = inttoptr i32 %_arg_1 to i32*
  store i32 %_local_5.0, i32* %9, align 4
  store i32 %0, i32* @__stack_pointer, align 4
  ret double %_param_0.0

blk_exit2:                                        ; preds = %allocator
  %10 = add i32 %5, 1021
  %11 = call double @ldexp(double %7, i32 %10)
  br label %blk_exit
}

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @rpl_vasnprintf(i32, i32, i32, i32) #0

; Function Attrs: null_pointer_is_valid
define internal i32 @floorlog10l(i64 %_arg_0, i64 %_arg_1) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -128
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = add i32 %0, -32
  %3 = add i32 %0, -4
  call void @frexpl(i32 %2, i64 %_arg_0, i64 %_arg_1, i32 %3)
  %calcOffset = add i32 %0, -32
  %4 = inttoptr i32 %calcOffset to i64*
  %5 = load i64, i64* %4, align 8
  %calcOffset1 = add i32 %0, -24
  %6 = inttoptr i32 %calcOffset1 to i64*
  %7 = load i64, i64* %6, align 8
  %8 = call i32 @__getf2(i64 %5, i64 %7, i64 0, i64 0)
  %brif_val.not = icmp sgt i32 %8, -1
  br i1 %brif_val.not, label %brif_next, label %blk_exit

blk_exit:                                         ; preds = %brif_next55, %blk_exit7, %brif_next, %allocator
  call void @abort()
  unreachable

brif_next:                                        ; preds = %allocator
  %9 = call i32 @__lttf2(i64 %5, i64 %7, i64 0, i64 4611404543450677248)
  %brif_val2.not = icmp slt i32 %9, 0
  br i1 %brif_val2.not, label %brif_next3, label %blk_exit

brif_next3:                                       ; preds = %brif_next
  %10 = call i32 @__eqtf2(i64 %5, i64 %7, i64 0, i64 0)
  %11 = icmp eq i32 %10, 0
  br i1 %11, label %blk_exit4, label %brif_next6

blk_exit4:                                        ; preds = %brif_next57, %brif_next3
  %_local_3.0 = phi i32 [ -2147483648, %brif_next3 ], [ %124, %brif_next57 ]
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %_local_3.0

brif_next6:                                       ; preds = %brif_next3
  %12 = call i32 @__lttf2(i64 %5, i64 %7, i64 0, i64 4611123068473966592)
  %brif_val8.not = icmp slt i32 %12, 0
  br i1 %brif_val8.not, label %brif_next9, label %blk_exit7

blk_exit7:                                        ; preds = %brif_next46, %blk_exit38, %brif_next6
  %_param_1.0 = phi i64 [ %57, %blk_exit38 ], [ %57, %brif_next46 ], [ %7, %brif_next6 ]
  %_param_0.0 = phi i64 [ %60, %blk_exit38 ], [ %60, %brif_next46 ], [ %5, %brif_next6 ]
  %13 = call i32 @__getf2(i64 %_param_0.0, i64 %_param_1.0, i64 0, i64 4611123068473966592)
  %brif_val54.not = icmp sgt i32 %13, -1
  br i1 %brif_val54.not, label %brif_next55, label %blk_exit

brif_next9:                                       ; preds = %brif_next6
  %14 = call i32 @__lttf2(i64 %5, i64 %7, i64 0, i64 4602397344195936256)
  %calcOffset10 = add i32 %0, -4
  %15 = inttoptr i32 %calcOffset10 to i32*
  %16 = load i32, i32* %15, align 4
  %brif_val12.not = icmp slt i32 %14, 0
  br i1 %brif_val12.not, label %brif_next13, label %blk_exit11

blk_exit11:                                       ; preds = %brif_next17, %brif_next9
  %_local_3.1 = phi i32 [ %63, %brif_next17 ], [ %16, %brif_next9 ]
  %_param_1.1 = phi i64 [ %67, %brif_next17 ], [ %7, %brif_next9 ]
  %_param_0.1 = phi i64 [ %65, %brif_next17 ], [ %5, %brif_next9 ]
  %17 = add i32 %0, -64
  call void @__multf3(i32 %17, i64 %_param_0.1, i64 %_param_1.1, i64 0, i64 4615908143078047744)
  %18 = call i32 @__lttf2(i64 %_param_0.1, i64 %_param_1.1, i64 0, i64 4606900943823306752)
  %19 = add i32 %0, -80
  %20 = inttoptr i32 %17 to i64*
  %21 = load i64, i64* %20, align 8
  %select_cond.not71 = icmp slt i32 %18, 0
  %22 = select i1 %select_cond.not71, i64 %21, i64 %_param_0.1
  %23 = add i32 %0, -56
  %24 = inttoptr i32 %23 to i64*
  %25 = load i64, i64* %24, align 8
  %select_cond21.not = icmp sgt i32 %18, -1
  %26 = select i1 %select_cond21.not, i64 %_param_1.1, i64 %25
  call void @__multf3(i32 %19, i64 %22, i64 %26, i64 0, i64 4613656343264362496)
  %27 = call i32 @__lttf2(i64 %22, i64 %26, i64 0, i64 4609152743636992000)
  %28 = add i32 %0, -96
  %29 = inttoptr i32 %19 to i64*
  %30 = load i64, i64* %29, align 8
  %select_cond23.not73 = icmp slt i32 %27, 0
  %31 = select i1 %select_cond23.not73, i64 %30, i64 %22
  %32 = add i32 %0, -72
  %33 = inttoptr i32 %32 to i64*
  %34 = load i64, i64* %33, align 8
  %select_cond25.not = icmp sgt i32 %27, -1
  %35 = select i1 %select_cond25.not, i64 %26, i64 %34
  call void @__multf3(i32 %28, i64 %31, i64 %35, i64 0, i64 4612530443357519872)
  %36 = call i32 @__lttf2(i64 %31, i64 %35, i64 0, i64 4610278643543834624)
  %37 = add i32 %0, -112
  %38 = inttoptr i32 %28 to i64*
  %39 = load i64, i64* %38, align 8
  %select_cond27.not75 = icmp slt i32 %36, 0
  %40 = select i1 %select_cond27.not75, i64 %39, i64 %31
  %41 = add i32 %0, -88
  %42 = inttoptr i32 %41 to i64*
  %43 = load i64, i64* %42, align 8
  %select_cond29.not = icmp sgt i32 %36, -1
  %44 = select i1 %select_cond29.not, i64 %35, i64 %43
  call void @__multf3(i32 %37, i64 %40, i64 %44, i64 0, i64 4611967493404098560)
  %45 = call i32 @__lttf2(i64 %40, i64 %44, i64 0, i64 4610841593497255936)
  %46 = inttoptr i32 %37 to i64*
  %47 = load i64, i64* %46, align 8
  %select_cond31.not77 = icmp slt i32 %45, 0
  %48 = select i1 %select_cond31.not77, i64 %47, i64 %40
  %49 = add i32 %0, -104
  %50 = inttoptr i32 %49 to i64*
  %51 = load i64, i64* %50, align 8
  %select_cond33.not89 = icmp slt i32 %45, 0
  %52 = select i1 %select_cond33.not89, i64 %51, i64 %44
  call void @__addtf3(i32 %1, i64 %48, i64 %52, i64 %48, i64 %52)
  %53 = call i32 @__lttf2(i64 %48, i64 %52, i64 0, i64 4611123068473966592)
  %54 = add i32 %0, -120
  %55 = inttoptr i32 %54 to i64*
  %56 = load i64, i64* %55, align 8
  %select_cond35.not79 = icmp slt i32 %53, 0
  %57 = select i1 %select_cond35.not79, i64 %56, i64 %52
  %58 = inttoptr i32 %1 to i64*
  %59 = load i64, i64* %58, align 8
  %select_cond37.not90 = icmp slt i32 %53, 0
  %60 = select i1 %select_cond37.not90, i64 %59, i64 %48
  br i1 %select_cond21.not, label %brif_next40, label %blk_exit38

brif_next13:                                      ; preds = %brif_next9
  %61 = add i32 %0, -40
  br label %loop_entry

loop_entry:                                       ; preds = %loop_entry, %brif_next13
  %_local_3.2 = phi i32 [ %16, %brif_next13 ], [ %63, %loop_entry ]
  %_param_1.2 = phi i64 [ %7, %brif_next13 ], [ %67, %loop_entry ]
  %_param_0.2 = phi i64 [ %5, %brif_next13 ], [ %65, %loop_entry ]
  %62 = add i32 %0, -48
  call void @__multf3(i32 %62, i64 %_param_0.2, i64 %_param_1.2, i64 0, i64 4620411742705418240)
  %63 = add i32 %_local_3.2, -32
  %64 = inttoptr i32 %62 to i64*
  %65 = load i64, i64* %64, align 8
  %66 = inttoptr i32 %61 to i64*
  %67 = load i64, i64* %66, align 8
  %68 = call i32 @__lttf2(i64 %65, i64 %67, i64 0, i64 4602397344195936256)
  %brif_val16.not = icmp sgt i32 %68, -1
  br i1 %brif_val16.not, label %brif_next17, label %loop_entry

brif_next17:                                      ; preds = %loop_entry
  store i32 %63, i32* %15, align 4
  br label %blk_exit11

blk_exit38:                                       ; preds = %brif_next46, %brif_next44, %brif_next42, %brif_next40, %blk_exit11
  %69 = add i32 %_local_3.1, -16
  %70 = select i1 %select_cond21.not, i32 %_local_3.1, i32 %69
  %71 = add i32 %70, -8
  %72 = select i1 %select_cond25.not, i32 %70, i32 %71
  %73 = add i32 %72, -4
  %74 = select i1 %select_cond29.not, i32 %72, i32 %73
  %75 = add i32 %74, -2
  %76 = call i32 @__lttf2(i64 %40, i64 %44, i64 0, i64 4610841593497255936)
  %select_cond52.not85 = icmp slt i32 %76, 0
  %77 = select i1 %select_cond52.not85, i32 %75, i32 %74
  %78 = call i32 @__lttf2(i64 %48, i64 %52, i64 0, i64 4611123068473966592)
  %.lobit86.neg = ashr i32 %78, 31
  %79 = add i32 %.lobit86.neg, %77
  store i32 %79, i32* %15, align 4
  br label %blk_exit7

brif_next40:                                      ; preds = %blk_exit11
  %80 = call i32 @__lttf2(i64 %22, i64 %26, i64 0, i64 4609152743636992000)
  %brif_val41.not = icmp sgt i32 %80, -1
  br i1 %brif_val41.not, label %brif_next42, label %blk_exit38

brif_next42:                                      ; preds = %brif_next40
  %81 = call i32 @__lttf2(i64 %31, i64 %35, i64 0, i64 4610278643543834624)
  %brif_val43.not = icmp sgt i32 %81, -1
  br i1 %brif_val43.not, label %brif_next44, label %blk_exit38

brif_next44:                                      ; preds = %brif_next42
  %82 = call i32 @__lttf2(i64 %40, i64 %44, i64 0, i64 4610841593497255936)
  %brif_val45.not = icmp sgt i32 %82, -1
  br i1 %brif_val45.not, label %brif_next46, label %blk_exit38

brif_next46:                                      ; preds = %brif_next44
  %83 = call i32 @__lttf2(i64 %48, i64 %52, i64 0, i64 4611123068473966592)
  %brif_val47.not = icmp slt i32 %83, 0
  br i1 %brif_val47.not, label %blk_exit38, label %blk_exit7

brif_next55:                                      ; preds = %blk_exit7
  %84 = call i32 @__lttf2(i64 %_param_0.0, i64 %_param_1.0, i64 0, i64 4611404543450677248)
  %brif_val56.not = icmp slt i32 %84, 0
  br i1 %brif_val56.not, label %brif_next57, label %blk_exit

brif_next57:                                      ; preds = %brif_next55
  %85 = call double @__trunctfdf2(i64 %_param_0.0, i64 %_param_1.0)
  %86 = fmul double %85, 0x3FF6A09E667F3BCD
  %87 = fcmp olt double %85, 0x3FE6A09E667F3BCD
  %88 = select i1 %87, double %86, double %85
  %89 = fmul double %88, 0x3FF306FE0A31B715
  %90 = fcmp olt double %88, 0x3FEAE89F995AD3AD
  %91 = select i1 %90, double %89, double %88
  %92 = fmul double %91, 0x3FF172B83C7D517B
  %93 = fcmp olt double %91, 0x3FED5818DCFBA487
  %94 = select i1 %93, double %92, double %91
  %95 = fmul double %94, 0x3FF0B5586CF9890F
  %96 = fcmp olt double %94, 0x3FEEA4AFA2A490DA
  %97 = select i1 %96, double %95, double %94
  %98 = fsub double 1.000000e+00, %97
  %99 = fmul double %98, 0xBFF71547652B82FE
  %100 = fmul double %98, 2.500000e-01
  %101 = fadd double %100, 0x3FD5555555555555
  %102 = fmul double %98, %101
  %103 = fadd double %102, 5.000000e-01
  %104 = fmul double %98, %103
  %105 = fadd double %104, 1.000000e+00
  %106 = fmul double %99, %105
  %calcOffset62 = add i32 %0, -4
  %107 = inttoptr i32 %calcOffset62 to i32*
  %108 = load i32, i32* %107, align 4
  %109 = sitofp i32 %108 to double
  %110 = fadd double %109, -5.000000e-01
  %111 = select i1 %87, double %110, double %109
  %112 = fadd double %111, -2.500000e-01
  %113 = select i1 %90, double %112, double %111
  %114 = fadd double %113, -1.250000e-01
  %115 = select i1 %93, double %114, double %113
  %116 = fadd double %115, -6.250000e-02
  %117 = select i1 %96, double %116, double %115
  %118 = fadd double %106, %117
  %119 = fmul double %118, 0x3FD34413509F79FF
  %120 = fcmp olt double %119, 0.000000e+00
  %.neg = sext i1 %120 to i32
  %121 = call double @llvm.fabs.f64(double %119)
  %122 = fcmp uge double %121, 0x41E0000000000000
  %123 = fptosi double %119 to i32
  %storemerge = select i1 %122, i32 -2147483648, i32 %123
  %124 = add i32 %storemerge, %.neg
  br label %blk_exit4
}

; Function Attrs: null_pointer_is_valid
define internal i32 @floorlog10(double %_arg_0) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -16
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = add i32 %0, -4
  %3 = call double @frexp(double %_arg_0, i32 %2)
  %4 = fcmp ult double %3, 0.000000e+00
  %5 = fcmp uge double %3, 1.000000e+00
  %or.cond = or i1 %4, %5
  br i1 %or.cond, label %blk_exit, label %brif_next2

blk_exit:                                         ; preds = %blk_exit6, %allocator
  call void @abort()
  unreachable

brif_next2:                                       ; preds = %allocator
  %6 = fcmp oeq double %3, 0.000000e+00
  br i1 %6, label %blk_exit3, label %brif_next5

blk_exit3:                                        ; preds = %brif_next38, %brif_next2
  %_local_2.0 = phi i32 [ -2147483648, %brif_next2 ], [ %82, %brif_next38 ]
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %_local_2.0

brif_next5:                                       ; preds = %brif_next2
  %7 = fcmp uge double %3, 5.000000e-01
  br i1 %7, label %blk_exit6, label %brif_next8

blk_exit6:                                        ; preds = %blk_exit19, %blk_exit9, %brif_next5
  %_param_0.0 = phi double [ %3, %brif_next5 ], [ %27, %blk_exit9 ], [ %27, %blk_exit19 ]
  %8 = fcmp ult double %_param_0.0, 5.000000e-01
  %9 = fcmp uge double %_param_0.0, 1.000000e+00
  %or.cond59 = or i1 %8, %9
  br i1 %or.cond59, label %blk_exit, label %brif_next38

brif_next8:                                       ; preds = %brif_next5
  %calcOffset = add i32 %0, -4
  %10 = inttoptr i32 %calcOffset to i32*
  %11 = load i32, i32* %10, align 4
  %12 = fcmp uge double %3, 0x3DF0000000000000
  br i1 %12, label %blk_exit9, label %loop_entry

blk_exit9:                                        ; preds = %brif_next13, %brif_next8
  %_local_2.1 = phi i32 [ %11, %brif_next8 ], [ %32, %brif_next13 ]
  %_param_0.1 = phi double [ %3, %brif_next8 ], [ %33, %brif_next13 ]
  %13 = fmul double %_param_0.1, 6.553600e+04
  %14 = fcmp olt double %_param_0.1, 0x3EF0000000000000
  %15 = select i1 %14, double %13, double %_param_0.1
  %16 = fmul double %15, 2.560000e+02
  %17 = fcmp olt double %15, 3.906250e-03
  %18 = select i1 %17, double %16, double %15
  %19 = fmul double %18, 1.600000e+01
  %20 = fcmp olt double %18, 6.250000e-02
  %21 = select i1 %20, double %19, double %18
  %22 = fmul double %21, 4.000000e+00
  %23 = fcmp olt double %21, 2.500000e-01
  %24 = select i1 %23, double %22, double %21
  %25 = fcmp olt double %24, 5.000000e-01
  %26 = select i1 %25, double %24, double -0.000000e+00
  %27 = fadd double %24, %26
  %28 = select i1 %14, i1 true, i1 %17
  %or.cond53 = xor i1 %28, true
  %29 = fcmp uge double %18, 6.250000e-02
  %or.cond55 = select i1 %or.cond53, i1 %29, i1 false
  %30 = fcmp uge double %21, 2.500000e-01
  %or.cond57 = select i1 %or.cond55, i1 %30, i1 false
  %31 = fcmp uge double %24, 5.000000e-01
  %or.cond61 = select i1 %or.cond57, i1 %31, i1 false
  br i1 %or.cond61, label %blk_exit6, label %blk_exit19

loop_entry:                                       ; preds = %loop_entry, %brif_next8
  %_local_2.2 = phi i32 [ %11, %brif_next8 ], [ %32, %loop_entry ]
  %_param_0.2 = phi double [ %3, %brif_next8 ], [ %33, %loop_entry ]
  %32 = add i32 %_local_2.2, -32
  %33 = fmul double %_param_0.2, 0x41F0000000000000
  %34 = fcmp olt double %33, 0x3DF0000000000000
  br i1 %34, label %loop_entry, label %brif_next13

brif_next13:                                      ; preds = %loop_entry
  store i32 %32, i32* %10, align 4
  br label %blk_exit9

blk_exit19:                                       ; preds = %blk_exit9
  %.neg = sext i1 %25 to i32
  %35 = add i32 %_local_2.1, -16
  %36 = select i1 %14, i32 %35, i32 %_local_2.1
  %37 = add i32 %36, -8
  %38 = select i1 %17, i32 %37, i32 %36
  %39 = add i32 %38, -4
  %40 = select i1 %20, i32 %39, i32 %38
  %41 = add i32 %40, -2
  %42 = select i1 %23, i32 %41, i32 %40
  %43 = add i32 %42, %.neg
  store i32 %43, i32* %10, align 4
  br label %blk_exit6

brif_next38:                                      ; preds = %blk_exit6
  %44 = fmul double %_param_0.0, 0x3FF6A09E667F3BCD
  %45 = fcmp olt double %_param_0.0, 0x3FE6A09E667F3BCD
  %46 = select i1 %45, double %44, double %_param_0.0
  %47 = fmul double %46, 0x3FF306FE0A31B715
  %48 = fcmp olt double %46, 0x3FEAE89F995AD3AD
  %49 = select i1 %48, double %47, double %46
  %50 = fmul double %49, 0x3FF172B83C7D517B
  %51 = fcmp olt double %49, 0x3FED5818DCFBA487
  %52 = select i1 %51, double %50, double %49
  %53 = fmul double %52, 0x3FF0B5586CF9890F
  %54 = fcmp olt double %52, 0x3FEEA4AFA2A490DA
  %55 = select i1 %54, double %53, double %52
  %56 = fsub double 1.000000e+00, %55
  %57 = fmul double %56, 0xBFF71547652B82FE
  %58 = fmul double %56, 2.500000e-01
  %59 = fadd double %58, 0x3FD5555555555555
  %60 = fmul double %56, %59
  %61 = fadd double %60, 5.000000e-01
  %62 = fmul double %56, %61
  %63 = fadd double %62, 1.000000e+00
  %64 = fmul double %57, %63
  %calcOffset43 = add i32 %0, -4
  %65 = inttoptr i32 %calcOffset43 to i32*
  %66 = load i32, i32* %65, align 4
  %67 = sitofp i32 %66 to double
  %68 = fadd double %67, -5.000000e-01
  %69 = select i1 %45, double %68, double %67
  %70 = fadd double %69, -2.500000e-01
  %71 = select i1 %48, double %70, double %69
  %72 = fadd double %71, -1.250000e-01
  %73 = select i1 %51, double %72, double %71
  %74 = fadd double %73, -6.250000e-02
  %75 = select i1 %54, double %74, double %73
  %76 = fadd double %64, %75
  %77 = fmul double %76, 0x3FD34413509F79FF
  %78 = fcmp olt double %77, 0.000000e+00
  %.neg62 = sext i1 %78 to i32
  %79 = call double @llvm.fabs.f64(double %77)
  %80 = fcmp uge double %79, 0x41E0000000000000
  %81 = fptosi double %77 to i32
  %storemerge = select i1 %80, i32 -2147483648, i32 %81
  %82 = add i32 %storemerge, %.neg62
  br label %blk_exit3
}

; Function Attrs: null_pointer_is_valid
define internal i32 @scale10_round_decimal_long_double(i64 %_arg_0, i64 %_arg_1, i32 %_arg_2) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -416
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = call i32 @rpl_malloc(i32 16)
  %brif_val.not = icmp eq i32 %2, 0
  br i1 %brif_val.not, label %blk_exit1, label %blk_exit2

blk_exit:                                         ; preds = %brif_next88, %brif_next80, %brif_next78, %brif_next69, %brif_next67, %brif_next59, %brif_next57, %brif_next48, %brif_next46, %brif_next38, %brif_next36, %brif_next27, %brif_next25, %brif_next17, %brif_next15, %brif_next7, %brif_next5, %blk_exit2
  call void @abort()
  unreachable

blk_exit1:                                        ; preds = %blk_exit92, %allocator
  %storemerge = phi i32 [ %193, %blk_exit92 ], [ 0, %allocator ]
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %storemerge

blk_exit2:                                        ; preds = %allocator
  %3 = add i32 %0, -32
  %4 = add i32 %0, -4
  call void @frexpl(i32 %3, i64 %_arg_0, i64 %_arg_1, i32 %4)
  %5 = inttoptr i32 %3 to i64*
  %6 = load i64, i64* %5, align 8
  %calcOffset3 = add i32 %0, -24
  %7 = inttoptr i32 %calcOffset3 to i64*
  %8 = load i64, i64* %7, align 8
  %9 = call i32 @__getf2(i64 %6, i64 %8, i64 0, i64 0)
  %brif_val4.not = icmp sgt i32 %9, -1
  br i1 %brif_val4.not, label %brif_next5, label %blk_exit

brif_next5:                                       ; preds = %blk_exit2
  %10 = call i32 @__lttf2(i64 %6, i64 %8, i64 0, i64 4611404543450677248)
  %brif_val6.not = icmp slt i32 %10, 0
  br i1 %brif_val6.not, label %brif_next7, label %blk_exit

brif_next7:                                       ; preds = %brif_next5
  %11 = add i32 %0, -48
  call void @__addtf3(i32 %11, i64 %6, i64 %8, i64 %6, i64 %8)
  %12 = add i32 %0, -64
  %13 = inttoptr i32 %11 to i64*
  %14 = load i64, i64* %13, align 8
  %15 = add i32 %0, -40
  %16 = inttoptr i32 %15 to i64*
  %17 = load i64, i64* %16, align 8
  %18 = call i32 @__fixtfsi(i64 %14, i64 %17)
  call void @__floatunsitf(i32 %12, i32 %18)
  %19 = add i32 %0, -80
  %20 = inttoptr i32 %12 to i64*
  %21 = load i64, i64* %20, align 8
  %22 = add i32 %0, -56
  %23 = inttoptr i32 %22 to i64*
  %24 = load i64, i64* %23, align 8
  call void @__subtf3(i32 %19, i64 %14, i64 %17, i64 %21, i64 %24)
  %25 = inttoptr i32 %19 to i64*
  %26 = load i64, i64* %25, align 8
  %27 = add i32 %0, -72
  %28 = inttoptr i32 %27 to i64*
  %29 = load i64, i64* %28, align 8
  %30 = call i32 @__getf2(i64 %26, i64 %29, i64 0, i64 0)
  %brif_val14.not = icmp sgt i32 %30, -1
  br i1 %brif_val14.not, label %brif_next15, label %blk_exit

brif_next15:                                      ; preds = %brif_next7
  %31 = call i32 @__lttf2(i64 %26, i64 %29, i64 0, i64 4611404543450677248)
  %brif_val16.not = icmp slt i32 %31, 0
  br i1 %brif_val16.not, label %brif_next17, label %blk_exit

brif_next17:                                      ; preds = %brif_next15
  %32 = add i32 %0, -96
  call void @__multf3(i32 %32, i64 %26, i64 %29, i64 0, i64 4615908143078047744)
  %33 = add i32 %0, -112
  %34 = inttoptr i32 %32 to i64*
  %35 = load i64, i64* %34, align 8
  %36 = add i32 %0, -88
  %37 = inttoptr i32 %36 to i64*
  %38 = load i64, i64* %37, align 8
  %39 = call i32 @__fixtfsi(i64 %35, i64 %38)
  call void @__floatunsitf(i32 %33, i32 %39)
  %40 = add i32 %0, -128
  %41 = inttoptr i32 %33 to i64*
  %42 = load i64, i64* %41, align 8
  %43 = add i32 %0, -104
  %44 = inttoptr i32 %43 to i64*
  %45 = load i64, i64* %44, align 8
  call void @__subtf3(i32 %40, i64 %35, i64 %38, i64 %42, i64 %45)
  %46 = inttoptr i32 %40 to i64*
  %47 = load i64, i64* %46, align 8
  %48 = add i32 %0, -120
  %49 = inttoptr i32 %48 to i64*
  %50 = load i64, i64* %49, align 8
  %51 = call i32 @__getf2(i64 %47, i64 %50, i64 0, i64 0)
  %brif_val24.not = icmp sgt i32 %51, -1
  br i1 %brif_val24.not, label %brif_next25, label %blk_exit

brif_next25:                                      ; preds = %brif_next17
  %52 = call i32 @__lttf2(i64 %47, i64 %50, i64 0, i64 4611404543450677248)
  %brif_val26.not = icmp slt i32 %52, 0
  br i1 %brif_val26.not, label %brif_next27, label %blk_exit

brif_next27:                                      ; preds = %brif_next25
  %53 = shl i32 %18, 16
  %54 = or i32 %53, %39
  %calcOffset28 = add i32 %2, 12
  %55 = inttoptr i32 %calcOffset28 to i32*
  store i32 %54, i32* %55, align 4
  %56 = add i32 %0, -144
  call void @__multf3(i32 %56, i64 %47, i64 %50, i64 0, i64 4615908143078047744)
  %57 = add i32 %0, -160
  %58 = inttoptr i32 %56 to i64*
  %59 = load i64, i64* %58, align 8
  %60 = add i32 %0, -136
  %61 = inttoptr i32 %60 to i64*
  %62 = load i64, i64* %61, align 8
  %63 = call i32 @__fixtfsi(i64 %59, i64 %62)
  call void @__floatunsitf(i32 %57, i32 %63)
  %64 = add i32 %0, -176
  %65 = inttoptr i32 %57 to i64*
  %66 = load i64, i64* %65, align 8
  %67 = add i32 %0, -152
  %68 = inttoptr i32 %67 to i64*
  %69 = load i64, i64* %68, align 8
  call void @__subtf3(i32 %64, i64 %59, i64 %62, i64 %66, i64 %69)
  %70 = inttoptr i32 %64 to i64*
  %71 = load i64, i64* %70, align 8
  %72 = add i32 %0, -168
  %73 = inttoptr i32 %72 to i64*
  %74 = load i64, i64* %73, align 8
  %75 = call i32 @__getf2(i64 %71, i64 %74, i64 0, i64 0)
  %brif_val35.not = icmp sgt i32 %75, -1
  br i1 %brif_val35.not, label %brif_next36, label %blk_exit

brif_next36:                                      ; preds = %brif_next27
  %76 = call i32 @__lttf2(i64 %71, i64 %74, i64 0, i64 4611404543450677248)
  %brif_val37.not = icmp slt i32 %76, 0
  br i1 %brif_val37.not, label %brif_next38, label %blk_exit

brif_next38:                                      ; preds = %brif_next36
  %77 = add i32 %0, -192
  call void @__multf3(i32 %77, i64 %71, i64 %74, i64 0, i64 4615908143078047744)
  %78 = add i32 %0, -208
  %79 = inttoptr i32 %77 to i64*
  %80 = load i64, i64* %79, align 8
  %81 = add i32 %0, -184
  %82 = inttoptr i32 %81 to i64*
  %83 = load i64, i64* %82, align 8
  %84 = call i32 @__fixtfsi(i64 %80, i64 %83)
  call void @__floatunsitf(i32 %78, i32 %84)
  %85 = add i32 %0, -224
  %86 = inttoptr i32 %78 to i64*
  %87 = load i64, i64* %86, align 8
  %88 = add i32 %0, -200
  %89 = inttoptr i32 %88 to i64*
  %90 = load i64, i64* %89, align 8
  call void @__subtf3(i32 %85, i64 %80, i64 %83, i64 %87, i64 %90)
  %91 = inttoptr i32 %85 to i64*
  %92 = load i64, i64* %91, align 8
  %93 = add i32 %0, -216
  %94 = inttoptr i32 %93 to i64*
  %95 = load i64, i64* %94, align 8
  %96 = call i32 @__getf2(i64 %92, i64 %95, i64 0, i64 0)
  %brif_val45.not = icmp sgt i32 %96, -1
  br i1 %brif_val45.not, label %brif_next46, label %blk_exit

brif_next46:                                      ; preds = %brif_next38
  %97 = call i32 @__lttf2(i64 %92, i64 %95, i64 0, i64 4611404543450677248)
  %brif_val47.not = icmp slt i32 %97, 0
  br i1 %brif_val47.not, label %brif_next48, label %blk_exit

brif_next48:                                      ; preds = %brif_next46
  %98 = shl i32 %63, 16
  %99 = or i32 %98, %84
  %calcOffset49 = add i32 %2, 8
  %100 = inttoptr i32 %calcOffset49 to i32*
  store i32 %99, i32* %100, align 4
  %101 = add i32 %0, -240
  call void @__multf3(i32 %101, i64 %92, i64 %95, i64 0, i64 4615908143078047744)
  %102 = add i32 %0, -256
  %103 = inttoptr i32 %101 to i64*
  %104 = load i64, i64* %103, align 8
  %105 = add i32 %0, -232
  %106 = inttoptr i32 %105 to i64*
  %107 = load i64, i64* %106, align 8
  %108 = call i32 @__fixtfsi(i64 %104, i64 %107)
  call void @__floatunsitf(i32 %102, i32 %108)
  %109 = add i32 %0, -272
  %110 = inttoptr i32 %102 to i64*
  %111 = load i64, i64* %110, align 8
  %112 = add i32 %0, -248
  %113 = inttoptr i32 %112 to i64*
  %114 = load i64, i64* %113, align 8
  call void @__subtf3(i32 %109, i64 %104, i64 %107, i64 %111, i64 %114)
  %115 = inttoptr i32 %109 to i64*
  %116 = load i64, i64* %115, align 8
  %117 = add i32 %0, -264
  %118 = inttoptr i32 %117 to i64*
  %119 = load i64, i64* %118, align 8
  %120 = call i32 @__getf2(i64 %116, i64 %119, i64 0, i64 0)
  %brif_val56.not = icmp sgt i32 %120, -1
  br i1 %brif_val56.not, label %brif_next57, label %blk_exit

brif_next57:                                      ; preds = %brif_next48
  %121 = call i32 @__lttf2(i64 %116, i64 %119, i64 0, i64 4611404543450677248)
  %brif_val58.not = icmp slt i32 %121, 0
  br i1 %brif_val58.not, label %brif_next59, label %blk_exit

brif_next59:                                      ; preds = %brif_next57
  %122 = add i32 %0, -288
  call void @__multf3(i32 %122, i64 %116, i64 %119, i64 0, i64 4615908143078047744)
  %123 = add i32 %0, -304
  %124 = inttoptr i32 %122 to i64*
  %125 = load i64, i64* %124, align 8
  %126 = add i32 %0, -280
  %127 = inttoptr i32 %126 to i64*
  %128 = load i64, i64* %127, align 8
  %129 = call i32 @__fixtfsi(i64 %125, i64 %128)
  call void @__floatunsitf(i32 %123, i32 %129)
  %130 = add i32 %0, -320
  %131 = inttoptr i32 %123 to i64*
  %132 = load i64, i64* %131, align 8
  %133 = add i32 %0, -296
  %134 = inttoptr i32 %133 to i64*
  %135 = load i64, i64* %134, align 8
  call void @__subtf3(i32 %130, i64 %125, i64 %128, i64 %132, i64 %135)
  %136 = inttoptr i32 %130 to i64*
  %137 = load i64, i64* %136, align 8
  %138 = add i32 %0, -312
  %139 = inttoptr i32 %138 to i64*
  %140 = load i64, i64* %139, align 8
  %141 = call i32 @__getf2(i64 %137, i64 %140, i64 0, i64 0)
  %brif_val66.not = icmp sgt i32 %141, -1
  br i1 %brif_val66.not, label %brif_next67, label %blk_exit

brif_next67:                                      ; preds = %brif_next59
  %142 = call i32 @__lttf2(i64 %137, i64 %140, i64 0, i64 4611404543450677248)
  %brif_val68.not = icmp slt i32 %142, 0
  br i1 %brif_val68.not, label %brif_next69, label %blk_exit

brif_next69:                                      ; preds = %brif_next67
  %143 = shl i32 %108, 16
  %144 = or i32 %143, %129
  %calcOffset70 = add i32 %2, 4
  %145 = inttoptr i32 %calcOffset70 to i32*
  store i32 %144, i32* %145, align 4
  %146 = add i32 %0, -336
  call void @__multf3(i32 %146, i64 %137, i64 %140, i64 0, i64 4615908143078047744)
  %147 = add i32 %0, -352
  %148 = inttoptr i32 %146 to i64*
  %149 = load i64, i64* %148, align 8
  %150 = add i32 %0, -328
  %151 = inttoptr i32 %150 to i64*
  %152 = load i64, i64* %151, align 8
  %153 = call i32 @__fixtfsi(i64 %149, i64 %152)
  call void @__floatunsitf(i32 %147, i32 %153)
  %154 = add i32 %0, -368
  %155 = inttoptr i32 %147 to i64*
  %156 = load i64, i64* %155, align 8
  %157 = add i32 %0, -344
  %158 = inttoptr i32 %157 to i64*
  %159 = load i64, i64* %158, align 8
  call void @__subtf3(i32 %154, i64 %149, i64 %152, i64 %156, i64 %159)
  %160 = inttoptr i32 %154 to i64*
  %161 = load i64, i64* %160, align 8
  %162 = add i32 %0, -360
  %163 = inttoptr i32 %162 to i64*
  %164 = load i64, i64* %163, align 8
  %165 = call i32 @__getf2(i64 %161, i64 %164, i64 0, i64 0)
  %brif_val77.not = icmp sgt i32 %165, -1
  br i1 %brif_val77.not, label %brif_next78, label %blk_exit

brif_next78:                                      ; preds = %brif_next69
  %166 = call i32 @__lttf2(i64 %161, i64 %164, i64 0, i64 4611404543450677248)
  %brif_val79.not = icmp slt i32 %166, 0
  br i1 %brif_val79.not, label %brif_next80, label %blk_exit

brif_next80:                                      ; preds = %brif_next78
  %167 = add i32 %0, -384
  call void @__multf3(i32 %167, i64 %161, i64 %164, i64 0, i64 4615908143078047744)
  %168 = add i32 %0, -400
  %169 = inttoptr i32 %167 to i64*
  %170 = load i64, i64* %169, align 8
  %171 = add i32 %0, -376
  %172 = inttoptr i32 %171 to i64*
  %173 = load i64, i64* %172, align 8
  %174 = call i32 @__fixtfsi(i64 %170, i64 %173)
  call void @__floatunsitf(i32 %168, i32 %174)
  %175 = inttoptr i32 %168 to i64*
  %176 = load i64, i64* %175, align 8
  %177 = add i32 %0, -392
  %178 = inttoptr i32 %177 to i64*
  %179 = load i64, i64* %178, align 8
  call void @__subtf3(i32 %1, i64 %170, i64 %173, i64 %176, i64 %179)
  %180 = inttoptr i32 %1 to i64*
  %181 = load i64, i64* %180, align 8
  %182 = add i32 %0, -408
  %183 = inttoptr i32 %182 to i64*
  %184 = load i64, i64* %183, align 8
  %185 = call i32 @__getf2(i64 %181, i64 %184, i64 0, i64 0)
  %brif_val87.not = icmp sgt i32 %185, -1
  br i1 %brif_val87.not, label %brif_next88, label %blk_exit

brif_next88:                                      ; preds = %brif_next80
  %186 = call i32 @__lttf2(i64 %181, i64 %184, i64 0, i64 4611404543450677248)
  %brif_val89.not = icmp slt i32 %186, 0
  br i1 %brif_val89.not, label %brif_next90, label %blk_exit

brif_next90:                                      ; preds = %brif_next88
  %187 = shl i32 %153, 16
  %188 = or i32 %187, %174
  %189 = inttoptr i32 %2 to i32*
  store i32 %188, i32* %189, align 4
  %brif_val93.not = icmp eq i32 %54, 0
  br i1 %brif_val93.not, label %brif_next94, label %blk_exit92

blk_exit92:                                       ; preds = %brif_next98, %brif_next96, %brif_next94, %brif_next90
  %_local_5.0 = phi i32 [ %195, %brif_next98 ], [ 2, %brif_next96 ], [ 3, %brif_next94 ], [ 4, %brif_next90 ]
  %190 = inttoptr i32 %4 to i32*
  %191 = load i32, i32* %190, align 4
  %192 = add i32 %191, -113
  %193 = call i32 @scale10_round_decimal_decoded(i32 %192, i32 %_local_5.0, i32 %2, i32 %2, i32 %_arg_2)
  br label %blk_exit1

brif_next94:                                      ; preds = %brif_next90
  %brif_val95.not = icmp eq i32 %99, 0
  br i1 %brif_val95.not, label %brif_next96, label %blk_exit92

brif_next96:                                      ; preds = %brif_next94
  %brif_val97.not = icmp eq i32 %144, 0
  br i1 %brif_val97.not, label %brif_next98, label %blk_exit92

brif_next98:                                      ; preds = %brif_next96
  %194 = icmp ne i32 %188, 0
  %195 = zext i1 %194 to i32
  br label %blk_exit92
}

; Function Attrs: null_pointer_is_valid
define internal i32 @scale10_round_decimal_double(double %_arg_0, i32 %_arg_1) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -16
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = call i32 @rpl_malloc(i32 8)
  %brif_val.not = icmp eq i32 %2, 0
  br i1 %brif_val.not, label %blk_exit1, label %blk_exit2

blk_exit:                                         ; preds = %brif_next38, %brif_next30, %brif_next22, %brif_next14, %brif_next6, %blk_exit2
  call void @abort()
  unreachable

blk_exit1:                                        ; preds = %brif_next41, %allocator
  %storemerge = phi i32 [ %52, %brif_next41 ], [ 0, %allocator ]
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %storemerge

blk_exit2:                                        ; preds = %allocator
  %3 = add i32 %0, -4
  %4 = call double @frexp(double %_arg_0, i32 %3)
  %5 = fcmp ult double %4, 0.000000e+00
  %6 = fcmp uge double %4, 1.000000e+00
  %or.cond = or i1 %5, %6
  br i1 %or.cond, label %blk_exit, label %brif_next6

brif_next6:                                       ; preds = %blk_exit2
  %7 = fmul double %4, 3.200000e+01
  %8 = call double @llvm.fabs.f64(double %7)
  %9 = fcmp uge double %8, 0x41E0000000000000
  %10 = fptosi double %7 to i32
  %storemerge43 = select i1 %9, i32 -2147483648, i32 %10
  %11 = uitofp i32 %storemerge43 to double
  %12 = fsub double %7, %11
  %13 = fcmp ult double %12, 0.000000e+00
  %14 = fcmp uge double %12, 1.000000e+00
  %or.cond49 = or i1 %13, %14
  br i1 %or.cond49, label %blk_exit, label %brif_next14

brif_next14:                                      ; preds = %brif_next6
  %15 = fmul double %12, 6.553600e+04
  %16 = call double @llvm.fabs.f64(double %15)
  %17 = fcmp uge double %16, 0x41E0000000000000
  %18 = fptosi double %15 to i32
  %storemerge44 = select i1 %17, i32 -2147483648, i32 %18
  %19 = uitofp i32 %storemerge44 to double
  %20 = fsub double %15, %19
  %21 = fcmp ult double %20, 0.000000e+00
  %22 = fcmp uge double %20, 1.000000e+00
  %or.cond51 = or i1 %21, %22
  br i1 %or.cond51, label %blk_exit, label %brif_next22

brif_next22:                                      ; preds = %brif_next14
  %23 = shl i32 %storemerge43, 16
  %24 = or i32 %23, %storemerge44
  %calcOffset = add i32 %2, 4
  %25 = inttoptr i32 %calcOffset to i32*
  store i32 %24, i32* %25, align 4
  %26 = fmul double %20, 6.553600e+04
  %27 = call double @llvm.fabs.f64(double %26)
  %28 = fcmp uge double %27, 0x41E0000000000000
  %29 = fptosi double %26 to i32
  %storemerge45 = select i1 %28, i32 -2147483648, i32 %29
  %30 = uitofp i32 %storemerge45 to double
  %31 = fsub double %26, %30
  %32 = fcmp ult double %31, 0.000000e+00
  %33 = fcmp uge double %31, 1.000000e+00
  %or.cond53 = or i1 %32, %33
  br i1 %or.cond53, label %blk_exit, label %brif_next30

brif_next30:                                      ; preds = %brif_next22
  %34 = fmul double %31, 6.553600e+04
  %35 = call double @llvm.fabs.f64(double %34)
  %36 = fcmp uge double %35, 0x41E0000000000000
  %37 = fptosi double %34 to i32
  %storemerge46 = select i1 %36, i32 -2147483648, i32 %37
  %38 = uitofp i32 %storemerge46 to double
  %39 = fsub double %34, %38
  %40 = fcmp ult double %39, 0.000000e+00
  %41 = fcmp uge double %39, 1.000000e+00
  %or.cond55 = or i1 %40, %41
  br i1 %or.cond55, label %blk_exit, label %brif_next38

brif_next38:                                      ; preds = %brif_next30
  %42 = shl i32 %storemerge45, 16
  %43 = or i32 %42, %storemerge46
  %44 = inttoptr i32 %2 to i32*
  store i32 %43, i32* %44, align 4
  %45 = fcmp oeq double %39, 0.000000e+00
  br i1 %45, label %blk_exit, label %brif_next41

brif_next41:                                      ; preds = %brif_next38
  %46 = inttoptr i32 %3 to i32*
  %47 = load i32, i32* %46, align 4
  %48 = add i32 %47, -53
  %49 = icmp ne i32 %43, 0
  %50 = zext i1 %49 to i32
  %select_cond.not = icmp eq i32 %24, 0
  %51 = select i1 %select_cond.not, i32 %50, i32 2
  %52 = call i32 @scale10_round_decimal_decoded(i32 %48, i32 %51, i32 %2, i32 %2, i32 %_arg_1)
  br label %blk_exit1
}

; Function Attrs: null_pointer_is_valid
define internal i32 @scale10_round_decimal_decoded(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2, i32 %_arg_3, i32 %_arg_4) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -16
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = add i32 %_arg_4, %_arg_0
  %3 = icmp slt i32 %2, %_arg_4
  %4 = select i1 %3, i32 %2, i32 %_arg_4
  %5 = icmp sgt i32 %2, 0
  %6 = icmp sgt i32 %_arg_4, 0
  %7 = select i1 %6, i1 %5, i1 false
  %8 = select i1 %7, i32 %4, i32 0
  %9 = sub i32 %2, %8
  %10 = call i32 @llvm.abs.i32(i32 %9, i1 false)
  %11 = lshr i32 %10, 5
  %12 = add nuw nsw i32 %11, 1
  %13 = sub i32 %_arg_4, %8
  %14 = call i32 @llvm.abs.i32(i32 %13, i1 false)
  %15 = uitofp i32 %14 to float
  %16 = fmul float %15, 0x3FB29374C0000000
  %17 = fcmp uge float %16, 0x41E0000000000000
  %18 = fptosi float %16 to i32
  %storemerge = select i1 %17, i32 0, i32 %18
  %19 = add i32 %12, %storemerge
  %20 = shl i32 %19, 2
  %21 = add i32 %20, 4
  %22 = call i32 @rpl_malloc(i32 %21)
  %23 = icmp eq i32 %22, 0
  br i1 %23, label %blk_exit5, label %brif_next7

blk_exit4:                                        ; preds = %blk_exit256, %blk_exit46, %blk_exit5
  %_local_13.0 = phi i32 [ 0, %blk_exit5 ], [ %408, %blk_exit256 ], [ 0, %blk_exit46 ]
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %_local_13.0

blk_exit5:                                        ; preds = %blk_exit45, %allocator
  call void @rpl_free(i32 %_arg_3)
  br label %blk_exit4

brif_next7:                                       ; preds = %allocator
  %24 = inttoptr i32 %22 to i32*
  store i32 1, i32* %24, align 4
  %25 = icmp eq i32 %13, 0
  br i1 %25, label %blk_exit8, label %loop_entry

blk_exit8:                                        ; preds = %blk_exit11, %brif_next7
  %_local_20.0 = phi i64 [ 0, %brif_next7 ], [ %_local_20.2, %blk_exit11 ]
  %_param_4.0 = phi i32 [ 1, %brif_next7 ], [ %_param_4.2, %blk_exit11 ]
  %_local_23.0 = phi i32 [ 0, %brif_next7 ], [ %_local_23.2, %blk_exit11 ]
  %26 = and i32 %10, 31
  %brif_val51.not = icmp sgt i32 %13, -1
  br i1 %brif_val51.not, label %brif_next52, label %blk_exit50

loop_entry:                                       ; preds = %blk_exit11, %brif_next7
  %_local_22.0 = phi i64 [ %_local_22.1, %blk_exit11 ], [ 0, %brif_next7 ]
  %_local_20.1 = phi i64 [ %_local_20.2, %blk_exit11 ], [ 0, %brif_next7 ]
  %_local_16.0 = phi i32 [ %27, %blk_exit11 ], [ 0, %brif_next7 ]
  %_param_4.1 = phi i32 [ %_param_4.2, %blk_exit11 ], [ 1, %brif_next7 ]
  %_local_23.1 = phi i32 [ %_local_23.2, %blk_exit11 ], [ 0, %brif_next7 ]
  %27 = add i32 %_local_16.0, 13
  %28 = icmp eq i32 %_param_4.1, 0
  br i1 %28, label %blk_exit11, label %brif_next13

blk_exit11:                                       ; preds = %blk_exit39, %blk_exit31, %loop_entry
  %_local_22.1 = phi i64 [ %_local_22.0, %loop_entry ], [ %_local_22.3, %blk_exit39 ], [ %_local_22.3, %blk_exit31 ]
  %_local_20.2 = phi i64 [ %_local_20.1, %loop_entry ], [ %_local_20.5, %blk_exit39 ], [ %_local_20.5, %blk_exit31 ]
  %_param_4.2 = phi i32 [ 0, %loop_entry ], [ %89, %blk_exit39 ], [ %_param_4.1, %blk_exit31 ]
  %_local_23.2 = phi i32 [ %_local_23.1, %loop_entry ], [ %_local_23.3, %blk_exit39 ], [ %_local_23.3, %blk_exit31 ]
  %.not = icmp ult i32 %14, %27
  br i1 %.not, label %blk_exit8, label %loop_entry

brif_next13:                                      ; preds = %loop_entry
  %29 = sub i32 %14, %_local_16.0
  %30 = icmp ugt i32 %27, %14
  %.op = shl i32 %29, 2
  %.op.op = add i32 %.op, 14896
  %31 = select i1 %30, i32 %.op.op, i32 14948
  %32 = inttoptr i32 %31 to i32*
  %33 = load i32, i32* %32, align 4
  %34 = zext i32 %33 to i64
  %35 = and i32 %_param_4.1, 3
  %36 = add i32 %_param_4.1, -4
  %37 = icmp ult i32 %36, -3
  br i1 %37, label %blk_exit17, label %blk_exit16

blk_exit16:                                       ; preds = %loop_entry20, %brif_next13
  %_local_22.2 = phi i64 [ %68, %loop_entry20 ], [ %_local_22.0, %brif_next13 ]
  %_local_20.3 = phi i64 [ %70, %loop_entry20 ], [ 0, %brif_next13 ]
  %_local_19.0 = phi i32 [ %39, %loop_entry20 ], [ 0, %brif_next13 ]
  %_local_23.3 = phi i32 [ %62, %loop_entry20 ], [ %_local_23.1, %brif_next13 ]
  %38 = icmp eq i32 %35, 0
  br i1 %38, label %blk_exit31, label %brif_next33

blk_exit17:                                       ; preds = %brif_next13
  %39 = and i32 %_param_4.1, -4
  br label %loop_entry20

loop_entry20:                                     ; preds = %loop_entry20, %blk_exit17
  %_local_20.4 = phi i64 [ 0, %blk_exit17 ], [ %70, %loop_entry20 ]
  %_local_19.1 = phi i32 [ 0, %blk_exit17 ], [ %72, %loop_entry20 ]
  %_param_4.3 = phi i32 [ %22, %blk_exit17 ], [ %71, %loop_entry20 ]
  %40 = inttoptr i32 %_param_4.3 to i32*
  %41 = load i32, i32* %40, align 4
  %42 = zext i32 %41 to i64
  %43 = mul nuw i64 %34, %42
  %44 = add i64 %43, %_local_20.4
  %45 = trunc i64 %44 to i32
  store i32 %45, i32* %40, align 4
  %46 = add i32 %_param_4.3, 4
  %47 = inttoptr i32 %46 to i32*
  %48 = load i32, i32* %47, align 4
  %49 = zext i32 %48 to i64
  %50 = mul nuw i64 %34, %49
  %51 = lshr i64 %44, 32
  %52 = add i64 %50, %51
  %53 = trunc i64 %52 to i32
  store i32 %53, i32* %47, align 4
  %54 = add i32 %_param_4.3, 8
  %55 = inttoptr i32 %54 to i32*
  %56 = load i32, i32* %55, align 4
  %57 = zext i32 %56 to i64
  %58 = mul nuw i64 %34, %57
  %59 = lshr i64 %52, 32
  %60 = add i64 %58, %59
  %61 = trunc i64 %60 to i32
  store i32 %61, i32* %55, align 4
  %62 = add i32 %_param_4.3, 12
  %63 = inttoptr i32 %62 to i32*
  %64 = load i32, i32* %63, align 4
  %65 = zext i32 %64 to i64
  %66 = mul nuw i64 %34, %65
  %67 = lshr i64 %60, 32
  %68 = add i64 %66, %67
  %69 = trunc i64 %68 to i32
  store i32 %69, i32* %63, align 4
  %70 = lshr i64 %68, 32
  %71 = add i32 %_param_4.3, 16
  %72 = add i32 %_local_19.1, 4
  %.not333 = icmp eq i32 %39, %72
  br i1 %.not333, label %blk_exit16, label %loop_entry20

blk_exit31:                                       ; preds = %loop_entry34, %blk_exit16
  %_local_22.3 = phi i64 [ %_local_22.2, %blk_exit16 ], [ %80, %loop_entry34 ]
  %_local_20.5 = phi i64 [ %_local_20.3, %blk_exit16 ], [ %83, %loop_entry34 ]
  %73 = icmp ugt i64 %_local_22.3, 4294967295
  br i1 %73, label %blk_exit39, label %blk_exit11

brif_next33:                                      ; preds = %blk_exit16
  %74 = shl i32 %_local_19.0, 2
  %75 = add i32 %22, %74
  br label %loop_entry34

loop_entry34:                                     ; preds = %loop_entry34, %brif_next33
  %_local_20.6 = phi i64 [ %_local_20.3, %brif_next33 ], [ %83, %loop_entry34 ]
  %_param_4.4 = phi i32 [ %75, %brif_next33 ], [ %82, %loop_entry34 ]
  %_param_0.0 = phi i32 [ %35, %brif_next33 ], [ %84, %loop_entry34 ]
  %76 = inttoptr i32 %_param_4.4 to i32*
  %77 = load i32, i32* %76, align 4
  %78 = zext i32 %77 to i64
  %79 = mul nuw i64 %34, %78
  %80 = add i64 %79, %_local_20.6
  %81 = trunc i64 %80 to i32
  store i32 %81, i32* %76, align 4
  %82 = add i32 %_param_4.4, 4
  %83 = lshr i64 %80, 32
  %84 = add i32 %_param_0.0, -1
  %brif_val37.not = icmp eq i32 %84, 0
  br i1 %brif_val37.not, label %blk_exit31, label %loop_entry34

blk_exit39:                                       ; preds = %blk_exit31
  %85 = shl i32 %_param_4.1, 2
  %86 = add i32 %22, %85
  %87 = trunc i64 %_local_20.5 to i32
  %88 = inttoptr i32 %86 to i32*
  store i32 %87, i32* %88, align 4
  %89 = add i32 %_param_4.1, 1
  br label %blk_exit11

blk_exit45:                                       ; preds = %brif_next54, %blk_exit47
  call void @rpl_free(i32 %22)
  br label %blk_exit5

blk_exit46:                                       ; preds = %blk_exit184, %blk_exit138, %brif_next137, %blk_exit135
  %_param_2.0 = phi i32 [ %226, %blk_exit138 ], [ %216, %brif_next137 ], [ %214, %blk_exit135 ], [ %274, %blk_exit184 ]
  call void @rpl_free(i32 %22)
  call void @rpl_free(i32 %_arg_3)
  %.not325 = icmp eq i32 %_param_2.0, -1
  br i1 %.not325, label %blk_exit4, label %blk_exit247

blk_exit47:                                       ; preds = %blk_exit50
  %90 = add i32 %11, %_arg_1
  %91 = shl i32 %90, 2
  %92 = add i32 %91, 4
  %93 = call i32 @rpl_malloc(i32 %92)
  %94 = icmp eq i32 %93, 0
  br i1 %94, label %blk_exit45, label %brif_next162

blk_exit48:                                       ; preds = %brif_next54
  %95 = shl i32 %_param_4.0, 2
  %96 = add i32 %22, %95
  %97 = icmp ult i32 %10, 32
  br i1 %97, label %blk_exit138, label %brif_next140

blk_exit49:                                       ; preds = %brif_next52, %blk_exit50
  %98 = icmp eq i32 %26, 0
  br i1 %98, label %blk_exit59, label %brif_next61

blk_exit50:                                       ; preds = %blk_exit8
  %99 = icmp sgt i32 %9, 0
  br i1 %99, label %blk_exit47, label %blk_exit49

brif_next52:                                      ; preds = %blk_exit8
  %brif_val53.not = icmp slt i32 %9, 0
  br i1 %brif_val53.not, label %brif_next54, label %blk_exit49

brif_next54:                                      ; preds = %brif_next52
  %100 = call i32 @multiply(i32 %_arg_1, i32 %_arg_2, i32 %_param_4.0, i32 %22, i32 %1)
  %.not323 = icmp eq i32 %100, -1
  br i1 %.not323, label %blk_exit45, label %blk_exit48

blk_exit59:                                       ; preds = %brif_next89, %blk_exit80, %brif_next61, %blk_exit49
  %_param_4.5 = phi i32 [ %_param_4.0, %blk_exit49 ], [ %_param_4.0, %blk_exit80 ], [ %153, %brif_next89 ], [ 0, %brif_next61 ]
  %101 = icmp ult i32 %10, 32
  br i1 %101, label %blk_exit91, label %brif_next93

brif_next61:                                      ; preds = %blk_exit49
  %brif_val63.not = icmp eq i32 %_param_4.0, 0
  br i1 %brif_val63.not, label %blk_exit59, label %blk_exit62

blk_exit62:                                       ; preds = %brif_next61
  %102 = and i32 %_param_4.0, 3
  %103 = zext i32 %26 to i64
  %104 = add i32 %_param_4.0, -4
  %105 = icmp ult i32 %104, -3
  br i1 %105, label %blk_exit66, label %blk_exit65

blk_exit65:                                       ; preds = %loop_entry69, %blk_exit62
  %_local_22.4 = phi i64 [ %138, %loop_entry69 ], [ 0, %blk_exit62 ]
  %_local_20.7 = phi i64 [ %136, %loop_entry69 ], [ %_local_20.0, %blk_exit62 ]
  %_param_0.1 = phi i32 [ %139, %loop_entry69 ], [ %22, %blk_exit62 ]
  %106 = icmp eq i32 %102, 0
  br i1 %106, label %blk_exit80, label %loop_entry83

blk_exit66:                                       ; preds = %blk_exit62
  %107 = and i32 %_param_4.0, -4
  br label %loop_entry69

loop_entry69:                                     ; preds = %loop_entry69, %blk_exit66
  %_local_22.5 = phi i64 [ 0, %blk_exit66 ], [ %138, %loop_entry69 ]
  %_local_23.4 = phi i32 [ %107, %blk_exit66 ], [ %140, %loop_entry69 ]
  %_param_0.2 = phi i32 [ %22, %blk_exit66 ], [ %139, %loop_entry69 ]
  %108 = inttoptr i32 %_param_0.2 to i32*
  %109 = load i32, i32* %108, align 4
  %110 = zext i32 %109 to i64
  %111 = shl i64 %110, %103
  %112 = add nuw i64 %111, %_local_22.5
  %113 = trunc i64 %112 to i32
  store i32 %113, i32* %108, align 4
  %114 = add i32 %_param_0.2, 4
  %115 = inttoptr i32 %114 to i32*
  %116 = load i32, i32* %115, align 4
  %117 = zext i32 %116 to i64
  %118 = shl i64 %117, %103
  %119 = lshr i64 %112, 32
  %120 = add nuw i64 %118, %119
  %121 = trunc i64 %120 to i32
  store i32 %121, i32* %115, align 4
  %122 = add i32 %_param_0.2, 8
  %123 = inttoptr i32 %122 to i32*
  %124 = load i32, i32* %123, align 4
  %125 = zext i32 %124 to i64
  %126 = shl i64 %125, %103
  %127 = lshr i64 %120, 32
  %128 = add nuw i64 %126, %127
  %129 = trunc i64 %128 to i32
  store i32 %129, i32* %123, align 4
  %130 = add i32 %_param_0.2, 12
  %131 = inttoptr i32 %130 to i32*
  %132 = load i32, i32* %131, align 4
  %133 = zext i32 %132 to i64
  %134 = shl i64 %133, %103
  %135 = lshr i64 %128, 32
  %136 = add nuw i64 %134, %135
  %137 = trunc i64 %136 to i32
  store i32 %137, i32* %131, align 4
  %138 = lshr i64 %136, 32
  %139 = add i32 %_param_0.2, 16
  %140 = add i32 %_local_23.4, -4
  %brif_val78.not = icmp eq i32 %140, 0
  br i1 %brif_val78.not, label %blk_exit65, label %loop_entry69

blk_exit80:                                       ; preds = %loop_entry83, %blk_exit65
  %_local_22.6 = phi i64 [ %_local_22.4, %blk_exit65 ], [ %149, %loop_entry83 ]
  %_local_20.8 = phi i64 [ %_local_20.7, %blk_exit65 ], [ %146, %loop_entry83 ]
  %_param_0.3 = phi i32 [ %_param_0.1, %blk_exit65 ], [ %148, %loop_entry83 ]
  %141 = icmp ult i64 %_local_20.8, 4294967296
  br i1 %141, label %blk_exit59, label %brif_next89

loop_entry83:                                     ; preds = %loop_entry83, %blk_exit65
  %_local_22.7 = phi i64 [ %_local_22.4, %blk_exit65 ], [ %149, %loop_entry83 ]
  %_local_19.2 = phi i32 [ %102, %blk_exit65 ], [ %150, %loop_entry83 ]
  %_param_0.4 = phi i32 [ %_param_0.1, %blk_exit65 ], [ %148, %loop_entry83 ]
  %142 = inttoptr i32 %_param_0.4 to i32*
  %143 = load i32, i32* %142, align 4
  %144 = zext i32 %143 to i64
  %145 = shl i64 %144, %103
  %146 = add i64 %145, %_local_22.7
  %147 = trunc i64 %146 to i32
  store i32 %147, i32* %142, align 4
  %148 = add i32 %_param_0.4, 4
  %149 = lshr i64 %146, 32
  %150 = add i32 %_local_19.2, -1
  %brif_val86.not = icmp eq i32 %150, 0
  br i1 %brif_val86.not, label %blk_exit80, label %loop_entry83

brif_next89:                                      ; preds = %blk_exit80
  %151 = trunc i64 %_local_22.6 to i32
  %152 = inttoptr i32 %_param_0.3 to i32*
  store i32 %151, i32* %152, align 4
  %153 = add i32 %_param_4.0, 1
  br label %blk_exit59

blk_exit91:                                       ; preds = %blk_exit125, %blk_exit59
  %_param_4.6 = phi i32 [ %_param_4.5, %blk_exit59 ], [ %202, %blk_exit125 ]
  br i1 %brif_val51.not, label %brif_next137, label %blk_exit135

brif_next93:                                      ; preds = %blk_exit59
  %154 = icmp eq i32 %_param_4.5, 0
  br i1 %154, label %blk_exit94, label %brif_next96

blk_exit94:                                       ; preds = %loop_entry107, %blk_exit97, %brif_next93
  %155 = add nsw i32 %11, -1
  %156 = and i32 %11, 7
  %157 = icmp eq i32 %156, 0
  br i1 %157, label %blk_exit118, label %brif_next120

brif_next96:                                      ; preds = %brif_next93
  %158 = add i32 %_param_4.5, -1
  %159 = and i32 %_param_4.5, 3
  %160 = icmp eq i32 %159, 0
  br i1 %160, label %blk_exit97, label %brif_next99

blk_exit97:                                       ; preds = %loop_entry100, %brif_next96
  %_local_23.5 = phi i32 [ %_param_4.5, %brif_next96 ], [ %170, %loop_entry100 ]
  %161 = icmp ult i32 %158, 3
  br i1 %161, label %blk_exit94, label %brif_next106

brif_next99:                                      ; preds = %brif_next96
  %162 = shl i32 %_param_4.5, 2
  %163 = add i32 %162, %22
  %164 = add i32 %_param_4.5, %11
  %165 = shl i32 %164, 2
  %166 = add i32 %165, %22
  br label %loop_entry100

loop_entry100:                                    ; preds = %loop_entry100, %brif_next99
  %_local_21.0 = phi i32 [ %159, %brif_next99 ], [ %171, %loop_entry100 ]
  %_local_19.3.in = phi i32 [ %166, %brif_next99 ], [ %_local_19.3, %loop_entry100 ]
  %_local_23.6 = phi i32 [ %_param_4.5, %brif_next99 ], [ %170, %loop_entry100 ]
  %_param_0.5.in = phi i32 [ %163, %brif_next99 ], [ %_param_0.5, %loop_entry100 ]
  %_param_0.5 = add i32 %_param_0.5.in, -4
  %_local_19.3 = add i32 %_local_19.3.in, -4
  %167 = inttoptr i32 %_param_0.5 to i32*
  %168 = load i32, i32* %167, align 4
  %169 = inttoptr i32 %_local_19.3 to i32*
  store i32 %168, i32* %169, align 4
  %170 = add i32 %_local_23.6, -1
  %171 = add i32 %_local_21.0, -1
  %brif_val103.not = icmp eq i32 %171, 0
  br i1 %brif_val103.not, label %blk_exit97, label %loop_entry100

brif_next106:                                     ; preds = %blk_exit97
  %172 = shl i32 %_local_23.5, 2
  %173 = add i32 %172, %22
  %174 = add i32 %_local_23.5, %11
  %175 = shl i32 %174, 2
  %176 = add i32 %175, %22
  br label %loop_entry107

loop_entry107:                                    ; preds = %loop_entry107, %brif_next106
  %_local_19.4.in = phi i32 [ %176, %brif_next106 ], [ %_local_19.4, %loop_entry107 ]
  %_local_23.7 = phi i32 [ %_local_23.5, %brif_next106 ], [ %195, %loop_entry107 ]
  %_param_0.6.in = phi i32 [ %173, %brif_next106 ], [ %_param_0.6, %loop_entry107 ]
  %_param_0.6 = add i32 %_param_0.6.in, -16
  %_local_19.4 = add i32 %_local_19.4.in, -16
  %177 = add i32 %_local_19.4.in, -4
  %178 = add i32 %_param_0.6.in, -4
  %179 = inttoptr i32 %178 to i32*
  %180 = load i32, i32* %179, align 4
  %181 = inttoptr i32 %177 to i32*
  store i32 %180, i32* %181, align 4
  %182 = add i32 %_local_19.4.in, -8
  %183 = add i32 %_param_0.6.in, -8
  %184 = inttoptr i32 %183 to i32*
  %185 = load i32, i32* %184, align 4
  %186 = inttoptr i32 %182 to i32*
  store i32 %185, i32* %186, align 4
  %187 = add i32 %_local_19.4.in, -12
  %188 = add i32 %_param_0.6.in, -12
  %189 = inttoptr i32 %188 to i32*
  %190 = load i32, i32* %189, align 4
  %191 = inttoptr i32 %187 to i32*
  store i32 %190, i32* %191, align 4
  %192 = inttoptr i32 %_param_0.6 to i32*
  %193 = load i32, i32* %192, align 4
  %194 = inttoptr i32 %_local_19.4 to i32*
  store i32 %193, i32* %194, align 4
  %195 = add i32 %_local_23.7, -4
  %brif_val116.not = icmp eq i32 %195, 0
  br i1 %brif_val116.not, label %blk_exit94, label %loop_entry107

blk_exit118:                                      ; preds = %loop_entry121, %blk_exit94
  %_local_19.5 = phi i32 [ %11, %blk_exit94 ], [ %200, %loop_entry121 ]
  %196 = icmp ult i32 %155, 7
  br i1 %196, label %blk_exit125, label %brif_next127

brif_next120:                                     ; preds = %blk_exit94
  %197 = shl nuw nsw i32 %11, 2
  %198 = add i32 %197, %22
  br label %loop_entry121

loop_entry121:                                    ; preds = %loop_entry121, %brif_next120
  %_local_19.6 = phi i32 [ %11, %brif_next120 ], [ %200, %loop_entry121 ]
  %_local_23.8 = phi i32 [ %156, %brif_next120 ], [ %201, %loop_entry121 ]
  %_param_0.7.in = phi i32 [ %198, %brif_next120 ], [ %_param_0.7, %loop_entry121 ]
  %_param_0.7 = add i32 %_param_0.7.in, -4
  %199 = inttoptr i32 %_param_0.7 to i32*
  store i32 0, i32* %199, align 4
  %200 = add i32 %_local_19.6, -1
  %201 = add i32 %_local_23.8, -1
  %brif_val123.not = icmp eq i32 %201, 0
  br i1 %brif_val123.not, label %blk_exit118, label %loop_entry121

blk_exit125:                                      ; preds = %loop_entry128, %blk_exit118
  %202 = add i32 %_param_4.5, %11
  br label %blk_exit91

brif_next127:                                     ; preds = %blk_exit118
  %203 = shl i32 %_local_19.5, 2
  %204 = add i32 %203, %22
  br label %loop_entry128

loop_entry128:                                    ; preds = %loop_entry128, %brif_next127
  %_local_19.7 = phi i32 [ %_local_19.5, %brif_next127 ], [ %212, %loop_entry128 ]
  %_param_0.8.in = phi i32 [ %204, %brif_next127 ], [ %_param_0.8, %loop_entry128 ]
  %_param_0.8 = add i32 %_param_0.8.in, -32
  %205 = inttoptr i32 %_param_0.8 to i64*
  store i64 0, i64* %205, align 8
  %206 = add i32 %_param_0.8.in, -8
  %207 = inttoptr i32 %206 to i64*
  store i64 0, i64* %207, align 8
  %208 = add i32 %_param_0.8.in, -16
  %209 = inttoptr i32 %208 to i64*
  store i64 0, i64* %209, align 8
  %210 = add i32 %_param_0.8.in, -24
  %211 = inttoptr i32 %210 to i64*
  store i64 0, i64* %211, align 8
  %212 = add i32 %_local_19.7, -8
  %brif_val133.not = icmp eq i32 %212, 0
  br i1 %brif_val133.not, label %blk_exit125, label %loop_entry128

blk_exit135:                                      ; preds = %blk_exit91
  %213 = add i32 %0, -8
  %214 = call i32 @divide(i32 %_arg_1, i32 %_arg_2, i32 %_param_4.6, i32 %22, i32 %213)
  br label %blk_exit46

brif_next137:                                     ; preds = %blk_exit91
  %215 = add i32 %0, -8
  %216 = call i32 @multiply(i32 %_arg_1, i32 %_arg_2, i32 %_param_4.6, i32 %22, i32 %215)
  br label %blk_exit46

blk_exit138:                                      ; preds = %loop_entry154, %blk_exit142, %blk_exit48
  %217 = shl nuw nsw i32 %11, 2
  %218 = add i32 %96, %217
  %219 = shl i32 1, %26
  %220 = inttoptr i32 %218 to i32*
  store i32 %219, i32* %220, align 4
  %221 = inttoptr i32 %1 to i32*
  %222 = load i32, i32* %221, align 4
  %calcOffset160 = add i32 %0, -12
  %223 = inttoptr i32 %calcOffset160 to i32*
  %224 = load i32, i32* %223, align 4
  %225 = add i32 %0, -8
  %226 = call i32 @divide(i32 %222, i32 %224, i32 %12, i32 %96, i32 %225)
  call void @rpl_free(i32 %100)
  br label %blk_exit46

brif_next140:                                     ; preds = %blk_exit48
  %227 = icmp ugt i32 %11, 1
  %228 = select i1 %227, i32 %11, i32 1
  %229 = and i32 %228, 7
  %230 = add nsw i32 %228, -1
  %231 = icmp ult i32 %230, 7
  br i1 %231, label %blk_exit142, label %brif_next144

blk_exit142:                                      ; preds = %loop_entry145, %brif_next140
  %_local_23.9 = phi i32 [ 0, %brif_next140 ], [ %233, %loop_entry145 ]
  %232 = icmp eq i32 %229, 0
  br i1 %232, label %blk_exit138, label %brif_next153

brif_next144:                                     ; preds = %brif_next140
  %233 = and i32 %228, 67108856
  %234 = add i32 %96, 16
  br label %loop_entry145

loop_entry145:                                    ; preds = %loop_entry145, %brif_next144
  %_local_23.10 = phi i32 [ 0, %brif_next144 ], [ %243, %loop_entry145 ]
  %_param_0.9 = phi i32 [ %234, %brif_next144 ], [ %242, %loop_entry145 ]
  %235 = inttoptr i32 %_param_0.9 to i64*
  store i64 0, i64* %235, align 8
  %236 = add i32 %_param_0.9, 8
  %237 = inttoptr i32 %236 to i64*
  store i64 0, i64* %237, align 8
  %238 = add i32 %_param_0.9, -8
  %239 = inttoptr i32 %238 to i64*
  store i64 0, i64* %239, align 8
  %240 = add i32 %_param_0.9, -16
  %241 = inttoptr i32 %240 to i64*
  store i64 0, i64* %241, align 8
  %242 = add i32 %_param_0.9, 32
  %243 = add i32 %_local_23.10, 8
  %.not324 = icmp eq i32 %233, %243
  br i1 %.not324, label %blk_exit142, label %loop_entry145

brif_next153:                                     ; preds = %blk_exit142
  %244 = add i32 %_param_4.0, %_local_23.9
  %245 = shl i32 %244, 2
  %246 = add i32 %22, %245
  br label %loop_entry154

loop_entry154:                                    ; preds = %loop_entry154, %brif_next153
  %_local_19.8 = phi i32 [ %229, %brif_next153 ], [ %249, %loop_entry154 ]
  %_param_4.7 = phi i32 [ %246, %brif_next153 ], [ %248, %loop_entry154 ]
  %247 = inttoptr i32 %_param_4.7 to i32*
  store i32 0, i32* %247, align 4
  %248 = add i32 %_param_4.7, 4
  %249 = add i32 %_local_19.8, -1
  %brif_val156.not = icmp eq i32 %249, 0
  br i1 %brif_val156.not, label %blk_exit138, label %loop_entry154

brif_next162:                                     ; preds = %blk_exit47
  %250 = icmp ult i32 %10, 32
  br i1 %250, label %blk_exit163, label %brif_next165

blk_exit163:                                      ; preds = %loop_entry180, %blk_exit167, %brif_next162
  %_local_23.11 = phi i32 [ %_local_23.0, %brif_next162 ], [ %_local_23.12, %blk_exit167 ], [ %_local_23.12, %loop_entry180 ]
  %_param_0.10 = phi i32 [ %93, %brif_next162 ], [ %_param_0.11, %blk_exit167 ], [ %269, %loop_entry180 ]
  %251 = icmp eq i32 %26, 0
  br i1 %251, label %blk_exit187, label %brif_next189

brif_next165:                                     ; preds = %brif_next162
  %252 = icmp ugt i32 %11, 1
  %253 = select i1 %252, i32 %11, i32 1
  %254 = and i32 %253, 7
  %255 = add nsw i32 %253, -8
  %256 = icmp ult i32 %255, -7
  br i1 %256, label %blk_exit168, label %blk_exit167

blk_exit167:                                      ; preds = %loop_entry171, %brif_next165
  %_local_23.12 = phi i32 [ 0, %loop_entry171 ], [ %_local_23.0, %brif_next165 ]
  %_param_0.11 = phi i32 [ %266, %loop_entry171 ], [ %93, %brif_next165 ]
  %257 = icmp eq i32 %254, 0
  br i1 %257, label %blk_exit163, label %loop_entry180

blk_exit168:                                      ; preds = %brif_next165
  %258 = and i32 %253, 67108856
  br label %loop_entry171

loop_entry171:                                    ; preds = %loop_entry171, %blk_exit168
  %_local_23.13 = phi i32 [ %258, %blk_exit168 ], [ %267, %loop_entry171 ]
  %_param_0.12 = phi i32 [ %93, %blk_exit168 ], [ %266, %loop_entry171 ]
  %259 = inttoptr i32 %_param_0.12 to i64*
  store i64 0, i64* %259, align 8
  %260 = add i32 %_param_0.12, 24
  %261 = inttoptr i32 %260 to i64*
  store i64 0, i64* %261, align 8
  %262 = add i32 %_param_0.12, 16
  %263 = inttoptr i32 %262 to i64*
  store i64 0, i64* %263, align 8
  %264 = add i32 %_param_0.12, 8
  %265 = inttoptr i32 %264 to i64*
  store i64 0, i64* %265, align 8
  %266 = add i32 %_param_0.12, 32
  %267 = add i32 %_local_23.13, -8
  %brif_val176.not = icmp eq i32 %267, 0
  br i1 %brif_val176.not, label %blk_exit167, label %loop_entry171

loop_entry180:                                    ; preds = %loop_entry180, %blk_exit167
  %_local_19.9 = phi i32 [ %254, %blk_exit167 ], [ %270, %loop_entry180 ]
  %_param_0.13 = phi i32 [ %_param_0.11, %blk_exit167 ], [ %269, %loop_entry180 ]
  %268 = inttoptr i32 %_param_0.13 to i32*
  store i32 0, i32* %268, align 4
  %269 = add i32 %_param_0.13, 4
  %270 = add i32 %_local_19.9, -1
  %brif_val182.not = icmp eq i32 %270, 0
  br i1 %brif_val182.not, label %blk_exit163, label %loop_entry180

blk_exit184:                                      ; preds = %brif_next245, %blk_exit236, %loop_entry206, %blk_exit196, %brif_next189, %blk_exit187
  %_param_0.14 = phi i32 [ %_param_0.10, %blk_exit187 ], [ %_param_0.16, %blk_exit196 ], [ %332, %loop_entry206 ], [ %_param_0.10, %brif_next189 ], [ %_param_0.20, %blk_exit236 ], [ %391, %brif_next245 ]
  %271 = sub i32 %_param_0.14, %93
  %272 = ashr i32 %271, 2
  %273 = add i32 %0, -8
  %274 = call i32 @divide(i32 %272, i32 %93, i32 %_param_4.0, i32 %22, i32 %273)
  call void @rpl_free(i32 %93)
  br label %blk_exit46

blk_exit185:                                      ; preds = %brif_next235, %brif_next191
  %_local_22.8 = phi i64 [ %372, %brif_next235 ], [ 0, %brif_next191 ]
  %_local_20.9 = phi i64 [ %369, %brif_next235 ], [ %_local_20.0, %brif_next191 ]
  %_local_23.14 = phi i32 [ %376, %brif_next235 ], [ %_local_23.11, %brif_next191 ]
  %_param_2.1 = phi i32 [ %374, %brif_next235 ], [ %_arg_2, %brif_next191 ]
  %_param_0.15 = phi i32 [ %373, %brif_next235 ], [ %_param_0.10, %brif_next191 ]
  %275 = icmp eq i32 %279, 0
  br i1 %275, label %blk_exit236, label %loop_entry239

blk_exit186:                                      ; preds = %brif_next191
  %276 = and i32 %_arg_1, -4
  br label %loop_entry225

blk_exit187:                                      ; preds = %blk_exit163
  %277 = icmp eq i32 %_arg_1, 0
  br i1 %277, label %blk_exit184, label %brif_next195

brif_next189:                                     ; preds = %blk_exit163
  %278 = icmp eq i32 %_arg_1, 0
  br i1 %278, label %blk_exit184, label %brif_next191

brif_next191:                                     ; preds = %brif_next189
  %279 = and i32 %_arg_1, 3
  %280 = zext i32 %26 to i64
  %281 = add i32 %_arg_1, -4
  %282 = icmp ult i32 %281, -3
  br i1 %282, label %blk_exit186, label %blk_exit185

brif_next195:                                     ; preds = %blk_exit187
  %283 = add i32 %_arg_1, -1
  %284 = and i32 %_arg_1, 7
  %285 = icmp eq i32 %284, 0
  br i1 %285, label %blk_exit196, label %loop_entry199

blk_exit196:                                      ; preds = %loop_entry199, %brif_next195
  %_param_2.2 = phi i32 [ %_arg_2, %brif_next195 ], [ %292, %loop_entry199 ]
  %_param_1.0 = phi i32 [ %_arg_1, %brif_next195 ], [ %290, %loop_entry199 ]
  %_param_0.16 = phi i32 [ %_param_0.10, %brif_next195 ], [ %291, %loop_entry199 ]
  %286 = icmp ult i32 %283, 7
  br i1 %286, label %blk_exit184, label %loop_entry206

loop_entry199:                                    ; preds = %loop_entry199, %brif_next195
  %_local_19.10 = phi i32 [ %284, %brif_next195 ], [ %293, %loop_entry199 ]
  %_param_2.3 = phi i32 [ %_arg_2, %brif_next195 ], [ %292, %loop_entry199 ]
  %_param_1.1 = phi i32 [ %_arg_1, %brif_next195 ], [ %290, %loop_entry199 ]
  %_param_0.17 = phi i32 [ %_param_0.10, %brif_next195 ], [ %291, %loop_entry199 ]
  %287 = inttoptr i32 %_param_2.3 to i32*
  %288 = load i32, i32* %287, align 4
  %289 = inttoptr i32 %_param_0.17 to i32*
  store i32 %288, i32* %289, align 4
  %290 = add i32 %_param_1.1, -1
  %291 = add i32 %_param_0.17, 4
  %292 = add i32 %_param_2.3, 4
  %293 = add i32 %_local_19.10, -1
  %brif_val202.not = icmp eq i32 %293, 0
  br i1 %brif_val202.not, label %blk_exit196, label %loop_entry199

loop_entry206:                                    ; preds = %loop_entry206, %blk_exit196
  %_param_2.4 = phi i32 [ %_param_2.2, %blk_exit196 ], [ %333, %loop_entry206 ]
  %_param_1.2 = phi i32 [ %_param_1.0, %blk_exit196 ], [ %334, %loop_entry206 ]
  %_param_0.18 = phi i32 [ %_param_0.16, %blk_exit196 ], [ %332, %loop_entry206 ]
  %294 = inttoptr i32 %_param_2.4 to i32*
  %295 = load i32, i32* %294, align 4
  %296 = inttoptr i32 %_param_0.18 to i32*
  store i32 %295, i32* %296, align 4
  %297 = add i32 %_param_0.18, 4
  %298 = add i32 %_param_2.4, 4
  %299 = inttoptr i32 %298 to i32*
  %300 = load i32, i32* %299, align 4
  %301 = inttoptr i32 %297 to i32*
  store i32 %300, i32* %301, align 4
  %302 = add i32 %_param_0.18, 8
  %303 = add i32 %_param_2.4, 8
  %304 = inttoptr i32 %303 to i32*
  %305 = load i32, i32* %304, align 4
  %306 = inttoptr i32 %302 to i32*
  store i32 %305, i32* %306, align 4
  %307 = add i32 %_param_0.18, 12
  %308 = add i32 %_param_2.4, 12
  %309 = inttoptr i32 %308 to i32*
  %310 = load i32, i32* %309, align 4
  %311 = inttoptr i32 %307 to i32*
  store i32 %310, i32* %311, align 4
  %312 = add i32 %_param_0.18, 16
  %313 = add i32 %_param_2.4, 16
  %314 = inttoptr i32 %313 to i32*
  %315 = load i32, i32* %314, align 4
  %316 = inttoptr i32 %312 to i32*
  store i32 %315, i32* %316, align 4
  %317 = add i32 %_param_0.18, 20
  %318 = add i32 %_param_2.4, 20
  %319 = inttoptr i32 %318 to i32*
  %320 = load i32, i32* %319, align 4
  %321 = inttoptr i32 %317 to i32*
  store i32 %320, i32* %321, align 4
  %322 = add i32 %_param_0.18, 24
  %323 = add i32 %_param_2.4, 24
  %324 = inttoptr i32 %323 to i32*
  %325 = load i32, i32* %324, align 4
  %326 = inttoptr i32 %322 to i32*
  store i32 %325, i32* %326, align 4
  %327 = add i32 %_param_0.18, 28
  %328 = add i32 %_param_2.4, 28
  %329 = inttoptr i32 %328 to i32*
  %330 = load i32, i32* %329, align 4
  %331 = inttoptr i32 %327 to i32*
  store i32 %330, i32* %331, align 4
  %332 = add i32 %_param_0.18, 32
  %333 = add i32 %_param_2.4, 32
  %334 = add i32 %_param_1.2, -8
  %brif_val223.not = icmp eq i32 %334, 0
  br i1 %brif_val223.not, label %blk_exit184, label %loop_entry206

loop_entry225:                                    ; preds = %loop_entry225, %blk_exit186
  %_local_22.9 = phi i64 [ 0, %blk_exit186 ], [ %372, %loop_entry225 ]
  %_local_23.15 = phi i32 [ %276, %blk_exit186 ], [ %375, %loop_entry225 ]
  %_param_2.5 = phi i32 [ %_arg_2, %blk_exit186 ], [ %374, %loop_entry225 ]
  %_param_0.19 = phi i32 [ %_param_0.10, %blk_exit186 ], [ %373, %loop_entry225 ]
  %335 = inttoptr i32 %_param_2.5 to i32*
  %336 = load i32, i32* %335, align 4
  %337 = zext i32 %336 to i64
  %338 = shl i64 %337, %280
  %339 = add nuw i64 %338, %_local_22.9
  %340 = trunc i64 %339 to i32
  %341 = inttoptr i32 %_param_0.19 to i32*
  store i32 %340, i32* %341, align 4
  %342 = add i32 %_param_0.19, 4
  %343 = add i32 %_param_2.5, 4
  %344 = inttoptr i32 %343 to i32*
  %345 = load i32, i32* %344, align 4
  %346 = zext i32 %345 to i64
  %347 = shl i64 %346, %280
  %348 = lshr i64 %339, 32
  %349 = add nuw i64 %347, %348
  %350 = trunc i64 %349 to i32
  %351 = inttoptr i32 %342 to i32*
  store i32 %350, i32* %351, align 4
  %352 = add i32 %_param_0.19, 8
  %353 = add i32 %_param_2.5, 8
  %354 = inttoptr i32 %353 to i32*
  %355 = load i32, i32* %354, align 4
  %356 = zext i32 %355 to i64
  %357 = shl i64 %356, %280
  %358 = lshr i64 %349, 32
  %359 = add nuw i64 %357, %358
  %360 = trunc i64 %359 to i32
  %361 = inttoptr i32 %352 to i32*
  store i32 %360, i32* %361, align 4
  %362 = add i32 %_param_0.19, 12
  %363 = add i32 %_param_2.5, 12
  %364 = inttoptr i32 %363 to i32*
  %365 = load i32, i32* %364, align 4
  %366 = zext i32 %365 to i64
  %367 = shl i64 %366, %280
  %368 = lshr i64 %359, 32
  %369 = add nuw i64 %367, %368
  %370 = trunc i64 %369 to i32
  %371 = inttoptr i32 %362 to i32*
  store i32 %370, i32* %371, align 4
  %372 = lshr i64 %369, 32
  %373 = add i32 %_param_0.19, 16
  %374 = add i32 %_param_2.5, 16
  %375 = add i32 %_local_23.15, -4
  %brif_val234.not = icmp eq i32 %375, 0
  br i1 %brif_val234.not, label %brif_next235, label %loop_entry225

brif_next235:                                     ; preds = %loop_entry225
  %376 = add i32 %_param_0.19, 12
  br label %blk_exit185

blk_exit236:                                      ; preds = %loop_entry239, %blk_exit185
  %_local_22.10 = phi i64 [ %_local_22.8, %blk_exit185 ], [ %387, %loop_entry239 ]
  %_local_20.10 = phi i64 [ %_local_20.9, %blk_exit185 ], [ %382, %loop_entry239 ]
  %_local_23.16 = phi i32 [ %_local_23.14, %blk_exit185 ], [ %_param_0.21, %loop_entry239 ]
  %_param_0.20 = phi i32 [ %_param_0.15, %blk_exit185 ], [ %385, %loop_entry239 ]
  %377 = icmp ult i64 %_local_20.10, 4294967296
  br i1 %377, label %blk_exit184, label %brif_next245

loop_entry239:                                    ; preds = %loop_entry239, %blk_exit185
  %_local_22.11 = phi i64 [ %_local_22.8, %blk_exit185 ], [ %387, %loop_entry239 ]
  %_local_19.11 = phi i32 [ %279, %blk_exit185 ], [ %388, %loop_entry239 ]
  %_param_2.6 = phi i32 [ %_param_2.1, %blk_exit185 ], [ %386, %loop_entry239 ]
  %_param_0.21 = phi i32 [ %_param_0.15, %blk_exit185 ], [ %385, %loop_entry239 ]
  %378 = inttoptr i32 %_param_2.6 to i32*
  %379 = load i32, i32* %378, align 4
  %380 = zext i32 %379 to i64
  %381 = shl i64 %380, %280
  %382 = add i64 %381, %_local_22.11
  %383 = trunc i64 %382 to i32
  %384 = inttoptr i32 %_param_0.21 to i32*
  store i32 %383, i32* %384, align 4
  %385 = add i32 %_param_0.21, 4
  %386 = add i32 %_param_2.6, 4
  %387 = lshr i64 %382, 32
  %388 = add i32 %_local_19.11, -1
  %brif_val242.not = icmp eq i32 %388, 0
  br i1 %brif_val242.not, label %blk_exit236, label %loop_entry239

brif_next245:                                     ; preds = %blk_exit236
  %389 = trunc i64 %_local_22.10 to i32
  %390 = inttoptr i32 %_param_0.20 to i32*
  store i32 %389, i32* %390, align 4
  %391 = add i32 %_local_23.16, 8
  br label %blk_exit184

blk_exit247:                                      ; preds = %blk_exit46
  %calcOffset252 = add i32 %0, -8
  %392 = inttoptr i32 %calcOffset252 to i32*
  %393 = load i32, i32* %392, align 4
  %394 = uitofp i32 %393 to float
  %395 = fmul float %394, 0x3FF1205BC0000000
  %396 = fcmp uge float %395, 0x41F0000000000000
  %397 = fptoui float %395 to i32
  %calcOffset255 = add i32 %0, -4
  %398 = inttoptr i32 %calcOffset255 to i32*
  %399 = load i32, i32* %398, align 4
  %.op334 = mul i32 %397, 9
  %400 = select i1 %396, i32 0, i32 %.op334
  %401 = add i32 %400, %8
  %402 = add i32 %401, 9
  %403 = icmp ult i32 %402, %8
  %404 = select i1 %403, i32 -1, i32 %402
  %405 = add i32 %404, 1
  %406 = icmp eq i32 %404, -1
  %407 = select i1 %406, i32 -1, i32 %405
  %408 = call i32 @rpl_malloc(i32 %407)
  %409 = icmp eq i32 %408, 0
  br i1 %409, label %blk_exit256, label %brif_next260

blk_exit256:                                      ; preds = %blk_exit318, %blk_exit247
  call void @rpl_free(i32 %_param_2.0)
  br label %blk_exit4

brif_next260:                                     ; preds = %blk_exit247
  %410 = icmp eq i32 %8, 0
  br i1 %410, label %blk_exit261, label %brif_next263

blk_exit261:                                      ; preds = %loop_entry274, %blk_exit264, %brif_next260
  %_local_23.17 = phi i32 [ %408, %brif_next260 ], [ %_local_23.18, %blk_exit264 ], [ %420, %loop_entry274 ]
  %411 = icmp eq i32 %393, 0
  br i1 %411, label %blk_exit278, label %brif_next280

brif_next263:                                     ; preds = %brif_next260
  %412 = add i32 %4, -1
  %413 = and i32 %4, 7
  %brif_val266.not = icmp eq i32 %413, 0
  br i1 %brif_val266.not, label %blk_exit264, label %loop_entry268

blk_exit264:                                      ; preds = %loop_entry268, %brif_next263
  %_local_6.0 = phi i32 [ %416, %loop_entry268 ], [ %4, %brif_next263 ]
  %_local_23.18 = phi i32 [ %417, %loop_entry268 ], [ %408, %brif_next263 ]
  %414 = icmp ult i32 %412, 7
  br i1 %414, label %blk_exit261, label %loop_entry274

loop_entry268:                                    ; preds = %loop_entry268, %brif_next263
  %_local_6.1 = phi i32 [ %416, %loop_entry268 ], [ %4, %brif_next263 ]
  %_param_4.8 = phi i32 [ %418, %loop_entry268 ], [ %413, %brif_next263 ]
  %_local_23.19 = phi i32 [ %417, %loop_entry268 ], [ %408, %brif_next263 ]
  %415 = inttoptr i32 %_local_23.19 to i8*
  store i8 48, i8* %415, align 1
  %416 = add i32 %_local_6.1, -1
  %417 = add i32 %_local_23.19, 1
  %418 = add i32 %_param_4.8, -1
  %brif_val270.not = icmp eq i32 %418, 0
  br i1 %brif_val270.not, label %blk_exit264, label %loop_entry268

loop_entry274:                                    ; preds = %loop_entry274, %blk_exit264
  %_local_6.2 = phi i32 [ %_local_6.0, %blk_exit264 ], [ %421, %loop_entry274 ]
  %_local_23.20 = phi i32 [ %_local_23.18, %blk_exit264 ], [ %420, %loop_entry274 ]
  %419 = inttoptr i32 %_local_23.20 to i64*
  store i64 3472328296227680304, i64* %419, align 8
  %420 = add i32 %_local_23.20, 8
  %421 = add i32 %_local_6.2, -8
  %brif_val276.not = icmp eq i32 %421, 0
  br i1 %brif_val276.not, label %blk_exit261, label %loop_entry274

blk_exit278:                                      ; preds = %blk_exit293, %blk_exit261
  %_local_23.21 = phi i32 [ %_local_23.17, %blk_exit261 ], [ %494, %blk_exit293 ]
  br label %loop_entry312

brif_next280:                                     ; preds = %blk_exit261
  %422 = add i32 %399, -8
  br label %loop_entry281

loop_entry281:                                    ; preds = %blk_exit293, %brif_next280
  %_local_21.1 = phi i32 [ %393, %brif_next280 ], [ %500, %blk_exit293 ]
  %_local_23.22 = phi i32 [ %_local_23.17, %brif_next280 ], [ %494, %blk_exit293 ]
  %423 = and i32 %_local_21.1, 1
  %424 = shl i32 %_local_21.1, 2
  %.not327 = icmp eq i32 %_local_21.1, 1
  br i1 %.not327, label %brif_next285, label %blk_exit283

blk_exit282:                                      ; preds = %loop_entry286, %brif_next285
  %_local_18.0 = phi i64 [ 0, %brif_next285 ], [ %445, %loop_entry286 ]
  %_param_4.9 = phi i32 [ %428, %brif_next285 ], [ %_param_4.10, %loop_entry286 ]
  %425 = icmp eq i32 %423, 0
  br i1 %425, label %blk_exit293, label %brif_next295

blk_exit283:                                      ; preds = %loop_entry281
  %426 = and i32 %_local_21.1, -2
  %427 = add i32 %422, %424
  br label %loop_entry286

brif_next285:                                     ; preds = %loop_entry281
  %428 = add i32 %399, %424
  br label %blk_exit282

loop_entry286:                                    ; preds = %loop_entry286, %blk_exit283
  %_local_18.1 = phi i64 [ 0, %blk_exit283 ], [ %445, %loop_entry286 ]
  %_param_4.10 = phi i32 [ %427, %blk_exit283 ], [ %446, %loop_entry286 ]
  %_param_0.22 = phi i32 [ %426, %blk_exit283 ], [ %447, %loop_entry286 ]
  %429 = add i32 %_param_4.10, 4
  %430 = shl i64 %_local_18.1, 32
  %431 = inttoptr i32 %429 to i32*
  %432 = load i32, i32* %431, align 4
  %433 = zext i32 %432 to i64
  %434 = or i64 %430, %433
  %435 = udiv i64 %434, 1000000000
  %436 = trunc i64 %435 to i32
  store i32 %436, i32* %431, align 4
  %.neg331 = mul i64 %435, -1000000000
  %437 = add i64 %.neg331, %433
  %438 = shl i64 %437, 32
  %439 = inttoptr i32 %_param_4.10 to i32*
  %440 = load i32, i32* %439, align 4
  %441 = zext i32 %440 to i64
  %442 = or i64 %438, %441
  %443 = udiv i64 %442, 1000000000
  %444 = trunc i64 %443 to i32
  store i32 %444, i32* %439, align 4
  %.neg332 = mul i64 %443, -1000000000
  %445 = add i64 %.neg332, %442
  %446 = add i32 %_param_4.10, -8
  %447 = add i32 %_param_0.22, -2
  %brif_val291.not = icmp eq i32 %447, 0
  br i1 %brif_val291.not, label %blk_exit282, label %loop_entry286

blk_exit293:                                      ; preds = %brif_next295, %blk_exit282
  %_local_18.2 = phi i64 [ %_local_18.0, %blk_exit282 ], [ %509, %brif_next295 ]
  %448 = trunc i64 %_local_18.2 to i32
  %449 = udiv i32 %448, 100000000
  %450 = urem i32 %449, 10
  %calcOffset298 = add i32 %_local_23.22, 8
  %451 = inttoptr i32 %calcOffset298 to i8*
  %452 = trunc i32 %450 to i8
  %453 = or i8 %452, 48
  store i8 %453, i8* %451, align 1
  %454 = udiv i32 %448, 1000000
  %455 = urem i32 %454, 10
  %calcOffset299 = add i32 %_local_23.22, 6
  %456 = inttoptr i32 %calcOffset299 to i8*
  %457 = trunc i32 %455 to i8
  %458 = or i8 %457, 48
  store i8 %458, i8* %456, align 1
  %459 = udiv i32 %448, 100000
  %460 = urem i32 %459, 10
  %calcOffset300 = add i32 %_local_23.22, 5
  %461 = inttoptr i32 %calcOffset300 to i8*
  %462 = trunc i32 %460 to i8
  %463 = or i8 %462, 48
  store i8 %463, i8* %461, align 1
  %464 = udiv i32 %448, 10000
  %465 = urem i32 %464, 10
  %calcOffset301 = add i32 %_local_23.22, 4
  %466 = inttoptr i32 %calcOffset301 to i8*
  %467 = trunc i32 %465 to i8
  %468 = or i8 %467, 48
  store i8 %468, i8* %466, align 1
  %469 = udiv i32 %448, 1000
  %470 = urem i32 %469, 10
  %calcOffset302 = add i32 %_local_23.22, 3
  %471 = inttoptr i32 %calcOffset302 to i8*
  %472 = trunc i32 %470 to i8
  %473 = or i8 %472, 48
  store i8 %473, i8* %471, align 1
  %474 = udiv i32 %448, 100
  %475 = urem i32 %474, 10
  %calcOffset303 = add i32 %_local_23.22, 2
  %476 = inttoptr i32 %calcOffset303 to i8*
  %477 = trunc i32 %475 to i8
  %478 = or i8 %477, 48
  store i8 %478, i8* %476, align 1
  %479 = udiv i32 %448, 10000000
  %480 = and i32 %479, 255
  %481 = urem i32 %480, 10
  %calcOffset304 = add i32 %_local_23.22, 7
  %482 = inttoptr i32 %calcOffset304 to i8*
  %483 = trunc i32 %481 to i8
  %484 = or i8 %483, 48
  store i8 %484, i8* %482, align 1
  %485 = udiv i32 %448, 10
  %486 = urem i32 %485, 10
  %calcOffset305 = add i32 %_local_23.22, 1
  %487 = inttoptr i32 %calcOffset305 to i8*
  %488 = trunc i32 %486 to i8
  %489 = or i8 %488, 48
  store i8 %489, i8* %487, align 1
  %.neg328 = mul i32 %485, -10
  %490 = add i32 %.neg328, %448
  %491 = inttoptr i32 %_local_23.22 to i8*
  %492 = trunc i32 %490 to i8
  %493 = or i8 %492, 48
  store i8 %493, i8* %491, align 1
  %494 = add i32 %_local_23.22, 9
  %495 = add i32 %_local_21.1, -1
  %496 = shl i32 %495, 2
  %497 = add i32 %399, %496
  %498 = inttoptr i32 %497 to i32*
  %499 = load i32, i32* %498, align 4
  %select_cond308.not = icmp eq i32 %499, 0
  %500 = select i1 %select_cond308.not, i32 %495, i32 %_local_21.1
  %brif_val309.not = icmp eq i32 %500, 0
  br i1 %brif_val309.not, label %blk_exit278, label %loop_entry281

brif_next295:                                     ; preds = %blk_exit282
  %501 = add i32 %_param_4.9, -4
  %502 = shl i64 %_local_18.0, 32
  %503 = inttoptr i32 %501 to i32*
  %504 = load i32, i32* %503, align 4
  %505 = zext i32 %504 to i64
  %506 = or i64 %502, %505
  %507 = udiv i64 %506, 1000000000
  %508 = trunc i64 %507 to i32
  store i32 %508, i32* %503, align 4
  %.neg = mul i64 %507, -1000000000
  %509 = add i64 %.neg, %506
  br label %blk_exit293

blk_exit311:                                      ; preds = %brif_next314, %loop_entry312
  %.not330 = icmp eq i32 %_local_23.23, %408
  br i1 %.not330, label %brif_next320, label %blk_exit318

loop_entry312:                                    ; preds = %brif_next314, %blk_exit278
  %_local_23.23 = phi i32 [ %_local_23.21, %blk_exit278 ], [ %510, %brif_next314 ]
  %.not329 = icmp ugt i32 %_local_23.23, %408
  br i1 %.not329, label %brif_next314, label %blk_exit311

brif_next314:                                     ; preds = %loop_entry312
  %510 = add i32 %_local_23.23, -1
  %511 = inttoptr i32 %510 to i8*
  %512 = load i8, i8* %511, align 1
  %513 = icmp eq i8 %512, 48
  br i1 %513, label %loop_entry312, label %blk_exit311

blk_exit318:                                      ; preds = %brif_next320, %blk_exit311
  %_param_4.11 = phi i32 [ %516, %brif_next320 ], [ %_local_23.23, %blk_exit311 ]
  %514 = inttoptr i32 %_param_4.11 to i8*
  store i8 0, i8* %514, align 1
  br label %blk_exit256

brif_next320:                                     ; preds = %blk_exit311
  %515 = inttoptr i32 %408 to i8*
  store i8 48, i8* %515, align 1
  %516 = add i32 %408, 1
  br label %blk_exit318
}

; Function Attrs: null_pointer_is_valid
define internal i32 @multiply(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2, i32 %_arg_3, i32 %_arg_4) #0 {
allocator:
  %0 = icmp ugt i32 %_arg_0, %_arg_2
  %1 = select i1 %0, i32 %_arg_2, i32 %_arg_0
  %brif_val.not = icmp eq i32 %1, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit

return:                                           ; preds = %blk_exit71, %brif_next, %blk_exit
  %return_0 = phi i32 [ 0, %brif_next ], [ %5, %blk_exit71 ], [ -1, %blk_exit ]
  ret i32 %return_0

blk_exit:                                         ; preds = %allocator
  %2 = select i1 %0, i32 %_arg_0, i32 %_arg_2
  %3 = add i32 %1, %2
  %4 = shl i32 %3, 2
  %5 = call i32 @rpl_malloc(i32 %4)
  %brif_val3.not = icmp eq i32 %5, 0
  br i1 %brif_val3.not, label %return, label %blk_exit1

brif_next:                                        ; preds = %allocator
  %6 = inttoptr i32 %_arg_4 to i64*
  store i64 0, i64* %6, align 8
  br label %return

blk_exit1:                                        ; preds = %blk_exit
  %7 = icmp eq i32 %2, 0
  br i1 %7, label %blk_exit6, label %brif_next8

blk_exit5:                                        ; preds = %loop_entry67, %blk_exit55, %blk_exit45
  %8 = add i32 %4, %5
  br label %loop_entry72

blk_exit6:                                        ; preds = %blk_exit1
  %9 = icmp ugt i32 %1, 1
  %10 = select i1 %9, i32 %1, i32 1
  %11 = and i32 %10, 7
  %12 = add i32 %10, -1
  %13 = icmp ult i32 %12, 7
  br i1 %13, label %blk_exit55, label %brif_next57

brif_next8:                                       ; preds = %blk_exit1
  %14 = add i32 %2, -1
  %15 = and i32 %2, 7
  %16 = icmp eq i32 %15, 0
  br i1 %16, label %blk_exit9, label %brif_next11

blk_exit9:                                        ; preds = %loop_entry, %brif_next8
  %_param_2.0 = phi i32 [ %2, %brif_next8 ], [ %21, %loop_entry ]
  %17 = icmp ult i32 %14, 7
  br i1 %17, label %brif_next26, label %brif_next17

brif_next11:                                      ; preds = %brif_next8
  %18 = shl i32 %2, 2
  %19 = add i32 %18, %5
  br label %loop_entry

loop_entry:                                       ; preds = %loop_entry, %brif_next11
  %_local_11.0 = phi i32 [ %15, %brif_next11 ], [ %22, %loop_entry ]
  %_param_2.1 = phi i32 [ %2, %brif_next11 ], [ %21, %loop_entry ]
  %_param_0.0.in = phi i32 [ %19, %brif_next11 ], [ %_param_0.0, %loop_entry ]
  %_param_0.0 = add i32 %_param_0.0.in, -4
  %20 = inttoptr i32 %_param_0.0 to i32*
  store i32 0, i32* %20, align 4
  %21 = add i32 %_param_2.1, -1
  %22 = add i32 %_local_11.0, -1
  %brif_val13.not = icmp eq i32 %22, 0
  br i1 %brif_val13.not, label %blk_exit9, label %loop_entry

brif_next17:                                      ; preds = %blk_exit9
  %23 = shl i32 %_param_2.0, 2
  %24 = add i32 %23, %5
  br label %loop_entry18

loop_entry18:                                     ; preds = %loop_entry18, %brif_next17
  %_param_2.2 = phi i32 [ %_param_2.0, %brif_next17 ], [ %32, %loop_entry18 ]
  %_param_0.1.in = phi i32 [ %24, %brif_next17 ], [ %_param_0.1, %loop_entry18 ]
  %_param_0.1 = add i32 %_param_0.1.in, -32
  %25 = inttoptr i32 %_param_0.1 to i64*
  store i64 0, i64* %25, align 8
  %26 = add i32 %_param_0.1.in, -8
  %27 = inttoptr i32 %26 to i64*
  store i64 0, i64* %27, align 8
  %28 = add i32 %_param_0.1.in, -16
  %29 = inttoptr i32 %28 to i64*
  store i64 0, i64* %29, align 8
  %30 = add i32 %_param_0.1.in, -24
  %31 = inttoptr i32 %30 to i64*
  store i64 0, i64* %31, align 8
  %32 = add i32 %_param_2.2, -8
  %brif_val23.not = icmp eq i32 %32, 0
  br i1 %brif_val23.not, label %brif_next26, label %loop_entry18

brif_next26:                                      ; preds = %loop_entry18, %blk_exit9
  %33 = select i1 %0, i32 %_arg_1, i32 %_arg_3
  %34 = select i1 %0, i32 %_arg_3, i32 %_arg_1
  %35 = icmp ugt i32 %1, 1
  %36 = select i1 %35, i32 %1, i32 1
  %37 = and i32 %2, -2
  %38 = and i32 %2, 1
  br label %loop_entry30

loop_entry30:                                     ; preds = %blk_exit45, %brif_next26
  %_param_3.0 = phi i32 [ %5, %brif_next26 ], [ %77, %blk_exit45 ]
  %_param_1.0 = phi i32 [ 0, %brif_next26 ], [ %78, %blk_exit45 ]
  %39 = shl i32 %_param_1.0, 2
  %40 = add i32 %34, %39
  %41 = inttoptr i32 %40 to i32*
  %42 = load i32, i32* %41, align 4
  %43 = zext i32 %42 to i64
  %brif_val34.not = icmp eq i32 %14, 0
  br i1 %brif_val34.not, label %blk_exit32, label %loop_entry36

blk_exit32:                                       ; preds = %loop_entry36, %loop_entry30
  %_local_11.1 = phi i32 [ %37, %loop_entry36 ], [ 0, %loop_entry30 ]
  %_local_17.0 = phi i64 [ %68, %loop_entry36 ], [ 0, %loop_entry30 ]
  %44 = icmp eq i32 %38, 0
  br i1 %44, label %blk_exit45, label %brif_next47

loop_entry36:                                     ; preds = %loop_entry36, %loop_entry30
  %_local_11.2 = phi i32 [ %71, %loop_entry36 ], [ 0, %loop_entry30 ]
  %_local_17.1 = phi i64 [ %68, %loop_entry36 ], [ 0, %loop_entry30 ]
  %_param_2.3 = phi i32 [ %70, %loop_entry36 ], [ %33, %loop_entry30 ]
  %_param_0.2 = phi i32 [ %69, %loop_entry36 ], [ %_param_3.0, %loop_entry30 ]
  %45 = inttoptr i32 %_param_2.3 to i32*
  %46 = load i32, i32* %45, align 4
  %47 = zext i32 %46 to i64
  %48 = mul nuw i64 %43, %47
  %49 = add i64 %48, %_local_17.1
  %50 = inttoptr i32 %_param_0.2 to i32*
  %51 = load i32, i32* %50, align 4
  %52 = zext i32 %51 to i64
  %53 = add i64 %49, %52
  %54 = trunc i64 %53 to i32
  store i32 %54, i32* %50, align 4
  %55 = add i32 %_param_0.2, 4
  %56 = add i32 %_param_2.3, 4
  %57 = inttoptr i32 %56 to i32*
  %58 = load i32, i32* %57, align 4
  %59 = zext i32 %58 to i64
  %60 = mul nuw i64 %43, %59
  %61 = lshr i64 %53, 32
  %62 = add i64 %60, %61
  %63 = inttoptr i32 %55 to i32*
  %64 = load i32, i32* %63, align 4
  %65 = zext i32 %64 to i64
  %66 = add i64 %62, %65
  %67 = trunc i64 %66 to i32
  store i32 %67, i32* %63, align 4
  %68 = lshr i64 %66, 32
  %69 = add i32 %_param_0.2, 8
  %70 = add i32 %_param_2.3, 8
  %71 = add i32 %_local_11.2, 2
  %.not80 = icmp eq i32 %37, %71
  br i1 %.not80, label %blk_exit32, label %loop_entry36

blk_exit45:                                       ; preds = %brif_next47, %blk_exit32
  %_local_17.2 = phi i64 [ %_local_17.0, %blk_exit32 ], [ %94, %brif_next47 ]
  %72 = add i32 %_param_1.0, %2
  %73 = shl i32 %72, 2
  %74 = add i32 %5, %73
  %75 = trunc i64 %_local_17.2 to i32
  %76 = inttoptr i32 %74 to i32*
  store i32 %75, i32* %76, align 4
  %77 = add i32 %_param_3.0, 4
  %78 = add i32 %_param_1.0, 1
  %.not = icmp eq i32 %78, %36
  br i1 %.not, label %blk_exit5, label %loop_entry30

brif_next47:                                      ; preds = %blk_exit32
  %79 = add i32 %_local_11.1, %_param_1.0
  %80 = shl i32 %79, 2
  %81 = add i32 %5, %80
  %82 = shl i32 %_local_11.1, 2
  %83 = add i32 %33, %82
  %84 = inttoptr i32 %83 to i32*
  %85 = load i32, i32* %84, align 4
  %86 = zext i32 %85 to i64
  %87 = mul nuw i64 %43, %86
  %88 = add i64 %87, %_local_17.0
  %89 = inttoptr i32 %81 to i32*
  %90 = load i32, i32* %89, align 4
  %91 = zext i32 %90 to i64
  %92 = add i64 %88, %91
  %93 = trunc i64 %92 to i32
  store i32 %93, i32* %89, align 4
  %94 = lshr i64 %92, 32
  br label %blk_exit45

blk_exit55:                                       ; preds = %loop_entry58, %blk_exit6
  %_local_11.3 = phi i32 [ 0, %blk_exit6 ], [ %97, %loop_entry58 ]
  %95 = icmp eq i32 %11, 0
  br i1 %95, label %blk_exit5, label %brif_next66

brif_next57:                                      ; preds = %blk_exit6
  %96 = add i32 %5, 16
  %97 = and i32 %10, -8
  br label %loop_entry58

loop_entry58:                                     ; preds = %loop_entry58, %brif_next57
  %_local_11.4 = phi i32 [ 0, %brif_next57 ], [ %106, %loop_entry58 ]
  %_param_0.3 = phi i32 [ %96, %brif_next57 ], [ %105, %loop_entry58 ]
  %98 = inttoptr i32 %_param_0.3 to i64*
  store i64 0, i64* %98, align 8
  %99 = add i32 %_param_0.3, 8
  %100 = inttoptr i32 %99 to i64*
  store i64 0, i64* %100, align 8
  %101 = add i32 %_param_0.3, -8
  %102 = inttoptr i32 %101 to i64*
  store i64 0, i64* %102, align 8
  %103 = add i32 %_param_0.3, -16
  %104 = inttoptr i32 %103 to i64*
  store i64 0, i64* %104, align 8
  %105 = add i32 %_param_0.3, 32
  %106 = add i32 %_local_11.4, 8
  %.not81 = icmp eq i32 %97, %106
  br i1 %.not81, label %blk_exit55, label %loop_entry58

brif_next66:                                      ; preds = %blk_exit55
  %107 = shl i32 %_local_11.3, 2
  %108 = add i32 %5, %107
  br label %loop_entry67

loop_entry67:                                     ; preds = %loop_entry67, %brif_next66
  %_param_2.4 = phi i32 [ %11, %brif_next66 ], [ %111, %loop_entry67 ]
  %_param_0.4 = phi i32 [ %108, %brif_next66 ], [ %110, %loop_entry67 ]
  %109 = inttoptr i32 %_param_0.4 to i32*
  store i32 0, i32* %109, align 4
  %110 = add i32 %_param_0.4, 4
  %111 = add i32 %_param_2.4, -1
  %brif_val69.not = icmp eq i32 %111, 0
  br i1 %brif_val69.not, label %blk_exit5, label %loop_entry67

blk_exit71:                                       ; preds = %brif_next74, %loop_entry72
  %calcOffset78 = add i32 %_arg_4, 4
  %112 = inttoptr i32 %calcOffset78 to i32*
  store i32 %5, i32* %112, align 4
  %113 = inttoptr i32 %_arg_4 to i32*
  store i32 %_local_8.0, i32* %113, align 4
  br label %return

loop_entry72:                                     ; preds = %brif_next74, %blk_exit5
  %_local_8.0 = phi i32 [ %3, %blk_exit5 ], [ %115, %brif_next74 ]
  %_param_0.5.in = phi i32 [ %8, %blk_exit5 ], [ %_param_0.5, %brif_next74 ]
  %_param_0.5 = add i32 %_param_0.5.in, -4
  %114 = icmp eq i32 %_local_8.0, 0
  br i1 %114, label %blk_exit71, label %brif_next74

brif_next74:                                      ; preds = %loop_entry72
  %115 = add i32 %_local_8.0, -1
  %116 = inttoptr i32 %_param_0.5 to i32*
  %117 = load i32, i32* %116, align 4
  %118 = icmp eq i32 %117, 0
  br i1 %118, label %loop_entry72, label %blk_exit71
}

; Function Attrs: null_pointer_is_valid
define internal i32 @divide(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2, i32 %_arg_3, i32 %_arg_4) #0 {
allocator:
  %0 = shl i32 %_arg_0, 2
  %1 = add i32 %0, 8
  %2 = call i32 @rpl_malloc(i32 %1)
  %brif_val.not = icmp eq i32 %2, 0
  br i1 %brif_val.not, label %return, label %blk_exit

return:                                           ; preds = %blk_exit8, %blk_exit6, %allocator
  %return_0 = phi i32 [ -1, %blk_exit8 ], [ %2, %blk_exit6 ], [ -1, %allocator ]
  ret i32 %return_0

blk_exit:                                         ; preds = %allocator
  %3 = add i32 %2, %0
  %4 = add i32 %0, %_arg_1
  br label %loop_entry

blk_exit1:                                        ; preds = %brif_next3, %loop_entry
  %5 = add i32 %_arg_2, 1
  %6 = shl i32 %_arg_2, 2
  %7 = add i32 %6, %_arg_3
  %8 = add i32 %7, -4
  br label %loop_entry10

loop_entry:                                       ; preds = %brif_next3, %blk_exit
  %_local_9.0 = phi i32 [ %_arg_0, %blk_exit ], [ %12, %brif_next3 ]
  %_local_8.0.in = phi i32 [ %4, %blk_exit ], [ %_local_8.0, %brif_next3 ]
  %_local_7.0 = phi i32 [ %3, %blk_exit ], [ %11, %brif_next3 ]
  %_param_0.0 = phi i32 [ %_arg_0, %blk_exit ], [ %10, %brif_next3 ]
  %_local_8.0 = add i32 %_local_8.0.in, -4
  %9 = icmp eq i32 %_local_9.0, 0
  br i1 %9, label %blk_exit1, label %brif_next3

brif_next3:                                       ; preds = %loop_entry
  %10 = add i32 %_param_0.0, 3
  %11 = add i32 %_local_7.0, -4
  %12 = add i32 %_local_9.0, -1
  %13 = shl i32 %12, 2
  %14 = add i32 %13, %_arg_1
  %15 = inttoptr i32 %14 to i32*
  %16 = load i32, i32* %15, align 4
  %17 = icmp eq i32 %16, 0
  br i1 %17, label %loop_entry, label %blk_exit1

blk_exit6:                                        ; preds = %loop_entry206, %brif_next198, %brif_next194, %blk_exit17, %blk_exit7
  %_local_17.0 = phi i32 [ %_local_17.1, %blk_exit7 ], [ %_local_17.2, %loop_entry206 ], [ %_local_17.4, %brif_next194 ], [ %_local_17.4, %blk_exit17 ], [ %_local_17.4, %brif_next198 ]
  %_local_16.0 = phi i32 [ %_local_16.1, %blk_exit7 ], [ %_local_16.2, %loop_entry206 ], [ %_local_16.4, %brif_next194 ], [ %_local_16.4, %blk_exit17 ], [ %_local_16.4, %brif_next198 ]
  %_local_13.0 = phi i32 [ %23, %blk_exit7 ], [ %_local_13.2, %loop_entry206 ], [ %_local_13.4, %brif_next194 ], [ %_local_13.4, %blk_exit17 ], [ 0, %brif_next198 ]
  call void @rpl_free(i32 %_local_17.0)
  %18 = inttoptr i32 %_arg_4 to i32*
  store i32 %_local_13.0, i32* %18, align 4
  %calcOffset215 = add i32 %_arg_4, 4
  %19 = inttoptr i32 %calcOffset215 to i32*
  store i32 %_local_16.0, i32* %19, align 4
  br label %return

blk_exit7:                                        ; preds = %brif_next210, %blk_exit18
  %_local_17.1 = phi i32 [ %_local_17.2, %brif_next210 ], [ %_local_17.3, %blk_exit18 ]
  %_local_16.1 = phi i32 [ %_local_16.2, %brif_next210 ], [ %_local_16.3, %blk_exit18 ]
  %_local_13.1 = phi i32 [ %_local_13.2, %brif_next210 ], [ 0, %blk_exit18 ]
  %20 = shl i32 %_local_13.1, 2
  %21 = add i32 %_local_16.1, %20
  %22 = inttoptr i32 %21 to i32*
  store i32 1, i32* %22, align 4
  %23 = add i32 %_local_13.1, 1
  br label %blk_exit6

blk_exit8:                                        ; preds = %brif_next47
  call void @rpl_free(i32 %2)
  br label %return

blk_exit9:                                        ; preds = %blk_exit65, %loop_entry10
  call void @abort()
  unreachable

loop_entry10:                                     ; preds = %brif_next12, %blk_exit1
  %_local_9.1 = phi i32 [ %5, %blk_exit1 ], [ %25, %brif_next12 ]
  %_param_0.1 = phi i32 [ 0, %blk_exit1 ], [ %27, %brif_next12 ]
  %24 = icmp eq i32 %_local_9.1, 1
  br i1 %24, label %blk_exit9, label %brif_next12

brif_next12:                                      ; preds = %loop_entry10
  %25 = add i32 %_local_9.1, -1
  %26 = add i32 %8, %_param_0.1
  %27 = add i32 %_param_0.1, -4
  %28 = inttoptr i32 %26 to i32*
  %29 = load i32, i32* %28, align 4
  %30 = icmp eq i32 %29, 0
  br i1 %30, label %loop_entry10, label %brif_next15

brif_next15:                                      ; preds = %brif_next12
  %.not = icmp ult i32 %_local_9.0, %25
  br i1 %.not, label %brif_next23, label %blk_exit21

blk_exit16:                                       ; preds = %blk_exit18, %blk_exit17
  %_local_17.2 = phi i32 [ %_local_17.3, %blk_exit18 ], [ %_local_17.4, %blk_exit17 ]
  %_local_16.2 = phi i32 [ %_local_16.3, %blk_exit18 ], [ %_local_16.4, %blk_exit17 ]
  %_local_13.2 = phi i32 [ %_local_13.3, %blk_exit18 ], [ %_local_13.4, %blk_exit17 ]
  br label %loop_entry206

blk_exit17:                                       ; preds = %brif_next198
  %31 = inttoptr i32 %_local_16.4 to i8*
  %32 = load i8, i8* %31, align 1
  %33 = and i8 %32, 1
  %brif_val204.not.not = icmp eq i8 %33, 0
  br i1 %brif_val204.not.not, label %blk_exit6, label %blk_exit16

blk_exit18:                                       ; preds = %blk_exit189, %blk_exit20
  %_local_17.3 = phi i32 [ %_local_17.5, %blk_exit20 ], [ %_local_17.4, %blk_exit189 ]
  %_local_16.3 = phi i32 [ %_local_16.5, %blk_exit20 ], [ %_local_16.4, %blk_exit189 ]
  %_local_13.3 = phi i32 [ %_local_13.5, %blk_exit20 ], [ %_local_13.4, %blk_exit189 ]
  %brif_val201.not = icmp eq i32 %_local_13.3, 0
  br i1 %brif_val201.not, label %blk_exit7, label %blk_exit16

blk_exit19:                                       ; preds = %loop_entry169, %blk_exit20
  %_local_17.4 = phi i32 [ %_local_17.5, %blk_exit20 ], [ %_local_17.6, %loop_entry169 ]
  %_local_16.4 = phi i32 [ %_local_16.5, %blk_exit20 ], [ %82, %loop_entry169 ]
  %_local_13.4 = phi i32 [ %_local_13.5, %blk_exit20 ], [ %341, %loop_entry169 ]
  %_local_7.1 = phi i32 [ %_local_7.2, %blk_exit20 ], [ 0, %loop_entry169 ]
  %_param_3.0 = phi i32 [ %_param_3.1, %blk_exit20 ], [ %_local_21.0, %loop_entry169 ]
  %34 = add i32 %2, %27
  %35 = add i32 %_param_3.0, %27
  br label %loop_entry178

blk_exit20:                                       ; preds = %blk_exit170, %blk_exit27, %brif_next23
  %_local_17.5 = phi i32 [ 0, %brif_next23 ], [ 0, %blk_exit27 ], [ %_local_17.6, %blk_exit170 ]
  %_local_16.5 = phi i32 [ %39, %brif_next23 ], [ %42, %blk_exit27 ], [ %82, %blk_exit170 ]
  %_local_13.5 = phi i32 [ 0, %brif_next23 ], [ %48, %blk_exit27 ], [ %341, %blk_exit170 ]
  %_local_7.2 = phi i32 [ %_local_9.0, %brif_next23 ], [ %_local_7.3, %blk_exit27 ], [ %342, %blk_exit170 ]
  %_param_3.1 = phi i32 [ %_arg_3, %brif_next23 ], [ %_arg_3, %blk_exit27 ], [ %_local_21.0, %blk_exit170 ]
  %36 = icmp ugt i32 %_local_7.2, %25
  br i1 %36, label %blk_exit18, label %blk_exit19

blk_exit21:                                       ; preds = %brif_next15
  %.not219 = icmp eq i32 %25, 1
  br i1 %.not219, label %brif_next26, label %blk_exit24

brif_next23:                                      ; preds = %brif_next15
  %37 = shl i32 %_local_9.0, 2
  %38 = call i32 @memcpy(i32 %2, i32 %_arg_1, i32 %37)
  %39 = add i32 %2, %37
  br label %blk_exit20

blk_exit24:                                       ; preds = %blk_exit21
  %40 = call i32 @llvm.ctlz.i32(i32 %29, i1 true), !range !0
  %41 = icmp eq i32 %40, 0
  br i1 %41, label %blk_exit45, label %brif_next47

brif_next26:                                      ; preds = %blk_exit21
  %42 = add i32 %2, 4
  br i1 %9, label %blk_exit27, label %brif_next29

blk_exit27:                                       ; preds = %blk_exit36, %brif_next35, %brif_next26
  %_local_7.3 = phi i32 [ 0, %brif_next26 ], [ 1, %blk_exit36 ], [ 0, %brif_next35 ]
  %43 = add i32 %_local_9.0, -1
  %44 = shl i32 %43, 2
  %45 = add i32 %42, %44
  %46 = inttoptr i32 %45 to i32*
  %47 = load i32, i32* %46, align 4
  %select_cond.not = icmp eq i32 %47, 0
  %48 = select i1 %select_cond.not, i32 %43, i32 %_local_9.0
  br label %blk_exit20

brif_next29:                                      ; preds = %brif_next26
  %49 = inttoptr i32 %_arg_3 to i32*
  %50 = load i32, i32* %49, align 4
  %51 = zext i32 %50 to i64
  br label %loop_entry31

loop_entry31:                                     ; preds = %loop_entry31, %brif_next29
  %_local_19.0 = phi i64 [ 0, %brif_next29 ], [ %61, %loop_entry31 ]
  %_local_10.0 = phi i32 [ %_local_7.0, %brif_next29 ], [ %62, %loop_entry31 ]
  %_local_5.0 = phi i32 [ %_local_8.0, %brif_next29 ], [ %63, %loop_entry31 ]
  %_param_0.2 = phi i32 [ 0, %brif_next29 ], [ %64, %loop_entry31 ]
  %52 = shl i64 %_local_19.0, 32
  %53 = inttoptr i32 %_local_5.0 to i32*
  %54 = load i32, i32* %53, align 4
  %55 = zext i32 %54 to i64
  %56 = or i64 %52, %55
  %57 = udiv i64 %56, %51
  %58 = trunc i64 %57 to i32
  %59 = inttoptr i32 %_local_10.0 to i32*
  store i32 %58, i32* %59, align 4
  %60 = mul i64 %57, %51
  %61 = sub i64 %56, %60
  %62 = add i32 %_local_10.0, -4
  %63 = add i32 %_local_5.0, -4
  %64 = add i32 %_param_0.2, 1
  %.not220 = icmp eq i32 %_local_9.0, %64
  br i1 %.not220, label %brif_next35, label %loop_entry31

brif_next35:                                      ; preds = %loop_entry31
  %65 = trunc i64 %61 to i32
  %brif_val37.not = icmp eq i32 %65, 0
  br i1 %brif_val37.not, label %blk_exit27, label %blk_exit36

blk_exit36:                                       ; preds = %brif_next35
  %66 = inttoptr i32 %2 to i32*
  store i32 %65, i32* %66, align 4
  br label %blk_exit27

blk_exit41:                                       ; preds = %blk_exit45, %blk_exit42
  %.pre-phi = phi i32 [ %.pre, %blk_exit42 ], [ %7, %blk_exit45 ]
  %_local_21.0 = phi i32 [ %91, %blk_exit42 ], [ %_arg_3, %blk_exit45 ]
  %_local_17.6 = phi i32 [ %91, %blk_exit42 ], [ 0, %blk_exit45 ]
  %67 = add i32 %2, %6
  %68 = add i32 %67, %27
  %69 = add i32 %.pre-phi, %27
  %70 = inttoptr i32 %69 to i32*
  %71 = load i32, i32* %70, align 4
  %72 = zext i32 %71 to i64
  %73 = shl nuw i64 %72, 32
  %74 = add i32 %69, -4
  %75 = inttoptr i32 %74 to i32*
  %76 = load i32, i32* %75, align 4
  %77 = zext i32 %76 to i64
  %78 = or i64 %73, %77
  %79 = and i32 %25, -2
  %80 = and i32 %25, 1
  %81 = sub i32 %_local_9.0, %25
  %82 = add i32 %67, %_param_0.1
  br label %loop_entry103

blk_exit42:                                       ; preds = %blk_exit92, %brif_next74
  %_local_7.4 = phi i32 [ %196, %blk_exit92 ], [ 0, %brif_next74 ]
  %_param_0.3 = phi i32 [ %_param_0.9, %blk_exit92 ], [ %2, %brif_next74 ]
  %83 = inttoptr i32 %_param_0.3 to i32*
  store i32 %_local_7.4, i32* %83, align 4
  %.pre = add i32 %91, %6
  br label %blk_exit41

blk_exit43:                                       ; preds = %loop_entry81, %blk_exit75
  %_local_18.0 = phi i64 [ %192, %loop_entry81 ], [ 0, %blk_exit75 ]
  %_param_1.0 = phi i32 [ %194, %loop_entry81 ], [ %_arg_1, %blk_exit75 ]
  %_param_0.4 = phi i32 [ %193, %loop_entry81 ], [ %2, %blk_exit75 ]
  %84 = icmp eq i32 %152, 0
  br i1 %84, label %blk_exit92, label %brif_next94

blk_exit44:                                       ; preds = %blk_exit75
  %85 = and i32 %_local_9.0, -4
  br label %loop_entry81

blk_exit45:                                       ; preds = %blk_exit24
  %86 = shl i32 %_local_9.0, 2
  %87 = call i32 @memcpy(i32 %2, i32 %_arg_1, i32 %86)
  %88 = add i32 %2, %86
  %89 = inttoptr i32 %88 to i32*
  store i32 0, i32* %89, align 4
  br label %blk_exit41

brif_next47:                                      ; preds = %blk_exit24
  %90 = add i32 %6, %_param_0.1
  %91 = call i32 @rpl_malloc(i32 %90)
  %92 = icmp eq i32 %91, 0
  br i1 %92, label %blk_exit8, label %brif_next49

brif_next49:                                      ; preds = %brif_next47
  %93 = and i32 %25, 3
  %94 = zext i32 %40 to i64
  %95 = add i32 %_local_9.1, -5
  %96 = icmp ult i32 %95, -3
  br i1 %96, label %blk_exit51, label %blk_exit50

blk_exit50:                                       ; preds = %loop_entry54, %brif_next49
  %_local_20.0 = phi i64 [ %136, %loop_entry54 ], [ 0, %brif_next49 ]
  %_local_18.1 = phi i64 [ %133, %loop_entry54 ], [ 0, %brif_next49 ]
  %_param_3.2 = phi i32 [ %138, %loop_entry54 ], [ %_arg_3, %brif_next49 ]
  %_param_0.5 = phi i32 [ %137, %loop_entry54 ], [ %91, %brif_next49 ]
  %97 = icmp eq i32 %93, 0
  br i1 %97, label %blk_exit65, label %loop_entry68

blk_exit51:                                       ; preds = %brif_next49
  %98 = and i32 %25, -4
  br label %loop_entry54

loop_entry54:                                     ; preds = %loop_entry54, %blk_exit51
  %_local_20.1 = phi i64 [ 0, %blk_exit51 ], [ %136, %loop_entry54 ]
  %_local_7.5 = phi i32 [ %98, %blk_exit51 ], [ %139, %loop_entry54 ]
  %_param_3.3 = phi i32 [ %_arg_3, %blk_exit51 ], [ %138, %loop_entry54 ]
  %_param_0.6 = phi i32 [ %91, %blk_exit51 ], [ %137, %loop_entry54 ]
  %99 = inttoptr i32 %_param_3.3 to i32*
  %100 = load i32, i32* %99, align 4
  %101 = zext i32 %100 to i64
  %102 = shl i64 %101, %94
  %103 = add nuw i64 %102, %_local_20.1
  %104 = trunc i64 %103 to i32
  %105 = inttoptr i32 %_param_0.6 to i32*
  store i32 %104, i32* %105, align 4
  %106 = add i32 %_param_0.6, 4
  %107 = add i32 %_param_3.3, 4
  %108 = inttoptr i32 %107 to i32*
  %109 = load i32, i32* %108, align 4
  %110 = zext i32 %109 to i64
  %111 = shl i64 %110, %94
  %112 = lshr i64 %103, 32
  %113 = add nuw i64 %111, %112
  %114 = trunc i64 %113 to i32
  %115 = inttoptr i32 %106 to i32*
  store i32 %114, i32* %115, align 4
  %116 = add i32 %_param_0.6, 8
  %117 = add i32 %_param_3.3, 8
  %118 = inttoptr i32 %117 to i32*
  %119 = load i32, i32* %118, align 4
  %120 = zext i32 %119 to i64
  %121 = shl i64 %120, %94
  %122 = lshr i64 %113, 32
  %123 = add nuw i64 %121, %122
  %124 = trunc i64 %123 to i32
  %125 = inttoptr i32 %116 to i32*
  store i32 %124, i32* %125, align 4
  %126 = add i32 %_param_0.6, 12
  %127 = add i32 %_param_3.3, 12
  %128 = inttoptr i32 %127 to i32*
  %129 = load i32, i32* %128, align 4
  %130 = zext i32 %129 to i64
  %131 = shl i64 %130, %94
  %132 = lshr i64 %123, 32
  %133 = add nuw i64 %131, %132
  %134 = trunc i64 %133 to i32
  %135 = inttoptr i32 %126 to i32*
  store i32 %134, i32* %135, align 4
  %136 = lshr i64 %133, 32
  %137 = add i32 %_param_0.6, 16
  %138 = add i32 %_param_3.3, 16
  %139 = add i32 %_local_7.5, -4
  %brif_val63.not = icmp eq i32 %139, 0
  br i1 %brif_val63.not, label %blk_exit50, label %loop_entry54

blk_exit65:                                       ; preds = %loop_entry68, %blk_exit50
  %_local_18.2 = phi i64 [ %_local_18.1, %blk_exit50 ], [ %145, %loop_entry68 ]
  %140 = icmp ugt i64 %_local_18.2, 4294967295
  br i1 %140, label %blk_exit9, label %brif_next74

loop_entry68:                                     ; preds = %loop_entry68, %blk_exit50
  %_local_20.2 = phi i64 [ %150, %loop_entry68 ], [ %_local_20.0, %blk_exit50 ]
  %_local_7.6 = phi i32 [ %151, %loop_entry68 ], [ %93, %blk_exit50 ]
  %_param_3.4 = phi i32 [ %149, %loop_entry68 ], [ %_param_3.2, %blk_exit50 ]
  %_param_0.7 = phi i32 [ %148, %loop_entry68 ], [ %_param_0.5, %blk_exit50 ]
  %141 = inttoptr i32 %_param_3.4 to i32*
  %142 = load i32, i32* %141, align 4
  %143 = zext i32 %142 to i64
  %144 = shl i64 %143, %94
  %145 = add i64 %144, %_local_20.2
  %146 = trunc i64 %145 to i32
  %147 = inttoptr i32 %_param_0.7 to i32*
  store i32 %146, i32* %147, align 4
  %148 = add i32 %_param_0.7, 4
  %149 = add i32 %_param_3.4, 4
  %150 = lshr i64 %145, 32
  %151 = add i32 %_local_7.6, -1
  %brif_val71.not = icmp eq i32 %151, 0
  br i1 %brif_val71.not, label %blk_exit65, label %loop_entry68

brif_next74:                                      ; preds = %blk_exit65
  br i1 %9, label %blk_exit42, label %blk_exit75

blk_exit75:                                       ; preds = %brif_next74
  %152 = and i32 %_local_9.0, 3
  %153 = add i32 %_local_9.0, -4
  %154 = icmp ult i32 %153, -3
  br i1 %154, label %blk_exit44, label %blk_exit43

loop_entry81:                                     ; preds = %loop_entry81, %blk_exit44
  %_local_18.3 = phi i64 [ 0, %blk_exit44 ], [ %192, %loop_entry81 ]
  %_local_7.7 = phi i32 [ %85, %blk_exit44 ], [ %195, %loop_entry81 ]
  %_param_1.1 = phi i32 [ %_arg_1, %blk_exit44 ], [ %194, %loop_entry81 ]
  %_param_0.8 = phi i32 [ %2, %blk_exit44 ], [ %193, %loop_entry81 ]
  %155 = inttoptr i32 %_param_1.1 to i32*
  %156 = load i32, i32* %155, align 4
  %157 = zext i32 %156 to i64
  %158 = shl i64 %157, %94
  %159 = add nuw i64 %158, %_local_18.3
  %160 = trunc i64 %159 to i32
  %161 = inttoptr i32 %_param_0.8 to i32*
  store i32 %160, i32* %161, align 4
  %162 = add i32 %_param_0.8, 4
  %163 = add i32 %_param_1.1, 4
  %164 = inttoptr i32 %163 to i32*
  %165 = load i32, i32* %164, align 4
  %166 = zext i32 %165 to i64
  %167 = shl i64 %166, %94
  %168 = lshr i64 %159, 32
  %169 = add nuw i64 %167, %168
  %170 = trunc i64 %169 to i32
  %171 = inttoptr i32 %162 to i32*
  store i32 %170, i32* %171, align 4
  %172 = add i32 %_param_0.8, 8
  %173 = add i32 %_param_1.1, 8
  %174 = inttoptr i32 %173 to i32*
  %175 = load i32, i32* %174, align 4
  %176 = zext i32 %175 to i64
  %177 = shl i64 %176, %94
  %178 = lshr i64 %169, 32
  %179 = add nuw i64 %177, %178
  %180 = trunc i64 %179 to i32
  %181 = inttoptr i32 %172 to i32*
  store i32 %180, i32* %181, align 4
  %182 = add i32 %_param_0.8, 12
  %183 = add i32 %_param_1.1, 12
  %184 = inttoptr i32 %183 to i32*
  %185 = load i32, i32* %184, align 4
  %186 = zext i32 %185 to i64
  %187 = shl i64 %186, %94
  %188 = lshr i64 %179, 32
  %189 = add nuw i64 %187, %188
  %190 = trunc i64 %189 to i32
  %191 = inttoptr i32 %182 to i32*
  store i32 %190, i32* %191, align 4
  %192 = lshr i64 %189, 32
  %193 = add i32 %_param_0.8, 16
  %194 = add i32 %_param_1.1, 16
  %195 = add i32 %_local_7.7, -4
  %brif_val90.not = icmp eq i32 %195, 0
  br i1 %brif_val90.not, label %blk_exit43, label %loop_entry81

blk_exit92:                                       ; preds = %loop_entry95, %blk_exit43
  %_local_18.4 = phi i64 [ %_local_18.0, %blk_exit43 ], [ %207, %loop_entry95 ]
  %_param_0.9 = phi i32 [ %_param_0.4, %blk_exit43 ], [ %205, %loop_entry95 ]
  %196 = trunc i64 %_local_18.4 to i32
  br label %blk_exit42

brif_next94:                                      ; preds = %blk_exit43
  %197 = and i32 %_param_0.0, 3
  br label %loop_entry95

loop_entry95:                                     ; preds = %loop_entry95, %brif_next94
  %_local_18.5 = phi i64 [ %_local_18.0, %brif_next94 ], [ %207, %loop_entry95 ]
  %_local_7.8 = phi i32 [ %197, %brif_next94 ], [ %208, %loop_entry95 ]
  %_param_1.2 = phi i32 [ %_param_1.0, %brif_next94 ], [ %206, %loop_entry95 ]
  %_param_0.10 = phi i32 [ %_param_0.4, %brif_next94 ], [ %205, %loop_entry95 ]
  %198 = inttoptr i32 %_param_1.2 to i32*
  %199 = load i32, i32* %198, align 4
  %200 = zext i32 %199 to i64
  %201 = shl i64 %200, %94
  %202 = add i64 %201, %_local_18.5
  %203 = trunc i64 %202 to i32
  %204 = inttoptr i32 %_param_0.10 to i32*
  store i32 %203, i32* %204, align 4
  %205 = add i32 %_param_0.10, 4
  %206 = add i32 %_param_1.2, 4
  %207 = lshr i64 %202, 32
  %208 = add i32 %_local_7.8, -1
  %brif_val98.not = icmp eq i32 %208, 0
  br i1 %brif_val98.not, label %blk_exit92, label %loop_entry95

loop_entry103:                                    ; preds = %blk_exit104, %blk_exit41
  %_param_0.11 = phi i32 [ %81, %blk_exit41 ], [ %216, %blk_exit104 ]
  %209 = add i32 %_param_0.11, %25
  %210 = shl i32 %209, 2
  %211 = add i32 %2, %210
  %212 = inttoptr i32 %211 to i32*
  %213 = load i32, i32* %212, align 4
  %.not221 = icmp ult i32 %213, %71
  br i1 %.not221, label %brif_next110, label %blk_exit107

blk_exit104:                                      ; preds = %blk_exit159, %blk_exit134, %blk_exit106.blk_exit104_crit_edge
  %.pre-phi225 = phi i32 [ %.pre224, %blk_exit106.blk_exit104_crit_edge ], [ %218, %blk_exit159 ], [ %218, %blk_exit134 ]
  %_param_0.12 = phi i32 [ 0, %blk_exit106.blk_exit104_crit_edge ], [ %327, %blk_exit159 ], [ %_local_28.0, %blk_exit134 ]
  %214 = add i32 %82, %.pre-phi225
  %215 = inttoptr i32 %214 to i32*
  store i32 %_param_0.12, i32* %215, align 4
  %216 = add i32 %_param_0.11, -1
  %brif_val166.not = icmp eq i32 %_param_0.11, 0
  br i1 %brif_val166.not, label %brif_next167, label %loop_entry103

blk_exit105:                                      ; preds = %brif_next113, %blk_exit107, %blk_exit106
  %_local_28.0 = phi i32 [ %234, %blk_exit106 ], [ -1, %blk_exit107 ], [ -1, %brif_next113 ]
  %217 = zext i32 %_local_28.0 to i64
  %218 = shl i32 %_param_0.11, 2
  %219 = add i32 %2, %218
  br label %loop_entry125

blk_exit106:                                      ; preds = %brif_next113, %brif_next110
  %_local_7.9 = phi i32 [ %247, %brif_next110 ], [ %252, %brif_next113 ]
  %_local_28.1 = phi i32 [ %248, %brif_next110 ], [ -1, %brif_next113 ]
  %220 = zext i32 %_local_28.1 to i64
  %221 = mul nuw i64 %77, %220
  %222 = zext i32 %_local_7.9 to i64
  %223 = shl nuw i64 %222, 32
  %224 = add i32 %211, -8
  %225 = inttoptr i32 %224 to i32*
  %226 = load i32, i32* %225, align 4
  %227 = zext i32 %226 to i64
  %228 = or i64 %223, %227
  %229 = sub i64 %221, %228
  %230 = icmp ugt i64 %229, %78
  %231 = select i1 %230, i32 -2, i32 -1
  %232 = icmp ugt i64 %221, %228
  %233 = select i1 %232, i32 %231, i32 0
  %234 = add i32 %_local_28.1, %233
  %235 = icmp eq i32 %234, 0
  br i1 %235, label %blk_exit106.blk_exit104_crit_edge, label %blk_exit105

blk_exit106.blk_exit104_crit_edge:                ; preds = %blk_exit106
  %.pre224 = shl i32 %_param_0.11, 2
  br label %blk_exit104

blk_exit107:                                      ; preds = %loop_entry103
  %236 = icmp ugt i32 %213, %71
  br i1 %236, label %blk_exit105, label %brif_next113

brif_next110:                                     ; preds = %loop_entry103
  %237 = zext i32 %213 to i64
  %238 = shl nuw i64 %237, 32
  %239 = add i32 %211, -4
  %240 = inttoptr i32 %239 to i32*
  %241 = load i32, i32* %240, align 4
  %242 = zext i32 %241 to i64
  %243 = or i64 %238, %242
  %244 = udiv i64 %243, %72
  %245 = trunc i64 %244 to i32
  %246 = mul i32 %71, %245
  %247 = sub i32 %241, %246
  %248 = trunc i64 %244 to i32
  br label %blk_exit106

brif_next113:                                     ; preds = %blk_exit107
  %249 = add i32 %211, -4
  %250 = inttoptr i32 %249 to i32*
  %251 = load i32, i32* %250, align 4
  %252 = add i32 %251, %71
  %253 = icmp ult i32 %252, %251
  br i1 %253, label %blk_exit105, label %blk_exit106

blk_exit122:                                      ; preds = %loop_entry125
  %254 = icmp eq i32 %80, 0
  br i1 %254, label %blk_exit134, label %brif_next136

loop_entry125:                                    ; preds = %loop_entry125, %blk_exit105
  %_local_19.2 = phi i64 [ 0, %blk_exit105 ], [ %282, %loop_entry125 ]
  %_local_10.1 = phi i32 [ %79, %blk_exit105 ], [ %285, %loop_entry125 ]
  %_local_7.11 = phi i32 [ %_local_21.0, %blk_exit105 ], [ %284, %loop_entry125 ]
  %_param_0.14 = phi i32 [ %219, %blk_exit105 ], [ %283, %loop_entry125 ]
  %255 = inttoptr i32 %_local_7.11 to i32*
  %256 = load i32, i32* %255, align 4
  %257 = zext i32 %256 to i64
  %258 = mul nuw i64 %217, %257
  %259 = add i64 %258, %_local_19.2
  %260 = inttoptr i32 %_param_0.14 to i32*
  %261 = load i32, i32* %260, align 4
  %262 = xor i32 %261, -1
  %263 = zext i32 %262 to i64
  %264 = add i64 %259, %263
  %265 = trunc i64 %264 to i32
  %266 = xor i32 %265, -1
  store i32 %266, i32* %260, align 4
  %267 = add i32 %_param_0.14, 4
  %268 = add i32 %_local_7.11, 4
  %269 = inttoptr i32 %268 to i32*
  %270 = load i32, i32* %269, align 4
  %271 = zext i32 %270 to i64
  %272 = mul nuw i64 %217, %271
  %273 = lshr i64 %264, 32
  %274 = add i64 %272, %273
  %275 = inttoptr i32 %267 to i32*
  %276 = load i32, i32* %275, align 4
  %277 = xor i32 %276, -1
  %278 = zext i32 %277 to i64
  %279 = add i64 %274, %278
  %280 = trunc i64 %279 to i32
  %281 = xor i32 %280, -1
  store i32 %281, i32* %275, align 4
  %282 = lshr i64 %279, 32
  %283 = add i32 %_param_0.14, 8
  %284 = add i32 %_local_7.11, 8
  %285 = add i32 %_local_10.1, -2
  %brif_val132.not = icmp eq i32 %285, 0
  br i1 %brif_val132.not, label %blk_exit122, label %loop_entry125

blk_exit134:                                      ; preds = %brif_next136, %blk_exit122
  %_local_19.3 = phi i64 [ %282, %blk_exit122 ], [ %301, %brif_next136 ]
  %286 = load i32, i32* %212, align 4
  %287 = trunc i64 %_local_19.3 to i32
  %288 = icmp ult i32 %286, %287
  br i1 %288, label %loop_entry148, label %blk_exit104

brif_next136:                                     ; preds = %blk_exit122
  %289 = inttoptr i32 %284 to i32*
  %290 = load i32, i32* %289, align 4
  %291 = zext i32 %290 to i64
  %292 = mul nuw i64 %217, %291
  %293 = add i64 %292, %282
  %294 = inttoptr i32 %283 to i32*
  %295 = load i32, i32* %294, align 4
  %296 = xor i32 %295, -1
  %297 = zext i32 %296 to i64
  %298 = add i64 %293, %297
  %299 = trunc i64 %298 to i32
  %300 = xor i32 %299, -1
  store i32 %300, i32* %294, align 4
  %301 = lshr i64 %298, 32
  br label %blk_exit134

blk_exit144:                                      ; preds = %loop_entry148
  br i1 %254, label %blk_exit159, label %brif_next161

loop_entry148:                                    ; preds = %loop_entry148, %blk_exit134
  %_local_11.0 = phi i32 [ %326, %loop_entry148 ], [ %79, %blk_exit134 ]
  %_local_7.13 = phi i32 [ %323, %loop_entry148 ], [ 0, %blk_exit134 ]
  %_local_5.2 = phi i32 [ %324, %loop_entry148 ], [ %219, %blk_exit134 ]
  %_param_0.16 = phi i32 [ %325, %loop_entry148 ], [ %_local_21.0, %blk_exit134 ]
  %302 = inttoptr i32 %_param_0.16 to i32*
  %303 = load i32, i32* %302, align 4
  %304 = add i32 %303, %_local_7.13
  %305 = inttoptr i32 %_local_5.2 to i32*
  %306 = load i32, i32* %305, align 4
  %307 = add i32 %304, %306
  store i32 %307, i32* %305, align 4
  %308 = add i32 %_local_5.2, 4
  %309 = add i32 %_param_0.16, 4
  %310 = inttoptr i32 %309 to i32*
  %311 = load i32, i32* %310, align 4
  %312 = xor i32 %306, -1
  %313 = icmp uge i32 %303, %312
  %314 = icmp ugt i32 %303, %312
  %select_cond153.not = icmp eq i32 %_local_7.13, 0
  %.v = select i1 %select_cond153.not, i1 %314, i1 %313
  %315 = zext i1 %.v to i32
  %316 = add i32 %311, %315
  %317 = inttoptr i32 %308 to i32*
  %318 = load i32, i32* %317, align 4
  %319 = add i32 %316, %318
  store i32 %319, i32* %317, align 4
  %320 = xor i32 %318, -1
  %321 = icmp uge i32 %311, %320
  %322 = icmp ugt i32 %311, %320
  %.v223 = select i1 %.v, i1 %321, i1 %322
  %323 = zext i1 %.v223 to i32
  %324 = add i32 %_local_5.2, 8
  %325 = add i32 %_param_0.16, 8
  %326 = add i32 %_local_11.0, -2
  %brif_val157.not = icmp eq i32 %326, 0
  br i1 %brif_val157.not, label %blk_exit144, label %loop_entry148

blk_exit159:                                      ; preds = %brif_next161, %blk_exit144
  %327 = add i32 %_local_28.0, -1
  br label %blk_exit104

brif_next161:                                     ; preds = %blk_exit144
  %328 = inttoptr i32 %325 to i32*
  %329 = load i32, i32* %328, align 4
  %330 = add i32 %329, %323
  %331 = inttoptr i32 %324 to i32*
  %332 = load i32, i32* %331, align 4
  %333 = add i32 %330, %332
  store i32 %333, i32* %331, align 4
  br label %blk_exit159

brif_next167:                                     ; preds = %blk_exit104
  %334 = shl i32 %_local_9.0, 2
  %335 = add i32 %2, %334
  %336 = inttoptr i32 %335 to i32*
  %337 = load i32, i32* %336, align 4
  %338 = icmp ne i32 %337, 0
  %339 = zext i1 %338 to i32
  %340 = add i32 %_local_9.0, %339
  %341 = sub i32 %340, %25
  br label %loop_entry169

loop_entry169:                                    ; preds = %blk_exit170, %brif_next167
  %_local_22.0 = phi i32 [ %68, %brif_next167 ], [ %345, %blk_exit170 ]
  %_local_7.14 = phi i32 [ %_local_9.1, %brif_next167 ], [ %342, %blk_exit170 ]
  %.not222 = icmp eq i32 %_local_7.14, 1
  br i1 %.not222, label %blk_exit19, label %blk_exit170

blk_exit170:                                      ; preds = %loop_entry169
  %342 = add i32 %_local_7.14, -1
  %343 = inttoptr i32 %_local_22.0 to i32*
  %344 = load i32, i32* %343, align 4
  %345 = add i32 %_local_22.0, -4
  %346 = icmp eq i32 %344, 0
  br i1 %346, label %loop_entry169, label %blk_exit20

loop_entry178:                                    ; preds = %brif_next196, %blk_exit19
  %_local_15.0 = phi i32 [ %_local_9.1, %blk_exit19 ], [ %347, %brif_next196 ]
  %_local_11.1 = phi i32 [ %35, %blk_exit19 ], [ %366, %brif_next196 ]
  %_local_8.1 = phi i32 [ %34, %blk_exit19 ], [ %367, %brif_next196 ]
  %347 = add i32 %_local_15.0, -1
  %348 = icmp ugt i32 %347, %_local_7.1
  %349 = icmp eq i32 %_local_15.0, 1
  %or.cond = or i1 %348, %349
  br i1 %or.cond, label %blk_exit179, label %brif_next183

blk_exit179:                                      ; preds = %brif_next183, %loop_entry178
  %_local_5.3 = phi i32 [ 0, %loop_entry178 ], [ %353, %brif_next183 ]
  %.not216 = icmp ult i32 %347, %_local_7.1
  br i1 %.not216, label %brif_next187, label %blk_exit185

brif_next183:                                     ; preds = %loop_entry178
  %350 = add i32 %_local_8.1, %6
  %351 = inttoptr i32 %350 to i32*
  %352 = load i32, i32* %351, align 4
  %353 = lshr i32 %352, 31
  br label %blk_exit179

blk_exit185:                                      ; preds = %brif_next187, %blk_exit179
  %_local_10.2 = phi i32 [ %359, %brif_next187 ], [ 0, %blk_exit179 ]
  %354 = or i32 %_local_10.2, %_local_5.3
  %.not217 = icmp ult i32 %347, %25
  br i1 %.not217, label %brif_next191, label %blk_exit189

brif_next187:                                     ; preds = %blk_exit179
  %355 = add i32 %_local_8.1, %6
  %356 = add i32 %355, 4
  %357 = inttoptr i32 %356 to i32*
  %358 = load i32, i32* %357, align 4
  %359 = shl i32 %358, 1
  br label %blk_exit185

blk_exit189:                                      ; preds = %brif_next191, %blk_exit185
  %_local_5.4 = phi i32 [ %364, %brif_next191 ], [ 0, %blk_exit185 ]
  %360 = icmp ugt i32 %354, %_local_5.4
  br i1 %360, label %blk_exit18, label %brif_next194

brif_next191:                                     ; preds = %blk_exit185
  %361 = add i32 %_local_11.1, %6
  %362 = add i32 %361, 4
  %363 = inttoptr i32 %362 to i32*
  %364 = load i32, i32* %363, align 4
  br label %blk_exit189

brif_next194:                                     ; preds = %blk_exit189
  %365 = icmp ult i32 %354, %_local_5.4
  br i1 %365, label %blk_exit6, label %brif_next196

brif_next196:                                     ; preds = %brif_next194
  %366 = add i32 %_local_11.1, -4
  %367 = add i32 %_local_8.1, -4
  %brif_val197.not = icmp eq i32 %347, 0
  br i1 %brif_val197.not, label %brif_next198, label %loop_entry178

brif_next198:                                     ; preds = %brif_next196
  %brif_val199.not = icmp eq i32 %_local_13.4, 0
  br i1 %brif_val199.not, label %blk_exit6, label %blk_exit17

loop_entry206:                                    ; preds = %brif_next210, %blk_exit16
  %_local_10.3 = phi i32 [ %_local_13.2, %blk_exit16 ], [ %372, %brif_next210 ]
  %_param_0.17 = phi i32 [ %_local_16.2, %blk_exit16 ], [ %371, %brif_next210 ]
  %368 = inttoptr i32 %_param_0.17 to i32*
  %369 = load i32, i32* %368, align 4
  %370 = add i32 %369, 1
  store i32 %370, i32* %368, align 4
  %.not218 = icmp eq i32 %369, -1
  br i1 %.not218, label %brif_next210, label %blk_exit6

brif_next210:                                     ; preds = %loop_entry206
  %371 = add i32 %_param_0.17, 4
  %372 = add i32 %_local_10.3, -1
  %373 = icmp eq i32 %372, 0
  br i1 %373, label %blk_exit7, label %loop_entry206
}

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @rpl_vfprintf(i32, i32, i32) #0

; Function Attrs: null_pointer_is_valid
declare i32 @__main_void() #0

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare i64 @llvm.fshl.i64(i64, i64, i64) #1

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare i32 @llvm.fshl.i32(i32, i32, i32) #1

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare double @llvm.fabs.f64(double) #1

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.fabs.f32(float) #1

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare i32 @llvm.ctlz.i32(i32, i1 immarg) #1

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare i32 @llvm.usub.sat.i32(i32, i32) #1

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare { i32, i1 } @llvm.umul.with.overflow.i32(i32, i32) #1

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare i32 @llvm.uadd.sat.i32(i32, i32) #1

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare i32 @llvm.abs.i32(i32, i1 immarg) #1

attributes #0 = { null_pointer_is_valid }
attributes #1 = { nocallback nofree nosync nounwind readnone speculatable willreturn }

!0 = !{i32 0, i32 33}
