#!/bin/sh

if [ -x "~/.bashrc" ]
then
	. ~/.bashrc
fi

cd /srv/PiBomb
sudo /usr/bin/nice -n -20 /srv/PiBomb/pibomb
