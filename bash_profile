#!/bin/sh

if [ -x "~/.bashrc" ]
then
	. ~/.bashrc
fi

cd /srv/PiBomb
sudo /srv/PiBomb/pibomb
