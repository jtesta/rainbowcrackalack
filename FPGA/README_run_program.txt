Instructions For Generating Tables on Amazon EC2 FPGA F1 Instance


1.) Start an f1.2xlarge instance with the FPGA development tools.  As of the time of this writing (March 2019), the latest image is AMI ID: ami-0da0d9ed98b33a214.

  * Pro-tip: instead of using an on-demand instance, make a spot request for a 70% discount (!).


2.) Log in as root, and configure AWS API key:

ssh -i key.pem centos@1.2.3.4
$ sudo -i
# aws configure


2.) Set up development tools:

# pushd ~; git clone https://github.com/aws/aws-fpga; cd aws-fpga; source sdaccel_setup.sh; source sdaccel_runtime_setup.sh; popd


3.) Follow additional instructions given in the error message output above.  As of the time of this writing, they are (double check that these are up-to-date):

# curl -s https://s3.amazonaws.com/aws-fpga-developer-ami/1.5.0/Patches/XRT_2018_2_XDF_RC5/xrt_201802.2.1.0_7.5.1804-xrt.rpm -o xrt_201802.2.1.0_7.5.1804-xrt.rpm
# curl -s https://s3.amazonaws.com/aws-fpga-developer-ami/1.5.0/Patches/XRT_2018_2_XDF_RC5/xrt_201802.2.1.0_7.5.1804-aws.rpm -o xrt_201802.2.1.0_7.5.1804-aws.rpm
# yum remove -y xrt-aws xrt
# yum install -y *.rpm


4.) Reboot the machine (this may or may not be necessary...).


5.) Initialize development tools once more:

# pushd ~; cd aws-fpga; source sdaccel_setup.sh; source sdaccel_runtime_setup.sh; popd
# ln -s /root/aws-fpga/SDAccel/examples/xilinx_2018.2/libs /root/aws-fpga/SDAccel/examples/xilinx_2018.2/getting_started/libs


6.) Place Rainbow Crackalack FPGA directory in correct location:

# cp -R ~/rainbowcrackalack/fpga ~/aws-fpga/SDAccel/examples/xilinx/getting_started/host/rainbowcrackalack


7.) Download *.awsxclbin file:

# cd ~/aws-fpga/SDAccel/examples/xilinx/getting_started/host/rainbowcrackalack
# BUCKET=[s3 bucket name here]
# aws s3 cp s3://$BUCKET/crackalack_fpga_ntlm8.awsxclbin .


8.) Compile & run host application:

# ./make_hostprog.sh
# ./crackalack_fpga_gen ~ 0 128
