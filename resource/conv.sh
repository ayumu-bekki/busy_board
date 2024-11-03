#!/bin/bash
OUTDIR="../main/sounds"
files=`find . -maxdepth 1 -type f -name '*.wav'`
for file in $files
do
    name=`basename $file .wav`
    echo $name 
    sox $file -t raw ${name}.pcm
    xxd --include ${name}.pcm | sed 's/unsigned char/constexpr uint8_t/' | sed 's/unsigned int/constexpr size_t/' > ${OUTDIR}/${name}.h
done

rm *.pcm


