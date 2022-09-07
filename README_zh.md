# NewIP内核协议栈

## 简介

NewIP支持可变长多语义地址，可变长定制化报头封装，通过精简报文头开销，提升数据传输效率。

```
备注：
IPv4地址长度固定4字节，IPv6地址长度固定16字节。
IPv4网络层报头长度20~60字节，IPv6网络层报头长度40字节。
```

NewIP灵活极简报文头如下图所示，通过LLC Header中的EtherType = 0xEADD标识。Bitmap是一组由0和1组成的二进制序列，每个二进制位的数值用于表示特定目标特性的存在性。

![image-20220901152326770](figures/image-20220901152326770.png)

NewIP内核协议栈架构图如下，用户态调用Socket API创建NewIP socket，采用NewIP极简帧头封装进行收发包。

![image-20220901152539801](figures/image-20220901152539801.png)



## 目录

NewIP内核协议栈主要代码目录结构如下：

```
/foundation/communication/sfc/newip
├── patches
│   └── linux-5.10        # NewIP内核侵入式修改对应的补丁文件
└── code
    ├── common            # NewIP 通用代码
    └── linux             # NewIP Linux内核代码
        ├── include       # NewIP 头文件
        │   ├── linux
        │   ├── net
        │   └── uapi
        └── net
            └── newip     # NewIP 功能代码
```

## 编译构建

NewIP内核协议栈使能，需要在对应款型的内核defconfig文件中设置CONFIG_NEWIP=y，删除out/kernel目录后重新编译。

kernel\linux\config\linux-5.10\arch\arm64\configs\rk3568_standard_defconfig

```c
CONFIG_NEWIP=y
```

代码编译完成后，通过下面命令可以确认newip协议栈代码是否使能成功。

```c
root@nip-server:~/harmony_master/harmony# find out/ -name *nip*.o
out/rk3568/obj/third_party/glib/glib/glib_source/guniprop.o
out/kernel/OBJ/linux-5.10/net/newip/nip_addrconf_core.o
out/kernel/OBJ/linux-5.10/net/newip/nip_hdr_decap.o
out/kernel/OBJ/linux-5.10/net/newip/nip_addr.o
out/kernel/OBJ/linux-5.10/net/newip/nip_checksum.o
out/kernel/OBJ/linux-5.10/net/newip/tcp_nip_output.o
...
```

禁用NewIP内核协议栈，删除CONFIG_NEWIP使能开关，删除out/kernel目录后重新编译。

```c
# CONFIG_NEWIP is not set
```

## 说明

### 报头字段说明

NewIP报文格式由NewIP报头与Payload两部分组成，如下图所示：

![image-20220901155226078](figures/image-20220901155226078.png)

1)	Dispatch：指示封装子类，数值0b0表示其为极简封装子类，长度为1比特；(0b表示后面数值为二进制)。

2)	Bitmap：变长，Bitmap默认为紧跟在Dispatch有效位后面的7比特，Bitmap字段长度可持续扩展。Bitmap最后一位置0表示Bitmap结束，最后一位置1表示Bitmap向后扩展1 Byte，直至最后一位置0。
2)	Value: 标识字段的值，长度为1 Byte的整数倍，类型及长度由报头字段语义表确定。



### 可变长地址格式说明

NewIP采用自解释编码，编码格式如下所示：

| First Byte | Semantics                                                    | 地址段有效范围                                               |
| ---------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 0x00       | Address is 0                                                 | 【1字节】0 ~ 220 (0x00 ~ 0xDC)                               |
| 0x01       | Address is 1                                                 |                                                              |
| 0x02       | Address is 2                                                 |                                                              |
| ...        | ...                                                          |                                                              |
| 0xDC       | Address is 220                                               |                                                              |
| 0xDD       | An 16-bit address, which is 0 + 256 * (0xDD - 0xDD) + the last byte value | 【2字节】221 ~ 255 (0x**DD**DD ~ 0x**DD**FF)                 |
| 0xDE       | An 16-bit address, which is 0 + 256 * (0xDE - 0xDD) + the last byte value | 【2字节】256 ~ 511 (0x**DE**00 ~ 0x**DE**FF)                 |
| 0xDF       | An 16-bit address, which is 0 + 256 * (0xDF - 0xDD) + the last byte value | 【2字节】512 ~ 767 (0x**DF**00 ~ 0x**DF**FF)                 |
|            |                                                              |                                                              |
| ...        | ...                                                          |                                                              |
| 0xF0       | An 16-bit address, which is 0 + 256 * (0xF0 - 0xDD) + the last byte value | 【2字节】4864 ~ 5119 (0x**F0**00 ~ 0x**F0**FF)               |
| 0xF1       | An 16-bit address is followed                                | 【3字节】5120 ~ 65535 (0x**F1** 1400 ~ 0x**F1** FFFF)        |
| 0xF2       | An 32-bit address is followed                                | 【5字节】65536 ~ 4,294,967,295 (0x**F2** 0001 0000 ~ 0x**F2** FFFF FFFF) |
| 0xF3       | An 48-bit address is followed                                | 【7字节】4,294,967,296 ~ 281,474,976,710,655 (0x**F3** 0001 0000 0000 ~ 0x**F3** FFFF FFFF FFFF) |
| 0xFE       | An 56-bit address is followed                                | 【8字节】0 ~ 72,057,594,037,927,935 (0x**FE**00 0000 0000 0000 ~ 0x**FE**FF FFFF FFFF FFFF) |

### 接口说明

NewIP短地址结构如下：

```c
enum nip_8bit_addr_index {
	NIP_8BIT_ADDR_INDEX_0 = 0,
	NIP_8BIT_ADDR_INDEX_1 = 1,
	NIP_8BIT_ADDR_INDEX_2 = 2,
	NIP_8BIT_ADDR_INDEX_3 = 3,
	NIP_8BIT_ADDR_INDEX_4 = 4,
	NIP_8BIT_ADDR_INDEX_5 = 5,
	NIP_8BIT_ADDR_INDEX_6 = 6,
	NIP_8BIT_ADDR_INDEX_7 = 7,
	NIP_8BIT_ADDR_INDEX_MAX,
};

enum nip_16bit_addr_index {
	NIP_16BIT_ADDR_INDEX_0 = 0,
	NIP_16BIT_ADDR_INDEX_1 = 1,
	NIP_16BIT_ADDR_INDEX_2 = 2,
	NIP_16BIT_ADDR_INDEX_3 = 3,
	NIP_16BIT_ADDR_INDEX_MAX,
};

enum nip_32bit_addr_index {
	NIP_32BIT_ADDR_INDEX_0 = 0,
	NIP_32BIT_ADDR_INDEX_1 = 1,
	NIP_32BIT_ADDR_INDEX_MAX,
};

#define nip_addr_field8 v.u.field8
#define nip_addr_field16 v.u.field16
#define nip_addr_field32 v.u.field32

#pragma pack(1)
struct nip_addr_field {
	union {
		unsigned char   field8[NIP_8BIT_ADDR_INDEX_MAX];
		unsigned short field16[NIP_16BIT_ADDR_INDEX_MAX]; /* big-endian */
		unsigned int   field32[NIP_32BIT_ADDR_INDEX_MAX]; /* big-endian */
	} u;
};

struct nip_addr {
	unsigned char bitlen;	/* The address length is in bit (not byte) */
	struct nip_addr_field v;
};
#pragma pack()

/* The following structure must be larger than V4. System calls use V4.
 * If the definition is smaller than V4, the read process will have memory overruns
 * v4: include\linux\socket.h --> sockaddr (16Byte)
 */
#define POD_SOCKADDR_SIZE 3
struct sockaddr_nin {
	unsigned short sin_family; /* [2Byte] AF_NINET */
	unsigned short sin_port;   /* [2Byte] Transport layer port, big-endian */
	struct nip_addr sin_addr;  /* [9Byte] NIP address */

	unsigned char sin_zero[POD_SOCKADDR_SIZE]; /* [3Byte] Byte alignment */
};
```



NewIP协议socket接口列表如下：

| 函数     | 输入                                                         | 输出                                           | 返回值           | 接口具体描述                                                 |
| -------- | ------------------------------------------------------------ | ---------------------------------------------- | ---------------- | ------------------------------------------------------------ |
| socket   | int **domain**, int type, int **protocol**                   | NA                                             | Socket句柄sockfd | 创建NewIP 协议类型socket，并返回socket实例所对应的句柄。**domain参数填写 AF_NINET，表示创建NewIP协议类型socket。protocol参数填写IPPROTO_TCP或IPPROTO_UDP**。 |
| bind     | int sockfd, const **struct sockaddr_nin** *myaddr, socklen_t addrlen | NA                                             | int，返回错误码  | 将创建的socket绑定到指定的IP地址和端口上。**myaddr->sin_family填写AF_NINET**。 |
| listen   | int socket, int backlog                                      | NA                                             | int，返回错误码  | 服务端监听NewIP地址和端口                                    |
| connect  | int sockfd, const **struct sockaddr_nin** *addr, aocklen_t addrlen | NA                                             | int，返回错误码  | 客户端创建至服务端的连接                                     |
| accept   | int sockfd, **struct sockaddr_nin** *address, socklen_t *address_len | NA                                             | 返回socket的fd   | 服务端返回已建链成功的socket                                 |
| send     | int sockfd, const void *msg, int len, unsigned int flags, const **struct sockaddr_nin** *dst_addr, int addrlen | NA                                             | int，返回错误码  | 用于socket已连接的NewIP类型数据发送                          |
| recv     | int sockfd, size_t len, int flags, **struct sockaddr_nin** *src_addr, | void  **buf, int* *fromlen                     | int，返回错误码  | 用于socket已连接的NewIP类型数据接收                          |
| close    | int sockfd                                                   | NA                                             | int，返回错误码  | 关闭socket，释放资源                                         |
| ioctl    | int sockfd, unsigned long cmd, ...                           | NA                                             | int，返回错误码  | 对NewIP协议栈相关信息进行查询或更改。                        |
| sendto   | int sockfd, const void *msg, int len, unsigned int flags, const **struct sockaddr** *dst_addr, int addrlen | NA                                             | int，返回错误码  | 用于socket无连接的NewIP类型数据发送                          |
| recvfrom | int sockfd, size_t len, int flags,                           | void *buf, struct sockaddr *from, int *fromlen | int，返回错误码  | 用于socket无连接的NewIP类型数据接收                          |

### 使用说明

NewIP可变长地址配置，路由配置，UDP/TCP收发包demo代码链接如下，NewIP协议栈用户态接口使用方法可以参考demo代码。

https://gitee.com/openharmony-sig/communication_sfc_newip/tree/master/examples

| 文件名                | 功能                          |
| --------------------- | ----------------------------- |
| nip_addr_cfg_demo.c   | NewIP可变长地址配置demo代码   |
| nip_route_cfg_demo.c  | NewIP路由配置demo代码         |
| nip_udp_server_demo.c | NewIP UDP收发包服务端demo代码 |
| nip_udp_client_demo.c | NewIP UDP收发包客户端demo代码 |
| nip_tcp_server_demo.c | NewIP TCP收发包服务端demo代码 |
| nip_tcp_client_demo.c | NewIP TCP收发包客户端demo代码 |
| nip_lib.c             | 接口索引获取等API接口demo代码 |

**基础操作步骤：**

1、将demo代码拷贝到Linux编译机上，make clean，make all编译demo代码。

2、将编译生成二级制文件上传到xxx开发板。

3、执行“ifconfig xxx up”开启网卡设备，xxx表示网卡名，比如eth0，wlan0。

4、在1号开发板sh下执行“./nip_addr_cfg_demo server”给服务端配置0xDE00（2字节）变长地址，在2号开发板sh下执行“./nip_addr_cfg_demo client”给客户端配置0x50（1字节）变长地址，通过“cat /proc/net/nip_addr”查看内核地址配置结果。

5、在1号开发板sh下执行“./nip_route_cfg_demo server”给服务端配置路由，在2号开发板sh下执行“./nip_route_cfg_demo client”给客户端配置路由，通过“cat /proc/net/nip_route”查看内核路由配置结果。

以上步骤操作完成后，可以进行UDP/TCP收发包，收发包demo默认使用上面步骤中配置的地址和路由。



**UDP收发包操作步骤：**

先在服务端执行“./nip_udp_server_demo”，然后再在客户端执行“./nip_udp_client_demo”，客户端会发送10个NewIP报文，客户端收到报文后再发送给服务端。

```
服务端sh窗口打印内容：
Received -- 1661826989 498038 NIP_UDP #      0 -- from 0x50:57605
Sending  -- 1661826989 498038 NIP_UDP #      0 -- to 0x50:57605
Received -- 1661826990  14641 NIP_UDP #      1 -- from 0x50:57605
Sending  -- 1661826990  14641 NIP_UDP #      1 -- to 0x50:57605
Received -- 1661826990 518388 NIP_UDP #      2 -- from 0x50:57605
Sending  -- 1661826990 518388 NIP_UDP #      2 -- to 0x50:57605
...
Received -- 1661827011 590576 NIP_UDP #      9 -- from 0x50:37758
Sending  -- 1661827011 590576 NIP_UDP #      9 -- to 0x50:37758

客户端sh窗口打印内容：
Received --1661827007  55221 NIP_UDP #      0 sock 3 success:     1/     1/no=     0
Received --1661827007 557926 NIP_UDP #      1 sock 3 success:     2/     2/no=     1
Received --1661827008  62653 NIP_UDP #      2 sock 3 success:     3/     3/no=     2
...
Received --1661827011 590576 NIP_UDP #      9 sock 3 success:    10/    10/no=     9
```



**TCP收发包操作步骤：**

先在服务端执行“./nip_tcp_server_demo”，然后再在客户端执行“./nip_tcp_client_demo”，客户端会发送10个NewIP报文，客户端收到报文后再发送给服务端。

```
服务端sh窗口打印内容：
Received -- 1661760202 560605 NIP_TCP #      0 --:1024
Sending  -- 1661760202 560605 NIP_TCP #      0 --:1024
Received -- 1661760203  69254 NIP_TCP #      1 --:1024
Sending  -- 1661760203  69254 NIP_TCP #      1 --:1024
Received -- 1661760203 571604 NIP_TCP #      2 --:1024
Sending  -- 1661760203 571604 NIP_TCP #      2 --:1024
...
Received -- 1661760207  86544 NIP_TCP #      9 --:1024
Sending  -- 1661760207  86544 NIP_TCP #      9 --:1024

客户端sh窗口打印内容：
Received --1661760202 560605 NIP_TCP #      0 sock 3 success:     1/     1/no=     0
Received --1661760203  69254 NIP_TCP #      1 sock 3 success:     2/     2/no=     1
...
Received --1661760207  86544 NIP_TCP #      9 sock 3 success:    10/    10/no=     9
```
