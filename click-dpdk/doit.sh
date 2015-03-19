#/bin/bash
git pull
make -j 16
sudo ./userlevel/click -j 5 -I [p786p1,p786p2,p785p1,p785p2] blindbox.click5
