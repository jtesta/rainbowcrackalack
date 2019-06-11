Instructions For Compiling FPGA Kernel On Amazon EC2


1.) Start a z1d.2xlarge instance with the FPGA development tools.  As of the time of this writing (March 2019), the latest image is AMI ID: ami-0da0d9ed98b33a214.

  * Pro-tip: instead of using an on-demand instance, make a spot request for a 70% discount (!).
  * Also note: a smaller/cheaper instance can be used to cut down on costs, such as t2.2xlarge or r5.xlarge.  According to the FPGA vendor, 32 GB of RAM is required.  


2.) Install the build tools (as root):

# yum -y install ocl-icd ocl-icd-devel opencl-headers libstdc++-static kernel-headers kernel-devel gcc-c++ gcc gdb libstdc++-static make opencv python git libjpeg-turbo-devel libpng12-devel libtiff-devel compat-libtiff3


3.) Shrink down the swap space so more is usable during compilation (as root):

# swapoff /swapfile; rm -f /swapfile; dd if=/dev/zero of=/swapfile bs=512M count=16; chmod 0600 /swapfile; mkswap /swapfile; swapon /swapfile


4.) Update the swapiness (tells the kernel to not use swap space as aggressively):

# sysctl vm.swappiness=10


--- NOTE: all commands below are run as the centos user--NOT as root! ---


5.) Set your AWS keys:

$ aws configure


6.) Place the Rainbow Crackalack sources in /home/centos/rainbowcrackalack.


7.) Set up the FPGA tools.

$ cd ~
$ git clone https://github.com/aws/aws-fpga
$ cd aws-fpga; source sdaccel_setup.sh


8.) Place the FPGA code into the example directory to compile:

$ cp -R ~/rainbowcrackalack/fpga $SDACCEL_DIR/examples/xilinx/getting_started/host/rainbowcrackalack


9.) Begin compilation:

$ screen -S generate
$ cd $SDACCEL_DIR/examples/xilinx/getting_started/host/rainbowcrackalack
$ ./make_kernel.sh


10.) Wait about 2 hours (on the z1d.2xlarge instance) for compilation to complete.

  * Monitor the ~/fpga_kernel_compile.txt file for updates.
  * After compiling the kernel, the host application will be executed.  This *should* fail.  Continue on anyway.


11.) Create an S3 bucket and folders to hold AFI compilation logs:

$ REGION=us-east-1 [update as necessary]
$ BUCKET=[give a name to your bucket]

$ aws s3 mb s3://$BUCKET --region $REGION
$ aws s3 mb s3://$BUCKET/dcp
$ aws s3 mb s3://$BUCKET/logs


12.) Create the AFI image:

$ cd ~
$ cd aws-fpga; source sdaccel_setup.sh
$ cd $SDACCEL_DIR/examples/xilinx/getting_started/host/rainbowcrackalack
$ $SDACCEL_DIR/tools/create_sdaccel_afi.sh -xclbin=xclbin/crackalack_fpga_ntlm8.hw.xilinx_aws-vu9p-f1-04261818_dynamic_5_0.xclbin -o=crackalack_fpga_ntlm8 -s3_bucket=$BUCKET -s3_dcp_key=dcp -s3_logs_key=logs


13.) Copy *.awsxclbin file to S3 bucket:

$ aws s3 cp crackalack_fpga_ntlm8.awsxclbin s3://$BUCKET/


14.) Check on progress of AFI creation:

$ aws ec2 describe-fpga-images --owners self


  * After ~15 minutes or so, the "Code" field should transition from "pending" to "available."  Once this is done, compilation is complete and the kernel is ready to be run on a real FPGA instance!
