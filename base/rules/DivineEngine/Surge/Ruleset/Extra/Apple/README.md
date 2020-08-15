## 说明

Apple 已知域名均已收录于 China.list 中并作直连策略，如无确切目的不需要额外添加。

对于一些 Apple 被「墙」或者主动「墙」的服务如 App Store Preview、Moveis Trailers、查询外汇、国际航线的 Spotlight、维基百科查询的 Dictionary 均已收录于 Global.list 进行代理。

该目录只是一时兴起想做一些关于 Apple 各子域名具体作用的收录，鉴于 Apple 在国内大体良好的 CDN 部署个人不建议对其进行代理，当然既然进到了这个目录可能 Apple 的某些服务在你所在地区堪忧，相比以前对于 Apple 整体域名全部代理，该目录收录的一些细分分流文件如 App Store 应用下载、系统更新的专项代理应该更适合你。

### 分流文件说明

**Apple.list**

是 Apple 服务的总体整理，如您想对 Apple 服务均进行代理可以使用该分流文件，需要注意的是建议放置于 Global.list 与 China.list 之间，因 Global.list 有 Apple 对于中国大陆不可用服务的代理行为，如您的 Apple 策略经常在代理与直连来回切换会导致 Global.list 中的规则失效。

**BlockiOSUpdate.list**

iOS 设备屏蔽系统的「软件更新」之用，故而需要注意的是策略选择为 `REJECT`

**其他**

其他分流文件均为代理策略，文件名极其内容已说明其主要作用。