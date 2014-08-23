tree -L 1


stop adbd;setprop service.adb.tcp.port 5555;start adbd 

http://125.39.118.37/otaupdate/update

lunch g24ref_g24ref_a706_780TPC-user
   make otapackage -j4

   单独编译kernel:
   ./device/amlogic/g24ref_a706_780TPC/quick_build_kernel.sh  bootimage   meson6_g24_a706_780TPC_jbmr1_defconfig

   单独编译u-boot:
   make m6_g24_A706_config
   make -j4


make m6_mbx_th8_config && make -j8


2) if uboot support secure mode
 10      $make distclean
 11      $make m6_mbx_th8_secure_config 
 12      $make -j8



am start -n com.amlogic.osdoverlay/.HdmiInActivity

aapt dump badging xxx.apk


busybox ifconfig eth0 hw ether 00:11:22:33:44:66
env set ethaddr 00:11:22:33:44:55



 cd sys/class/aml                                              
root@android:/sys/class/aml_keys/aml_keys # echo mac > key_name
root@android:/sys/class/aml_keys/aml_keys # cat key_read
root@android:/sys/class/aml_keys/aml_keys # echo mac > key_name
33353a33613a33363a3631 > key_write                                            <
root@android:/sys/class/aml_keys/aml_keys # cat key_read
root@android:/sys/class/aml_keys/aml_keys # cat /sys/class/                    at        

git init
git add README.md
git commit -m "first commit"
git remote add origin git@github.com:nicky1990/aml_res.git
git push -u origin master""


git log --stat -2
git commit --amend -m "aaaaaaaaaaa"
git reset --soft HEAD^^

git checkout oldbranche                //将其他分支的修改应用到oldbranche
git cherry-pick 2fa2f80


git fetch origin master:tmp
git diff tmp
git merge tmp
git branch -d tmp


set bootcmd run compatible_boot


git checkout  -b test 4b17dbf              创建基于住分支 4b17dbf or tag 的 test 分支 并切换到 test分支

git stash                            保存当前修改未提交修改的代码
git stash pop             

git reset --hard    # 恢复最近一次提交过的状态，即放弃上次提交后的所有本次修改 

git ci --amend      # 修改最后一次提交记录  
nano 

ctrl+o + ctrl x

git revert <$id>    # 恢复某次提交的状态，恢复动作本身也创建了一次提交对象  
git revert HEAD     # 恢复最后一次提交的状态  

git br -v           # 查看各个分支最后提交信息  
git br --merged     # 查看已经被合并到当前分支的分支  
git br --no-merged  # 查看尚未被合并到当前分支的分支  


git diff > ../sync.patch         # 生成补丁  
git apply ../sync.patch          # 打补丁  
git apply --check ../sync.patch  #测试补丁能否成功  

git remote set-head origin master   # 设置远程仓库的HEAD指向master分支 

git config --global diff.tool bc3
git config --global difftool.bc3.path "/usr/bin/bcompare"

git log --pretty=oneline 文件名



./gerrit_th.sh zhaoxinhai 
git add .
git commit
//git commit --amend   还原上一次的提交
git push gerrit HEAD:refs/for/jb-mr1-1225-timehold


http://27.154.234.214:8081/#/settings/

http://27.154.234.214:8000/redmine/projects/
zhaoxinhai
th-zxh
http://27.154.234.214:8000/redmine/login

zhaoxinhai
th-zhao 

3、执行gerrit_th
./gerrit_th chenww
执行git remote -v可以发现多了gerrit branch
4、提交代码的操作
git add xxxx
git commit
git push gerrit HEAD:refs/for/master （这里master需要对应修改为你当前工作对应的服务器branch名称，例如remotes/timehold-server/jb-timehold，就用jb-timehold替换master。 使用原有方式push代码会提示出错）
5、代码提交后的审核工作
打开http://27.154.234.214:8081




0x000000000000-0x000000060000 : "bootloader"
0x000000080000-0x000000088000 : "ubootenv"
""""



0x000000800000-0x000001000000 : "logo"
0x000001000000-0x000001800000 : "aml_logo"
0x000001800000-0x000002000000 : "recovery"
0x000002000000-0x000002800000 : "boot"
0x000002800000-0x000043000000 : "system"
 NAND bbt detect factory Bad block at 14c00000 
 0x000043000000-0x000063000000 : "cache"
 0x000063000000-0x000073000000 : "backup"
 0x000073000000-0x0000fc400000 : "data"
 
 usb start
 fatload usb 0 82000000 boot.img

 nand info
 nand erase 2000000 800000
 nand write 82000000 2000000 800000

 git clone ssh://git@27.154.234.214/repository/timehold/mx-SettingEx-pbs.git

awk {print } < adbkey.pub | openssl base64 -A -d -a | openssl md5 -c | awk {print } | tr [:lower:] [:upper:]


LOCAL_DEX_PREOPT := false
