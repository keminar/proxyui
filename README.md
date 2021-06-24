# proxyui
proxy manager for windows

[下载包](http://cloudme.io/)


网络请求
---
```
# 非公司域名，不经过抓包
+-----------+      +-----------+      +----------+
| 手机/电脑 | <==> | 代理程序1 | <==> | Internet |
+-----------+      +-----------+      +----------+

# 公司域名，代理到不同环境
+-----------+      +-----------+      +-----------+      +----------+
| 手机/电脑 | <==> | 代理程序1 | <==> | 代理程序2 | <==> | Internet |
+-----------+      +-----------+      +-----------+      +----------+

# 公司域名，需要先抓包，同时代理到不同环境
+-----------+      +-----------+      +----------+      +-----------+      +----------+
| 手机/电脑 | <==> | 代理程序1 | <==> | charles  | <==> | 代理程序2 | <==> | Internet |
+-----------+      +-----------+      +----------+      +-----------+      +----------+
```

快速使用
---
```
系统代理：为windows操作系统中设置->网络->代理的快捷操作，方便快速切换不同的代理，推荐设置为代理程序1使用的端口

代理程序：
    1.程序路径：选择本地anyproxy的路径
      启动参数：主代理配置文件，将各类域名按需要分发，如不需要经过charles和代理程序2的域名直接请求，其它的转到各自目标端口
    2.程序路径：选择本地anyproxy的路径
      启动参数：开发环境配置文件，接收从代理程序1或charles过来的请求，并按配置文件的目标地址请求，可配置测试环境，线上等
      下拉选项：修改后会联动修改启动参数，需要手动点重启生效，是快捷切换配置的一种方式

最小化：托盘图标点右键，可以显示出代理程序2支持的环境，左键选择要切换的目标环境，服务会自动重启并生效

额外说明：
    通过Charle抓包的请求如何再转发到代理程序2？
    Proxy -> External Proxy Settings -> 选中 Use External proxy servers 先中 Web Proxy （HTTP）和 Secure Web Proxy（HTTPS）
    并在后面的Web Proxy Server中写入代理程序2的ip和端口，点确认保存
    
```

优点：
---
  * 对Anyproxy可视化启动停止，易于操作
  * 手机可以常连代理程序1，访问不同环境不需要一直调整手机代理设置
  * 电脑上一键切换环境