To run the RX2 stratum you need to activate large pages, to do so you'll need to add:

- vm.nr_hugepages=3840 to /etc/sysctl.conf
- reboot


Libraries needed to compile:

sudo apt install libldap2-dev
sudo apt install libkrb5-dev
sudo apt install librtmp-dev
sudo apt install libmysqlclient-dev
sudo apt install libpsl-dev
sudo apt install libnghttp2-dev