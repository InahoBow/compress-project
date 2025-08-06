#!/bin/bash
HOME=`pwd`
OUT=$HOME/../pre_processing/out

STEP=step2
OUTZ=$HOME/$STEP/out.z
MYOUT=$HOME/$STEP/out

MYDEFLATE=$HOME/../encoding/mydeflate/mydeflate
MYSHRINK=$HOME/../encoding/zerobyte_suppression/zerobyte_suppression

DEFAULT_W=15
DEFAULT_M=8
SMALL_W=10
SMALL_M=3
DEFAULT_NAME=$DEFAULT_W-$DEFAULT_M
SMALL_NAME=$SMALL_W-$SMALL_M

echo ""
echo "$0 $STEP"
cat $OUT/readme
filenamez=$HOME/$STEP/$(cat $OUT/readme | awk '{print $3}' | sed 's/,//').statis.$STEP.$SMALL_NAME.csv
echo "statistics file :$filenamez"
echo ""

mkdir -p $HOME/$STEP/out.z  $HOME/$STEP/out
rm -f $HOME/$STEP/*sta*csv $HOME/$STEP/ou*/*


stati()
{
	bfile=$1
	wfile=$2
	#echo $bfile $wfile
	count=$(echo $bfile | awk -F '.' '{print $4}')
	db_small_z=$(echo $bfile | sed "s/$DEFAULT_NAME/$SMALL_NAME/g")
	db_s=$(echo $bfile | sed "s/$DEFAULT_NAME.z/s/g")
	raw_default_z=$(echo $bfile | sed 's/diff_bit/raw/g')	
	raw_small_z=$(echo $bfile | sed 's/diff_bit/raw/g' | sed "s/$DEFAULT_NAME/$SMALL_NAME/g")

	db_default_len=$(ls -l $bfile | awk '{print $5}')
	db_small_len=$(ls -l $db_small_z | awk '{print $5}')
	db_s_len=$(ls -l $db_s | awk '{print $5}')
	raw_default_len=$(ls -l $raw_default_z | awk '{print $5}')
	raw_small_len=$(ls -l $raw_small_z | awk '{print $5}')

	echo "$count:   raw $raw_default_len, rawsmall $raw_small_len, db $db_default_len, dbsmall $db_small_len, dbzero $db_s_len"
	echo "$count,$raw_default_len,$raw_small_len,$db_default_len,$db_small_len,$db_s_len" >> $wfile
	#echo "$count,$rawfile_len,$difffile_len,$bytefile_len,$bitfile_len,$diff_bytefile_len,$diff_bitfile_len" >> $wfile
}

#collect file
cp $OUT/raw*res* $OUT/diff_bit*res* $MYOUT/.

echo "Enter $MYOUT, deflate *"
cd $MYOUT
for file in ./raw*; do
	$MYDEFLATE -w $DEFAULT_W -m $DEFAULT_M -c $OUT/$file $OUTZ/$file.$DEFAULT_NAME.z
	$MYDEFLATE -w $SMALL_W -m $SMALL_M -c $OUT/$file $OUTZ/$file.$SMALL_NAME.z
done
for file in ./diff_bit*; do
	$MYDEFLATE -w $DEFAULT_W -m $DEFAULT_M -c $OUT/$file $OUTZ/$file.$DEFAULT_NAME.z
	$MYDEFLATE -w $SMALL_W -m $SMALL_M -c $OUT/$file $OUTZ/$file.$SMALL_NAME.z
	$MYSHRINK  -c $OUT/$file $OUTZ/$file.s
done


cd $HOME

echo "####$STEP statistics####"
echo ",raw,rawsmall,db,dbsmall,dbzero" > $filenamez
for bitdiff_file in $OUTZ/diff_bit*$DEFAULT_NAME*z; do
	stati $bitdiff_file $filenamez
done


echo ""
echo "finished"

