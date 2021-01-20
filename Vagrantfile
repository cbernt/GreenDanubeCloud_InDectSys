# -*- mode: ruby -*-
# vi: set ft=ruby :

### configuration parameters ###
# BRIDGE ADAPTER - if you have mutiple network adapater to choose from you may speciify one here to prevent vagrant to prompt every time!
$BRIDGE_ADAPTER = "Realtek USB GbE Family Controller" # some value or empty

Vagrant.configure("2") do |config|
	
  
	# ===================================
	# DEVELOPMENT
	# ===================================
	config.vm.define "develop" do |develop|
	
		
		develop.vm.box = "hashicorp/precise64"
		develop.disksize.size = '50GB'
		if($BRIDGE_ADAPTER == "")
			develop.vm.network "public_network"
		else
			develop.vm.network "public_network", bridge: $BRIDGE_ADAPTER
		end	
		develop.vm.provider "virtualbox" do |vb|
			# Display the VirtualBox GUI when booting the machine
			vb.gui = true
			vb.cpus = 4
			# Customize the amount of memory on the VM:
			vb.memory = "6000"
			# enable promiscuous mode on the public network
			#vb.customize ["modifyvm", :id, "--nicpromisc0", "allow-all"]
			vb.customize ["modifyvm", :id, "--nicpromisc1", "allow-all"]
			vb.customize ["modifyvm", :id, "--nicpromisc2", "allow-all"]
			vb.customize ["modifyvm", :id, "--nicpromisc3", "allow-all"]
		end
		
		develop.vm.provision "shell", inline: <<-SHELL
			sudo apt-get update			
			sudo apt-get -y install build-essential
			sudo apt-get -y install squid3 ccze			
			sudo apt-get -y install net-tools git mc valgrind openssh-server qemu-user-static binfmt-support			
								
			# Create directory for Cache, on this case cache directory placed on directory
			sudo mkdir -p /home/squid/cache/
			sudo chmod 777 /home/squid/cache/
			sudo chown proxy:proxy /home/squid/cache/
			
			#Create swap directory
			sudo squid3 -z
			sudo service squid3 restart				
																															
			cd /vagrant
			
			
			echo "vm ready for build. loggin via ssh"
			
		SHELL
	end
   
end
