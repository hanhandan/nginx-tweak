# 测试环境 #

60MB大小的文件，均匀分布在10个盘中。
随机选择这些文件随机分块读取32KB内容。

# 测试结果 #

## 2015-03-20 ##

   命令执行如下：

       # echo 1 > /proc/sys/vm/drop_caches
       # echo 0 > /proc/sys/vm/drop_caches
       # ./disktest/disktest -d 1099511627776 -t 40
       57 seconds elapsed, 3276800000 bytes readed, 57487719 bytes per second

   开40个线程进行测试，100000次读取耗时57秒，54.82MB每秒。
