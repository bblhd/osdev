CFLAGS="-std=gnu99 -ffreestanding -O2 -Wall -Wextra -Werror -Wno-trigraphs"
LDFLAGS="-O2 -nostdlib"
BUILDDIR="build"
BINNAME="$BUILDDIR/out.bin"
ISONAME="$BUILDDIR/possum.iso"

NASM_FILES=
GAS_FILES=
C_FILES=
INCLUDE_SUBSOURCES=
INCLUDE_DIRS=

# finds the paths of all source files
for dir in src/*; do
    if [[ -d $dir ]]; then
		for file in $dir/*; do
		    if [[ -f $file ]]; then
				if [[ $file == *.s ]]; then
					NASM_FILES="$NASM_FILES $file"
				elif [[ $file == *.c ]]; then
					C_FILES="$C_FILES $file"
				fi
		    fi
		done
	    if [[ -d "$dir/include" ]]; then
			INCLUDE_SUBSOURCES="$INCLUDE_SUBSOURCES -I$dir/include"
			INCLUDE_DIRS="$INCLUDE_DIRS $dir/include"
	    fi
	fi
done

#echo "include sources:$INCLUDE_SUBSOURCES"

# makes script recompile C files whose headers have changed
for file in $C_FILES; do
	ofile="$BUILDDIR/$(basename $file | sed 's/\./_/g').o"
	if [ -f "$ofile" ]; then
		oldHeaders="no"
		
		headerFiles=$(grep "^#include" $file | sed 's/^#include [<"]//' | sed 's/[>"]$//')
		
		for hfile in $headerFiles; do
			for dir in $INCLUDE_DIRS; do
				if [ -f "$dir/$hfile" ]; then
					hfile="$dir/$hfile"
					break
				fi
			done
			if [ "$hfile" -nt "$ofile" ]; then
				oldHeaders="yes"
				break
			fi
		done
		
		if [ $oldHeaders == "yes" ]; then
			rm "$ofile"
		fi
	fi
done

if [ ! -d "$BUILDDIR" ]; then
	mkdir "$BUILDDIR"
fi

compiledNew="no"
FAILED="no"
O_FILES=

# compiles all files in a list that have changed scince last compile
function compile_fileset() {
	name=$1
	compiler=$2
	filelist=$3
	for file in $filelist; do
		ofile="$BUILDDIR/$(basename $file | sed 's/\./_/g').o"
		
		if [ ! -f "$ofile" ] || [ "$file" -nt "$ofile" ]; then
			echo -ne "\033[94m[compiling $name]\033[0m $file -> $ofile\n"
			if ! $compiler "$file" -o "$ofile"; then
				FAILED="yes"
			fi
			compiledNew="yes"
		fi
		O_FILES="$O_FILES $ofile"
	done
}

compile_fileset nasm "nasm -f elf" "$NASM_FILES"
compile_fileset c "i686-elf-gcc -c $CFLAGS $INCLUDE_SUBSOURCES" "$C_FILES"

if [ $FAILED == "yes" ]; then
	echo -ne "\n\033[41m\033[97m FAILED TO COMPILE ALL FILES, ABORTING \033[0m\n\n"
	exit
fi

# removes compiled files of source files that have been deleted
for builtofile in $BUILDDIR/*.o; do
	matchexists="no"
	for ofile in $O_FILES; do
		if [ "$builtofile" == "$ofile" ]; then
			matchexists="yes"
			break
		fi
	done
	if [ $matchexists == "no" ]; then
		rm "$builtofile"
	fi
done

# links all compiled files if any files were recently compiled
if [ ! -f "$BINNAME" ] || [ $compiledNew == "yes" ]; then
	echo -ne "\033[95m[linking]\033[0m object files -> $BINNAME\n"
	if ! i686-elf-ld -T linker.ld $LDFLAGS -o $BINNAME $O_FILES; then
		FAILED="yes"
	fi
fi


if [ $FAILED == "yes" ]; then
	echo -ne "\n\033[41m\033[97m FAILED TO LINK, ABORTING \033[0m\n\n"
	exit
fi

FINISH_MODE="$1"

if [ -n FINISH_MODE ]; then
	# make new iso if bin file was recently linked
	if [ "$FINISH_MODE" != "qemu" ]; then
		if [ ! -f "$ISONAME" ] || [ "$BINNAME" -nt "$ISONAME" ]; then
			echo -ne "\033[91m[making iso]\033[0m $BINNAME -> $ISONAME\n"
		
			mkdir -p $BUILDDIR/isodir/boot/grub
			cp "$BINNAME" $BUILDDIR/isodir/boot/os.bin
			echo "menuentry \"possum\" { multiboot /boot/os.bin }" > $BUILDDIR/isodir/boot/grub/grub.cfg
			2>/dev/null 1>/dev/null grub-mkrescue -o "$ISONAME" $BUILDDIR/isodir
		fi
	fi
	
	if [[ "$FINISH_MODE" == /dev/* ]]; then
		echo -ne "\033[93m[writing device]\033[0m $ISONAME -> $FINISH_MODE\n"
		sudo dd if=$ISONAME of=$FINISH_MODE bs=1M status=progress && sync
	elif [ "$FINISH_MODE" == "qemu" ]; then
		echo -ne "\033[93m[qemu]\033[0m kernel $BINNAME\n"
		qemu-system-i386 -kernel "$BINNAME"
	fi
fi
