## REST安全问题考虑
1. appkey是什么东西？
2. apptoken是什么东西？
3. OAuth2是个什么东西？

### 客户端认证方式
1. 同一域名使用，一般用HTTPS+用户名/密码，返回token（令牌）
2. 第三方登陆用oauth方案


接口：http://localhost:8888/sysmonitor/...

设备列表
在列表添加一个监视项 POST /app
在列表删除一个监视项 DEL  /app
获取整个监视列表 GET /app

单个进程
更改一个进程的信息 PUT app/appName
查询一个进程的信息 GET app/appName

不支持：
1.未对重复进程做处理
2.不支持获取进程列表
3.退出程序时未加处理，将缓存写入磁盘

1.ajax调用rest接口同源和跨源问题  
2.rest接口appkey和OAuth的使用



REST示例：

获取： `GET http://localhost:38889/sysmonitor/app`

添加:

```
POST http://localhost:38889/sysmonitor/app HTTP/1.1
User-Agent: Fiddler
Host: localhost:38889
Content-Type: application/json
Content-Length: 37

{"appName":"SMF.exe","storage":false}
```

