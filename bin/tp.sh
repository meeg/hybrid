#1/bin/bash
if [ -z "$1" ]
then
	echo "tp.sh <timing data dir> <baseline data file> <output dir> <output filename>"
	exit
fi

mkdir -p $3
./meeg_baseline $2 -o $3/$4

./meeg_tp $1/*cal?_[1-8]x3_125ns.bin -fr -o $3/$4
mkdir -p $3/fits
mv $3/$4_tp_fit* $3/fits

./meeg_t0res $3/$4.base $3/$4 $1/*cal?_1x3_125ns.bin -o $3/$4_anfit_1 &
./meeg_t0res $3/$4.base $3/$4 $1/*cal?_2x3_125ns.bin -o $3/$4_anfit_2 &
./meeg_t0res $3/$4.base $3/$4 $1/*cal?_3x3_125ns.bin -o $3/$4_anfit_3 &
./meeg_t0res $3/$4.base $3/$4 $1/*cal?_4x3_125ns.bin -o $3/$4_anfit_4
./meeg_t0res $3/$4.base $3/$4 $1/*cal?_5x3_125ns.bin -o $3/$4_anfit_5 &
./meeg_t0res $3/$4.base $3/$4 $1/*cal?_6x3_125ns.bin -o $3/$4_anfit_6 &
./meeg_t0res $3/$4.base $3/$4 $1/*cal?_7x3_125ns.bin -o $3/$4_anfit_7 &
./meeg_t0res $3/$4.base $3/$4 $1/*cal?_8x3_125ns.bin -o $3/$4_anfit_8
./meeg_t0res $3/$4.base $3/$4 $1/*cal?_1x3_125ns.bin -o $3/$4_linfit_1 -u &
./meeg_t0res $3/$4.base $3/$4 $1/*cal?_2x3_125ns.bin -o $3/$4_linfit_2 -u &
./meeg_t0res $3/$4.base $3/$4 $1/*cal?_3x3_125ns.bin -o $3/$4_linfit_3 -u &
./meeg_t0res $3/$4.base $3/$4 $1/*cal?_4x3_125ns.bin -o $3/$4_linfit_4 -u
./meeg_t0res $3/$4.base $3/$4 $1/*cal?_5x3_125ns.bin -o $3/$4_linfit_5 -u &
./meeg_t0res $3/$4.base $3/$4 $1/*cal?_6x3_125ns.bin -o $3/$4_linfit_6 -u &
./meeg_t0res $3/$4.base $3/$4 $1/*cal?_7x3_125ns.bin -o $3/$4_linfit_7 -u &
./meeg_t0res $3/$4.base $3/$4 $1/*cal?_8x3_125ns.bin -o $3/$4_linfit_8 -u

./meeg_t0res $3/$4.base $3/$4 $1/*cal?_[1-8]x3_125ns.bin -o $3/$4_anfit &
./meeg_t0res $3/$4.base $3/$4 $1/*cal?_[1-8]x3_125ns.bin -o $3/$4_linfit -u 
