CFLAGS="-std=gnu99 -ffreestanding -O2 -Wall -Wextra -Werror -Wno-parentheses -Wno-trigraphs -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-unused-variable -Wno-sign-compare"
LDFLAGS="-O2 -nostdlib"
BUILDDIR="build"
BINNAME="$BUILDDIR/out.bin"
ISONAME="$BUILDDIR/osdev.iso"

GAS_FILES=
C_FILES=
INCLUDE_SUBSOURCES=
INCLUDE_DIRS=

for dir in src/*; do
    if [[ -d $dir ]]; then
		for file in $dir/*; do
		    if [[ -f $file ]]; then
				if [[ $file == *.s ]]; then
					GAS_FILES="$GAS_FILES $file"
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

#if [ ! -f "src/flipt/flipt_opcodes.c" ] || [ "src/flipt/flipt_opgen.lua" -nt "src/flipt/flipt_opcodes.c" ]; then
	#echo -ne "\033[96m[generating flipt files]\033[0m lua src/flipt/flipt_opgen.lua\n"
	#cd src/flipt
	#lua5.2 flipt_opgen.lua
	#cd ../..
#fi

if [ ! -d "$BUILDDIR" ]; then
	mkdir "$BUILDDIR"
fi

O_FILES=

FAILED="no"

function compile_fileset() {
	name=$1
	compiler=$2
	filelist=$3
	for file in $filelist; do
		ofile="$BUILDDIR/$(basename $file | sed 's/\./_/g').o"
		
		oldHeaders="no"
		if [[ $file == *.c ]]; then
			headerFiles=$(grep "^#include" $file | sed 's/^#include [<"]//' | sed 's/[>"]$//')
			
			for hfile in $headerFiles; do
				for dir in $INCLUDE_DIRS; do
					if [ -f "$dir/$hfile" ]; then
						hfile="$dir/$hfile"
						break
					fi
				done
				if [ -f "$ofile" ] && [ "$hfile" -nt "$ofile" ]; then
					oldHeaders="yes"
					break
				fi
			done
		fi
		
		if [ ! -f "$ofile" ] || [ "$file" -nt "$ofile" ] || [ $oldHeaders == "yes" ]; then
			echo -ne "\033[94m[compiling $name]\033[0m $file -> $ofile\n"
			if ! $compiler "$file" -o "$ofile"; then
				FAILED="yes"
			fi
		fi
		O_FILES="$O_FILES $ofile"
	done
}

compile_fileset gnu-asm "i686-elf-gcc -c -x assembler-with-cpp" "$GAS_FILES"
compile_fileset c "i686-elf-gcc -c $CFLAGS $INCLUDE_SUBSOURCES" "$C_FILES"

if [ $FAILED == "yes" ]; then
	echo -ne "\n\033[41m\033[97m FAILED TO COMPILE ALL FILES, ABORTING \033[0m\n\n"
	exit
fi

for buildofile in $BUILDDIR/*.o; do
	matchexists="no"
	for ofile in $O_FILES; do
		if [ "$buildofile" == "$ofile" ]; then
			matchexists="yes"
			break
		fi
	done
	if [ $matchexists == "no" ]; then
		rm "$buildofile"
	fi
done

shouldlink="no"
for file in $O_FILES; do
	if [ ! -f "$BINNAME" ] || [ "$file" -nt "$BINNAME" ]; then
		shouldlink="yes"
	fi
done

if [ $shouldlink == "yes" ]; then
	echo -ne "\033[95m[linking]\033[0m object files -> $BINNAME\n"
	if ! i686-elf-ld -T tools/linker.ld $LDFLAGS -o $BINNAME $O_FILES; then
		FAILED="yes"
	fi
fi


if [ $FAILED == "yes" ]; then
	echo -ne "\n\033[41m\033[97m FAILED TO LINK, ABORTING \033[0m\n\n"
	exit
fi

FINISH_MODE="$1"

if [ -n FINISH_MODE ]; then
	if [ ! -f "$ISONAME" ] || [ "$BINNAME" -nt "$ISONAME" ]; then
		echo -ne "\033[91m[making iso]\033[0m $BINNAME -> $ISONAME\n"
	
		mkdir -p $BUILDDIR/isodir/boot/grub
		cp "$BINNAME" $BUILDDIR/isodir/boot/os.bin
		echo "menuentry \"osdev\" { multiboot /boot/os.bin }" > $BUILDDIR/isodir/boot/grub/grub.cfg
		2>/dev/null 1>/dev/null grub-mkrescue -o "$ISONAME" $BUILDDIR/isodir
	fi
	
	if [[ "$FINISH_MODE" == /dev/* ]]; then
		echo -ne "\033[93m[writing device]\033[0m $ISONAME -> $FINISH_MODE\n"
		sudo dd if=$ISONAME of=$FINISH_MODE status=progress && sync
	elif [ "$FINISH_MODE" == "qemu" ]; then
		echo -ne "\033[93m[qemu]\033[0m cdrom $ISONAME\n"
		qemu-system-i386 -cdrom "$ISONAME"
	fi
fi
