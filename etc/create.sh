convert -size 2970x2100 xc:white -density 10x10 -units pixelspercentimeter -fill white result.png
h=5
v=4
for j in $(seq 0 3);do
echo -ne Creating calibration patterns $j of 4 \\r 
x=$(($j/2*2470+250)) 
y=$(($j%2*1600+250));
ix=$(($j/2*15+40));
iy=$(($j%2*30+40)); 
convert result.png \
-fill white -stroke black -draw "ellipse $x,$y 200,200 0,360" \
-fill black -stroke none  -draw "ellipse $x,$y 150,150 0,360" \
-fill white -stroke none  -draw "ellipse $x,$y $ix,$iy 0,360" \
-fill black -stroke none  -draw "line $(($x-200)),$y $(($x-190)),$y" \
-fill black -stroke none  -draw "line $(($x+200)),$y $(($x+190)),$y" \
-fill black -stroke none  -draw "line $x,$(($y+200)) $x,$(($y+190))" \
-fill black -stroke none  -draw "line $x,$(($y-200)) $x,$(($y-190))" \
-fill black -stroke none  -draw "point $x,$y" \
result.png
done

for j in $(seq 0 $(($v-1)));do
for i in $(seq 0 $(($h-1)));do
echo -ne Creating actual robot patterns $(($i+$j*$h)) of  $(($v*$h)) \\r 
x=$(($i*400+650)); 
y=$(($j*400+250));
ix=$(($j*15+40));
iy=$(($i*15+40)); 
convert result.png \
-fill white -stroke black -draw "ellipse $x,$y 200,200 0,360" \
-fill black -stroke none -draw "ellipse $x,$y 130,160 0,360" \
-fill white -stroke none -draw "ellipse $x,$(($y+10)) $ix,$iy 0,360" \
result.png
r0=$(printf %.3f $(echo $ix/130|bc -l))
r1=$(printf %.3f $(echo $iy/160|bc -l))
#echo $r1 $r0
done
done
convert result.png -density 100x100 result_25.pdf
