#!/bin/bash
HOME=`pwd`
OUT=$HOME/../pre_processing/out

STEP=step1
OUTZ=$HOME/$STEP/out.z
OUTS=$HOME/$STEP/out.s
OUTS_BACK=$HOME/$STEP/out.s.back

MYDEFLATE=$HOME/../encoding/mydeflate/mydeflate
MYSHRINK=$HOME/../encoding/zerobyte_suppression/zerobyte_suppression

echo ""
echo "$0 $STEP"
cat $OUT/readme
filenamez=$HOME/$STEP/$(cat $OUT/readme | awk '{print $3}' | sed 's/,//').statis.z.$STEP.csv
filenames=$HOME/$STEP/$(cat $OUT/readme | awk '{print $3}' | sed 's/,//').statis.s.$STEP.csv
echo "statistics file :$filenamez"
echo ""

mkdir -p $HOME/$STEP/out.z  $HOME/$STEP/out.s 
rm -f $HOME/$STEP/*sta*csv $HOME/$STEP/out*/*


stati()
{
	bfile=$1
	wfile=$2
	#echo $bfile $wfile
	count=$(echo $bfile | awk -F '.' '{print $4}')
	bitfile=$(echo $bfile | sed 's/byte/bit/g')	
	difffile=$(echo $bfile | sed 's/byte/diff/g')	
	rawfile=$(echo $bfile | sed 's/byte/raw/g')	
	diff_bytefile=$(echo $bfile | sed 's/byte/diff_byte/g')	
	diff_bitfile=$(echo $bitfile | sed 's/bit/diff_bit/g')	
	bytefile_len=$(ls -l $bfile | awk '{print $5}')
	bitfile_len=$(ls -l $bitfile | awk '{print $5}')
	difffile_len=$(ls -l $difffile | awk '{print $5}')
	rawfile_len=$(ls -l $rawfile | awk '{print $5}')
	diff_bytefile_len=$(ls -l $diff_bytefile | awk '{print $5}')
	diff_bitfile_len=$(ls -l $diff_bitfile | awk '{print $5}')
	echo "$count:   raw $rawfile_len, diff $difffile_len, byte $bytefile_len, bit $bitfile_len, diff_byte $diff_bytefile_len, diff_bit $diff_bitfile_len"
	echo "$count,$rawfile_len,$difffile_len,$bytefile_len,$bitfile_len,$diff_bytefile_len,$diff_bitfile_len" >> $wfile
}



echo "Enter $OUT, deflate *"
cd $OUT
for file in ./*res*; do
	$MYDEFLATE -w 15 -m 8 -c $OUT/$file $OUTZ/$file.z
	$MYSHRINK  -c $OUT/$file $OUTS/$file.s
done

cd $HOME

echo "####mydeflate####"
echo ",raw,diff,byte,bit,diff_byte,diff_bit" > $filenamez
for bytefile in $OUTZ/byt*z; do
	stati $bytefile $filenamez
done

#echo "####shrnk####"
#echo ",raw,diff,byte,bit,diff_byte,diff_bit" > $filenames
#for bytefile in $OUTS/byt*s; do
#	stati $bytefile $filenames
#done
#
#cd $OUT
#for file in ./*res*; do
#	#echo "$MYSHRINK  -x $OUTS/$file.s $OUTS_BACK/$file "
#	$MYSHRINK  -x $OUTS/$file.s $OUTS_BACK/$file 
#done

echo ""
echo "finished"

