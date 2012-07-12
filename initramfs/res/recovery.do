on multi-csc
	echo 
	echo "-- Appling Multi-CSC..."
	mount /system
	echo "Applied the CSC-code : <salse_code>"
	cp -y -f -r -v /system/csc/<salse_code> /
	cmp -r /system/csc/<salse_code> /
	echo "Successfully applied multi-CSC."

on factory-out
	mount /data
	mkdir system system 0770 /data/factory
	write /data/factory/.resetverify 1
	unmount /data