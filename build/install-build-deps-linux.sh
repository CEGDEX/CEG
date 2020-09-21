#/bin/bash

cd linux/
rm -rf linux.zip
wget depends.CEGcdn.com:36002/v8_target/linux.zip

down_md5_nums=`md5sum linux.zip | cut -d ' ' -f1`
true_md5_nums="67a227a59acd375ab21d9edeb555a33a"
echo $true_md5_nums
echo $down_md5_nums

if [ "$true_md5_nums"x != "$down_md5_nums"x ]; then  
	echo "$md5_nums error, true md5:" $true_md5_nums "down md5:" $down_md5_nums
	exit
fi

unzip linux.zip
rm ../../src/3rd/v8_target/linux/ -rf
mv  linux ../../src/3rd/v8_target/linux -f
rm linux.zip -rf
mkdir -p ../../bin/

cp ../../src/3rd/v8_target/linux/*.bin ../../bin/
cp ../../src/3rd/v8_target/linux/*.dat ../../bin/
