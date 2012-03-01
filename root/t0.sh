#1/bin/bash
if [ -z "$1" ]
then
	echo "tp.sh <timing data dir> <baseline data file> <output dir> <output filename>"
	exit
fi

mkdir -p $3
../bin/meeg_baseline $2 -o $3/$4

../bin/meeg_tp $1/*cal?_[1-8]x3_125ns.bin -fr -o $3/$4
mkdir -p $3/fits
mv $3/$4_tp_fit* $3/fits

../bin/meeg_t0res $3/$4.base $3/$4 $1/*cal?_1x3_125ns.bin -o $3/$4_anfit_1 &
../bin/meeg_t0res $3/$4.base $3/$4 $1/*cal?_2x3_125ns.bin -o $3/$4_anfit_2 &
../bin/meeg_t0res $3/$4.base $3/$4 $1/*cal?_3x3_125ns.bin -o $3/$4_anfit_3 &
../bin/meeg_t0res $3/$4.base $3/$4 $1/*cal?_4x3_125ns.bin -o $3/$4_anfit_4
../bin/meeg_t0res $3/$4.base $3/$4 $1/*cal?_5x3_125ns.bin -o $3/$4_anfit_5 &
../bin/meeg_t0res $3/$4.base $3/$4 $1/*cal?_6x3_125ns.bin -o $3/$4_anfit_6 &
../bin/meeg_t0res $3/$4.base $3/$4 $1/*cal?_7x3_125ns.bin -o $3/$4_anfit_7 &
../bin/meeg_t0res $3/$4.base $3/$4 $1/*cal?_8x3_125ns.bin -o $3/$4_anfit_8
../bin/meeg_t0res $3/$4.base $3/$4 $1/*cal?_1x3_125ns.bin -o $3/$4_linfit_1 -u &
../bin/meeg_t0res $3/$4.base $3/$4 $1/*cal?_2x3_125ns.bin -o $3/$4_linfit_2 -u &
../bin/meeg_t0res $3/$4.base $3/$4 $1/*cal?_3x3_125ns.bin -o $3/$4_linfit_3 -u &
../bin/meeg_t0res $3/$4.base $3/$4 $1/*cal?_4x3_125ns.bin -o $3/$4_linfit_4 -u
../bin/meeg_t0res $3/$4.base $3/$4 $1/*cal?_5x3_125ns.bin -o $3/$4_linfit_5 -u &
../bin/meeg_t0res $3/$4.base $3/$4 $1/*cal?_6x3_125ns.bin -o $3/$4_linfit_6 -u &
../bin/meeg_t0res $3/$4.base $3/$4 $1/*cal?_7x3_125ns.bin -o $3/$4_linfit_7 -u &
../bin/meeg_t0res $3/$4.base $3/$4 $1/*cal?_8x3_125ns.bin -o $3/$4_linfit_8 -u

../bin/meeg_t0res $3/$4.base $3/$4 $1/*cal?_[1-8]x3_125ns.bin -o $3/$4_anfit &
../bin/meeg_t0res $3/$4.base $3/$4 $1/*cal?_[1-8]x3_125ns.bin -o $3/$4_linfit -u 

mkdir -p $3/neg
mv $3/*_neg $3/*_neg.png $3/neg
