for f in `find src tests -type f`
do
	sed -i -e "s/$1/$2/g" "$f"
done
for f in `find src tests -type f -name "*$1*"`
do
	NEW_NAME=`echo $f | sed "s/$1/$2/g"`
	echo "$f -> $NEW_NAME"
	mv $f $NEW_NAME
done

